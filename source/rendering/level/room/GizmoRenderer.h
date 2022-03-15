#ifndef GAME_GIZMORENDERER_H
#define GAME_GIZMORENDERER_H

#include <graphics/camera.h>
#include <graphics/shader_asset.h>
#include <graphics/3d/mesh.h>

class GizmoRenderer
{

	ShaderAsset pointsShader, linesShader, trisShader;
	GLuint vertBuffer = 0, vertArray = 0;

public:

	GizmoRenderer();

	void beginFrame(float deltaTime, const vec2 &viewportSize, const Camera &);

	bool gizmo(const char *id, vec3 &pos, quat &rot, vec3 &scale);

	void endFrame(const Camera &);


};

#endif
