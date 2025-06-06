#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec3 fragColor;

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

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 1, binding = 1) uniform ModelUbo {
    // x = ka, y = kd, z = ks, w = alpha
    vec4 lightingProperties;
    // x = hasTexture, yzw = unused
    vec4 flags;
} modelUbo;

layout(set = 2, binding = 0) uniform ShadowUbo {
    mat4 lightViewMatrix;
    mat4 lightProjectionMatrix;
    // x: shadow map size, y: PCF samples, z: bias, w: shadow strength
    vec4 shadowParams;
} shadowUbo;

layout(set = 2, binding = 1) uniform sampler2DShadow shadowMap;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

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

vec3 phong( vec3 n, vec3 l, vec3 v,
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
    vec3 N = normalize( fragNormalWorld );
    vec3 V = normalize(fragPosWorld - globalUbo.cameraPosition.xyz);

    vec3 diffuseColor = fragColor;
    if (modelUbo.flags.x == 1) {
        diffuseColor = texture(texSampler, fragUV).rgb;
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

    outColor = vec4(light, 1.0f);
}