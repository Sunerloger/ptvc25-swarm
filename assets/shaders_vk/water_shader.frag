#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragColor;
// View-space position for lighting
layout(location = 2) in vec3 fragPosView;
// View-space normal for lighting
layout(location = 3) in vec3 fragNormalView;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
} ubo;

layout(location = 0) out vec4 outColor;

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

void main() {
    // Base color and alpha
    vec3 baseColor;
    float alpha;
    if (push.hasTexture == 1) {
        vec4 tex = texture(texSampler, fragUV);
        baseColor = tex.rgb;
        alpha = tex.a * 0.6;
    } else {
        baseColor = fragColor;
        alpha = 0.6;
    }
    // Compute view-space lighting
    // View direction (camera at origin in view space)
    vec3 viewDir = normalize(-fragPosView);
    // Hardcoded sun direction in world space, transformed to view space
    vec3 lightDirWorld = normalize(vec3(0.0, 1.0, 0.0));
    vec3 lightDirView = normalize((ubo.view * vec4(lightDirWorld, 0.0)).xyz);
    // Ambient component
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * baseColor;
    // Use interpolated analytic normal
    vec3 normalView = normalize(fragNormalView);
    // Diffuse component
    float diff = max(dot(normalView, lightDirView), 0.0);
    vec3 diffuse = diff * baseColor;
    // Specular component (Blinn-Phong)
    float specStrength = 0.00001;
    vec3 halfwayDir = normalize(lightDirView + viewDir);
    float shininess = 64.0;
    float spec = pow(max(dot(normalView, halfwayDir), 0.0), shininess);
    vec3 specular = specStrength * spec * vec3(1.0);
    // Combine components
    vec3 color = ambient + diffuse + specular;
    outColor = vec4(color, alpha);
}