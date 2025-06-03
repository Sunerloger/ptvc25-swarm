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

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

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

    light += phong(
        N, -globalUbo.sunDirection.xyz, -V,
        diffuseColor * globalUbo.sunColor.rgb, modelUbo.lightingProperties.y,
        globalUbo.sunColor.rgb, modelUbo.lightingProperties.z,
        modelUbo.lightingProperties.w,
        false, vec3(1.0)
    );

    outColor = vec4(light, 1.0f);
}