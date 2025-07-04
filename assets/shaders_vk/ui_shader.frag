#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    
    vec4 sunDirection;
    // rgb + intensity in .w
    vec4 sunColor;
    
    // camera position in world space
    vec4 cameraPosition;
} ubo;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    int hasTexture;
} push;

void main() {
    vec4 texC = texture(texSampler, fragUV);

    if (push.hasTexture == 1) {
        outColor = vec4(fragColor, 1.0) * texC;
    } else {
        outColor = vec4(fragColor, 1.0);
    }
}