#version 450

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
    mat4 modelMatrix;
    mat4 normalMatrix;

    // x = time, yzw = unused
    vec4 timeData;
} push;

layout(location = 0) out vec2  fragUV;
layout(location = 1) out vec3  posWorld;
layout(location = 2) out vec3  normWorld;

void main() {
    // 1) Base world‐space position
    vec3 worldPos0 = (push.modelMatrix * vec4(position, 1.0)).xyz;

    // 2) Three superposed wave components
    float w1 = sin(worldPos0.x * modelUbo.waveParams1.x + push.timeData.x * modelUbo.waveParams1.y) * modelUbo.waveParams2.x;
    float w2 = sin(worldPos0.z * modelUbo.waveParams1.x + push.timeData.x * modelUbo.waveParams1.z) * modelUbo.waveParams2.y;
    float w3 = sin((worldPos0.x + worldPos0.z) * modelUbo.waveParams2.w + push.timeData.x * modelUbo.waveParams1.w) * modelUbo.waveParams2.z;

    // 3) Combine and scale vertically
    float baseWave = (w1 + w2 + w3) * 0.5;
    vec3 displaced = worldPos0 + vec3(0.0, baseWave * modelUbo.tessParams.w, 0.0);

    // 4) Compute derivatives for analytic normal
    float amp = modelUbo.tessParams.w * 0.5;
    float dYdx = ( cos(worldPos0.x * modelUbo.waveParams1.x + push.timeData.x * modelUbo.waveParams1.y) * modelUbo.waveParams1.x * modelUbo.waveParams2.x
                 + cos((worldPos0.x + worldPos0.z) * modelUbo.waveParams2.w + push.timeData.x * modelUbo.waveParams1.w) * modelUbo.waveParams2.w * modelUbo.waveParams2.z )
                 * amp;
    float dYdz = ( cos(worldPos0.z * modelUbo.waveParams1.x + push.timeData.x * modelUbo.waveParams1.z) * modelUbo.waveParams1.x * modelUbo.waveParams2.y
                 + cos((worldPos0.x + worldPos0.z) * modelUbo.waveParams2.w + push.timeData.x * modelUbo.waveParams1.w) * modelUbo.waveParams2.w * modelUbo.waveParams2.z )
                 * amp;
    vec3 nW = normalize(vec3(-dYdx, 1.0, -dYdz));

    // 5) Pass to fragment shader
    posWorld  = displaced;
    normWorld = nW;
    fragUV    = uv * 1.0 + modelUbo.textureParams.zw * push.timeData.x;

    // 6) Final clip-space position
    vec4 viewPos = globalUbo.view * vec4(displaced, 1.0);
    gl_Position  = globalUbo.projection * viewPos;
}