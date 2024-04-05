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

layout (push_constant) uniform Push {
    float scale;
    vec3 translation;
} push;

void main() {

    gl_Position.x = push.translation.x + push.scale * position.x / ubo.aspectRatio;
    gl_Position.y = push.translation.y + push.scale * position.y;
    gl_Position.z = push.translation.z + push.scale * position.z;
//    gl_Position.x = push.scale * position.x / ubo.aspectRatio;
//    gl_Position.y = push.scale * position.y;
//    gl_Position.z = push.scale * position.z;
    gl_Position.w = 1.0;
    fragColor = color;
}