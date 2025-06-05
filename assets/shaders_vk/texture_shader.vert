#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec3 fragColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    
    vec4 sunDirection;
    // rgb + intensity in .w
    vec4 sunColor;
    
    // camera position in world space
    vec4 cameraPosition;
} globalUbo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    fragPosWorld = (push.modelMatrix * vec4(position, 1.0)).xyz;
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragUV = uv;
    fragColor = color;
    gl_Position = globalUbo.projection * globalUbo.view * push.modelMatrix * vec4(position, 1.0);
}