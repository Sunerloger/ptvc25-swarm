#include "PhysicsSimulation.h"

#include "../scene/SceneManager.h"

namespace physics {

    PhysicsSimulation::PhysicsSimulation() {

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

        this->physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);

        this->body_activation_listener = shared_ptr<MyBodyActivationListener>(new MyBodyActivationListener());
        this->physics_system.SetBodyActivationListener(body_activation_listener.get());

        this->contact_listener = shared_ptr<MyContactListener>(new MyContactListener());
        this->physics_system.SetContactListener(contact_listener.get());

        // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
        // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
        BodyInterface& body_interface = this->physics_system.GetBodyInterface();

        // debugSettings.mDrawShape = true;
        // debugSettings.mDrawVelocity = true;
    }

    PhysicsSimulation::~PhysicsSimulation() {
        
        // each physics object removes and destroys its body when it is destroyed

        // Unregisters all types with the factory and cleans up the default material
        UnregisterTypes();

        // Destroy the factory
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }

    PhysicsSystem& PhysicsSimulation::getPhysicsSystem() {
        return physics_system;
    }

    void PhysicsSimulation::simulate() {

        ++step;

        physics_system.Update(cPhysicsDeltaTime, cCollisionSteps, temp_allocator.get(), job_system.get());
    }

    // edits should happen via returned pointers/references of scene manager and to physics objects only via locks outside of physics update
    void PhysicsSimulation::preSimulation() {

        SceneManager& sceneManager = SceneManager::getInstance();

        // remove objects before and after the physics step to clean up removed objects due to collisions + something like shooting (before)
        sceneManager.removeStaleObjects();

        if (sceneManager.isBroadPhaseOptimizationNeeded()) {
            // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance for many objects.
            // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
            // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
            physics_system.OptimizeBroadPhase();
        }
    }

    // edits should happen via returned pointers/references of scene manager and to physics objects only via locks outside of physics update
    void PhysicsSimulation::postSimulation(bool debugPlayer, bool debugEnemies) {

        SceneManager& sceneManager = SceneManager::getInstance();

        // objects are not removed in callbacks but before and after the physics step to prevent deadlocks
        sceneManager.removeStaleObjects();

        Player* player = sceneManager.getPlayer();

        // DebugPlayer returns an invalid body ID
        if (player && player->getBodyID() != JPH::BodyID(JPH::BodyID::cInvalidBodyID)) {
            static_cast<physics::PhysicsPlayer*>(player)->postSimulation();
        }

        if (debugPlayer) {
            player->printInfo(step);
        }

        const vector<weak_ptr<Enemy>> enemies = sceneManager.getActiveEnemies();
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

        // TODO Draw bodies
        // physics_system->DrawBodies(this->debugSettings, this->debugRenderer, nullptr);
    }
}