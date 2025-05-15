#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

struct PointLight {
    vec3 position;
    float intensity;
    float radius;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    vec4 sunDirection;
    vec4 sunColor;
    PointLight pointLights[50];
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    // UV offset for texture animation (scroll)
    vec2 uvOffset;
    // Animation time for wave calculation
    float time;
    // Texture usage flag
    int hasTexture;
} push;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 posWorld;
layout(location = 3) out vec3 normWorld;

void main() {
    // 1) Base world-space position
    vec3 worldPos0 = (push.modelMatrix * vec4(position,1.0)).xyz;

    // 2) Wave displacement in world-space
    float w1 = sin(worldPos0.x * 40.0 + push.time * 2.0) * 0.3;
    float w2 = sin(worldPos0.z * 40.0 + push.time * 1.5) * 0.2;
    float w3 = sin((worldPos0.x + worldPos0.z) * 15.0 + push.time * 1.0) * 0.1;
    float wave = (w1 + w2 + w3) * 0.5;
    vec3 displaced = worldPos0 + vec3(0.0, wave, 0.0);
    // Amplify the wave height
    float waveHeight = 2.0; // Increase this value to make waves higher
    displaced.y += wave * waveHeight;
    // 3) Analytical normal via partial derivatives
    float dYdx = cos(worldPos0.x * 20.0 + push.time*2.0)*20.0*0.3
               + cos((worldPos0.x+worldPos0.z)*15.0+push.time*1.0)*15.0*0.1;
    float dYdz = cos(worldPos0.z * 20.0 + push.time*1.5)*20.0*0.2
               + cos((worldPos0.x+worldPos0.z)*15.0+push.time*1.0)*15.0*0.1;
    vec3 nW = normalize(vec3(-dYdx, 1.0, -dYdz));

    // 4) Output for frag stage
    posWorld  = displaced;
    normWorld = normalize((push.normalMatrix * vec4(nW,0.0)).xyz);
    fragUV    = uv * 10.0 + push.uvOffset;
    fragColor = color;

    // 5) Finally compute clip‐space position
    vec4 viewPos = ubo.view * vec4(displaced,1.0);
    gl_Position = ubo.projection * viewPos;
}