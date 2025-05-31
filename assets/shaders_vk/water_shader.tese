#version 450

layout(quads, fractional_odd_spacing, ccw) in;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    vec4 sunDirection;
    vec4 sunColor;
} globalUbo;

layout(set = 1, binding = 1) uniform Ubo {
    // x = maxTessLevel, max tessellation subdivisions
    // y = minTessDistance, within minTessDistance the tessellation has maxTessLevels
    // z = maxTessDistance, tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
    // w = heightScale, multiplication factor for waves
    vec4 tessParams;

    // x = waveFrequency, y = timeScale1, z = timeScale2, w = timeScale3
    vec4 waveParams1;

    // x = waveAmplitude1, y = waveAmplitude2, z = waveAmplitude3, w = diagonal wave frequency
    vec4 waveParams2;

    // xy = textureRepetition, how often the texture repeats across the whole tessellation object
    // zw = uvOffset, scroll UV coordinates per time unit to animate water surface
    vec4 textureParams;

    // x = ka, y = kd, z = ks, w = shininess
    vec4 materialProperties;

    // xyz = default color, w = transparency
    vec4 color;

    // x = hasTexture, yzw = unused
    vec4 flags;
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

    // three superposed wave components
    float w1 = sin(basePos.x * modelUbo.waveParams1.x + push.timeData.x * modelUbo.waveParams1.y) * modelUbo.waveParams2.x;
    float w2 = sin(basePos.z * modelUbo.waveParams1.x + push.timeData.x * modelUbo.waveParams1.z) * modelUbo.waveParams2.y;
    float w3 = sin((basePos.x + basePos.z) * modelUbo.waveParams2.w + push.timeData.x * modelUbo.waveParams1.w) * modelUbo.waveParams2.z;

    // combine and scale vertically
    float baseWave = (w1 + w2 + w3) * 0.5;
    vec3 displaced = basePos + vec3(0.0, baseWave * modelUbo.tessParams.w, 0.0);

    // compute derivatives for analytic normal
    float amp = modelUbo.tessParams.w * 0.5;
    float dYdx = ( cos(basePos.x * modelUbo.waveParams1.x + push.timeData.x * modelUbo.waveParams1.y) * modelUbo.waveParams1.x * modelUbo.waveParams2.x
                 + cos((basePos.x + basePos.z) * modelUbo.waveParams2.w + push.timeData.x * modelUbo.waveParams1.w) * modelUbo.waveParams2.w * modelUbo.waveParams2.z )
                 * amp;
    float dYdz = ( cos(basePos.z * modelUbo.waveParams1.x + push.timeData.x * modelUbo.waveParams1.z) * modelUbo.waveParams1.x * modelUbo.waveParams2.y
                 + cos((basePos.x + basePos.z) * modelUbo.waveParams2.w + push.timeData.x * modelUbo.waveParams1.w) * modelUbo.waveParams2.w * modelUbo.waveParams2.z )
                 * amp;
    vec3 nW = normalize(vec3(-dYdx, 1.0, -dYdz));

    fragPosWorld  = displaced;
    fragNormWorld = nW;

    gl_Position  = globalUbo.projection * globalUbo.view * vec4(displaced, 1.0);
}