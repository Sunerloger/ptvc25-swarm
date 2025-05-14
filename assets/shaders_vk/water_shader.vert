#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
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
// View-space position for lighting
// View-space position for lighting
layout(location = 2) out vec3 fragPosView;
// View-space normal for lighting
layout(location = 3) out vec3 fragNormalView;

void main() {
    // (Wave calculations moved below using world-space positions)
    // Transform local position to world space
    vec4 worldPos0 = push.modelMatrix * vec4(position, 1.0);
    vec3 planePos = worldPos0.xyz;
    // Compute multi-directional wave in world space
    float wave1 = sin(planePos.x * 20.0 + push.time * 2.0) * 0.3;
    float wave2 = sin(planePos.z * 20.0 + push.time * 1.5) * 0.2;
    float wave3 = sin((planePos.x + planePos.z) * 15.0 + push.time * 1.0) * 0.1;
    float wave = (wave1 + wave2 + wave3) * 3.0;
    // Apply vertical displacement in world space
    vec3 displacedWorld = planePos + vec3(0.0, wave, 0.0);
    // Compute view-space position
    vec4 viewPos = ubo.view * vec4(displacedWorld, 1.0);
    fragPosView = viewPos.xyz;
    // Analytic normal in world space via wave derivatives
    float dYdx = cos(planePos.x * 20.0 + push.time * 2.0) * 20.0 * 0.3
               + cos((planePos.x + planePos.z) * 15.0 + push.time * 1.0) * 15.0 * 0.1;
    float dYdz = cos(planePos.z * 20.0 + push.time * 1.5) * 20.0 * 0.2
               + cos((planePos.x + planePos.z) * 15.0 + push.time * 1.0) * 15.0 * 0.1;
    vec3 normalWorld = normalize(vec3(-dYdx, 1.0, -dYdz));
    // Transform normal to view space (ignore model scale)
    fragNormalView = normalize(mat3(ubo.view) * normalWorld);
    // Final clip position
    gl_Position = ubo.projection * viewPos;
    // Tile UV and scroll
    fragUV = uv * 10.0 + push.uvOffset;
    fragColor = color;
}