#version 450

layout(quads, fractional_even_spacing, ccw) in;

// Input from tessellation control shader
layout(location = 0) in vec3 fragColorTesc[];
layout(location = 1) in vec3 fragPosWorldTesc[];
layout(location = 2) in vec3 fragNormalWorldTesc[];
layout(location = 3) in vec2 fragTexCoordTesc[];
layout(location = 4) in vec2 rawUVTesc[];

// Output to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out float fragHeight;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    
    vec4 sunDirection;
    // rgb + intensity in .w
    vec4 sunColor;
    
    // camera position in world space
    vec4 cameraPosition;
} globalUbo;

layout(set = 1, binding = 0) uniform Ubo {
    // x = maxTessLevel, max tessellation subdivisions
    // y = minTessDistance, within minTessDistance the tessellation has maxTessLevels
    // z = maxTessDistance, tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
    // w = heightScale
    vec4 tessParams;

    // xy = textureRepetition, how often the texture repeats across the whole tessellation object
    // z = hasTexture
    // w = useHeightmapTexture
    vec4 textureParams;

    // x: ambient factor, y: diffuse factor, z: specular factor, w: shininess
    vec4 lightingProperties;
} modelUbo;

// optional heightmap texture
layout(set = 1, binding = 4) uniform sampler2D heightMap;

float sampleHeightOffset(vec2 uv) {
    // texture automatically normalized to [0,1]
    float hNorm = texture(heightMap, uv).r;
    return (hNorm * 2.0 - 1.0) * modelUbo.tessParams.w;
}

vec3 samplePosition(float uu, float vv, vec3 normal) {
    vec3 p0 = mix(fragPosWorldTesc[1], fragPosWorldTesc[0], uu);
    vec3 p1 = mix(fragPosWorldTesc[2], fragPosWorldTesc[3], uu);
    vec3 p = mix(p0, p1, vv);

    vec2 t0 = mix(rawUVTesc[1], rawUVTesc[0], uu);
    vec2 t1 = mix(rawUVTesc[2], rawUVTesc[3], uu);
    vec2 t = mix(t0, t1, vv);
    p += normal * sampleHeightOffset(t);

    return p;
}

void main() {

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 t0 = mix(fragTexCoordTesc[1], fragTexCoordTesc[0], u);
    vec2 t1 = mix(fragTexCoordTesc[2], fragTexCoordTesc[3], u);
    fragTexCoord = mix(t0, t1, v);

    vec2 huv0 = mix(rawUVTesc[1], rawUVTesc[0], u);
    vec2 huv1 = mix(rawUVTesc[2], rawUVTesc[3], u);
    vec2 huv = mix(huv0, huv1, v);

    vec3 c0 = mix(fragColorTesc[1], fragColorTesc[0], u);
    vec3 c1 = mix(fragColorTesc[2], fragColorTesc[3], u);
    fragColor = mix(c0, c1, v);

    vec3 p0 = mix(fragPosWorldTesc[1], fragPosWorldTesc[0], u);
    vec3 p1 = mix(fragPosWorldTesc[2], fragPosWorldTesc[3], u);
    vec3 position = mix(p0, p1, v);

    // flat grid, but it could be rotated
    vec3 n0 = mix(fragNormalWorldTesc[1], fragNormalWorldTesc[0], u);
    vec3 n1 = mix(fragNormalWorldTesc[2], fragNormalWorldTesc[3], u);
    vec3 normal = normalize(mix(n0, n1, v));
    
    // Apply displacement mapping if heightmap is available -> otherwise, the texture is a flat grid
    if (modelUbo.textureParams.w > 0.5) {
        fragHeight = sampleHeightOffset(huv);   // heightScale is in params2.z
        position += normal * fragHeight;

        float texelU = 1.0 / textureSize(heightMap, 0).x;
        float texelV = 1.0 / textureSize(heightMap, 0).y;

        float hx0 = sampleHeightOffset(vec2(huv.x - texelU, huv.y));
        float hx1 = sampleHeightOffset(vec2(huv.x + texelU, huv.y));
        float hy0 = sampleHeightOffset(vec2(huv.x, huv.y - texelV));
        float hy1 = sampleHeightOffset(vec2(huv.x, huv.y + texelV));

        vec3 dX = vec3(1.0, hx1 - hx0, 0.0);
        vec3 dY = vec3(0.0, hy1 - hy0, 1.0);

        normal = normalize((push.normalMatrix * vec4(cross(dY, dX), 1.0)).xyz);
    } else {
        fragHeight = 0.0;
    }
    fragNormalWorld = normal;
    fragPosWorld = position;
    
    gl_Position = globalUbo.projection * globalUbo.view * vec4(position, 1.0);
}