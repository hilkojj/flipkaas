#include "ReactPhysicsRenderer.h"
#include <graphics/3d/vert_buffer.h>

ReactPhysicsRenderer::ReactPhysicsRenderer()
	:
	shader("ReactPhysics3D debug shader", "shaders/physics_debug.vert", "shaders/physics_debug.frag")
{
}

void ReactPhysicsRenderer::render(reactphysics3d::PhysicsWorld &reactWorld, const Camera &cam)
{
	auto &theOtherRenderer = reactWorld.getDebugRenderer();

	shader.use();
	glUniformMatrix4fv(shader.location("projection"), 1, GL_FALSE, &cam.combined[0][0]);

	glDisable(GL_CULL_FACE);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	auto nrOfTriVerts = theOtherRenderer.getNbTriangles() * 3;
	auto nrOfLineVerts = theOtherRenderer.getNbLines() * 2;

	// TRIS:
	if (!tris || maxNrOfTris < theOtherRenderer.getNbTriangles())
	{
		while (maxNrOfTris < theOtherRenderer.getNbTriangles())
			maxNrOfTris *= 2;

		tris = NULL;
		tris = std::make_shared<Mesh>("ReactPhysics3D tris", maxNrOfTris * 3u, VertAttributes().add_(
			{"POSITION", 3}
		).add_(
			{"COLOR", 4, 4, GL_UNSIGNED_BYTE, GL_TRUE}
		));

		VertBuffer::uploadSingleMesh(tris);
	}
	if (nrOfTriVerts > 0)
	{
		memcpy(&tris->vertices[0], theOtherRenderer.getTrianglesArray(), nrOfTriVerts * tris->attributes.getVertSize());
		tris->vertBuffer->reuploadVertices(tris, nrOfTriVerts);
		tris->renderArrays(GL_TRIANGLES, nrOfTriVerts);
	}

	// LINES:
	if (!lines || maxNrOfLines < theOtherRenderer.getNbLines())
	{
		while (maxNrOfLines < theOtherRenderer.getNbLines())
			maxNrOfLines *= 2;

		lines = NULL;
		lines = std::make_shared<Mesh>("ReactPhysics3D lines", maxNrOfLines * 2u, VertAttributes().add_(
			{ "POSITION", 3 }
		).add_(
			{ "COLOR", 4, 4, GL_UNSIGNED_BYTE, GL_TRUE }
		));

		VertBuffer::uploadSingleMesh(lines);
	}
	if (nrOfLineVerts > 0)
	{
		memcpy(&lines->vertices[0], theOtherRenderer.getLinesArray(), nrOfLineVerts * lines->attributes.getVertSize());
		lines->vertBuffer->reuploadVertices(lines, nrOfLineVerts);
		lines->renderArrays(GL_LINES, nrOfLineVerts);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
}
