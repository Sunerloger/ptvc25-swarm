#version 450

// Define patch type (triangle patch)
layout(triangles, equal_spacing, ccw) in;

// Input from tessellation control shader
layout(location = 0) in vec3 fragColorTesc[];
layout(location = 1) in vec3 fragPosWorldTesc[];
layout(location = 2) in vec3 fragNormalWorldTesc[];
layout(location = 3) in vec2 fragTexCoordTesc[];

// Output to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec2 fragTexCoord;

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

// Optional heightmap texture
layout(set = 1, binding = 1) uniform sampler2D heightMap;

void main() {
    // Interpolate attributes
    // Barycentric interpolation for triangles
    fragTexCoord = fragTexCoordTesc[0] * gl_TessCoord.x +
                   fragTexCoordTesc[1] * gl_TessCoord.y +
                   fragTexCoordTesc[2] * gl_TessCoord.z;

    fragColor = fragColorTesc[0] * gl_TessCoord.x +
                fragColorTesc[1] * gl_TessCoord.y +
                fragColorTesc[2] * gl_TessCoord.z;

    vec3 position = fragPosWorldTesc[0] * gl_TessCoord.x +
                    fragPosWorldTesc[1] * gl_TessCoord.y +
                    fragPosWorldTesc[2] * gl_TessCoord.z;
    
    // Apply displacement mapping if heightmap is available
    float height = 0.0;
    if (push.params2.w > 0.0 && push.params1.x > 0.0) {  // useHeightmapTexture is in params2.w, hasTexture is in params1.x  
        float hNorm = texture(heightMap, fragTexCoord).r;
+       height = (hNorm * 2.0 - 1.0) * push.params2.z;   // heightScale is in params2.z

        vec3 interpNormal = fragNormalWorldTesc[0] * gl_TessCoord.x +
                      fragNormalWorldTesc[1] * gl_TessCoord.y +
                      fragNormalWorldTesc[2] * gl_TessCoord.z;

        position.y += height;
    }
    
    // Set world position
    fragPosWorld = position;
    
    // Calculate final position
    gl_Position = ubo.projection * ubo.view * vec4(position, 1.0);
}