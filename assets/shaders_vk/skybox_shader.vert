#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

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

layout(location = 0) out vec3 fragTexCoords;

void main() {

    fragTexCoords = position;
    
    // create a view matrix with only rotation (no translation)
    mat4 rotationOnlyView = mat4(mat3(ubo.view));
    
    gl_Position = ubo.projection * rotationOnlyView * vec4(position, 1.0);
    
    // place the skybox at the far plane
    gl_Position = gl_Position.xyww;
}