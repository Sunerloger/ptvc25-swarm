#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

// enables quickly switching scenes
class Scene {

public:
	
	Scene();
	virtual ~Scene();

	JPH::BodyID getIdPlayer();
	JPH::BodyID getIdTerrain();

private:

	JPH::BodyID& player_id;
	JPH::BodyID& terrain_id;
	// Player player;
	// Terrain terrain;
	// std::vector<Enemy> enemies;
};