#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec4 params1;  // x: hasTexture, yz: tileScale, w: maxTessLevel
    vec4 params2;  // x: tessDistance, y: minTessDistance, z: heightScale, w: useHeightmapTexture
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    float aspectRatio;
} ubo;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out vec3 viewPosition;

void main() {
    // Pass vertex attributes to tessellation control shader
    fragColor = color;
    fragPosWorld = (push.modelMatrix * vec4(position, 1.0)).xyz;
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    
    // Apply tiling to texture coordinates
    fragTexCoord = uv * push.params1.yz;  // tileScale is in params1.yz
    
    // Pass view position for tessellation distance calculation
    viewPosition = (ubo.inverseView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    
    // No need to compute gl_Position as it will be done in the tessellation evaluation shader
}