#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    // UV offset for texture animation (scroll)
    vec2 uvOffset;
    // Animation time for wave calculation
    float time;
    // Texture usage flag
    int hasTexture;
} push;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragColor;

void main() {
    // Simple wave displacement using elapsed time
    float wave = sin((position.x + position.z) * 5.0 + push.time) * 0.2;
    vec3 displacedPos = position + vec3(0.0, wave, 0.0);
    gl_Position = ubo.projection * ubo.view * push.modelMatrix * vec4(displacedPos, 1.0);
    // Tile UV and scroll
    fragUV = uv * 10.0 + push.uvOffset;
    fragColor = color;
}