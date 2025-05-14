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
    vec4 sunDirection;
    vec4 sunColor;
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
        alpha = tex.a * 0.8;
    } else {
        baseColor = fragColor;
        alpha = 0.8;
    }
    // Compute view-space lighting
    // View direction (camera at origin in view space)
    vec3 viewDir = normalize(-fragPosView);
    // Sun direction comes from uniform
    vec3 lightDirView = normalize((ubo.view * vec4(ubo.sunDirection.xyz, 0.0)).xyz);
    // Ambient component
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * baseColor;
    // Use interpolated analytic normal
    vec3 normalView = normalize(fragNormalView);
    // Diffuse component
    float diff = max(dot(normalView, lightDirView), 0.0);
    // Apply sun color to diffuse
    vec3 diffuse = diff * baseColor * ubo.sunColor.rgb;
    // Specular component (Phong reflection, view-dependent)
    float specStrength = 0.00000001;          // moderate mirror-like intensity
    float shininess = 0.1;            // softer, visible highlight
    vec3 reflectDir = reflect(-lightDirView, normalView);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // Apply sun color to specular highlights
    vec3 specular = specStrength * spec * ubo.sunColor.rgb;
    // Combine components
    vec3 color = ambient + diffuse + specular;
    outColor = vec4(color, alpha);
}