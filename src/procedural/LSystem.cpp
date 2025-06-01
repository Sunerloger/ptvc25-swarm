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
			case 'F':	 // Move forward and draw
			case 'f': {	 // Move forward without drawing
				glm::vec3 newPosition = state.position + state.heading * state.stepLength;

				if (symbol == 'F') {
					// Generate cylinder for branch
					float endRadius = state.radius * params.radiusDecay;
					generateCylinder(state.position, newPosition,
						state.radius, endRadius,
						params.initialColor, geometry);
					state.radius = endRadius;
				}

				state.position = newPosition;
				state.stepLength *= params.lengthDecay;
				break;
			}

			case 'G': {  // Generate small leaflet cylinder
				glm::vec3 leafletEnd = state.position + state.heading * (state.stepLength * 0.6f);
				float leafletRadius = state.radius * 0.3f; // Thinner than main branch
				generateCylinder(state.position, leafletEnd, leafletRadius, leafletRadius * 0.5f, params.leafColor, geometry, 4);
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

			case 'L':  // Generate leaf
				generateLeaf(state.position, state.heading, params.leafColor, geometry);
				break;

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
		glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
		if (glm::length(right) < 0.1f) {
			right = glm::normalize(glm::cross(direction, glm::vec3(1, 0, 0)));
		}
		glm::vec3 up = glm::normalize(glm::cross(right, direction));

		uint32_t startVertexIndex = geometry.vertices.size();

		// Generate vertices
		for (int i = 0; i <= segments; ++i) {
			float angle = 2.0f * M_PI * i / segments;
			float cosAngle = std::cos(angle);
			float sinAngle = std::sin(angle);

			// Start circle
			glm::vec3 offsetStart = (right * cosAngle + up * sinAngle) * actualRadiusStart;

			// Use slightly darker color for stems compared to leaves
			glm::vec3 stemColor = glm::vec3(color.x * 0.7f, color.y * 0.7f, color.z * 0.7f);

			geometry.vertices.push_back({start + offsetStart,
				stemColor,
				glm::normalize(offsetStart),
				glm::vec2(float(i) / segments, 0.0f)});

			// End circle
			glm::vec3 offsetEnd = (right * cosAngle + up * sinAngle) * actualRadiusEnd;
			geometry.vertices.push_back({end + offsetEnd,
				stemColor,
				glm::normalize(offsetEnd),
				glm::vec2(float(i) / segments, 1.0f)});
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
		// Instead of 2D triangular leaves, create small 3D cylindrical leaflets
		// This makes ferns visible from all angles and look more realistic
		
		float leafletLength = 0.15f;
		float leafletRadius = 0.008f;  // Very thin but visible
		int numLeaflets = 4;  // Number of small leaflets per leaf node
		
		// Create multiple small cylindrical leaflets at different angles
		for (int i = 0; i < numLeaflets; i++) {
			float angle = (i * 360.0f / numLeaflets) + (rand() % 60 - 30); // Random variation
			float angleRad = glm::radians(angle);
			
			// Calculate leaflet direction with some random variation
			glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
			if (glm::length(right) < 0.1f) {
				right = glm::normalize(glm::cross(direction, glm::vec3(1, 0, 0)));
			}
			glm::vec3 up = glm::normalize(glm::cross(right, direction));
			
			// Rotate the leaflet direction around the main direction
			glm::vec3 leafletDir = glm::normalize(
				right * std::cos(angleRad) + up * std::sin(angleRad) + direction * 0.3f
			);
			
			// Create end position for the leaflet
			glm::vec3 leafletEnd = position + leafletDir * leafletLength;
			
			// Generate a small cylinder for this leaflet
			generateCylinder(position, leafletEnd, leafletRadius, leafletRadius * 0.5f, color, geometry, 4);
		}
		
		// Add a central small cylinder pointing forward (main leaflet)
		glm::vec3 mainLeafletEnd = position + direction * (leafletLength * 1.2f);
		generateCylinder(position, mainLeafletEnd, leafletRadius * 1.5f, leafletRadius * 0.3f, color, geometry, 4);
	}

	// Predefined plant types

	LSystem LSystem::createFern(unsigned int seed) {
		LSystem fern;
		fern.rng.seed(seed);

		// Improved fern with a strong main stem and side fronds
		// Start with main stem growth, then branch out
		fern.setAxiom("FFFFFF");  // Start with an even stronger main stem

		// Main stem continues growing while producing side fronds
		fern.addRule('F', "FF[+FG][-FG]", 0.4f);      // Main growth with side branches
		fern.addRule('F', "FFF[++FL][--FL]", 0.3f);   // Stronger main stem with complex fronds
		fern.addRule('F', "FF[+G][-G]", 0.2f);        // Simple side leaflets
		fern.addRule('F', "FFF", 0.1f);               // Pure main stem growth for strength
		
		// Leaflet generation - creates realistic frond structures
		fern.addRule('L', "FG[+G][-G]", 0.7f);        // Complex frond with sub-leaflets
		fern.addRule('L', "FGG", 0.3f);               // Simple frond with leaflets

		TurtleParameters params;
		params.stepLength = 0.5f;						 // Large steps for impressive size
		params.angleIncrement = 22.0f;					 // Slightly narrower for more upright growth
		params.radiusDecay = 0.88f;						 // Gradual taper to maintain thick main stem
		params.lengthDecay = 0.92f;						 // Slow length decrease for larger structure
		params.initialRadius = 0.1f;					 // Thick main stem for strong central trunk
		params.initialColor = glm::vec3(0.35f, 0.25f, 0.1f); // Brown stem color for main trunk
		params.leafColor = glm::vec3(0.15f, 0.8f, 0.2f);     // Bright green for fronds
		fern.setTurtleParameters(params);

		return fern;
	}

}  // namespace procedural
