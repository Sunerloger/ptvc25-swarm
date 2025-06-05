#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in float fragHeight;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    
    vec4 sunDirection;
    // rgb + intensity in .w
    vec4 sunColor;
    
    // camera position in world space
    vec4 cameraPosition;
} globalUbo;

// Terrain textures
layout(set = 1, binding = 0) uniform Ubo {
    // x = maxTessLevel, max tessellation subdivisions
    // y = minTessDistance, within minTessDistance the tessellation has maxTessLevels
    // z = maxTessDistance, tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
    // w = heightScale
    vec4 tessParams;

    // xy = textureRepetition, how often the texture repeats across the whole tessellation object
    // z = hasTexture
    // w = useHeightmapTexture
    vec4 textureParams;

    // x: ambient factor, y: diffuse factor, z: specular factor, w: shininess
    vec4 lightingProperties;
} modelUbo;

layout(set = 1, binding = 1) uniform sampler2D rockTexture;
layout(set = 1, binding = 2) uniform sampler2D grassTexture;
layout(set = 1, binding = 3) uniform sampler2D snowTexture;

vec3 phong(vec3 n, vec3 l, vec3 v,
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

    vec3 r = reflect(-l, n);
    
    return (diffuseF * diffuseC * max(0, dot(n, l)) + specularF * specularC * pow(max(0, dot(r, v)), alpha)) * att;
}

vec3 clampedReflect(vec3 I, vec3 N)
{
    return I - 2.0 * min(dot(N, I), 0.0) * N;
}

void main() {
    vec3 N = normalize(fragNormalWorld);
    vec3 V = normalize(fragPosWorld - globalUbo.cameraPosition.xyz);

    float normalizedHeight = clamp((fragHeight + modelUbo.tessParams.w) / (2.0 * modelUbo.tessParams.w), 0.0, 1.0);
    
    float wave = sin(fragPosWorld.x * 0.1) * cos(fragPosWorld.z * 0.1) * 0.1f;

    float rockBase    = 0.4;
    float grassBase   = 0.6;

    float rockThreshold = rockBase + wave;
    float grassThreshold = grassBase + wave;
    float blendRange = 0.015;
    
    vec2 scaledTexCoord = fragTexCoord * modelUbo.textureParams.xy;
    vec3 rockColor = texture(rockTexture, scaledTexCoord).rgb;
    vec3 grassColor = texture(grassTexture, scaledTexCoord).rgb;
    vec3 snowColor = texture(snowTexture, scaledTexCoord).rgb;
    
    float rockWeight = 1.0 - smoothstep(rockThreshold - blendRange, rockThreshold + blendRange, normalizedHeight);
    float grassWeight = smoothstep(rockThreshold - blendRange, rockThreshold + blendRange, normalizedHeight)
                      * (1.0 - smoothstep(grassThreshold - blendRange, grassThreshold + blendRange, normalizedHeight));
    float snowWeight = smoothstep(grassThreshold - blendRange, grassThreshold + blendRange, normalizedHeight);
    
    vec3 diffuseColor = rockColor * rockWeight + grassColor * grassWeight + snowColor * snowWeight;
    
    // slope-based blending (less snow on steep slopes)
    float slope = 1.0 - abs(dot(N, vec3(0.0, 1.0, 0.0)));
    if (slope > 0.5) {
        float slopeBlend = smoothstep(0.5, 0.9, slope);
        diffuseColor = mix(diffuseColor, rockColor, slopeBlend * snowWeight);
    }

    vec3 light = diffuseColor * modelUbo.lightingProperties.x;

    light += phong(
        N, -globalUbo.sunDirection.xyz, -V,
        diffuseColor * globalUbo.sunColor.rgb, modelUbo.lightingProperties.y,
        globalUbo.sunColor.rgb, modelUbo.lightingProperties.z,
        modelUbo.lightingProperties.w,
        false, vec3(1.0)
    );

    outColor = vec4(light, 1.0f);
}