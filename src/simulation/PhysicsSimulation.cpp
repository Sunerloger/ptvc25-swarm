#include "PhysicsSimulation.h"

namespace physics {

    PhysicsSimulation::PhysicsSimulation() : cPhysicsDeltaTime(1.0f / 60.0f) {

        // Register allocation hook. Here just malloc / free (overrideable, see Memory.h).
        RegisterDefaultAllocator();

        Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

        // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
        Factory::sInstance = new Factory();

        // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
        // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
        // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
        RegisterTypes();

        temp_allocator = shared_ptr<TempAllocator>(new TempAllocatorImpl(10 * 1024 * 1024));

        job_system = shared_ptr<JobSystemThreadPool>(new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1));

        this->broad_phase_layer_interface = shared_ptr<BPLayerInterfaceImpl>(new BPLayerInterfaceImpl());

        this->object_vs_broadphase_layer_filter = shared_ptr<ObjectVsBroadPhaseLayerFilterImpl>(new ObjectVsBroadPhaseLayerFilterImpl());

        this->object_vs_object_layer_filter = shared_ptr<ObjectLayerPairFilterImpl>(new ObjectLayerPairFilterImpl());

        this->physics_system = shared_ptr<PhysicsSystem>(new PhysicsSystem());
        this->physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);

        this->body_activation_listener = shared_ptr<MyBodyActivationListener>(new MyBodyActivationListener());
        this->physics_system->SetBodyActivationListener(body_activation_listener.get());

        this->contact_listener = shared_ptr<MyContactListener>(new MyContactListener());
        this->physics_system->SetContactListener(contact_listener.get());

        // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
        // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
        BodyInterface& body_interface = this->physics_system->GetBodyInterface();
    }

    PhysicsSimulation::~PhysicsSimulation() {
        
        // each physics object removes and destroys its body when it is destroyed

        // Unregisters all types with the factory and cleans up the default material
        UnregisterTypes();

        // Destroy the factory
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }

    void PhysicsSimulation::setPlayer(Player* newPlayer) {

        if (player != nullptr) {
            player->removePhysicsBody();
        }

        player = newPlayer;

        player->addPhysicsBody();

        // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance for many objects.
        // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
        // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
        this->physics_system->OptimizeBroadPhase();
    }

    void PhysicsSimulation::addScene(Scene* additionalScene) {

        // todo AddBodiesPrepare in BodyInterface for batch adding

        this->scenes.insert(std::pair<string, Scene*>(additionalScene->name, additionalScene));

        for (auto& enemy : additionalScene->enemies)
        {
            enemy->addPhysicsBody();
        }

        for (auto& sceneObject : additionalScene->physicsObjects)
        {
            sceneObject->addPhysicsBody();
        }

        // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance for many objects.
        // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
        // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
        this->physics_system->OptimizeBroadPhase();
    }

    void PhysicsSimulation::removeScene(string name) {
        auto it = this->scenes.find(name);

        if (it != scenes.end()) { // found scene in active scenes
            Scene* sceneToRemove = it->second;

            for (auto& enemy : sceneToRemove->enemies)
            {
                enemy->removePhysicsBody();
            }

            for (auto& sceneObject : sceneToRemove->physicsObjects)
            {
                sceneObject->removePhysicsBody();
            }
        }

        // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance for many objects.
        // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
        // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
        this->physics_system->OptimizeBroadPhase();
    }

    PhysicsSystem* PhysicsSimulation::getPhysicsSystem() {
        return physics_system.get();
    }

    void PhysicsSimulation::simulate() {
        ++this->step;

        BodyInterface& body_interface = this->physics_system->GetBodyInterface();

        // Output current position and velocity of the player, player needs to be set
        RVec3 position = body_interface.GetCenterOfMassPosition(player->getBodyID());
        Vec3 velocity = body_interface.GetLinearVelocity(player->getBodyID());
        cout << "Step " << this->step << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;

        // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
        const int cCollisionSteps = 1;

        // Step the world
        this->physics_system->Update(cPhysicsDeltaTime, cCollisionSteps, temp_allocator.get(), job_system.get());

        player->postSimulation();

        for (auto& nameToScene : this->scenes)
        {
            Scene* scene = nameToScene.second;
            for (auto& enemy : scene->enemies)
            {
                enemy->postSimulation();
            }
        }
    }
}