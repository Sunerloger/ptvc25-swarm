#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stack>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "TreeMaterial.h"

namespace procedural {

	using MaterialType = TreeMaterial::MaterialType;
	using MaterialGeometry = TreeGeometry::MaterialGeometry;

	// L-System grammar rule
	struct LSystemRule {
		char symbol;
		std::string replacement;
		float probability = 1.0f;  // For stochastic L-systems

		LSystemRule(char s, const std::string& r, float p = 1.0f)
			: symbol(s), replacement(r), probability(p) {}
	};

	struct TurtleParameters {
		float stepLength = 1.0f;
		float angleIncrement = 25.0f;
		float radiusDecay = 0.9f;
		float lengthDecay = 0.8f;
		float initialRadius = 0.1f;
		glm::vec3 initialColor = glm::vec3(0.4f, 0.2f, 0.1f);
		glm::vec3 leafColor = glm::vec3(0.2f, 0.8f, 0.3f);
	};

	struct TurtleState {
		glm::vec3 position;
		glm::vec3 heading;	// Forward direction
		glm::vec3 left;		// Left direction
		glm::vec3 up;		// Up direction
		float radius;
		float stepLength;
		int depth;

		TurtleState()
			: position(0.0f),
			  heading(0.0f, 1.0f, 0.0f),
			  left(-1.0f, 0.0f, 0.0f),
			  up(0.0f, 0.0f, 1.0f),
			  radius(0.1f),
			  stepLength(1.0f),
			  depth(0) {}
	};

	struct LSystemGeometry {
		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;
			glm::vec3 normal;
			glm::vec2 uv;
		};

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};

	class LSystem {
	   public:
		LSystem();
		~LSystem() = default;

		// Add production rules
		void addRule(char symbol, const std::string& replacement, float probability = 1.0f);
		void setAxiom(const std::string& axiom);

		// Generate string after n iterations
		std::string generate(int iterations) const;

		LSystemGeometry interpretToGeometry(const std::string& lSystemString,
			const TurtleParameters& params,
			const glm::vec3& startPosition = glm::vec3(0.0f),
			unsigned int seed = 0) const;

		// Enhanced method that creates separate geometry for bark and leaves
		TreeGeometry interpretToTreeGeometry(const std::string& lSystemString,
			const TurtleParameters& params,
			const glm::vec3& startPosition = glm::vec3(0.0f),
			unsigned int seed = 0) const;

		static LSystem createTree(unsigned int seed = 0);

		void setTurtleParameters(const TurtleParameters& params) {
			turtleParams = params;
		}
		const TurtleParameters& getTurtleParameters() const {
			return turtleParams;
		}

	   private:
		std::string axiom;
		std::unordered_map<char, std::vector<LSystemRule>> rules;
		TurtleParameters turtleParams;
		mutable std::mt19937 rng;

		std::string applyRules(char symbol) const;

		void processSymbol(char symbol, TurtleState& state,
			LSystemGeometry& geometry,
			std::stack<TurtleState>& stateStack,
			const TurtleParameters& params) const;

		void processSymbolForTree(char symbol, TurtleState& state,
			TreeGeometry& treeGeometry,
			std::stack<TurtleState>& stateStack,
			const TurtleParameters& params) const;

		void generateCylinder(const glm::vec3& start, const glm::vec3& end,
			float radiusStart, float radiusEnd,
			const glm::vec3& color, LSystemGeometry& geometry,
			int segments = 8) const;

		void generateCylinderForMaterial(const glm::vec3& start, const glm::vec3& end,
			float radiusStart, float radiusEnd,
			MaterialGeometry& geometry, MaterialType materialType,
			int segments = 8) const;

		// Generate leaf geometry - creates leaf quads at branch ends
		void generateLeafGeometry(const glm::vec3& position, const glm::vec3& direction,
			float size, MaterialGeometry& geometry,
			int leafCount = 3) const;
	};

}  // namespace procedural
