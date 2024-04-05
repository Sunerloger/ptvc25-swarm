#!/bin/bash

# Automatically find the path to the glslc compiler
GLSLC=$(which glslc)
if [ -z "$GLSLC" ]; then
    echo "glslc compiler not found. Make sure it's installed and in your PATH."
    exit 1
fi

# Define the directory for the shaders and output
SHADER_DIR="../assets/shaders_vk"
OUTPUT_DIR="../assets/shaders_vk"

# Compile the shaders
"$GLSLC" "$SHADER_DIR/simple_shader.vert" -o "$OUTPUT_DIR/simple_shader.vert.spv"
"$GLSLC" "$SHADER_DIR/simple_shader.frag" -o "$OUTPUT_DIR/simple_shader.frag.spv"
"$GLSLC" "$SHADER_DIR/point_light.vert" -o "$OUTPUT_DIR/point_light.vert.spv"
"$GLSLC" "$SHADER_DIR/point_light.frag" -o "$OUTPUT_DIR/point_light.frag.spv"
"$GLSLC" "$SHADER_DIR/hud.vert" -o "$OUTPUT_DIR/hud.vert.spv"
"$GLSLC" "$SHADER_DIR/hud.frag" -o "$OUTPUT_DIR/hud.frag.spv"

echo "Shader compilation finished."