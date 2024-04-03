#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

// provides scene information to the renderer and the physics engine
class Scene {

public:
	
	Scene();
	virtual ~Scene();

	void activate();
	void deactivate();

private:

	// Player player;
	// TODO store game objects in vectors: staticObjects (e.g. terrain), dynamicObjects (with behaviour e.g. enemies, whithout behaviour e.g. drops), player, 
	// TODO ui objects (not influenced by physics = no PhysicsEntities)

	// TODO at destruction of scene, bodies get destroyed
};