#include "LSystem.h"
#include "TreeMaterial.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace procedural {

	LSystem::LSystem() : rng(std::random_device{}()) {
		turtleParams.stepLength = 1.5f;							   // Default, will be changed by createTree
		turtleParams.angleIncrement = 30.0f;					   // Default, will be changed by createTree
		turtleParams.radiusDecay = 0.85f;						   // Default, will be changed by createTree
		turtleParams.lengthDecay = 0.99f;						   // Default, will be changed by createTree
		turtleParams.initialRadius = 0.5f;						   // Default, will be changed by createTree
		turtleParams.initialColor = glm::vec3(0.5f, 0.3f, 0.15f);  // Default brown
		turtleParams.leafColor = glm::vec3(0.1f, 0.5f, 0.1f);	   // Default green
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
			return std::string(1, symbol);
		}

		const auto& symbolRules = it->second;
		if (symbolRules.empty()) {
			return std::string(1, symbol);
		}

		if (symbolRules.size() == 1) {
			return symbolRules[0].replacement;
		}

		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		float random = dist(rng);
		float cumulative = 0.0f;

		for (const auto& rule : symbolRules) {
			cumulative += rule.probability;
			if (random <= cumulative) {
				return rule.replacement;
			}
		}

		return symbolRules.back().replacement;
	}

	LSystemGeometry LSystem::interpretToGeometry(const std::string& lSystemString,
		const TurtleParameters& params,
		const glm::vec3& startPosition,
		unsigned int seed) const {
		LSystemGeometry geometry;
		std::stack<TurtleState> stateStack;

		TurtleState state;
		state.position = startPosition;
		state.heading = glm::vec3(0.0f, 1.0f, 0.0f);
		state.left = glm::vec3(-1.0f, 0.0f, 0.0f);
		state.up = glm::vec3(0.0f, 0.0f, 1.0f);
		state.radius = params.initialRadius;
		state.stepLength = params.stepLength;
		state.depth = 0;

		rng.seed(seed);

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

				glm::vec3 segmentColor = params.leafColor;	// All segments are leafColor

				generateCylinder(state.position, newPosition,
					state.radius, endRadius,
					segmentColor, geometry);
				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;
			}
			case 'F': {	 // Branch segment: Move forward and draw
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;

				glm::vec3 segmentColor = params.leafColor;	// All segments are leafColor

				generateCylinder(state.position, newPosition,
					state.radius, endRadius,
					segmentColor, geometry);
				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;
			}
			case 'G': {	 // Generic segment: Move forward and draw
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;

				glm::vec3 segmentColor = params.leafColor;	// All segments are leafColor

				generateCylinder(state.position, newPosition,
					state.radius, endRadius,
					segmentColor, geometry);
				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;
			}
			case 'f': {	 // Move forward without drawing
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
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

				glm::vec3 segmentColor = params.leafColor;	// All segments are leafColor

				generateCylinder(state.position, newPosition,
					state.radius, endRadius,
					segmentColor, geometry);
			} break;

			case '|':  // Turn around
				state.heading = -state.heading;
				state.left = -state.left;
				break;

			default:
				break;
		}
	}

	void LSystem::generateCylinder(const glm::vec3& start, const glm::vec3& end,
		float radiusStart, float radiusEnd,
		const glm::vec3& color, LSystemGeometry& geometry,
		int segments) const {
		float minRadius = 0.01f;
		float actualRadiusStart = std::max(radiusStart, minRadius);
		float actualRadiusEnd = std::max(radiusEnd, minRadius);

		if (glm::distance(start, end) < 0.01f) {
			return;	 // Too short to be visible
		}

		glm::vec3 direction = glm::normalize(end - start);
		glm::vec3 cylinder_axis_right;
		glm::vec3 cylinder_axis_up;

		glm::vec3 temp_calc_right = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));	 // Try Y-axis as helper
		if (glm::dot(temp_calc_right, temp_calc_right) < 1e-12f) {						 // If direction is parallel to Y-axis
			temp_calc_right = glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f));
		}
		cylinder_axis_right = glm::normalize(temp_calc_right);
		cylinder_axis_up = glm::normalize(glm::cross(cylinder_axis_right, direction));

		uint32_t startVertexIndex = geometry.vertices.size();

		// Generate vertices
		for (int i = 0; i <= segments; ++i) {
			float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
			float cosAngle = std::cos(angle);
			float sinAngle = std::sin(angle);

			// Start circle
			glm::vec3 offsetStart = (cylinder_axis_right * cosAngle + cylinder_axis_up * sinAngle) * actualRadiusStart;

			geometry.vertices.push_back({start + offsetStart,
				color,						  // Use color directly
				glm::normalize(offsetStart),  // Normal for cylinder side is just the offset direction from center
				glm::vec2(static_cast<float>(i) / segments, 0.0f)});

			// End circle
			glm::vec3 offsetEnd = (cylinder_axis_right * cosAngle + cylinder_axis_up * sinAngle) * actualRadiusEnd;
			geometry.vertices.push_back({end + offsetEnd,
				color,						// Use color directly
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

	TreeGeometry LSystem::interpretToTreeGeometry(const std::string& lSystemString,
		const TurtleParameters& params,
		const glm::vec3& startPosition,
		unsigned int seed) const {
		TreeGeometry treeGeometry;
		std::stack<TurtleState> stateStack;

		TurtleState state;
		state.position = startPosition;
		state.heading = glm::vec3(0.0f, 1.0f, 0.0f);
		state.left = glm::vec3(-1.0f, 0.0f, 0.0f);
		state.up = glm::vec3(0.0f, 0.0f, 1.0f);
		state.radius = params.initialRadius;
		state.stepLength = params.stepLength;
		state.depth = 0;

		rng.seed(seed);

		for (char symbol : lSystemString) {
			processSymbolForTree(symbol, state, treeGeometry, stateStack, params);
		}

		return treeGeometry;
	}

	void LSystem::processSymbolForTree(char symbol, TurtleState& state,
		TreeGeometry& treeGeometry,
		std::stack<TurtleState>& stateStack,
		const TurtleParameters& params) const {
		switch (symbol) {
			case 'T': {	 // Trunk segment: Use bark material
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;

				generateCylinderForMaterial(state.position, newPosition,
					state.radius, endRadius,
					treeGeometry.bark, MaterialType::BARK);

				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;
			}
			case 'F': {	 // Branch segment: Always use bark material for branches
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;

				// All branches use bark material, regardless of thickness
				generateCylinderForMaterial(state.position, newPosition,
					state.radius, endRadius,
					treeGeometry.bark, MaterialType::BARK);

				// Add actual leaf geometry at the end of very thin branches
				if (endRadius < 0.03f) {
					std::cout << "Thin branch detected (radius " << endRadius << ") - adding leaf geometry" << std::endl;
					float leafSize = endRadius * 4.0f;
					generateLeafGeometry(newPosition, state.heading, leafSize,
						treeGeometry.leaves, 5);  // Increased leaf count
				}

				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;
			}
			case 'G': {	 // Generic segment: Use bark material
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;

				generateCylinderForMaterial(state.position, newPosition,
					state.radius, endRadius,
					treeGeometry.bark, MaterialType::BARK);

				state.radius = endRadius;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;
			}
			case 'f': {	 // Move forward without drawing
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
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

			case 'L':  // Leaf/branch end - use leaf geometry instead of cylinder
			{
				std::cout << "Processing 'L' symbol - generating leaf geometry" << std::endl;
				// Generate actual leaf quads at the branch end
				float leafSize = state.radius * 3.0f;  // Scale leaf size with branch radius
				generateLeafGeometry(state.position, state.heading, leafSize,
					treeGeometry.leaves, 5);  // Increased leaf count
			} break;

			case '|':  // Turn around
				state.heading = -state.heading;
				state.left = -state.left;
				break;

			default:
				break;
		}
	}

	void LSystem::generateCylinderForMaterial(const glm::vec3& start, const glm::vec3& end,
		float radiusStart, float radiusEnd,
		MaterialGeometry& geometry, MaterialType materialType,
		int segments) const {
		float minRadius = 0.01f;
		float actualRadiusStart = std::max(radiusStart, minRadius);
		float actualRadiusEnd = std::max(radiusEnd, minRadius);

		if (glm::distance(start, end) < 0.01f) {
			return;	 // Too short to be visible
		}

		glm::vec3 direction = glm::normalize(end - start);
		glm::vec3 cylinder_axis_right;
		glm::vec3 cylinder_axis_up;

		glm::vec3 temp_calc_right = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));	 // Try Y-axis as helper
		if (glm::dot(temp_calc_right, temp_calc_right) < 1e-12f) {						 // If direction is parallel to Y-axis
			temp_calc_right = glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f));
		}
		cylinder_axis_right = glm::normalize(temp_calc_right);
		cylinder_axis_up = glm::normalize(glm::cross(cylinder_axis_right, direction));

		uint32_t startVertexIndex = geometry.vertices.size();

		// Set color based on material type
		glm::vec3 color;
		switch (materialType) {
			case MaterialType::BARK:
				color = glm::vec3(0.4f, 0.2f, 0.1f);  // Brown bark color
				break;
			case MaterialType::LEAF:
				color = glm::vec3(0.2f, 0.6f, 0.2f);  // Green leaf color
				break;
		}

		// Generate vertices
		for (int i = 0; i <= segments; ++i) {
			float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
			float cosAngle = std::cos(angle);
			float sinAngle = std::sin(angle);

			// Start circle
			glm::vec3 offsetStart = (cylinder_axis_right * cosAngle + cylinder_axis_up * sinAngle) * actualRadiusStart;

			geometry.vertices.push_back({start + offsetStart,
				color,						  // Use material-specific color
				glm::normalize(offsetStart),  // Normal for cylinder side is just the offset direction from center
				glm::vec2(static_cast<float>(i) / segments, 0.0f)});

			// End circle
			glm::vec3 offsetEnd = (cylinder_axis_right * cosAngle + cylinder_axis_up * sinAngle) * actualRadiusEnd;
			geometry.vertices.push_back({end + offsetEnd,
				color,						// Use material-specific color
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

	void LSystem::generateLeafGeometry(const glm::vec3& position, const glm::vec3& direction,
		float size, MaterialGeometry& geometry,
		int leafCount) const {
		std::cout << "Generating " << leafCount << " leaf quads at position ("
				  << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

		// Generate multiple leaf quads around the branch end
		for (int leafIndex = 0; leafIndex < leafCount; ++leafIndex) {
			// Create rotation around the branch axis for each leaf
			float rotationAngle = (2.0f * glm::pi<float>() * leafIndex) / leafCount;
			float tiltAngle = 0.3f;	 // Slight random tilt for natural look

			// Create a coordinate system for the leaf
			glm::vec3 leafUp = direction;
			glm::vec3 leafRight = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
			if (glm::length(leafRight) < 0.1f) {
				// Handle case where direction is parallel to Y axis
				leafRight = glm::normalize(glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f)));
			}
			glm::vec3 leafForward = glm::normalize(glm::cross(leafRight, leafUp));

			// Apply rotation around the branch axis
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), rotationAngle, direction);
			leafRight = glm::vec3(rotation * glm::vec4(leafRight, 0.0f));
			leafForward = glm::vec3(rotation * glm::vec4(leafForward, 0.0f));

			// Apply slight tilt for more natural look
			glm::mat4 tilt = glm::rotate(glm::mat4(1.0f), tiltAngle, leafRight);
			leafForward = glm::vec3(tilt * glm::vec4(leafForward, 0.0f));
			leafUp = glm::vec3(tilt * glm::vec4(leafUp, 0.0f));

			// Create leaf quad vertices
			glm::vec3 leafColor = glm::vec3(0.2f, 0.8f, 0.3f);	// Green
			glm::vec3 leafNormal = glm::normalize(leafForward);

			uint32_t startIndex = geometry.vertices.size();

			// Leaf quad corners (positioned at branch end)
			glm::vec3 halfRight = leafRight * size * 0.5f;
			glm::vec3 halfUp = leafUp * size * 0.8f;  // Make leaves slightly elongated

			// Bottom-left
			geometry.vertices.push_back({position - halfRight,
				leafColor,
				leafNormal,
				glm::vec2(0.0f, 0.0f)});

			// Bottom-right
			geometry.vertices.push_back({position + halfRight,
				leafColor,
				leafNormal,
				glm::vec2(1.0f, 0.0f)});

			// Top-right
			geometry.vertices.push_back({position + halfRight + halfUp,
				leafColor,
				leafNormal,
				glm::vec2(1.0f, 1.0f)});

			// Top-left
			geometry.vertices.push_back({position - halfRight + halfUp,
				leafColor,
				leafNormal,
				glm::vec2(0.0f, 1.0f)});

			// Create two triangles for the quad
			geometry.indices.insert(geometry.indices.end(), {startIndex, startIndex + 1, startIndex + 2,
																startIndex, startIndex + 2, startIndex + 3});
		}
	}

	LSystem LSystem::createTree(unsigned int seed) {
		LSystem tree;
		tree.rng.seed(seed);

		// Axiom: Trunk (T) segments followed by branching (F) segments for the crown
		tree.setAxiom("TTTTTFFFFF");

		// Rule for trunk segments: T just becomes T (does not branch)
		tree.addRule('T', "T", 1.0f);

		// Enhanced 3D rules for branching segments (F) - forms a more realistic crown
		// Added 'L' to generate leaves more frequently along branches
		tree.addRule('F', "F[+&FL][-&FL][\\^FL][/^FL]L", 0.4f);	 // 3D branching with pitch and roll, and leaves
		tree.addRule('F', "F[+FL][-FL]L", 0.25f);
		// Traditional left/right split, with leaves
		tree.addRule('F', "FF[+&FL]L", 0.1f);
		// Branch left with pitch down, and leaves
		tree.addRule('F', "FF[-&FL]L", 0.1f);
		// Branch right with pitch down, and leaves
		tree.addRule('F', "FF[\\^FL]L", 0.075f);
		// Branch with roll left and pitch up, and leaves
		tree.addRule('F', "FF[/^FL]L", 0.075f);
		// Branch with roll right and pitch up, and leaves
		tree.addRule('F', "FL", 0.1f);	// Simple branch with a leaf

		TurtleParameters params;
		params.stepLength = 1.0f;
		params.angleIncrement = 25.0f;	// Reduced angle for more subtle branching
		params.radiusDecay = 0.85f;
		params.lengthDecay = 0.99f;
		params.initialRadius = 0.3f;
		params.initialColor = glm::vec3(0.15f, 0.8f, 0.2f);	 // Set to leafColor
		params.leafColor = glm::vec3(0.15f, 0.8f, 0.2f);
		tree.setTurtleParameters(params);

		return tree;
	}

}  // namespace procedural
