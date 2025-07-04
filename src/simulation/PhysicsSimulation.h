#pragma once

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>
#include <map>

#include "objects/ManagedPhysicsEntity.h"
#include "objects/actors/Player.h"
#include "objects/actors/PhysicsPlayer.h"

#include "PhysicsUtils.h"
#include "PhysicsConversions.h"
#include "CollisionSettings.h"
#include "CollisionHandler.h"

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

using namespace std;

using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

namespace physics {
	class PhysicsSimulation {
	public:
		PhysicsSimulation();
		virtual ~PhysicsSimulation();

		PhysicsSystem& getPhysicsSystem();

		void simulate();
		void preSimulation();
		void postSimulation(bool debugPlayer = false, bool debugEnemies = false);

		// We simulate the physics world in discrete time steps. e.g. 60 Hz is a good rate to update the physics system.
		const float cPhysicsDeltaTime = 1.0f / 60.0f;
		const uint maxPhysicsSubSteps = 5;

	private:

		// We need a temp allocator for temporary allocations during the physics update. We're
		// pre-allocating 10 MB to avoid having to do allocations during the physics update.
		// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
		// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
		// malloc / free.
		shared_ptr<TempAllocator> temp_allocator;

		// We need a job system that will execute physics jobs on multiple threads. Typically
		// you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		shared_ptr<JobSystem> job_system;

		// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
		// Note: Use something in the order of 65536.
		const uint cMaxBodies = 65536;

		// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
		const uint cNumBodyMutexes = 0;

		// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
		// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
		// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
		// Note: Use something in the order of 65536.
		const uint cMaxBodyPairs = 65536;

		// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
		// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
		// Note: Use something in the order of 10240.
		const uint cMaxContactConstraints = 10240;

		// Create mapping table from object layer to broadphase layer
		// Note: As this is an interface, PhysicsSystem will take a reference to this so THIS INSTANCE NEEDS TO STAY ALIVE!
		shared_ptr<BPLayerInterfaceImpl> broad_phase_layer_interface;

		// Create class that filters object vs broadphase layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so THIS INSTANCE NEEDS TO STAY ALIVE!
		shared_ptr<ObjectVsBroadPhaseLayerFilterImpl> object_vs_broadphase_layer_filter;

		// Create class that filters object vs object layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so THIS INSTANCE NEEDS TO STAY ALIVE!
		shared_ptr<ObjectLayerPairFilterImpl> object_vs_object_layer_filter;

		PhysicsSystem physics_system;

		// A body activation listener gets notified when bodies activate and go to sleep
		// Note that this is called from a job so whatever you do here needs to be thread safe.
		// Registering one is entirely optional. KEEP THIS ALIVE
		shared_ptr<MyBodyActivationListener> body_activation_listener;

		// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
		// Note that this is called from a job so whatever you do here needs to be thread safe.
		// Registering one is entirely optional. KEEP THIS ALIVE
		shared_ptr<MyContactListener> contact_listener;

		// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
		const int cCollisionSteps = 1;

		uint step = 0;

		// TODO implement this by overwriting JPH::DebugRenderer to get visual output for physics bodies
		// std::unique_ptr<VulkanJoltDebugRenderer> debugRenderer;
		// BodyManager::DrawSettings debugSettings;
		// optional:
		// std::unique_ptr<BodyFilter> debugFilter;

		// TODO load settings with ini reader
	};
}