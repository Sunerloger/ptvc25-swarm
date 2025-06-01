#include "LSystem.h"
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
		const float maxDepthForColorTransition = 10.0f;

		switch (symbol) {
			case 'T': {	 // Trunk segment: Move forward and draw
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;
				float endRadius = state.radius * params.radiusDecay;

				float transitionFactor = glm::clamp(static_cast<float>(state.depth) / maxDepthForColorTransition, 0.0f, 1.0f);
				glm::vec3 segmentColor = glm::mix(params.initialColor, params.leafColor, transitionFactor);

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

				float transitionFactor = glm::clamp(static_cast<float>(state.depth) / maxDepthForColorTransition, 0.0f, 1.0f);
				glm::vec3 segmentColor = glm::mix(params.initialColor, params.leafColor, transitionFactor);

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

				float transitionFactor = glm::clamp(static_cast<float>(state.depth) / maxDepthForColorTransition, 0.0f, 1.0f);
				glm::vec3 segmentColor = glm::mix(params.initialColor, params.leafColor, transitionFactor);

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

				float transitionFactor = glm::clamp(static_cast<float>(state.depth) / maxDepthForColorTransition, 0.0f, 1.0f);
				glm::vec3 segmentColor = glm::mix(params.initialColor, params.leafColor, transitionFactor);

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

	LSystem LSystem::createTree(unsigned int seed) {
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
		params.stepLength = 1.0f;
		params.angleIncrement = 30.0f;
		params.radiusDecay = 0.85f;
		params.lengthDecay = 0.99f;
		params.initialRadius = 0.3f;
		params.initialColor = glm::vec3(0.3f, 0.2f, 0.1f);
		params.leafColor = glm::vec3(0.15f, 0.8f, 0.2f);
		tree.setTurtleParameters(params);

		return tree;
	}

}  // namespace procedural
