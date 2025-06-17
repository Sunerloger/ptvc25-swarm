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

layout(set = 2, binding = 0) uniform ShadowUbo {
    mat4 lightViewMatrix;
    mat4 lightProjectionMatrix;
    // x: shadow map size, y: PCF samples, z: bias, w: shadow strength
    vec4 shadowParams;
} shadowUbo;

layout(set = 2, binding = 1) uniform sampler2DShadow shadowMap;

float calculateShadow(vec3 worldPos, vec3 normal) {
    vec4 posLightSpace = shadowUbo.lightProjectionMatrix * shadowUbo.lightViewMatrix * vec4(worldPos, 1.0);
    
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    
    // transform to [0,1]
    projCoords = projCoords * 0.5 + 0.5;
    
    // outside of light view = no shadow
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 1.0;
    }
    
    // bias to avoid shadow acne
    float bias = shadowUbo.shadowParams.z;
    
    // normal bias for surfaces at an angle to the light
    vec3 lightDir = normalize(globalUbo.sunDirection.xyz);
    float normalBias = max(bias * (1.0 - dot(normal, lightDir)), bias);
    
    float shadow = 0.0;
    int pcfSize = int(shadowUbo.shadowParams.y);
    float texelSize = 1.0 / shadowUbo.shadowParams.x;
    
    for (int x = -pcfSize/2; x <= pcfSize/2; x++) {
        for (int y = -pcfSize/2; y <= pcfSize/2; y++) {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - normalBias));
            shadow += pcfDepth;
        }
    }
    
    shadow /= (pcfSize + 1) * (pcfSize + 1);
    
    // shadow strength in shadowUbo.shadowParams.w
    return 1.0 - (shadowUbo.shadowParams.w * (1.0 - shadow));
}

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

    float shadowFactor = calculateShadow(fragPosWorld, N);
    
    vec3 phongLight = phong(
        N, -globalUbo.sunDirection.xyz, -V,
        diffuseColor * globalUbo.sunColor.rgb, modelUbo.lightingProperties.y,
        globalUbo.sunColor.rgb, modelUbo.lightingProperties.z,
        modelUbo.lightingProperties.w,
        false, vec3(1.0)
    );
    
    light += phongLight * shadowFactor;

    // outColor = vec4(light, 1.0f);

    // debug shadows
    vec4 posLightSpace = shadowUbo.lightProjectionMatrix * shadowUbo.lightViewMatrix * vec4(fragPosWorld, 1.0);
    
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    
    // original depth in NDC space (before [0,1] transform)
    float ndcDepth = projCoords.z;
    
    // transform to [0,1]
    float depth = (projCoords.z * 0.5 + 0.5);
    
    // calculate world distance to sun
    float worldDistToSun = length(fragPosWorld - globalUbo.sunDirection.xyz * 1000.0);
    
    // RGB visualization: R = depth, G = NDC depth, B = world distance
    outColor = vec4(depth, ndcDepth, worldDistToSun / 150.0, 1.0);
}