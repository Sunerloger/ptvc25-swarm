#version 450
layout(location = 0) in vec2 fragUV;
layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    outColor = texture(texSampler, fragUV);
}