#version 450

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 uiOrthographicProjection;
    vec4 sunDirection;
    vec4 sunColor;
} globalUbo;

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

layout(push_constant) uniform Push {
    // x = time, yzw = unused
    vec4 timeData;

    mat4 modelMatrix;
    mat4 normalMatrix;
    // x = patchCount, yzw = unused
    vec4 gridInfo;
} push;

layout(location = 0) in vec2 uv[];

layout(location = 0) out vec2 uvTesc[];

layout(vertices = 4) out;

// helper: map distance -> [0,1]
float mapDist(float d) {
    float minDist = modelUbo.tessParams.y;
    float maxDist = modelUbo.tessParams.z;
    if (maxDist <= minDist) {
        // if maxTessDistance is less than or equal to minTessDistance, no tessellation
        return 1.0;
    }
    return clamp((d - minDist) / (maxDist - minDist), 0.0, 1.0);
}

void main() {
    uvTesc[gl_InvocationID] = uv[gl_InvocationID];

    if (gl_InvocationID == 0) {
        // compute per-vertex distances
        float d0 = length((globalUbo.view * gl_in[0].gl_Position).xyz);
        float d1 = length((globalUbo.view * gl_in[1].gl_Position).xyz);
        float d2 = length((globalUbo.view * gl_in[2].gl_Position).xyz);
        float d3 = length((globalUbo.view * gl_in[3].gl_Position).xyz);

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

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}