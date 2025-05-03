#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec4 params1;  // x: hasTexture, yz: tileScale, w: maxTessLevel
    vec4 params2;  // x: tessDistance, y: minTessDistance, z: heightScale, w: useHeightmapTexture
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    float aspectRatio;
} ubo;

// Terrain texture
layout(set = 1, binding = 0) uniform sampler2D texSampler;

// Simple lighting calculation
vec3 calculateLighting(vec3 normal, vec3 color) {
    // Simple directional light from above
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    // Ambient light
    float ambient = 0.3;
    
    // Calculate final lighting
    return color * (diffuse + ambient);
}

void main() {

    vec3 dpdx = dFdx(fragPosWorld);
    vec3 dpdy = dFdy(fragPosWorld);
    vec3 adjustedNormal = normalize(cross(dpdx, dpdy)); // after tessellation

    // Get base color (from texture or vertex color)
    vec3 color = fragColor;
    if (push.params1.x > 0.0) {  // hasTexture is in params1.x
        color = texture(texSampler, fragTexCoord).rgb;
    }
    
    // Apply lighting
    vec3 litColor = calculateLighting(normalize(adjustedNormal), color);
    
    // Output final color
    outColor = vec4(litColor, 1.0);
}