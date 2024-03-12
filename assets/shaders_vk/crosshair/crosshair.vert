// Vertex shader
#version 450

layout(location = 0) in vec3 inPosition; // Updated to vec3

void main() {
    gl_Position = vec4(inPosition, 1.0);
}
