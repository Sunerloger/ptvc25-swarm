#pragma once

#include <glm/glm.hpp>
#include <iostream>

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
        
        // Left plane
        f.planes[0].x = vp[0][3] + vp[0][0];
        f.planes[0].y = vp[1][3] + vp[1][0];
        f.planes[0].z = vp[2][3] + vp[2][0];
        f.planes[0].w = vp[3][3] + vp[3][0];
        
        // Right plane
        f.planes[1].x = vp[0][3] - vp[0][0];
        f.planes[1].y = vp[1][3] - vp[1][0];
        f.planes[1].z = vp[2][3] - vp[2][0];
        f.planes[1].w = vp[3][3] - vp[3][0];
        
        // Bottom plane
        f.planes[2].x = vp[0][3] + vp[0][1];
        f.planes[2].y = vp[1][3] + vp[1][1];
        f.planes[2].z = vp[2][3] + vp[2][1];
        f.planes[2].w = vp[3][3] + vp[3][1];
        
        // Top plane
        f.planes[3].x = vp[0][3] - vp[0][1];
        f.planes[3].y = vp[1][3] - vp[1][1];
        f.planes[3].z = vp[2][3] - vp[2][1];
        f.planes[3].w = vp[3][3] - vp[3][1];
        
        // Near plane
        f.planes[4].x = vp[0][3] + vp[0][2];
        f.planes[4].y = vp[1][3] + vp[1][2];
        f.planes[4].z = vp[2][3] + vp[2][2];
        f.planes[4].w = vp[3][3] + vp[3][2];
        
        // Far plane
        f.planes[5].x = vp[0][3] - vp[0][2];
        f.planes[5].y = vp[1][3] - vp[1][2];
        f.planes[5].z = vp[2][3] - vp[2][2];
        f.planes[5].w = vp[3][3] - vp[3][2];
        
        f.normalizePlanes();
        
        return f;
    }

    // test an AABB in world space
    bool intersectsOBB(
        const glm::vec3& bbMin,
        const glm::vec3& bbMax,
        const glm::mat4& modelMatrix) const
    {
        glm::vec3 e = (bbMax - bbMin) * 0.5f;
        glm::vec3 c_obj = (bbMin + bbMax) * 0.5f;

        glm::vec3 c_world = glm::vec3(modelMatrix * glm::vec4(c_obj, 1.0f));
        glm::mat3 R = glm::mat3(modelMatrix);  // Use the model matrix directly, including any scaling
        
        glm::mat3 halfAxes = glm::mat3(
            R[0] * e.x,
            R[1] * e.y,
            R[2] * e.z
        );

        for (int i = 0; i < 6; ++i) {
            const glm::vec4& P = planes[i];
            glm::vec3 n = glm::vec3(P);

            // Compute the projection radius of the half-axes onto this plane's normal:
            // r = |n*halfAxes[:,0]| + |n*halfAxes[:,1]| + |n*halfAxes[:,2]|
            float r =
                std::abs(glm::dot(n, halfAxes[0])) +
                std::abs(glm::dot(n, halfAxes[1])) +
                std::abs(glm::dot(n, halfAxes[2]));

            // Signed distance from box center to plane:
            float s = glm::dot(n, c_world) + P.w;

            // If the box is completely on the "negative" side of any plane, it's culled:
            if (s + r < 0.0f) {
                return false;
            }
        }
        return true;
    }
};
