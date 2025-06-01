#version 450

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

    uv = normalizedXZ * modelUbo.textureParams.xy;

    gl_Position = push.modelMatrix * vec4(localX, 0.0, localZ, 1.0);
}