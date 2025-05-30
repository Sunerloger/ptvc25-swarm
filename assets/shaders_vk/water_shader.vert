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

    // xy = textureRepetition, how often the texture repeats across the whole tessellation object
    // zw = uvOffset, scroll UV coordinates per time unit to animate water surface
    vec4 textureParams;

    mat4 modelMatrix;
    mat4 normalMatrix;

    // x = hasTexture, yzw = unused
    vec4 flags;
} modelUbo;

layout(push_constant) uniform Push {
    // x = time, yzw = unused
    vec4 timeData;
} push;

layout(location = 0) out vec2  fragUV;
layout(location = 1) out vec3  posWorld;
layout(location = 2) out vec3  normWorld;

void main() {
    // 1) Base world‐space position
    vec3 worldPos0 = (modelUbo.modelMatrix * vec4(position, 1.0)).xyz;

    float waveFrequency = 100.0;
    float waveHeight    = 5.0;
    float timeScale1    = 2.0;
    float timeScale2    = 1.5;
    float timeScale3    = 1.0;

    // 2) Three superposed wave components
    float w1 = sin(worldPos0.x * waveFrequency + push.time * timeScale1) * 0.3;
    float w2 = sin(worldPos0.z * waveFrequency + push.time * timeScale2) * 0.2;
    float w3 = sin((worldPos0.x + worldPos0.z) * 15.0 + push.time * timeScale3) * 0.1;

    // 3) Combine and scale vertically
    float baseWave = (w1 + w2 + w3) * 0.5;
    vec3 displaced = worldPos0 + vec3(0.0, baseWave * waveHeight, 0.0);

    // 4) Compute derivatives for analytic normal
    float amp = waveHeight * 0.5;
    float dYdx = ( cos(worldPos0.x * waveFrequency + push.time * timeScale1) * waveFrequency * 0.3
                 + cos((worldPos0.x + worldPos0.z) * 15.0 + push.time * timeScale3) * 15.0 * 0.1 )
                 * amp;
    float dYdz = ( cos(worldPos0.z * waveFrequency + push.time * timeScale2) * waveFrequency * 0.2
                 + cos((worldPos0.x + worldPos0.z) * 15.0 + push.time * timeScale3) * 15.0 * 0.1 )
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