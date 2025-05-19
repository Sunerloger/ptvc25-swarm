#version 450

layout(location = 0) in vec3  position;
layout(location = 1) in vec3  color;
layout(location = 2) in vec3  normal;  // unused now
layout(location = 3) in vec2  uv;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    vec4 sunDirection;
    vec4 sunColor;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec2 uvOffset;
    float time;
    int  hasTexture;
} push;

layout(location = 0) out vec2  fragUV;
layout(location = 1) out vec3  fragColor;
layout(location = 2) out vec3  posWorld;
layout(location = 3) out vec3  normWorld;

void main() {
    // 1) Base world‐space position
    vec3 worldPos0 = (push.modelMatrix * vec4(position, 1.0)).xyz;

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
    fragUV    = uv * 1.0 + push.uvOffset;
    fragColor = color;

    // 6) Final clip-space position
    vec4 viewPos = ubo.view * vec4(displaced, 1.0);
    gl_Position  = ubo.projection * viewPos;
}