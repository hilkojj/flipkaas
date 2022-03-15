
#ifndef GAME_REACTPHYSICSRENDERER_H
#define GAME_REACTPHYSICSRENDERER_H

#include <reactphysics3d/reactphysics3d.h>
#include <graphics/camera.h>
#include <graphics/shader_asset.h>
#include <graphics/3d/mesh.h>

class ReactPhysicsRenderer
{

	ShaderAsset shader;
	SharedMesh tris, lines;

	int maxNrOfTris = 64, maxNrOfLines = 64;

  public:

	  ReactPhysicsRenderer();
	
	  void render(reactphysics3d::PhysicsWorld &, const Camera &);


};

#endif
