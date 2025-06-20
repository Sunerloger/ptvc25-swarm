#pragma once

#include <glm/glm.hpp>

struct Frustum {
    glm::vec4 planes[6];  // (A,B,C,D) for each plane

    // normalize each plane so length of normal vector ([A,B,C]) = 1
    void normalizePlanes() {
        for (int i = 0; i < 6; i++) {
            float l = glm::length(glm::vec3(planes[i]));
            planes[i] /= l;
        }
    }

    // build from VP
    static Frustum fromMatrix(const glm::mat4& vp) {
        Frustum f;
        // left
        f.planes[0] = vp[3] + vp[0];
        // right
        f.planes[1] = vp[3] - vp[0];
        // bottom
        f.planes[2] = vp[3] + vp[1];
        // top
        f.planes[3] = vp[3] - vp[1];
        // near
        f.planes[4] = vp[3] + vp[2];
        // far
        f.planes[5] = vp[3] - vp[2];
        f.normalizePlanes();
        return f;
    }

    // test an AABB in world space
    bool Frustum::intersectsOBB(
        const glm::vec3& bbMin,
        const glm::vec3& bbMax,
        const glm::mat4& modelMatrix) const
    {
        glm::vec3 e = (bbMax - bbMin) * 0.5f;
        glm::vec3 c_obj = (bbMin + bbMax) * 0.5f;

        glm::vec3 c_world = glm::vec3(modelMatrix * glm::vec4(c_obj, 1.0f));
        glm::mat3 R = glm::mat3(modelMatrix);  // assumes no non-uniform scale; otherwise orthonormalize
        glm::mat3 halfAxes = glm::mat3(
            R[0] * e.x,
            R[1] * e.y,
            R[2] * e.z
        );

        for (int i = 0; i < 6; ++i) {
            const glm::vec4& P = planes[i];
            glm::vec3 n = glm::vec3(P);

            // Compute the projection radius of the half-axes onto this plane’s normal:
            // r = |n*halfAxes[:,0]| + |n*halfAxes[:,1]| + |n*halfAxes[:,2]|
            float r =
                std::abs(glm::dot(n, halfAxes[0])) +
                std::abs(glm::dot(n, halfAxes[1])) +
                std::abs(glm::dot(n, halfAxes[2]));

            // Signed distance from box center to plane:
            float s = glm::dot(n, c_world) + P.w;

            // If the box is completely on the “negative” side of any plane, it’s culled:
            if (s + r < 0.0f) return false;
        }
        return true;
    }
};
