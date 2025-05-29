#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec4 params1;  // x: hasTexture, yz: textureRepetition, w: maxTessLevel
    vec4 params2;  // x: minTessDistance, y: maxTessDistance, z: heightScale, w: useHeightmapTexture
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
layout(location = 4) out vec2 rawUV;

void main() {
    fragColor = color;
    fragPosWorld = (push.modelMatrix * vec4(position, 1.0)).xyz;
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    
    // tiling texture coordinates
    fragTexCoord = uv * push.params1.yz;  // textureRepetition is in params1.yz
    rawUV = uv;

    gl_Position = vec4(position, 1.0);
}