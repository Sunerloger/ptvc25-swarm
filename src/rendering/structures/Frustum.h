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
        
        // Debug output for plane normals
        std::cout << "Frustum planes after creation:" << std::endl;
        for (int i = 0; i < 6; i++) {
            std::cout << "Plane " << i << " normal: ("
                      << f.planes[i].x << ", "
                      << f.planes[i].y << ", "
                      << f.planes[i].z << "), D: "
                      << f.planes[i].w << std::endl;
        }
        
        f.normalizePlanes();
        
        // Debug output for normalized plane normals
        std::cout << "Frustum planes after normalization:" << std::endl;
        for (int i = 0; i < 6; i++) {
            std::cout << "Plane " << i << " normal: ("
                      << f.planes[i].x << ", "
                      << f.planes[i].y << ", "
                      << f.planes[i].z << "), D: "
                      << f.planes[i].w << std::endl;
        }
        
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
        
        // Extract rotation matrix and handle non-uniform scaling by orthonormalizing
        glm::mat3 R = glm::mat3(modelMatrix);
        
        // Orthonormalize the rotation matrix to handle non-uniform scaling
        // Gram-Schmidt process
        R[0] = glm::normalize(R[0]);
        R[1] = glm::normalize(R[1] - glm::dot(R[1], R[0]) * R[0]);
        R[2] = glm::normalize(R[2] - glm::dot(R[2], R[0]) * R[0] - glm::dot(R[2], R[1]) * R[1]);
        
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

            // Debug output for specific objects (limit to avoid spam)
            static int debugCounter = 0;
            if (debugCounter < 10) {
                std::cout << "OBB test - Plane " << i << ": s=" << s << ", r=" << r 
                          << ", s+r=" << (s+r) << ", center=" 
                          << c_world.x << "," << c_world.y << "," << c_world.z 
                          << std::endl;
                debugCounter++;
            }

            // If the box is completely on the "negative" side of any plane, it's culled:
            if (s + r < 0.0f) {
                // Debug output for culled objects
                static int cullCounter = 0;
                if (cullCounter < 10) {
                    std::cout << "Object CULLED by plane " << i << ": s=" << s << ", r=" << r 
                              << ", center=" << c_world.x << "," << c_world.y << "," << c_world.z 
                              << std::endl;
                    cullCounter++;
                }
                return false;
            }
        }
        return true;
    }
};
