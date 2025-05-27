#include "LSystem.h"
#include <iostream>
#include <cmath>
#include <algorithm>

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
		if (radiusStart <= 0.001f && radiusEnd <= 0.001f) {
			return;	 // Too small to render
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
			glm::vec3 offsetStart = (right * cosAngle + up * sinAngle) * radiusStart;
			geometry.vertices.push_back({start + offsetStart,
				color,
				glm::normalize(offsetStart),
				glm::vec2(float(i) / segments, 0.0f)});

			// End circle
			glm::vec3 offsetEnd = (right * cosAngle + up * sinAngle) * radiusEnd;
			geometry.vertices.push_back({end + offsetEnd,
				color,
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
		float leafSize = 0.3f;
		glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
		if (glm::length(right) < 0.1f) {
			right = glm::normalize(glm::cross(direction, glm::vec3(1, 0, 0)));
		}
		glm::vec3 up = glm::normalize(glm::cross(right, direction));

		uint32_t startIndex = geometry.vertices.size();

		// Simple quad leaf
		geometry.vertices.insert(geometry.vertices.end(), {{position - right * leafSize - up * leafSize, color, direction, glm::vec2(0, 0)},
															  {position + right * leafSize - up * leafSize, color, direction, glm::vec2(1, 0)},
															  {position + right * leafSize + up * leafSize, color, direction, glm::vec2(1, 1)},
															  {position - right * leafSize + up * leafSize, color, direction, glm::vec2(0, 1)}});

		// Two triangles for the quad
		geometry.indices.insert(geometry.indices.end(), {startIndex, startIndex + 1, startIndex + 2,
															startIndex, startIndex + 2, startIndex + 3});
	}

	// Predefined plant types
	LSystem LSystem::createSimpleTree(unsigned int seed) {
		LSystem tree;
		tree.setAxiom("F");
		tree.addRule('F', "F[+F]F[-F]F");

		TurtleParameters params;
		params.stepLength = 2.0f;
		params.angleIncrement = 30.0f;
		params.radiusDecay = 0.85f;
		params.lengthDecay = 0.75f;
		params.initialRadius = 0.15f;
		tree.setTurtleParameters(params);

		return tree;
	}

	LSystem LSystem::createBush(unsigned int seed) {
		LSystem bush;
		bush.setAxiom("F");
		bush.addRule('F', "F[+F][-F][&F][^F]L");

		TurtleParameters params;
		params.stepLength = 1.0f;
		params.angleIncrement = 45.0f;
		params.radiusDecay = 0.9f;
		params.lengthDecay = 0.9f;
		params.initialRadius = 0.08f;
		bush.setTurtleParameters(params);

		return bush;
	}

	LSystem LSystem::createGrass(unsigned int seed) {
		LSystem grass;
		grass.setAxiom("F");
		grass.addRule('F', "F[+F]L");

		TurtleParameters params;
		params.stepLength = 0.5f;
		params.angleIncrement = 15.0f;
		params.radiusDecay = 0.95f;
		params.lengthDecay = 0.95f;
		params.initialRadius = 0.02f;
		params.leafColor = glm::vec3(0.1f, 0.9f, 0.2f);
		grass.setTurtleParameters(params);

		return grass;
	}

	LSystem LSystem::createFern(unsigned int seed) {
		LSystem fern;
		fern.rng.seed(seed);
		
		// Barnsley Fern-inspired L-system
		fern.setAxiom("X");
		
		// Main frond structure
		fern.addRule('X', "F[++X][--X]F[-FL][+FL]X", 0.7f);
		fern.addRule('X', "F[+++X][---X]F[-FL][+FL]X", 0.3f);
		
		// Branch and leaf generation
		fern.addRule('F', "FF", 0.9f);
		fern.addRule('F', "F[-F][+F]F", 0.1f);
		
		// Leaf placement rules
		fern.addRule('L', "[[L]]", 0.4f);  // Clustered leaves
		fern.addRule('L', "[L]", 0.6f);    // Single leaf
		
		TurtleParameters params;
		params.stepLength = 0.8f;
		params.angleIncrement = 25.0f;
		params.radiusDecay = 0.88f;
		params.lengthDecay = 0.85f;
		params.initialRadius = 0.05f;
		params.leafColor = glm::vec3(0.15f, 0.7f, 0.2f);
		fern.setTurtleParameters(params);

		return fern;
	}

	// VegetationManager implementation
	VegetationManager::VegetationManager() : rng(std::random_device{}()) {}

	std::vector<VegetationManager::VegetationSpawn> VegetationManager::generateVegetationPlacements(
		const std::vector<float>& heightmap,
		int gridSize,
		const glm::vec3& terrainScale,
		const glm::vec3& terrainPosition,
		int vegetationCount,
		unsigned int seed) {
		rng.seed(seed);
		std::vector<VegetationSpawn> spawns;
		spawns.reserve(vegetationCount);

		std::uniform_real_distribution<float> xDist(-terrainScale.x, terrainScale.x);
		std::uniform_real_distribution<float> zDist(-terrainScale.z, terrainScale.z);
		std::uniform_real_distribution<float> typeDist(0.0f, 1.0f);
		std::uniform_int_distribution<int> iterDist(2, 4);
		std::uniform_int_distribution<unsigned int> seedDist(0, 1000000);

		int attempts = 0;
		int maxAttempts = vegetationCount * 3;

		while (spawns.size() < vegetationCount && attempts < maxAttempts) {
			attempts++;

			// Generate random position
			glm::vec3 worldPos(
				terrainPosition.x + xDist(rng),
				0.0f,
				terrainPosition.z + zDist(rng));

			// Get height at this position
			float height = getHeightAtPosition(worldPos, heightmap, gridSize, terrainScale, terrainPosition);
			worldPos.y = terrainPosition.y + height * terrainScale.y;

			// Check if position is suitable for vegetation
			if (!isSuitableForVegetation(worldPos, heightmap, gridSize, terrainScale)) {
				continue;
			}

			// Determine vegetation type based on probabilities
			float typeRand = typeDist(rng);
			float cumulativeProb = 0.0f;

			VegetationSpawn spawn;
			spawn.position = worldPos;
			spawn.iterations = iterDist(rng);
			spawn.seed = seedDist(rng);

			cumulativeProb += treeDensity;
			if (typeRand <= cumulativeProb) {
				spawn.lsystem = LSystem::createSimpleTree();
				spawn.params = spawn.lsystem.getTurtleParameters();
				// Vary tree parameters slightly
				std::uniform_real_distribution<float> scaleDist(0.8f, 1.2f);
				spawn.params.stepLength *= scaleDist(rng);
				spawn.params.initialRadius *= scaleDist(rng);
			} else {
				cumulativeProb += bushDensity;
				if (typeRand <= cumulativeProb) {
					spawn.lsystem = LSystem::createBush();
					spawn.params = spawn.lsystem.getTurtleParameters();
				} else {
					spawn.lsystem = LSystem::createGrass();
					spawn.params = spawn.lsystem.getTurtleParameters();
				}
			}

			spawns.push_back(spawn);
		}

		std::cout << "Generated " << spawns.size() << " vegetation instances" << std::endl;
		return spawns;
	}

	std::vector<LSystemGeometry> VegetationManager::generateAllVegetationGeometry(
		const std::vector<VegetationSpawn>& spawns) {
		std::vector<LSystemGeometry> geometries;
		geometries.reserve(spawns.size());

		for (const auto& spawn : spawns) {
			std::string lsystemString = spawn.lsystem.generate(spawn.iterations);
			LSystemGeometry geometry = spawn.lsystem.interpretToGeometry(
				lsystemString, spawn.params, spawn.position, spawn.seed);
			geometries.push_back(geometry);
		}

		std::cout << "Generated geometry for " << geometries.size() << " vegetation instances" << std::endl;
		return geometries;
	}

	bool VegetationManager::isSuitableForVegetation(const glm::vec3& position,
		const std::vector<float>& heightmap,
		int gridSize,
		const glm::vec3& terrainScale) {
		// Check if position is too close to water level
		if (position.y < -5.0f) {
			return false;
		}

		// Check if slope is too steep by examining neighboring heights
		float currentHeight = getHeightAtPosition(position, heightmap, gridSize, terrainScale, glm::vec3(0));

		// Sample nearby positions to calculate slope
		float sampleDistance = 2.0f;
		glm::vec3 pos1 = position + glm::vec3(sampleDistance, 0, 0);
		glm::vec3 pos2 = position + glm::vec3(-sampleDistance, 0, 0);
		glm::vec3 pos3 = position + glm::vec3(0, 0, sampleDistance);
		glm::vec3 pos4 = position + glm::vec3(0, 0, -sampleDistance);

		float h1 = getHeightAtPosition(pos1, heightmap, gridSize, terrainScale, glm::vec3(0));
		float h2 = getHeightAtPosition(pos2, heightmap, gridSize, terrainScale, glm::vec3(0));
		float h3 = getHeightAtPosition(pos3, heightmap, gridSize, terrainScale, glm::vec3(0));
		float h4 = getHeightAtPosition(pos4, heightmap, gridSize, terrainScale, glm::vec3(0));

		float maxSlope = std::max({std::abs(h1 - currentHeight),
							 std::abs(h2 - currentHeight),
							 std::abs(h3 - currentHeight),
							 std::abs(h4 - currentHeight)}) /
						 sampleDistance;

		// Too steep for vegetation
		if (maxSlope > 0.5f) {
			return false;
		}

		return true;
	}

	float VegetationManager::getHeightAtPosition(const glm::vec3& worldPos,
		const std::vector<float>& heightmap,
		int gridSize,
		const glm::vec3& terrainScale,
		const glm::vec3& terrainPosition) {
		// Convert world position to heightmap coordinates
		float localX = (worldPos.x - terrainPosition.x) / terrainScale.x;
		float localZ = (worldPos.z - terrainPosition.z) / terrainScale.z;

		// Normalize to [0, 1] range
		float u = (localX + 1.0f) * 0.5f;
		float v = (localZ + 1.0f) * 0.5f;

		// Convert to heightmap indices
		float x = u * (gridSize - 1);
		float z = v * (gridSize - 1);

		// Clamp to valid range
		x = std::max(0.0f, std::min(float(gridSize - 1), x));
		z = std::max(0.0f, std::min(float(gridSize - 1), z));

		// Bilinear interpolation
		int x0 = int(x);
		int x1 = std::min(x0 + 1, gridSize - 1);
		int z0 = int(z);
		int z1 = std::min(z0 + 1, gridSize - 1);

		float fx = x - x0;
		float fz = z - z0;

		float h00 = heightmap[z0 * gridSize + x0];
		float h10 = heightmap[z0 * gridSize + x1];
		float h01 = heightmap[z1 * gridSize + x0];
		float h11 = heightmap[z1 * gridSize + x1];

		float h0 = h00 * (1 - fx) + h10 * fx;
		float h1 = h01 * (1 - fx) + h11 * fx;

		return h0 * (1 - fz) + h1 * fz;
	}

}  // namespace procedural
