#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

layout (location = 0) out vec3 fragColor;

struct PointLight {
    vec4 position;
    vec4 color;
};

layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int numLights;
    float aspectRatio;
} ubo;


void main() {
    gl_Position.x = position.x / ubo.aspectRatio;
    gl_Position.y = position.y;
    gl_Position.z = position.z;
    gl_Position.w = 1.0;
    fragColor = color;
}