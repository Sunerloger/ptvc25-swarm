#include "PhysicsSimulation.h"

namespace physics {

    PhysicsSimulation::PhysicsSimulation(std::shared_ptr<SceneManager> sceneManager, float cPhysicsDeltaTime = 1.0f / 60.0f) : cPhysicsDeltaTime(cPhysicsDeltaTime), sceneManager(sceneManager) {

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

        this->body_activation_listener = shared_ptr<MyBodyActivationListener>(new MyBodyActivationListener(sceneManager));
        this->physics_system->SetBodyActivationListener(body_activation_listener.get());

        this->contact_listener = shared_ptr<MyContactListener>(new MyContactListener(sceneManager));
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

    std::shared_ptr<PhysicsSystem> PhysicsSimulation::getPhysicsSystem() {
        return physics_system;
    }

    void PhysicsSimulation::simulate() {
        physics_system->Update(cPhysicsDeltaTime, cCollisionSteps, temp_allocator.get(), job_system.get());

        // objects are not removed in callbacks but after the physics step to prevent deadlocks
        sceneManager->removeStaleObjects();
    }

    // edits should happen via returned pointers/references of scene manager and to physics objects only via locks outside of physics update
    void PhysicsSimulation::preSimulation(MovementIntent movementIntent) {

        shared_ptr<Player> player = sceneManager->getPlayer();

        JPH::Vec3 movementDirection = GLMToRVec3(movementIntent.direction);

        // only update if something happened
        if (movementDirection != JPH::Vec3{ 0,0,0 } || movementIntent.jump) {
            player->handleMovement(movementDirection, movementIntent.jump);
        }

        sceneManager->updateEnemies();

        if (sceneManager->isBroadPhaseOptimizationNeeded()) {
            // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance for many objects.
            // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
            // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
            physics_system->OptimizeBroadPhase();
        }
    }

    // edits should happen via returned pointers/references of scene manager and to physics objects only via locks outside of physics update
    void PhysicsSimulation::postSimulation(bool debugPlayer, bool debugEnemies) {

        ++step;

        shared_ptr<Player> player = sceneManager->getPlayer();

        sceneManager->getPlayer()->postSimulation();

        if (debugPlayer) {
            player->printInfo(step);
        }

        const vector<weak_ptr<Enemy>> enemies = sceneManager->getActiveEnemies();
        for (auto& weak_enemy : enemies)
        {
            shared_ptr<Enemy> enemy = weak_enemy.lock();
            if (enemy) {
                enemy->postSimulation();
                if (debugEnemies) {
                    enemy->printInfo(step);
                }
            }
        }
    }
}