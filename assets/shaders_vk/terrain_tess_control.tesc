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
    vec4 params1;  // x: hasTexture, y and z: textureRepetition, w: maxTessLevel
    vec4 params2;  // x: minTessDistance, y: maxTessDistance, z: heightScale, w: useHeightmapTexture
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    float aspectRatio;
} ubo;

// helper: map distance -> [0,1]
float mapDist(float d) {
    return clamp((d - push.params2.x) / (push.params2.y - push.params2.x), 0.0, 1.0);
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
        float d0 = length(ubo.view * vec4(fragPosWorld[0], 1.0));
        float d1 = length(ubo.view * vec4(fragPosWorld[1], 1.0));
        float d2 = length(ubo.view * vec4(fragPosWorld[2], 1.0));
        float d3 = length(ubo.view * vec4(fragPosWorld[3], 1.0));

        float f0 = mapDist(d0);
        float f1 = mapDist(d1);
        float f2 = mapDist(d2);
        float f3 = mapDist(d3);
        float maxL = push.params1.w; // maxTessLevel
        
        gl_TessLevelOuter[0] = mix(maxL, 1.0, min(f0, f1));
        gl_TessLevelOuter[1] = mix(maxL, 1.0, min(f1, f2));
        gl_TessLevelOuter[2] = mix(maxL, 1.0, min(f2, f3));
        gl_TessLevelOuter[3] = mix(maxL, 1.0, min(f3, f0));
        
        gl_TessLevelInner[0] = max(gl_TessLevelOuter[0], gl_TessLevelOuter[2]);
        gl_TessLevelInner[1] = max(gl_TessLevelOuter[1], gl_TessLevelOuter[3]);
    }
}