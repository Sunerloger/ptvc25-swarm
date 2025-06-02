#version 450

#define MAX_WAVES 32

layout(quads, fractional_even_spacing, ccw) in;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    
    vec4 sunDirection;
    // rgb + a unused
    vec4 sunColor;
    
    // camera position in world space
    vec4 cameraPosition;
} globalUbo;

layout(set = 1, binding = 1) uniform Ubo {
    // x = maxTessLevel, max tessellation subdivisions
    // y = minTessDistance, within minTessDistance the tessellation has maxTessLevels
    // z = maxTessDistance, tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
    // w = unused
    vec4 tessParams;

    // xy = textureRepetition, how often the texture repeats across the whole tessellation object
    // zw = unused
    vec4 textureParams;

    // x = ka, y = kd, z = ks, w = alpha
    vec4 materialProperties;

    // xyz = default color, w = transparency
    vec4 color;

    // x = hasTexture, y = wave count, zw = unused
    vec4 flags;

    // xy = direction, z = steepness in [0,1], w = wavelength
    vec4 waves[MAX_WAVES];
} modelUbo;

layout(push_constant) uniform Push {
    // x = time, yzw = unused
    vec4 timeData;

    mat4 modelMatrix;
    mat4 normalMatrix;
    // x = patchCount, yzw = unused
    vec4 gridInfo;
} push;

layout(location = 0) in vec2 uvTesc[];

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormWorld;

// https://catlikecoding.com/unity/tutorials/flow/waves/
vec3 gerstnerPosition(vec2 direction, float a, float phase){
    return vec3(
        direction.x * a * cos(phase),
        a * sin(phase),
        direction.y * a * cos(phase)
    );
}

vec3 gerstnerTangent(vec2 direction, float steepness, float phase) {
    return vec3(
        -direction.x * direction.x *(steepness * sin(phase)),
        direction.x * (steepness * cos(phase)),
        -direction.x * direction.y *(steepness * sin(phase))
    );
}

vec3 gerstnerBinormal(vec2 direction, float steepness, float phase) {
    return vec3(
        -direction.x * direction.y *(steepness * sin(phase)),
        direction.y * (steepness * cos(phase)),
        -direction.y * direction.y *(steepness * sin(phase))
    );
}

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;

    vec3 pmix0 = mix(p0, p1, u);
    vec3 pmix1 = mix(p2, p3, u);
    vec3 basePos = mix(pmix0, pmix1, v);

    vec2 uv0 = mix(uvTesc[0], uvTesc[1], u);
    vec2 uv1 = mix(uvTesc[2], uvTesc[3], u);
    fragUV = mix(uv0, uv1, v);

    float t = push.timeData.x;
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);
    vec3 p = basePos;
    float numWaves = modelUbo.flags.y;
    if (numWaves > MAX_WAVES) {
        numWaves = MAX_WAVES;
    }

    for (int i = 0; i < numWaves; i++) {
        vec2 direction = normalize(modelUbo.waves[i].xy);
        float steepness = modelUbo.waves[i].z;
        float wavelength = modelUbo.waves[i].w;
        float k = 2.0 * 3.1415926 / wavelength; // wave number
        float c = sqrt(9.81 / k); // wave speed
        float phase = k * dot(direction, p.xz) - k * c * t; // phase of the wave
        float a = steepness / k; // amplitude

        p += gerstnerPosition(direction, a, phase);
        tangent += gerstnerTangent(direction, steepness, phase);
        binormal += gerstnerBinormal(direction, steepness, phase);
    }

    vec3 nW = normalize(cross(binormal, tangent));

    fragPosWorld  = p;
    fragNormWorld = nW;

    gl_Position  = globalUbo.projection * globalUbo.view * vec4(p, 1.0);
}