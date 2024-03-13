/*
 * Copyright 2023 TU Wien, Institute of Visual Computing & Human-Centered Technology.
 * This file is part of the GCG Lab Framework and must not be redistributed.
 *
 * Original version created by Lukas Gersthofer and Bernhard Steiner.
 * Vulkan edition created by Johannes Unterguggenberger (junt@cg.tuwien.ac.at).
 */

#include "Geometry.h"
#include "Utils.h"
#include <glm/gtc/constants.hpp>

#undef min
#undef max

// clang-format off
GeometryData createCrosshairGeometry(float size, float thickness, float aspectRatio) {
    GeometryData data;

    // Adjust the thickness and size of the lines based on the aspect ratio
    // to ensure uniform appearance in terms of screen proportions
    float horizontalLineLength = size;
    float verticalLineLength = size * aspectRatio;
    float horizontalLineThickness = thickness * aspectRatio;
    float verticalLineThickness = thickness;

    data.positions = {
            // Vertical line (centered, uses original size)
            glm::vec3(-verticalLineThickness, -verticalLineLength, 0.0f), // Bottom
            glm::vec3(verticalLineThickness, -verticalLineLength, 0.0f),  // Bottom
            glm::vec3(verticalLineThickness, verticalLineLength, 0.0f),   // Top
            glm::vec3(-verticalLineThickness, verticalLineLength, 0.0f),  // Top

            // Horizontal line (adjusted for aspect ratio)
            glm::vec3(-horizontalLineLength, -horizontalLineThickness, 0.0f), // Left
            glm::vec3(horizontalLineLength, -horizontalLineThickness, 0.0f),  // Left
            glm::vec3(horizontalLineLength, horizontalLineThickness, 0.0f),   // Right
            glm::vec3(-horizontalLineLength, horizontalLineThickness, 0.0f)   // Right
    };

    // Indices for two triangles (CCW order) for each line
    data.indices = {
            0, 1, 2, // First Triangle (vertical)
            2, 3, 0, // Second Triangle (vertical)
            4, 5, 6, // First Triangle (horizontal)
            6, 7, 4  // Second Triangle (horizontal)
    };

    return data;
}


//HUD
// clang-format off
GeometryData createHealthBarOutlineGeometry(float width, float height, float health_height, float aspectRatio) {
    GeometryData data;

    float edge = 1.0f;

    // Bottom left triangle from lower line
    data.positions.push_back(glm::vec3(edge, edge, 0.0f)); // Bottom right
    data.positions.push_back(glm::vec3(edge-width, edge, 0.0f)); // Bottom left
    data.positions.push_back(glm::vec3(edge-width,edge-height, 0.0f)); // Top left
    // Top right triangle from lower line
    data.positions.push_back(glm::vec3(edge, edge, 0.0f)); // Bottom right
    data.positions.push_back(glm::vec3(edge-width, edge-height, 0.0f)); // Top left
    data.positions.push_back(glm::vec3(edge, edge-height, 0.0f)); // Top right


    // Bottom left triangle from upper line
    data.positions.push_back(glm::vec3(edge, edge-health_height, 0.0f)); // Bottom right
    data.positions.push_back(glm::vec3(edge-width, edge-health_height, 0.0f)); // Bottom left
    data.positions.push_back(glm::vec3(edge-width, edge-height-health_height, 0.0f)); // Top left
    // Top right triangle from upper line
    data.positions.push_back(glm::vec3(edge, edge-health_height, 0.0f)); // Bottom right
    data.positions.push_back(glm::vec3(edge-width, edge-height-health_height, 0.0f)); // Top left
    data.positions.push_back(glm::vec3(edge, edge-height-health_height, 0.0f)); // Top right


    // Top left triangle from right line
    data.positions.push_back(glm::vec3(edge, edge, 0.0f)); // Bottom right
    data.positions.push_back(glm::vec3(edge, edge-height, 0.0f)); // Bottom left
    data.positions.push_back(glm::vec3(edge-height, edge-height-health_height, 0.0f)); // Top left
    // Bottom right triangle from right line
    data.positions.push_back(glm::vec3(edge, edge, 0.0f)); // Bottom right
    data.positions.push_back(glm::vec3(edge-height, edge-height-health_height, 0.0f)); // Top left
    data.positions.push_back(glm::vec3(edge, edge-height-health_height, 0.0f)); // Top right

    // Top left triangle from left line
    data.positions.push_back(glm::vec3(edge-width, edge, 0.0f)); // Bottom right
    data.positions.push_back(glm::vec3(edge-width+(height/aspectRatio), edge, 0.0f)); // Bottom left
    data.positions.push_back(glm::vec3(edge-width+(height/aspectRatio), edge-height-health_height, 0.0f)); // Top left
    // Bottom right triangle from left line
    data.positions.push_back(glm::vec3(edge-width, edge, 0.0f)); // Bottom right
    data.positions.push_back(glm::vec3(edge-width+(height/aspectRatio), edge-height-health_height, 0.0f)); // Top left
    data.positions.push_back(glm::vec3(edge-width, edge-height-health_height, 0.0f)); // Top right



    for (int i = 0; i < data.positions.size(); i++) {
        data.positions[i] = data.positions[i] + glm::vec3(-2.0f+width+height/aspectRatio, -2.0f+height*2+health_height, 0.0f);
        data.indices.push_back(i);
    }

    return data;
}

//HUD
GeometryData createHealthBarSquareGeometry(float width, float height, float aspectRatio, glm::vec3 translation) {
    GeometryData data;

    float edge = 1.0f;

    // Top right triangle
    data.positions.push_back(glm::vec3(edge, edge, 0.0f) + translation); // Bottom right
    data.positions.push_back(glm::vec3(edge, edge-height, 0.0f) + translation); // Top right
    data.positions.push_back(glm::vec3(edge-width/10, edge-height, 0.0f) + translation); // Top left


    // Bottom left triangle
    data.positions.push_back(glm::vec3(edge, edge, 0.0f) + translation); // Bottom right
    data.positions.push_back(glm::vec3(edge-width/10, edge, 0.0f) + translation); // Bottom left
    data.positions.push_back(glm::vec3(edge-width/10, edge-height, 0.0f) + translation); // Top left





    for (int i = 0; i < data.positions.size(); i++) {
        data.indices.push_back(i);
        data.positions[i] = data.positions[i] + glm::vec3(-2.0f+width+height/aspectRatio, -2.0f+height*2, 0.0f);
    }

    return data;
}


// clang-format off
GeometryData createBoxGeometry(float width, float height, float depth)
{
	GeometryData data;

	data.positions = {
		// front
		glm::vec3(-width / 2.0f, -height / 2.0f,  depth / 2.0f),
		glm::vec3(width / 2.0f, -height / 2.0f,  depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f,  depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f,  depth / 2.0f),
		// back
		glm::vec3(width / 2.0f, -height / 2.0f,  -depth / 2.0f),
		glm::vec3(-width / 2.0f, -height / 2.0f,  -depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f,  -depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f,  -depth / 2.0f),
		// right
		glm::vec3(width / 2.0f, -height / 2.0f,  depth / 2.0f),
		glm::vec3(width / 2.0f, -height / 2.0f,  -depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f,  -depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f,  depth / 2.0f),
		// left
		glm::vec3(-width / 2.0f, -height / 2.0f,  -depth / 2.0f),
		glm::vec3(-width / 2.0f, -height / 2.0f,  depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f,  depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f,  -depth / 2.0f),
		// top
		glm::vec3(-width / 2.0f, height / 2.0f,  -depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f,  depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f,  depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f,  -depth / 2.0f),
		// bottom
		glm::vec3(-width / 2.0f, -height / 2.0f,  -depth / 2.0f),
		glm::vec3(width / 2.0f, -height / 2.0f,  -depth / 2.0f),
		glm::vec3(width / 2.0f, -height / 2.0f,  depth / 2.0f),
		glm::vec3(-width / 2.0f, -height / 2.0f,  depth / 2.0f)
	};

	data.normals = {
		// front
		glm::vec3(0, 0, 1),
		glm::vec3(0, 0, 1),
		glm::vec3(0, 0, 1),
		glm::vec3(0, 0, 1),
		// back
		glm::vec3(0, 0, -1),
		glm::vec3(0, 0, -1),
		glm::vec3(0, 0, -1),
		glm::vec3(0, 0, -1),
		// right
		glm::vec3(1, 0, 0),
		glm::vec3(1, 0, 0),
		glm::vec3(1, 0, 0),
		glm::vec3(1, 0, 0),
		// left
		glm::vec3(-1, 0, 0),
		glm::vec3(-1, 0, 0),
		glm::vec3(-1, 0, 0),
		glm::vec3(-1, 0, 0),
		// top
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0),
		// bottom
		glm::vec3(0, -1, 0),
		glm::vec3(0, -1, 0),
		glm::vec3(0, -1, 0),
		glm::vec3(0, -1, 0)
	};

	data.textureCoordinates = {
		// front
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(1, 0),
		glm::vec2(0, 0),
		// back
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(1, 0),
		glm::vec2(0, 0),
		// right
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(1, 0),
		glm::vec2(0, 0),
		// left
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(1, 0),
		glm::vec2(0, 0),
		// top
		glm::vec2(0, 0),
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(1, 0),
		// bottom
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(1, 0),
		glm::vec2(0, 0)
	};

	data.indices = {
		// front
		0, 1, 2,
		2, 3, 0,
		// back
		4, 5, 6,
		6, 7, 4,
		// right
		8, 9, 10,
		10, 11, 8,
		// left
		12, 13, 14,
		14, 15, 12,
		// top
		16, 17, 18,
		18, 19, 16,
		// bottom
		20, 21, 22,
		22, 23, 20
	};
	return data;
}


GeometryData createCornellBoxGeometry(float width, float height, float depth)
{
	GeometryData data;

	data.positions = {
		// back
		glm::vec3(width / 2.0f, -height / 2.0f, -depth / 2.0f),
		glm::vec3(-width / 2.0f, -height / 2.0f, -depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f, -depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f, -depth / 2.0f),
		// right
		glm::vec3(width / 2.0f, -height / 2.0f, depth / 2.0f),
		glm::vec3(width / 2.0f, -height / 2.0f, -depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f, -depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f, depth / 2.0f),
		// left
		glm::vec3(-width / 2.0f, -height / 2.0f, -depth / 2.0f),
		glm::vec3(-width / 2.0f, -height / 2.0f, depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f, depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f, -depth / 2.0f),
		// top
		glm::vec3(-width / 2.0f, height / 2.0f, -depth / 2.0f),
		glm::vec3(-width / 2.0f, height / 2.0f, depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f, depth / 2.0f),
		glm::vec3(width / 2.0f, height / 2.0f, -depth / 2.0f),
		// bottom
		glm::vec3(-width / 2.0f, -height / 2.0f, -depth / 2.0f),
		glm::vec3(width / 2.0f, -height / 2.0f, -depth / 2.0f),
		glm::vec3(width / 2.0f, -height / 2.0f, depth / 2.0f),
		glm::vec3(-width / 2.0f, -height / 2.0f, depth / 2.0f)
	};

	data.normals = {
		// back
		glm::vec3(0, 0, 1),
		glm::vec3(0, 0, 1),
		glm::vec3(0, 0, 1),
		glm::vec3(0, 0, 1),
		// right
		glm::vec3(-1, 0, 0),
		glm::vec3(-1, 0, 0),
		glm::vec3(-1, 0, 0),
		glm::vec3(-1, 0, 0),
		// left
		glm::vec3(1, 0, 0),
		glm::vec3(1, 0, 0),
		glm::vec3(1, 0, 0),
		glm::vec3(1, 0, 0),
		// top
		glm::vec3(0, -1, 0),
		glm::vec3(0, -1, 0),
		glm::vec3(0, -1, 0),
		glm::vec3(0, -1, 0),
		// bottom
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0)
	};

	glm::vec3 colors[5] = {
		glm::vec3(1.0, 0.0, 0.0),    // left
		glm::vec3(0.0, 1.0, 0.0),    // right
		glm::vec3(0.96, 0.93, 0.85), // top
		glm::vec3(0.64, 0.64, 0.64), // bottom
		glm::vec3(0.76, 0.74, 0.68)  // back
	};

	data.colors = {
			colors[4],
			colors[4],
			colors[4],
			colors[4],

			colors[1],
			colors[1],
			colors[1],
			colors[1],

			colors[0],
			colors[0],
			colors[0],
			colors[0],

			colors[2],
			colors[2],
			colors[2],
			colors[2],

			colors[3],
			colors[3],
			colors[3],
			colors[3]
	};

	data.textureCoordinates = {
		// back
		glm::vec2(1, 1),
		glm::vec2(0, 1),
		glm::vec2(0, 0),
		glm::vec2(1, 0),
		// right
		glm::vec2(0, 0),
		glm::vec2(1, 0),
		glm::vec2(1, 1),
		glm::vec2(0, 1),
		// left
		glm::vec2(0, 0),
		glm::vec2(1, 0),
		glm::vec2(1, 1),
		glm::vec2(0, 1),
		// top
		glm::vec2(0, 1),
		glm::vec2(0, 0),
		glm::vec2(1, 0),
		glm::vec2(1, 1),
		// bottom
		glm::vec2(0, 0),
		glm::vec2(1, 0),
		glm::vec2(1, 1),
		glm::vec2(0, 1)
	};

	data.indices = {
		// back
		2, 1, 0,
		0, 3, 2,
		// right
		6, 5, 4,
		4, 7, 6,
		// left
		10, 9, 8,
		8, 11, 10,
		// top
		14, 13, 12,
		12, 15, 14,
		// bottom
		18, 17, 16,
		16, 19, 18
	};

	return data;
}
// clang-format on

GeometryData createCylinderGeometry(uint32_t segments, float height, float radius) {
    GeometryData data;


    // center vertices
    data.positions.push_back(glm::vec3(0, -height / 2.0f, 0));
    data.normals.push_back(glm::vec3(0, -1, 0));
    data.textureCoordinates.push_back(glm::vec2(0.5f, 0.5f));
    data.positions.push_back(glm::vec3(0, height / 2.0f, 0));
    data.normals.push_back(glm::vec3(0, 1, 0));
    data.textureCoordinates.push_back(glm::vec2(0.5f, 0.5f));

    // circle segments
    float angle_step = 2.0f * glm::pi<float>() / float(segments);
    for (uint32_t i = 0; i < segments; i++) {
        glm::vec3 circlePos = glm::vec3(glm::cos(i * angle_step) * radius, -height / 2.0f, glm::sin(i * angle_step) * radius);

        glm::vec2 squareToCirlceUV = glm::vec2((circlePos.x / radius) * 0.5f + 0.5f, (circlePos.z / radius) * 0.5f + 0.5f);

        // bottom ring vertex
        data.positions.push_back(circlePos);
        data.positions.push_back(circlePos);
        data.normals.push_back(glm::vec3(0, -1, 0));
        data.normals.push_back(glm::normalize(circlePos - glm::vec3(0, -height / 2.0f, 0)));
        data.textureCoordinates.push_back(glm::vec2(squareToCirlceUV.x, 1.0f - squareToCirlceUV.y));
        data.textureCoordinates.push_back(glm::vec2(1.0f - i * angle_step / (2.0f * glm::pi<float>()), 1.0f));

        // top ring vertex
        circlePos.y = height / 2.0f;
        data.positions.push_back(circlePos);
        data.positions.push_back(circlePos);
        data.normals.push_back(glm::vec3(0, 1, 0));
        data.normals.push_back(glm::normalize(circlePos - glm::vec3(0, height / 2.0f, 0)));
        data.textureCoordinates.push_back(squareToCirlceUV);
        data.textureCoordinates.push_back(glm::vec2(1.0f - i * angle_step / (2.0f * glm::pi<float>()), 0.0f));

        // bottom face
        data.indices.push_back(0);
        data.indices.push_back(2 + i * 4);
        data.indices.push_back(i == segments - 1 ? 2 : 2 + (i + 1) * 4);

        // top face
        data.indices.push_back(1);
        data.indices.push_back(i == segments - 1 ? 4 : (i + 2) * 4);
        data.indices.push_back((i + 1) * 4);

        // side faces
        data.indices.push_back(3 + i * 4);
        data.indices.push_back(i == segments - 1 ? 5 : 5 + (i + 1) * 4);
        data.indices.push_back(i == segments - 1 ? 3 : 3 + (i + 1) * 4);

        data.indices.push_back(i == segments - 1 ? 5 : 5 + (i + 1) * 4);
        data.indices.push_back(3 + i * 4);
        data.indices.push_back(5 + i * 4);
    }


    return data;
}

// Function to calculate binomial coefficient (n choose k)
int binomialCoefficient(int n, int k) {
    int result = 1;
    for (int i = 1; i <= k; ++i) {
        result *= (n - i + 1);
        result /= i;
    }
    return result;
}

// Function to calculate a point on the Bezier curve
glm::vec3 calculateBezierPoint(const std::vector<glm::vec3>& controlPoints, float t) {
    int n = controlPoints.size() - 1;
    glm::vec3 point(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= n; ++i) {
        float blend = binomialCoefficient(n, i) * pow(t, i) * pow(1 - t, n - i);
        point += controlPoints[i] * blend;
    }
    return point;
}

// Function to generate a Bezier curve and subdivide it into N segments
std::vector<glm::vec3> generateBezierCurve(const std::vector<glm::vec3>& controlPoints, int numSegments) {
    std::vector<glm::vec3> curvePoints;
    float deltaT = 1.0f / (numSegments);
    for (int i = 0; i <= numSegments; ++i) {
        float t = i * deltaT;
        glm::vec3 point = calculateBezierPoint(controlPoints, t);
        curvePoints.push_back(point);
    }

    return curvePoints;
}

GeometryData createBezierCylinderGeometry(unsigned int segments, std::vector<glm::vec3> controlPoints, unsigned int bezierSegments, float radius) {
    GeometryData data;
    std::vector<glm::vec3> bezierPoints = generateBezierCurve(controlPoints, bezierSegments);
    float v = 0;
    float angleStep = 2.0f * glm::pi<float>() / float(segments);
    for (int point = 0; point < bezierPoints.size(); point++) {
        glm::vec3 forwardAxis;
        if (point >= bezierPoints.size() - 1) {
            forwardAxis = -glm::normalize(bezierPoints[point - 1] - bezierPoints[point]);
        } else {
            forwardAxis = glm::normalize(bezierPoints[point + 1] - bezierPoints[point]);
        }
        glm::vec3 rightAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), forwardAxis));
        glm::vec3 upAxis = glm::normalize(glm::cross(forwardAxis, rightAxis));

        // Circle segments
        unsigned int startIndex = data.positions.size();
        if (point >= bezierPoints.size() - 1) {
            v += glm::min(glm::length(bezierPoints[point - 1] - bezierPoints[point]), 1.0f);
        } else {
            v += glm::min(glm::length(bezierPoints[point + 1] - bezierPoints[point]), 1.0f);
        }
        for (unsigned int i = 0; i < segments; i++) {
            float cosTheta = glm::cos(i * angleStep);
            float sinTheta = glm::sin(i * angleStep);
            glm::vec3 circlePos = bezierPoints[point] + cosTheta * radius * rightAxis + sinTheta * radius * upAxis;
            data.positions.push_back(circlePos);

            data.normals.push_back(circlePos - bezierPoints[point]);
            float u = static_cast<float>(i) / static_cast<float>(segments);
            data.textureCoordinates.push_back(glm::vec2(u, v));
            // Side faces
            if (point < bezierPoints.size() - 1) {
                data.indices.push_back(startIndex + i);
                data.indices.push_back(startIndex + (i + 1) % segments);
                data.indices.push_back(startIndex + segments + (i + 1) % segments);

                data.indices.push_back(startIndex + segments + (i + 1) % segments);
                data.indices.push_back(startIndex + segments + i);
                data.indices.push_back(startIndex + i % segments);
            }
        }
    }
    // top face
    data.positions.push_back(bezierPoints[bezierPoints.size() - 1]);
    data.normals.push_back(bezierPoints[bezierPoints.size() - 1] - bezierPoints[bezierPoints.size() - 2]);
    data.textureCoordinates.push_back(glm::vec2(0.5f, 0.5f));
    glm::vec3 forwardAxis = glm::normalize(bezierPoints[bezierPoints.size() - 1] - bezierPoints[bezierPoints.size() - 2]);
    glm::vec3 rightAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), forwardAxis));
    glm::vec3 upAxis = glm::normalize(glm::cross(forwardAxis, rightAxis));
    int numberpositions = data.positions.size() - 1;
    for (unsigned int i = 0; i <= segments; i++) {
        data.normals.push_back(bezierPoints[bezierPoints.size() - 1] - bezierPoints[bezierPoints.size() - 2]);
        glm::vec3 circlePosFlat = glm::vec3(glm::cos(i * angleStep) * radius, 0, glm::sin(i * angleStep) * radius);
        glm::vec2 squareToCirlceUV = glm::vec2((circlePosFlat.x / radius) * 0.5f + 0.5f, (circlePosFlat.z / radius) * 0.5f + 0.5f);
        data.textureCoordinates.push_back(squareToCirlceUV);
        float cosTheta = glm::cos(i * angleStep);
        float sinTheta = glm::sin(i * angleStep);

        glm::vec3 circlePos = bezierPoints[bezierPoints.size() - 1] + cosTheta * radius * rightAxis + sinTheta * radius * upAxis;
        data.positions.push_back(circlePos);
        data.indices.push_back(numberpositions + (i + 1));
        data.indices.push_back(numberpositions);
        data.indices.push_back(numberpositions + i);
    }

    // Bottom face
    data.positions.push_back(bezierPoints[0]);
    numberpositions = data.positions.size() - 1;
    forwardAxis = glm::normalize(bezierPoints[1] - bezierPoints[0]);
    rightAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), forwardAxis));
    upAxis = glm::normalize(glm::cross(forwardAxis, rightAxis));
    data.normals.push_back(bezierPoints[bezierPoints.size() - 1] - bezierPoints[bezierPoints.size() - 2]);
    data.textureCoordinates.push_back(glm::vec2(0.5f, 0.5f));
    for (unsigned int i = 0; i <= segments; i++) {
        data.normals.push_back(bezierPoints[bezierPoints.size() - 1] - bezierPoints[bezierPoints.size() - 2]);
        glm::vec3 circlePosFlat = glm::vec3(glm::cos(i * angleStep) * radius, 0, glm::sin(i * angleStep) * radius);
        glm::vec2 squareToCirlceUV = glm::vec2((circlePosFlat.x / radius) * 0.5f + 0.5f, (circlePosFlat.z / radius) * 0.5f + 0.5f);
        data.textureCoordinates.push_back(squareToCirlceUV);
        float cosTheta = glm::cos(i * angleStep);
        float sinTheta = glm::sin(i * angleStep);

        glm::vec3 circlePos = bezierPoints[0] + cosTheta * radius * rightAxis + sinTheta * radius * upAxis;
        data.positions.push_back(circlePos);
        data.indices.push_back(numberpositions);
        data.indices.push_back(numberpositions + (i + 1));
        data.indices.push_back(numberpositions + i);
    }
    return data;
}

GeometryData createSphereGeometry(uint32_t longitude_segments, uint32_t latitude_segments, float radius) {
    GeometryData data;


    data.positions.push_back(glm::vec3(0.0f, radius, 0.0f));
    data.positions.push_back(glm::vec3(0.0f, -radius, 0.0f));
    data.normals.push_back(glm::vec3(0.0f, radius, 0.0f));
    data.normals.push_back(glm::vec3(0.0f, -radius, 0.0f));
    data.textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    data.textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));

    // first and last ring
    for (uint32_t j = 0; j < longitude_segments; j++) {
        data.indices.push_back(0);
        data.indices.push_back(j == longitude_segments - 1 ? 2 : (j + 3));
        data.indices.push_back(2 + j);

        data.indices.push_back(2 + (latitude_segments - 2) * longitude_segments + j);
        data.indices.push_back(
            j == longitude_segments - 1 ? 2 + (latitude_segments - 2) * longitude_segments
                                        : 2 + (latitude_segments - 2) * longitude_segments + j + 1
        );
        data.indices.push_back(1);
    }

    // vertices and rings
    for (uint32_t i = 1; i < latitude_segments; i++) {
        float verticalAngle = float(i) * glm::pi<float>() / float(latitude_segments);
        for (uint32_t j = 0; j < longitude_segments; j++) {
            float horizontalAngle = float(j) * 2.0f * glm::pi<float>() / float(longitude_segments);
            glm::vec3 position = glm::vec3(
                radius * glm::sin(verticalAngle) * glm::cos(horizontalAngle),
                radius * glm::cos(verticalAngle),
                radius * glm::sin(verticalAngle) * glm::sin(horizontalAngle)
            );
            data.positions.push_back(position);
            data.normals.push_back(glm::normalize(position));
            data.textureCoordinates.push_back(glm::vec2(1.0f - horizontalAngle / (2.0f * glm::pi<float>()), verticalAngle / glm::pi<float>()));

            if (i == 1)
                continue;

            data.indices.push_back(2 + (i - 1) * longitude_segments + j);
            data.indices.push_back(j == longitude_segments - 1 ? 2 + (i - 2) * longitude_segments : 2 + (i - 2) * longitude_segments + j + 1);
            data.indices.push_back(j == longitude_segments - 1 ? 2 + (i - 1) * longitude_segments : 2 + (i - 1) * longitude_segments + j + 1);

            data.indices.push_back(j == longitude_segments - 1 ? 2 + (i - 2) * longitude_segments : 2 + (i - 2) * longitude_segments + j + 1);
            data.indices.push_back(2 + (i - 1) * longitude_segments + j);
            data.indices.push_back(2 + (i - 2) * longitude_segments + j);
        }
    }

    return data;
}



Geometry createAndUploadIntoGpuMemory(const GeometryData& geometry_data) {
    if (geometry_data.positions.empty()) {
        VKL_EXIT_WITH_ERROR("An empty GeometryData::positions vector has been passed to createAndUploadIntoGpuMemory(...)");
    }
    if (geometry_data.indices.empty()) {
        VKL_EXIT_WITH_ERROR("An empty GeometryData::indices vector has been passed to createAndUploadIntoGpuMemory(...)");
    }

    Geometry result;

    // Create vertex positions buffer and copy data into it:
    size_t positions_buffer_byte_size = geometry_data.positions.size() * sizeof(geometry_data.positions[0]);
    result.positionsBuffer = vklCreateHostCoherentBufferAndUploadData(
        static_cast<const void *>(geometry_data.positions.data()),
        positions_buffer_byte_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

    // Create vertex color buffer and copy data into it:
    result.colorsBuffer = VK_NULL_HANDLE;
    if (geometry_data.colors.size() > 0) {
        size_t colors_buffer_byte_size = geometry_data.colors.size() * sizeof(geometry_data.colors[0]);
        result.colorsBuffer = vklCreateHostCoherentBufferAndUploadData(
            geometry_data.colors.data(), colors_buffer_byte_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    // Create vertex normals buffer and copy data into it:
    size_t normals_buffer_byte_size = geometry_data.normals.size() * sizeof(geometry_data.normals[0]);
    result.normalsBuffer = vklCreateHostCoherentBufferAndUploadData(
        geometry_data.normals.data(), normals_buffer_byte_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    // Create vertex texture coordinates buffer and copy data into it:
    size_t texture_coordinates_buffer_byte_size = geometry_data.textureCoordinates.size() * sizeof(geometry_data.textureCoordinates[0]);
    result.textureCoordinatesBuffer = vklCreateHostCoherentBufferAndUploadData(
        geometry_data.textureCoordinates.data(),
        static_cast<VkDeviceSize>(texture_coordinates_buffer_byte_size),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );
    // Create indices buffer and copy data into it:
    size_t indices_buffer_byte_size = geometry_data.indices.size() * sizeof(geometry_data.indices[0]);
    result.indicesBuffer = vklCreateHostCoherentBufferAndUploadData(
        geometry_data.indices.data(),
        static_cast<VkDeviceSize>(indices_buffer_byte_size),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );
    // Also store the number of indices:
    result.numberOfIndices = static_cast<uint32_t>(geometry_data.indices.size());

    return result;
}

void destroyGeometryGpuMemory(const Geometry& geometry) {
    vklDestroyHostCoherentBufferAndItsBackingMemory(geometry.indicesBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(geometry.textureCoordinatesBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(geometry.normalsBuffer);
    if (geometry.colorsBuffer != VK_NULL_HANDLE) {
        vklDestroyHostCoherentBufferAndItsBackingMemory(geometry.colorsBuffer);
    }
    vklDestroyHostCoherentBufferAndItsBackingMemory(geometry.positionsBuffer);
}

