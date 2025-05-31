#version 450

layout(location = 0) in vec2  fragUV;
layout(location = 1) in vec3 posWorld;
layout(location = 2) in vec3 normWorld;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    vec4 sunDirection;
    vec4 sunColor;      // rgb + intensity in .w
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;

    // x = time, yzw = unused
    vec4 timeData;
} push;

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

layout(location = 0) out vec4 outColor;

// ------------------------------------------------------------
// phong helper from TU Wien
// ------------------------------------------------------------
vec3 phong( vec3 n, vec3 l, vec3 v,
            vec3 ambientC, float ambientF,
            vec3 diffuseC, float diffuseF,
            vec3 specularC, float specularF,
            float alpha, bool attenuate, vec3 attenuation)
{
    float d = length(l);
    l = normalize(l);
    float att = 1.0;
    if (attenuate) {
        att = 1.0 / (attenuation.x + d*attenuation.y + d*d*attenuation.z);
    }
    float NdotL = max(dot(n, l), 0.0);
    // only lit side contributes
    if (NdotL <= 0.0) return vec3(0.0);
    vec3 r = reflect(-l, n);
    float RdotV = max(dot(r, v), 0.0);
    return ( ambientF * ambientC + diffuseF * diffuseC * NdotL
           + specularF * specularC * pow(RdotV, alpha) )
           * att;
}

void main() {
    // 1) reconstruct view‐space normal & view‐space position
    vec3 Nw = normalize(normWorld);
    vec3 N  = normalize( (push.normalMatrix * vec4(Nw,0.0)).xyz );

    vec3 viewPos = (ubo.view * vec4(posWorld,1.0)).xyz;
    vec3 V = normalize(-viewPos); // toward camera at origin

    // 2) directional‐light direction (sunDirection in view‐space)
    vec3 sunDirVS = normalize((ubo.view * vec4(ubo.sunDirection.xyz, 0.0)).xyz);
    vec3 L = -sunDirVS;

    const vec3 baseColor = (modelUbo.flags.x > 0.5)
        ? texture(texSampler, fragUV + modelUbo.textureParams.zw * push.timeData.x).rgb
        : modelUbo.color.xyz;

    vec3 light = phong(
        N, L, V,
        baseColor, modelUbo.materialProperties.x,
        baseColor, modelUbo.materialProperties.y,
        ubo.sunColor.rgb * ubo.sunColor.w, modelUbo.materialProperties.z,
        modelUbo.materialProperties.w,
        false, vec3(1.0)
    );

    outColor = vec4(light, modelUbo.color.w);
}