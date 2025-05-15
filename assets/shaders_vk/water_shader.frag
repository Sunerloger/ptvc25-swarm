#version 450

layout(location = 0) in vec2  fragUV;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 posWorld;   // from your VS
layout(location = 3) in vec3 normWorld;  // from your VS

layout(set = 1, binding = 0) uniform sampler2D texSampler;

struct PointLight {
    vec3 position;
    float intensity;
    float radius;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    vec4 sunDirection;  // in view‐space, points *towards* the light
    vec4 sunColor;      // rgb + intensity in .w
    PointLight pointLights[50];
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec2 uvOffset;
    float time;
    int   hasTexture;
} push;

layout(location = 0) out vec4 outColor;

// ------------------------------------------------------------
// phong helper from TU Wien
// ------------------------------------------------------------
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
    float NdotL = max(dot(n, l), 0.0);
    // only lit side contributes
    if (NdotL <= 0.0) return vec3(0.0);
    vec3 r = reflect(-l, n);
    float RdotV = max(dot(r, v), 0.0);
    return ( diffuseF * diffuseC * NdotL
           + specularF * specularC * pow(RdotV, alpha) )
           * att;
}

void main() {
    // 1) reconstruct view‐space normal & view‐space position
    //    use your full normalMatrix (M⁻ᵀ) to get correct view‐space normal:
    vec3 Nw = normalize(normWorld);
    vec3 N  = normalize( (push.normalMatrix * vec4(Nw,0.0)).xyz );

    vec3 viewPos = (ubo.view * vec4(posWorld,1.0)).xyz;
    vec3 V = normalize(-viewPos); // toward camera at origin

    // 2) directional‐light direction (sunDirection in view‐space)
        //    make sure sunDirection was stored with w=0!  If not, reproject it here:
    vec3 sunDirVS = normalize((ubo.view * vec4(ubo.sunDirection.xyz, 0.0)).xyz);
    vec3 L = -sunDirVS;

    // 3) compute **only** the specular term:
    //    zero out diffuse by diffuseF = 0
    float shininess = 64.0;
    vec3 spec = phong(
        N,             // normal
        L,             // light dir
        V,             // view dir
        vec3(0.0),     // diffuse color (ignored)
        0.0,           // diffuse factor = 0
        ubo.sunColor.rgb,      // specular color
        ubo.sunColor.w,        // specular factor = sun intensity
        shininess,    // shininess exponent
        false,        // no attenuation
        vec3(1.0)     // unused
    );

    // 4) boost so you actually see it
    vec3  dbg       = spec;

    outColor = vec4(dbg, 1.0);
}