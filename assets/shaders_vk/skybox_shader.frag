#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 fragTexCoords;

layout(set = 1, binding = 0) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(skybox, fragTexCoords);
}