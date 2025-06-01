#version 450

layout(quads, fractional_odd_spacing, ccw) in;

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

layout(location = 0) in vec2 uvTesc[];

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormWorld;

// https://catlikecoding.com/unity/tutorials/flow/waves/
vec3 gerstner(vec3 position, float time, vec2 direction, float steepness, float wavelength){
	float displaced_x = position.x + (steepness/wavelength) * direction.x * cos(wavelength * dot(direction, position.xz) + speed * time);
	float displaced_y = position.y + amplitude * sin(wavelength * dot(direction, position.xz) + speed * time);
	float displaced_z = position.z + (steepness/wavelength) * direction.y * cos(wavelength * dot(direction, position.xz) + speed * time);
	return vec3(displaced_x, displaced_y, displaced_z);
}

vec3 gerstnerNormal(vec3 position, float time, vec2 direction, float steepness, float wavelength) {
    float cosComponent = cos(wavelength * dot(direction, position.xz + speed * time));
	float sinComponent = sin(wavelength * dot(direction, position.xz + speed * time));
	float x_normal = -direction.x * wavelength * amplitude * cosComponent;
	float y_normal = 1.0 - (steepness/wavelength) * wavelength * amplitude * sinComponent;
	float z_normal = -direction.y * wavelength * amplitude * cosComponent;
	return vec3(x_normal, y_normal, z_normal);
}

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;

    vec3 pmix0 = mix(p0, p1, u);
    vec3 pmix1 = mix(p2, p3, u);
    vec3 basePos = mix(pmix0, pmix1, v);

    vec2 uv0 = mix(uvTesc[0], uvTesc[1], u);
    vec2 uv1 = mix(uvTesc[2], uvTesc[3], u);
    fragUV = mix(uv0, uv1, v);

    float t = push.timeData.x;
    vec2 direction = modelUbo.waveParams1.xy;
    float steepness = modelUbo.waveParams1.z;
    float wavelength = modelUbo.waveParams1.w;

    vec3 displaced = gerstner(basePos, t, direction, steepness, wavelength);
    vec3 nW = normalize(gerstnerNormal(basePos, t, direction, steepness, wavelength));

    fragPosWorld  = displaced;
    fragNormWorld = nW;

    gl_Position  = globalUbo.projection * globalUbo.view * vec4(displaced, 1.0);
}