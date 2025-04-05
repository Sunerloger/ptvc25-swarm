#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace physics {

    // Callback for traces, connect this to your own trace function if you have one
    void TraceImpl(const char* inFMT, ...);

    #ifdef JPH_ENABLE_ASSERTS

    // Callback for asserts, connect this to your own assert handler if you have one
    bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine);

    #endif // JPH_ENABLE_ASSERTS

}