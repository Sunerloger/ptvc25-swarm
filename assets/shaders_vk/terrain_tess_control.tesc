#version 450

// Define patch size (3 control points for a triangle patch)
layout(vertices = 3) out;

// Input from vertex shader
layout(location = 0) in vec3 fragColor[];
layout(location = 1) in vec3 fragPosWorld[];
layout(location = 2) in vec3 fragNormalWorld[];
layout(location = 3) in vec2 fragTexCoord[];
layout(location = 4) in vec3 viewPosition[];

// Output to tessellation evaluation shader
layout(location = 0) out vec3 fragColorTesc[];
layout(location = 1) out vec3 fragPosWorldTesc[];
layout(location = 2) out vec3 fragNormalWorldTesc[];
layout(location = 3) out vec2 fragTexCoordTesc[];

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec4 params1;  // x: hasTexture, yz: tileScale, w: maxTessLevel
    vec4 params2;  // x: tessDistance, y: minTessDistance, z: heightScale, w: useHeightmapTexture
} push;

void main() {
    // Pass attributes to tessellation evaluation shader
    fragColorTesc[gl_InvocationID] = fragColor[gl_InvocationID];
    fragPosWorldTesc[gl_InvocationID] = fragPosWorld[gl_InvocationID];
    fragNormalWorldTesc[gl_InvocationID] = fragNormalWorld[gl_InvocationID];
    fragTexCoordTesc[gl_InvocationID] = fragTexCoord[gl_InvocationID];
    
    // Only the first invocation computes tessellation levels
    if (gl_InvocationID == 0) {
        // distances at the three control points
        float d0 = distance(viewPosition[0], fragPosWorld[0]);
        float d1 = distance(viewPosition[0], fragPosWorld[1]);
        float d2 = distance(viewPosition[0], fragPosWorld[2]);

        // helper to map a distance to a tess level
        float calcLevel(float dist) {
            float lvl = push.params1.w;
            if (dist > push.params2.x) {
                float f = 1.0 - clamp((dist - push.params2.x)/(push.params2.y - push.params2.x),0,1);
                lvl = mix(1.0, push.params1.w, f);
            }
            // snap to integer to avoid tiny mismatches
            return floor(lvl + 0.5);
        }
        
         // per-edge levels
        gl_TessLevelOuter[0] = calcLevel((d0 + d1)*0.5);  // edge between v0–v1
        gl_TessLevelOuter[1] = calcLevel((d1 + d2)*0.5);  // v1–v2
        gl_TessLevelOuter[2] = calcLevel((d2 + d0)*0.5);  // v2–v0

        // interior can be average
        gl_TessLevelInner[0] = calcLevel((d0 + d1 + d2)/3.0);
    }
}