#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 uiProjection;
    mat4 view;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    int hasTexture;
    int usePerspectiveProjection;
} push;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragColor;

void main() {
    if(push.usePerspectiveProjection == 1) {
        gl_Position = ubo.projection * push.modelMatrix * vec4(position, 1.0);
    } else {
        gl_Position = ubo.uiProjection * push.modelMatrix * vec4(position, 1.0);
    }
    fragUV = uv;
    fragColor = color;
}