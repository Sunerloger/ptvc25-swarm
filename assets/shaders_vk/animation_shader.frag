#version 450

// Input variables from the vertex shader.
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;

// Set 1: Texture sampler.
layout(set = 1, binding = 0) uniform sampler2D texSampler;

// Output color.
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragUV);
    outColor = texColor * vec4(fragColor, 1.0);
}
