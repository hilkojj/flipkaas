#include "GizmoRenderer.h"
#include <im3d.h>
#include <graphics/orthographic_camera.h>
#include <graphics/3d/perspective_camera.h>
#include <graphics/3d/vert_buffer.h>
#include <input/mouse_input.h>
#include <input/key_input.h>
#include "../../../game/Game.h"

GizmoRenderer::GizmoRenderer()
	:	pointsShader("Gizmo points", "shaders/im3d", "shaders/im3d", "shaders/im3d", false),
		linesShader("Gizmo lines", "shaders/im3d", "shaders/im3d", "shaders/im3d", false),
		trisShader("Gizmo tris", "shaders/im3d", "shaders/im3d", "shaders/im3d", false)
{
	pointsShader.definitions.define("POINTS");
	linesShader.definitions.define("LINES");
	trisShader.definitions.define("TRIANGLES");

	glGenBuffers(1, &vertBuffer);
	glGenVertexArrays(1, &vertArray);
	glBindVertexArray(vertArray);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Im3d::VertexData), (GLvoid *)offsetof(Im3d::VertexData, m_positionSize));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Im3d::VertexData), (GLvoid *)offsetof(Im3d::VertexData, m_color));
	glBindVertexArray(0);
	VertBuffer::currentlyBoundVao = 0;
}

// At the top of each frame, the application must fill the Im3d::AppData struct and then call Im3d::NewFrame().
// The example below shows how to do this, in particular how to generate the 'cursor ray' from a mouse position
// which is necessary for interacting with gizmos.
void GizmoRenderer::beginFrame(float deltaTime, const vec2 &viewportSize, const Camera &cam)
{
	Im3d::AppData &ad = Im3d::GetAppData();

	ad.m_deltaTime = deltaTime;
	ad.m_viewportSize = viewportSize;
	ad.m_viewOrigin = cam.position; // for VR use the head position
	ad.m_viewDirection = cam.direction;
	ad.m_worldUp = mu::Y; // used internally for generating orthonormal bases
	ad.m_projOrtho = !!dynamic_cast<const OrthographicCamera *>(&cam);

	auto perspectiveCam = dynamic_cast<const PerspectiveCamera *>(&cam);
	assert(!ad.m_projOrtho == !!perspectiveCam);

	// m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
	ad.m_projScaleY = ad.m_projOrtho
		? 1 // TODO // 2.0f / g_Example->m_camProj(1, 1) // use far plane height for an ortho projection
		: tanf(perspectiveCam->fOV * mu::DEGREES_TO_RAD * 0.5f) * 2.0f // or vertical fov for a perspective projection
		;

	// World space cursor ray from mouse position; for VR this might be the position/orientation of the HMD or a tracked controller.
	
	//if (ad.m_projOrtho)
//	{
		// TODO. See example of Im3d
//	}
//	else
	{
		ad.m_cursorRayOrigin = cam.position;
		ad.m_cursorRayDirection = normalize(cam.getCursorRayDirection());
	}
	
	// Set cull frustum planes. This is only required if IM3D_CULL_GIZMOS or IM3D_CULL_PRIMTIIVES is enable via im3d_config.h, or if any of the IsVisible() functions are called.
	//ad.setCullFrustum(g_Example->m_camViewProj, true);

	// Fill the key state array; using GetAsyncKeyState here but this could equally well be done via the window proc.
	// All key states have an equivalent (and more descriptive) 'Action_' enum.
	ad.m_keyDown[Im3d::Mouse_Left/*Im3d::Action_Select*/] = MouseInput::pressed(GLFW_MOUSE_BUTTON_LEFT);

	// The following key states control which gizmo to use for the generic Gizmo() function. Here using the left ctrl key as an additional predicate.
	bool ctrlDown = KeyInput::pressed(GLFW_KEY_LEFT_CONTROL);
	bool canSwitch = !Game::settings.keyInput.gizmoNeedCtrl || ctrlDown;
	ad.m_keyDown[Im3d::Key_L/*Action_GizmoLocal*/] = canSwitch && KeyInput::pressed(Game::settings.keyInput.gizmoToggleLocal);
	ad.m_keyDown[Im3d::Key_T/*Action_GizmoTranslation*/] = canSwitch && KeyInput::pressed(Game::settings.keyInput.gizmoTranslate);
	ad.m_keyDown[Im3d::Key_R/*Action_GizmoRotation*/] = canSwitch && KeyInput::pressed(Game::settings.keyInput.gizmoRotate);
	ad.m_keyDown[Im3d::Key_S/*Action_GizmoScale*/] = canSwitch && KeyInput::pressed(Game::settings.keyInput.gizmoScale);

	// Enable gizmo snapping by setting the translation/rotation/scale increments to be > 0
	ad.m_snapTranslation = ctrlDown ? 0.1f : 0.0f;
	ad.m_snapRotation = ctrlDown ? 30.0f * mu::DEGREES_TO_RAD : 0.0f;
	ad.m_snapScale = ctrlDown ? 0.5f : 0.0f;

	Im3d::NewFrame();
}

bool GizmoRenderer::gizmo(const char *id, vec3 &pos, quat &rot, vec3 &scale)
{
	Im3d::Vec3 p(pos);
	Im3d::Mat3 r(toMat3(rot));
	Im3d::Vec3 s(scale);

	auto idd = Im3d::MakeId(id);
	bool changed = Im3d::Gizmo(idd, p, r, s);

	pos = p;
	rot = toQuat(mat3(r));
	scale = s;

	return changed;
}

void GizmoRenderer::endFrame(const Camera &cam)
{
	Im3d::EndFrame();


	// Primitive rendering.

	// save off current state of blend enabled
	GLboolean blendEnabled;
	glGetBooleanv(GL_BLEND, &blendEnabled);

	// save off current state of src / dst blend functions
	GLint blendSrc;
	GLint blendDst;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glDisable(GL_DEPTH_TEST);

	for (int i = 0, n = Im3d::GetDrawListCount(); i < n; ++i)
	{
		const Im3d::DrawList &drawList = Im3d::GetDrawLists()[i];

		/*
		if (drawList.m_layerId == Im3d::MakeId("NamedLayer"))
		{
			// The application may group primitives into layers, which can be used to change the draw state (e.g. enable depth testing, use a different shader)
		}
		*/

		GLenum prim;
		ShaderAsset &sh = pointsShader;
		switch (drawList.m_primType)
		{
			case Im3d::DrawPrimitive_Points:
				prim = GL_POINTS;
				sh = pointsShader;
				glDisable(GL_CULL_FACE); // points are view-aligned
				break;
			case Im3d::DrawPrimitive_Lines:
				prim = GL_LINES;
				sh = linesShader;
				glDisable(GL_CULL_FACE); // lines are view-aligned
				break;
			case Im3d::DrawPrimitive_Triangles:
				prim = GL_TRIANGLES;
				sh = trisShader;
				//glEnable(GL_CULL_FACE); // culling valid for triangles, but optional
				break;
			default:
				IM3D_ASSERT(false);
				return;
		};

		glBindVertexArray(vertArray);
		VertBuffer::currentlyBoundVao = vertArray;
		glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)drawList.m_vertexCount * sizeof(Im3d::VertexData), (GLvoid *)drawList.m_vertexData, GL_STREAM_DRAW);

		sh.use();
		glUniform2f(sh.location("uViewport"), gu::width, gu::height);
		glUniformMatrix4fv(sh.location("uViewProjMatrix"), 1, false, &cam.combined[0][0]);
		
		glDrawArrays(prim, 0, (GLsizei)drawList.m_vertexCount);

		
	}
	glDisable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	// restore saved state of blend enabled and blend functions
	if (blendEnabled) {
		glEnable(GL_BLEND);
	}
	else {
		glDisable(GL_BLEND);
	}

	glBlendFunc(blendSrc, blendDst);
}

