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

    // test an AABB in *world* space
    bool intersectsAABB(const glm::vec3& bbMin,
        const glm::vec3& bbMax,
        const glm::mat4& modelMatrix) const
    {
        // for each plane, pick the “most outside” corner
        for (int i = 0; i < 6; i++) {
            const glm::vec4& P = planes[i];
            // choose the corner that’s farthest in plane normal direction
            glm::vec3 corner = {
              (P.x >= 0 ? bbMax.x : bbMin.x),
              (P.y >= 0 ? bbMax.y : bbMin.y),
              (P.z >= 0 ? bbMax.z : bbMin.z)
            };
            // transform into world
            glm::vec3 worldCorner = glm::vec3(modelMatrix * glm::vec4(corner, 1.0f));
            // plane test: A*x + B*y + C*z + D < 0 means fully outside
            if (glm::dot(glm::vec3(P), worldCorner) + P.w < 0.0f)
                return false;
        }
        return true;
    }
};
