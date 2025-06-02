#version 450

// 4 control points per quad patch, controls length of vertex arrays
layout(vertices = 4) out;

// input from vertex shader
layout(location = 0) in vec3 fragColor[];
layout(location = 1) in vec3 fragPosWorld[];
layout(location = 2) in vec3 fragNormalWorld[];
layout(location = 3) in vec2 fragTexCoord[];
layout(location = 4) in vec2 rawUV[];

// output to tessellation evaluation shader
layout(location = 0) out vec3 fragColorTesc[];
layout(location = 1) out vec3 fragPosWorldTesc[];
layout(location = 2) out vec3 fragNormalWorldTesc[];
layout(location = 3) out vec2 fragTexCoordTesc[];
layout(location = 4) out vec2 rawUVTesc[];

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

layout(set = 1, binding = 2) uniform Ubo {
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

// helper: map distance -> [0,1]
float mapDist(float d) {
    return clamp((d - modelUbo.tessParams.y) / (modelUbo.tessParams.z - modelUbo.tessParams.y), 0.0, 1.0);
}

void main() {

    fragColorTesc[gl_InvocationID] = fragColor[gl_InvocationID];
    fragPosWorldTesc[gl_InvocationID] = fragPosWorld[gl_InvocationID];
    fragNormalWorldTesc[gl_InvocationID] = fragNormalWorld[gl_InvocationID];
    fragTexCoordTesc[gl_InvocationID] = fragTexCoord[gl_InvocationID];
    rawUVTesc[gl_InvocationID] = rawUV[gl_InvocationID];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    if (gl_InvocationID == 0) {

        // compute per-vertex distances
        float d0 = length(globalUbo.cameraPosition - vec4(fragPosWorld[0], 1.0));
        float d1 = length(globalUbo.cameraPosition - vec4(fragPosWorld[1], 1.0));
        float d2 = length(globalUbo.cameraPosition - vec4(fragPosWorld[2], 1.0));
        float d3 = length(globalUbo.cameraPosition - vec4(fragPosWorld[3], 1.0));

        float f0 = mapDist(d0);
        float f1 = mapDist(d1);
        float f2 = mapDist(d2);
        float f3 = mapDist(d3);
        float maxL = modelUbo.tessParams.x;
        
        gl_TessLevelOuter[0] = mix(maxL, 1.0, min(f0, f1));
        gl_TessLevelOuter[1] = mix(maxL, 1.0, min(f1, f2));
        gl_TessLevelOuter[2] = mix(maxL, 1.0, min(f2, f3));
        gl_TessLevelOuter[3] = mix(maxL, 1.0, min(f3, f0));
        
        gl_TessLevelInner[0] = max(gl_TessLevelOuter[0], gl_TessLevelOuter[2]);
        gl_TessLevelInner[1] = max(gl_TessLevelOuter[1], gl_TessLevelOuter[3]);
    }
}