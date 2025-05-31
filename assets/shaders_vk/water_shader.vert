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

layout(location = 0) out vec2 uv;

void main() {
    int cornerID = gl_VertexIndex % 4;
    int patchID = int(gl_VertexIndex / 4);
    int gridSize = int(sqrt(push.gridInfo.x));

    // patch coordinates
    int px = patchID % gridSize;
    int py = int(patchID / gridSize);

    // offset within patch -> (0,0), (1,0), (0,1), (1,1)
    int ox = (cornerID == 1 || cornerID == 3) ? 1 : 0;
    int oy = (cornerID == 2 || cornerID == 3) ? 1 : 0;

    float step = 2.0 / gridSize;
    float localX = -1.0 + float(px) * step + float(ox) * step;
    float localZ = -1.0 + float(py) * step + float(oy) * step;

    // [-1, 1] -> [0, 1]
    vec2 normalizedXZ = (vec2(localX, localZ) + vec2(1.0)) * 0.5;

    uv = normalizedXZ * modelUbo.textureParams.xy + modelUbo.textureParams.zw * push.timeData.x;

    gl_Position = push.modelMatrix * vec4(localX, 0.0, localZ, 1.0);
}