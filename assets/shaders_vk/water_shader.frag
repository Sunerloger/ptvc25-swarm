#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormWorld;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    
    vec4 sunDirection;
    // rgb + a unused
    vec4 sunColor;
    
    // camera position in world space
    vec4 cameraPosition;
} globalUbo;

layout(push_constant) uniform Push {
    // x = time, yzw = unused
    vec4 timeData;

    mat4 modelMatrix;
    mat4 normalMatrix;
    // x = patchCount, yzw = unused
    vec4 gridInfo;
} push;

layout(set = 1, binding = 1) uniform Ubo {
    // x = maxTessLevel, max tessellation subdivisions
    // y = minTessDistance, within minTessDistance the tessellation has maxTessLevels
    // z = maxTessDistance, tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
    // w = unused
    vec4 tessParams;

    // xy = textureRepetition, how often the texture repeats across the whole tessellation object
    // zw = unused
    vec4 textureParams;

    // x = ka, y = kd, z = ks, w = alpha
    vec4 materialProperties;

    // xyz = default color, w = transparency
    vec4 color;

    // x = hasTexture, y = wave count, zw = unused
    vec4 flags;

    // xy = direction, z = steepness in [0,1], w = wavelength
    vec4 waves[];
} modelUbo;

layout(location = 0) out vec4 outColor;

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
    vec3 N  = normalize( fragNormWorld );
    vec3 V = normalize(fragPosWorld - globalUbo.cameraPosition.xyz);

    const vec3 diffuseColor = (modelUbo.flags.x > 0.5)
        ? texture(texSampler, fragUV).rgb
        : modelUbo.color.xyz;

    vec3 light = diffuseColor * modelUbo.materialProperties.x;

    light += phong(
        N, -globalUbo.sunDirection.xyz, -V,
        diffuseColor * globalUbo.sunColor.rgb, modelUbo.materialProperties.y,
        globalUbo.sunColor.rgb, modelUbo.materialProperties.z,
        modelUbo.materialProperties.w,
        false, vec3(1.0)
    );

    outColor = vec4(light, modelUbo.color.w);
}