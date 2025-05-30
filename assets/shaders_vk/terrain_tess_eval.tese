#version 450

layout(quads, fractional_odd_spacing, ccw) in;

// Input from tessellation control shader
layout(location = 0) in vec3 fragColorTesc[];
layout(location = 1) in vec3 fragPosWorldTesc[];
layout(location = 2) in vec3 fragNormalWorldTesc[];
layout(location = 3) in vec2 fragTexCoordTesc[];
layout(location = 4) in vec2 rawUVTesc[];

// Output to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out float fragHeight;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec4 params1;  // x: hasTexture, yz: textureRepetition, w: maxTessLevel
    vec4 params2;  // x: minTessDistance, y: maxTessDistance, z: heightScale, w: useHeightmapTexture
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

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 t0 = mix(fragTexCoordTesc[1], fragTexCoordTesc[0], u);
    vec2 t1 = mix(fragTexCoordTesc[2], fragTexCoordTesc[3], u);
    fragTexCoord = mix(t0, t1, v);

    vec3 c0 = mix(fragColorTesc[1], fragColorTesc[0], u);
    vec3 c1 = mix(fragColorTesc[2], fragColorTesc[3], u);
    fragColor = mix(c0, c1, v);

    vec3 p0 = mix(fragPosWorldTesc[1], fragPosWorldTesc[0], u);
    vec3 p1 = mix(fragPosWorldTesc[2], fragPosWorldTesc[3], u);
    vec3 position = mix(p0, p1, v);

    vec2 uv0 = mix(rawUVTesc[1], rawUVTesc[0], u);
    vec2 uv1 = mix(rawUVTesc[2], rawUVTesc[3], u);
    vec2 uv = mix(uv0, uv1, v);
    
    // Apply displacement mapping if heightmap is available
    if (push.params2.w > 0.5) {
        // texture automatically normalized to [0,1]
        float hNorm = texture(heightMap, uv).r;
        fragHeight = (hNorm * 2.0 - 1.0) * push.params2.z;   // heightScale is in params2.z
        position.y += fragHeight;
    } else {
        fragHeight = 0.0;
    }
    
    fragPosWorld = position;
    
    gl_Position = ubo.projection * ubo.view * vec4(position, 1.0);
}