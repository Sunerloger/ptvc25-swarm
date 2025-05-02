#version 450

// Define patch type (quad patch)
layout(quads, equal_spacing, ccw) in;

// Input from tessellation control shader
layout(location = 0) in vec3 fragColorTesc[];
layout(location = 1) in vec3 fragPosWorldTesc[];
layout(location = 2) in vec3 fragNormalWorldTesc[];
layout(location = 3) in vec2 fragTexCoordTesc[];

// Output to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;

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

// Bilinear interpolation for quad patch
vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2, vec2 v3) {
    vec2 a = mix(v0, v1, gl_TessCoord.x);
    vec2 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2, vec3 v3) {
    vec3 a = mix(v0, v1, gl_TessCoord.x);
    vec3 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

void main() {
    // Interpolate attributes
    fragTexCoord = interpolate2D(fragTexCoordTesc[0], fragTexCoordTesc[1], 
                                fragTexCoordTesc[2], fragTexCoordTesc[3]);
    
    fragColor = interpolate3D(fragColorTesc[0], fragColorTesc[1], 
                             fragColorTesc[2], fragColorTesc[3]);
    
    vec3 position = interpolate3D(fragPosWorldTesc[0], fragPosWorldTesc[1], 
                                 fragPosWorldTesc[2], fragPosWorldTesc[3]);
    
    fragNormalWorld = interpolate3D(fragNormalWorldTesc[0], fragNormalWorldTesc[1], 
                                   fragNormalWorldTesc[2], fragNormalWorldTesc[3]);
    
    // Apply displacement mapping if heightmap is available
    float height = 0.0;
    if (push.params2.w > 0.0 && push.params1.x > 0.0) {  // useHeightmapTexture is in params2.w, hasTexture is in params1.x
        // Sample heightmap and apply displacement along normal
        height = texture(heightMap, fragTexCoord).r * push.params2.z;  // heightScale is in params2.z
        position += fragNormalWorld * height;
    }
    
    // Set world position
    fragPosWorld = position;
    
    // Calculate final position
    gl_Position = ubo.projection * ubo.view * vec4(position, 1.0);
}