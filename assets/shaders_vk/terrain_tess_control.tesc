#version 450

// Define patch size (4 control points for a quad patch)
layout(vertices = 4) out;

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
        // Calculate the center of the patch
        vec3 center = (fragPosWorld[0] + fragPosWorld[1] + fragPosWorld[2] + fragPosWorld[3]) / 4.0;
        
        // Calculate distance from camera to patch center
        float distance = distance(viewPosition[0], center);
        
        // Calculate tessellation level based on distance
        float tessLevel = push.params1.w;  // maxTessLevel is in params1.w
        
        if (distance > push.params2.x) {  // tessDistance is in params2.x
            // Linearly decrease tessellation level with distance
            float factor = 1.0 - clamp((distance - push.params2.x) / (push.params2.y - push.params2.x), 0.0, 1.0);
            tessLevel = mix(1.0, push.params1.w, factor);
        }
        
        // Set tessellation levels for the patch
        gl_TessLevelOuter[0] = tessLevel;
        gl_TessLevelOuter[1] = tessLevel;
        gl_TessLevelOuter[2] = tessLevel;
        gl_TessLevelOuter[3] = tessLevel;
        
        gl_TessLevelInner[0] = tessLevel;
        gl_TessLevelInner[1] = tessLevel;
    }
}