#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
} ubo;

layout(location = 0) out vec4 outColor;

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

void main() {
    if (push.hasTexture == 1) {
        vec4 tex = texture(texSampler, fragUV);
        // Apply transparency
        outColor = vec4(tex.rgb, tex.a * 0.6);
    } else {
        outColor = vec4(fragColor, 0.6);
    }
}