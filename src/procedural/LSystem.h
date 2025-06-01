#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stack>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace procedural {

	enum class VegetationType {
		Fern
	};

	// L-System grammar rule
	struct LSystemRule {
		char symbol;
		std::string replacement;
		float probability = 1.0f;  // For stochastic L-systems

		LSystemRule(char s, const std::string& r, float p = 1.0f)
			: symbol(s), replacement(r), probability(p) {}
	};

	// Parameters for turtle graphics interpretation
	struct TurtleParameters {
		float stepLength = 1.0f;							   // Length of forward movement
		float angleIncrement = 25.0f;						   // Angle change in degrees
		float radiusDecay = 0.9f;							   // How much radius decreases per level
		float lengthDecay = 0.8f;							   // How much length decreases per level
		float initialRadius = 0.1f;							   // Starting radius for branches
		glm::vec3 initialColor = glm::vec3(0.4f, 0.2f, 0.1f);  // Brown for branches
		glm::vec3 leafColor = glm::vec3(0.2f, 0.8f, 0.3f);	   // Green for leaves
	};

	// Turtle state for interpreting L-system
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

	// Generated geometry from L-system
	struct LSystemGeometry {
		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;
			glm::vec3 normal;
			glm::vec2 uv;
		};

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		VegetationType type = VegetationType::Fern;
	};

	// L-System class for procedural plant generation
	class LSystem {
	   public:
		LSystem();
		~LSystem() = default;

		// Add production rules
		void addRule(char symbol, const std::string& replacement, float probability = 1.0f);
		void setAxiom(const std::string& axiom);

		// Generate string after n iterations
		std::string generate(int iterations) const;

		// Interpret L-system string into 3D geometry using turtle graphics
		LSystemGeometry interpretToGeometry(const std::string& lSystemString,
			const TurtleParameters& params,
			const glm::vec3& startPosition = glm::vec3(0.0f),
			unsigned int seed = 0) const;

		// Predefined plant types
		static LSystem createFern(unsigned int seed = 0);

		// Get/set parameters
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

		// Apply rules to a single character
		std::string applyRules(char symbol) const;

		// Turtle graphics interpretation helpers
		void processSymbol(char symbol, TurtleState& state,
			LSystemGeometry& geometry,
			std::stack<TurtleState>& stateStack,
			const TurtleParameters& params) const;

		void generateCylinder(const glm::vec3& start, const glm::vec3& end,
			float radiusStart, float radiusEnd,
			const glm::vec3& color, LSystemGeometry& geometry,
			int segments = 8) const;

		void generateLeaf(const glm::vec3& position, const glm::vec3& direction,
			const glm::vec3& color, LSystemGeometry& geometry) const;
	};

}  // namespace procedural
