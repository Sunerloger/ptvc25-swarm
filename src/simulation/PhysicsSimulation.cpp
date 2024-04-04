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

        temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);

        job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

        this->broad_phase_layer_interface = new BPLayerInterfaceImpl();

        this->object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();

        this->object_vs_object_layer_filter = new ObjectLayerPairFilterImpl();

        this->physics_system = new PhysicsSystem();
        this->physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);

        this->body_activation_listener = new MyBodyActivationListener();
        this->physics_system->SetBodyActivationListener(body_activation_listener);

        this->contact_listener = new MyContactListener();
        this->physics_system->SetContactListener(contact_listener);

        // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
        // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
        BodyInterface& body_interface = this->physics_system->GetBodyInterface();
    }

    PhysicsSimulation::~PhysicsSimulation() {
        
        // each physics object removes and destroys its body when it is destroyed

        delete contact_listener;
        contact_listener = nullptr;

        delete body_activation_listener;
        body_activation_listener = nullptr;

        delete physics_system;
        physics_system = nullptr;


        delete object_vs_object_layer_filter;
        object_vs_object_layer_filter = nullptr;

        delete object_vs_broadphase_layer_filter;
        object_vs_broadphase_layer_filter = nullptr;

        delete broad_phase_layer_interface;
        broad_phase_layer_interface = nullptr;


        delete job_system;
        job_system = nullptr;
        delete temp_allocator;
        temp_allocator = nullptr;

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
        this->scenes.insert(std::pair<string, Scene*>(additionalScene->name, additionalScene));

        for (auto& sceneObject : additionalScene->staticObjects)
        {
            sceneObject->addPhysicsBody();
        }

        for (auto& sceneObject : additionalScene->dynamicObjects)
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

            for (auto& sceneObject : sceneToRemove->staticObjects)
            {
                sceneObject->removePhysicsBody();
            }

            for (auto& sceneObject : sceneToRemove->dynamicObjects)
            {
                sceneObject->removePhysicsBody();
            }
        }

        // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance for many objects.
        // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
        // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
        this->physics_system->OptimizeBroadPhase();
    }

    BodyInterface* PhysicsSimulation::getBodyInterface() {
        return &physics_system->GetBodyInterface();
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
        this->physics_system->Update(cPhysicsDeltaTime, cCollisionSteps, temp_allocator, job_system);
    }
}