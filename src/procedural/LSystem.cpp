#include "LSystem.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace procedural {

	LSystem::LSystem() : rng(std::random_device{}()) {
		// Default turtle parameters
		turtleParams.stepLength = 1.0f;
		turtleParams.angleIncrement = 25.0f;
		turtleParams.radiusDecay = 0.9f;
		turtleParams.lengthDecay = 0.8f;
		turtleParams.initialRadius = 0.1f;
		turtleParams.initialColor = glm::vec3(0.4f, 0.2f, 0.1f);
		turtleParams.leafColor = glm::vec3(0.2f, 0.8f, 0.3f);
	}

	void LSystem::addRule(char symbol, const std::string& replacement, float probability) {
		rules[symbol].emplace_back(symbol, replacement, probability);
	}

	void LSystem::setAxiom(const std::string& axiom) {
		this->axiom = axiom;
	}

	std::string LSystem::generate(int iterations) const {
		std::string current = axiom;

		for (int i = 0; i < iterations; ++i) {
			std::string next = "";
			for (char c : current) {
				next += applyRules(c);
			}
			current = next;
		}

		return current;
	}

	std::string LSystem::applyRules(char symbol) const {
		auto it = rules.find(symbol);
		if (it == rules.end()) {
			// No rule for this symbol, return as-is
			return std::string(1, symbol);
		}

		const auto& symbolRules = it->second;
		if (symbolRules.empty()) {
			return std::string(1, symbol);
		}

		// For stochastic L-systems, choose rule based on probability
		if (symbolRules.size() == 1) {
			return symbolRules[0].replacement;
		}

		// Multiple rules - use probability
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		float random = dist(rng);
		float cumulative = 0.0f;

		for (const auto& rule : symbolRules) {
			cumulative += rule.probability;
			if (random <= cumulative) {
				return rule.replacement;
			}
		}

		// Fallback to last rule
		return symbolRules.back().replacement;
	}

	LSystemGeometry LSystem::interpretToGeometry(const std::string& lSystemString,
		const TurtleParameters& params,
		const glm::vec3& startPosition,
		unsigned int seed) const {
		LSystemGeometry geometry;
		std::stack<TurtleState> stateStack;

		// Initialize turtle state
		TurtleState state;
		state.position = startPosition;
		state.heading = glm::vec3(0.0f, 1.0f, 0.0f);
		state.left = glm::vec3(-1.0f, 0.0f, 0.0f);
		state.up = glm::vec3(0.0f, 0.0f, 1.0f);
		state.radius = params.initialRadius;
		state.stepLength = params.stepLength;
		state.depth = 0;

		// Set seed for consistent generation
		rng.seed(seed);

		// Process each symbol in the L-system string
		for (char symbol : lSystemString) {
			processSymbol(symbol, state, geometry, stateStack, params);
		}

		return geometry;
	}

	void LSystem::processSymbol(char symbol, TurtleState& state,
		LSystemGeometry& geometry,
		std::stack<TurtleState>& stateStack,
		const TurtleParameters& params) const {
		switch (symbol) {
			case 'T': {	 // Trunk segment: Move forward and draw
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;
				generateCylinder(state.position, newPosition,
					state.radius, endRadius,
					params.initialColor, geometry);
				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;	// Added break
			}
			case 'F': {	 // Branch segment: Move forward and draw
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;
				generateCylinder(state.position, newPosition,
					state.radius, endRadius,
					params.initialColor, geometry);
				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;	// Added break
			}
			case 'G': {	 // Generic segment: Move forward and draw
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;
				generateCylinder(state.position, newPosition,
					state.radius, endRadius,
					params.initialColor, geometry);
				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;	// Added break
			}
			case 'f': {	 // Move forward without drawing
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;	 // Still apply length decay
				break;
			}

			case '+':  // Turn left (yaw)
				state.heading = glm::normalize(
					state.heading * std::cos(glm::radians(params.angleIncrement)) +
					state.left * std::sin(glm::radians(params.angleIncrement)));
				state.left = glm::normalize(glm::cross(state.up, state.heading));
				break;

			case '-':  // Turn right (yaw)
				state.heading = glm::normalize(
					state.heading * std::cos(glm::radians(-params.angleIncrement)) +
					state.left * std::sin(glm::radians(-params.angleIncrement)));
				state.left = glm::normalize(glm::cross(state.up, state.heading));
				break;

			case '&':  // Pitch down
				state.heading = glm::normalize(
					state.heading * std::cos(glm::radians(params.angleIncrement)) +
					state.up * std::sin(glm::radians(params.angleIncrement)));
				state.up = glm::normalize(glm::cross(state.heading, state.left));
				break;

			case '^':  // Pitch up
				state.heading = glm::normalize(
					state.heading * std::cos(glm::radians(-params.angleIncrement)) +
					state.up * std::sin(glm::radians(-params.angleIncrement)));
				state.up = glm::normalize(glm::cross(state.heading, state.left));
				break;

			case '\\':	// Roll left
				state.left = glm::normalize(
					state.left * std::cos(glm::radians(params.angleIncrement)) +
					state.up * std::sin(glm::radians(params.angleIncrement)));
				state.up = glm::normalize(glm::cross(state.heading, state.left));
				break;

			case '/':  // Roll right
				state.left = glm::normalize(
					state.left * std::cos(glm::radians(-params.angleIncrement)) +
					state.up * std::sin(glm::radians(-params.angleIncrement)));
				state.up = glm::normalize(glm::cross(state.heading, state.left));
				break;

			case '[':  // Push state
				stateStack.push(state);
				state.depth++;
				break;

			case ']':  // Pop state
				if (!stateStack.empty()) {
					state = stateStack.top();
					stateStack.pop();
				}
				break;

			case 'L':  // Simple branch end (same as F for cylinder-only trees)
			{
				glm::vec3 newPosition = state.position + state.heading * (state.stepLength * 0.5f);
				float endRadius = state.radius * params.radiusDecay * 0.5f;	 // Thinner end
				generateCylinder(state.position, newPosition,
					state.radius, endRadius,
					params.initialColor, geometry);
			} break;

			case '|':  // Turn around
				state.heading = -state.heading;
				state.left = -state.left;
				break;

			default:
				// Ignore unknown symbols
				break;
		}
	}

	void LSystem::generateCylinder(const glm::vec3& start, const glm::vec3& end,
		float radiusStart, float radiusEnd,
		const glm::vec3& color, LSystemGeometry& geometry,
		int segments) const {
		// For fern stems, we need a minimum thickness to be visible
		float minRadius = 0.01f;
		float actualRadiusStart = std::max(radiusStart, minRadius);
		float actualRadiusEnd = std::max(radiusEnd, minRadius);

		// Skip rendering extremely tiny branches
		if (glm::distance(start, end) < 0.01f) {
			return;	 // Too short to be visible
		}

		glm::vec3 direction = glm::normalize(end - start);
		glm::vec3 cylinder_axis_right;
		glm::vec3 cylinder_axis_up;

		// Robustly calculate orthogonal axes for the cylinder caps
		glm::vec3 temp_calc_right = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));	 // Try Y-axis as helper
		if (glm::dot(temp_calc_right, temp_calc_right) < 1e-12f) {						 // If direction is parallel to Y-axis
			// Fallback: use X-axis as helper
			temp_calc_right = glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f));
		}
		cylinder_axis_right = glm::normalize(temp_calc_right);
		cylinder_axis_up = glm::normalize(glm::cross(cylinder_axis_right, direction));

		uint32_t startVertexIndex = geometry.vertices.size();

		// Generate vertices
		for (int i = 0; i <= segments; ++i) {
			float angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(segments);
			float cosAngle = std::cos(angle);
			float sinAngle = std::sin(angle);

			// Start circle
			glm::vec3 offsetStart = (cylinder_axis_right * cosAngle + cylinder_axis_up * sinAngle) * actualRadiusStart;

			// Use slightly darker color for stems compared to leaves
			glm::vec3 stemColor = glm::vec3(color.x * 0.7f, color.y * 0.7f, color.z * 0.7f);

			geometry.vertices.push_back({start + offsetStart,
				stemColor,
				glm::normalize(offsetStart),  // Normal for cylinder side is just the offset direction from center
				glm::vec2(static_cast<float>(i) / segments, 0.0f)});

			// End circle
			glm::vec3 offsetEnd = (cylinder_axis_right * cosAngle + cylinder_axis_up * sinAngle) * actualRadiusEnd;
			geometry.vertices.push_back({end + offsetEnd,
				stemColor,
				glm::normalize(offsetEnd),	// Normal for cylinder side
				glm::vec2(static_cast<float>(i) / segments, 1.0f)});
		}

		// Generate indices
		for (int i = 0; i < segments; ++i) {
			uint32_t bottomLeft = startVertexIndex + i * 2;
			uint32_t bottomRight = startVertexIndex + (i + 1) * 2;
			uint32_t topLeft = bottomLeft + 1;
			uint32_t topRight = bottomRight + 1;

			// Two triangles per quad
			geometry.indices.insert(geometry.indices.end(), {bottomLeft, topLeft, bottomRight,
																bottomRight, topLeft, topRight});
		}
	}

	void LSystem::generateLeaf(const glm::vec3& position, const glm::vec3& direction,
		const glm::vec3& color, LSystemGeometry& geometry) const {
		// Create realistic tree leaves with oval/elliptical shapes

		float leafLength = 0.12f;
		float leafWidth = 0.08f;  // Width for oval leaves
		int numLeaves = 3;		  // Fewer, larger leaves for trees

		// Create a small cluster of leaves at different angles
		for (int i = 0; i < numLeaves; i++) {
			float angle = (i * 120.0f) + (rand() % 40 - 20);  // 120-degree spacing with variation
			float angleRad = glm::radians(angle);

			// Calculate leaf direction with some random variation
			glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
			if (glm::length(right) < 0.1f) {
				right = glm::normalize(glm::cross(direction, glm::vec3(1, 0, 0)));
			}
			glm::vec3 up = glm::normalize(glm::cross(right, direction));

			// Rotate the leaf direction around the main direction
			glm::vec3 leafDir = glm::normalize(
				right * std::cos(angleRad) + up * std::sin(angleRad) + direction * 0.1f);

			// Create oval leaf shape
			generateOvalLeaf(position, leafDir, leafLength, leafWidth, color, geometry);
		}

		// Add a main central leaf
		generateOvalLeaf(position, direction, leafLength * 1.2f, leafWidth * 1.1f, color, geometry);
	}

	void LSystem::generateTriangularLeaflet(const glm::vec3& position, const glm::vec3& direction,
		float length, float width, const glm::vec3& color, LSystemGeometry& geometry) const {
		// Create a triangular leaflet for realistic fern fronds

		// Calculate orthogonal vectors for the leaflet plane
		glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
		if (glm::length(right) < 0.1f) {
			right = glm::normalize(glm::cross(direction, glm::vec3(1, 0, 0)));
		}
		glm::vec3 up = glm::normalize(glm::cross(right, direction));

		// Calculate triangle vertices
		glm::vec3 tip = position + direction * length;			  // Tip of the leaflet
		glm::vec3 baseLeft = position + right * (-width * 0.5f);  // Left base corner
		glm::vec3 baseRight = position + right * (width * 0.5f);  // Right base corner

		// Calculate normal for the triangle (facing towards the up direction mostly)
		glm::vec3 normal = glm::normalize(glm::cross(baseRight - baseLeft, tip - baseLeft));

		// Make sure normal points in a reasonable direction (not facing down)
		if (glm::dot(normal, glm::vec3(0, 1, 0)) < 0) {
			normal = -normal;
		}

		uint32_t startIndex = geometry.vertices.size();

		// Add vertices for the triangle
		geometry.vertices.push_back({tip, color, normal, glm::vec2(0.5f, 1.0f)});		 // Tip
		geometry.vertices.push_back({baseLeft, color, normal, glm::vec2(0.0f, 0.0f)});	 // Base left
		geometry.vertices.push_back({baseRight, color, normal, glm::vec2(1.0f, 0.0f)});	 // Base right

		// Add triangle indices (counter-clockwise for front face)
		geometry.indices.push_back(startIndex);		 // tip
		geometry.indices.push_back(startIndex + 1);	 // base left
		geometry.indices.push_back(startIndex + 2);	 // base right

		// Add the back face for double-sided rendering
		geometry.vertices.push_back({tip, color, -normal, glm::vec2(0.5f, 1.0f)});		  // Tip (back)
		geometry.vertices.push_back({baseLeft, color, -normal, glm::vec2(0.0f, 0.0f)});	  // Base left (back)
		geometry.vertices.push_back({baseRight, color, -normal, glm::vec2(1.0f, 0.0f)});  // Base right (back)

		// Add back face indices (clockwise for back face)
		geometry.indices.push_back(startIndex + 3);	 // tip (back)
		geometry.indices.push_back(startIndex + 5);	 // base right (back)
		geometry.indices.push_back(startIndex + 4);	 // base left (back)
	}

	void LSystem::generateOvalLeaf(const glm::vec3& position, const glm::vec3& direction,
		float length, float width, const glm::vec3& color, LSystemGeometry& geometry) const {
		// Create an oval-shaped leaf for trees using multiple triangles

		// Calculate orthogonal vectors for the leaf plane
		glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
		if (glm::length(right) < 0.1f) {
			right = glm::normalize(glm::cross(direction, glm::vec3(1, 0, 0)));
		}
		glm::vec3 up = glm::normalize(glm::cross(right, direction));

		// Create oval shape using multiple segments
		int segments = 6;
		std::vector<glm::vec3> points;

		// Generate oval points
		for (int i = 0; i <= segments; i++) {
			float t = float(i) / float(segments);
			float angle = t * 2.0f * M_PI;

			// Oval shape (ellipse)
			float x = std::cos(angle) * width * 0.5f;
			float y = std::sin(angle) * length * 0.5f;

			glm::vec3 point = position + right * x + direction * y;
			points.push_back(point);
		}

		// Calculate normal for the leaf
		glm::vec3 normal = glm::normalize(glm::cross(right, direction));
		if (glm::dot(normal, glm::vec3(0, 1, 0)) < 0) {
			normal = -normal;
		}

		uint32_t centerIndex = geometry.vertices.size();

		// Add center vertex
		geometry.vertices.push_back({position, color, normal, glm::vec2(0.5f, 0.5f)});

		// Add perimeter vertices
		for (const auto& point : points) {
			float u = 0.5f + (point.x - position.x) / width;
			float v = 0.5f + glm::dot(point - position, direction) / length;
			geometry.vertices.push_back({point, color, normal, glm::vec2(u, v)});
		}

		// Create triangles from center to perimeter
		for (int i = 0; i < segments; i++) {
			uint32_t curr = centerIndex + 1 + i;
			uint32_t next = centerIndex + 1 + ((i + 1) % (segments + 1));

			// Front face
			geometry.indices.push_back(centerIndex);
			geometry.indices.push_back(curr);
			geometry.indices.push_back(next);
		}

		// Add back face vertices for double-sided rendering
		uint32_t backCenterIndex = geometry.vertices.size();
		geometry.vertices.push_back({position, color, -normal, glm::vec2(0.5f, 0.5f)});

		for (const auto& point : points) {
			float u = 0.5f + (point.x - position.x) / width;
			float v = 0.5f + glm::dot(point - position, direction) / length;
			geometry.vertices.push_back({point, color, -normal, glm::vec2(u, v)});
		}

		// Create back face triangles (reversed winding)
		for (int i = 0; i < segments; i++) {
			uint32_t curr = backCenterIndex + 1 + i;
			uint32_t next = backCenterIndex + 1 + ((i + 1) % (segments + 1));

			// Back face (reversed winding)
			geometry.indices.push_back(backCenterIndex);
			geometry.indices.push_back(next);
			geometry.indices.push_back(curr);
		}
	}

	// Predefined plant types

	LSystem LSystem::createFern(unsigned int seed) {
		LSystem tree;
		tree.rng.seed(seed);

		// Axiom: Trunk (T) segments followed by branching (F) segments for the crown
		tree.setAxiom("TTTTTFFFFF");

		// Rule for trunk segments: T just becomes T (does not branch)
		tree.addRule('T', "T", 1.0f);

		// Rules for branching segments (F) - forms the crown
		tree.addRule('F', "F[+F][-F]", 0.7f);  // High probability left/right split
		tree.addRule('F', "FF[+F]", 0.15f);	   // Branch left only
		tree.addRule('F', "FF[-F]", 0.15f);	   // Branch right only

		TurtleParameters params;
		params.stepLength = 0.5f;							 // Medium steps for good proportion
		params.angleIncrement = 30.0f;						 // Clear branching angles
		params.radiusDecay = 0.8f;							 // More pronounced taper
		params.lengthDecay = 0.85f;							 // Good length reduction
		params.initialRadius = 0.12f;						 // Thicker trunk
		params.initialColor = glm::vec3(0.4f, 0.25f, 0.1f);	 // Brown bark color
		params.leafColor = glm::vec3(0.2f, 0.7f, 0.2f);		 // Green leaves (unused now)
		tree.setTurtleParameters(params);

		return tree;
	}

}  // namespace procedural
