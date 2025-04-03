#version 450

// Vertex input attributes.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in ivec4 inJointIndices;
layout(location = 5) in vec4 inJointWeights;

// Set 0: Global UBO.
layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 projection;
    mat4 view;
} ubo;

// Set 2: Animation data (joint matrices for skinning).
layout(set = 2, binding = 0) uniform AnimData {
    mat4 jointMatrices[100]; // Adjust size as needed.
} animData;

// Push constants: model and normal matrices.
layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

// Outputs to fragment shader.
layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;

void main() {
    // Compute the skinning matrix as the weighted sum of joint matrices.
    mat4 skinMatrix =
         inJointWeights.x * animData.jointMatrices[inJointIndices.x] +
         inJointWeights.y * animData.jointMatrices[inJointIndices.y] +
         inJointWeights.z * animData.jointMatrices[inJointIndices.z] +
         inJointWeights.w * animData.jointMatrices[inJointIndices.w];

    // Apply skinning to the vertex position.
    vec4 skinnedPos = skinMatrix * vec4(inPosition, 1.0);

    // Transform to world space.
    vec4 worldPos = push.modelMatrix * skinnedPos;

    // Compute final clip space position.
    gl_Position = ubo.projection * ubo.view * worldPos;

    fragUV = inUV;
    fragColor = inColor;
    // Transform the normal with the normal matrix.
    fragNormal = mat3(push.normalMatrix) * inNormal;
}
