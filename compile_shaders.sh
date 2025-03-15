#!/bin/bash
# Simple script to compile shaders manually without using CMake

# Determine OS and set path to glslc compiler
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    GLSLC="${VULKAN_SDK}/macOS/bin/glslc"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    # Windows
    GLSLC="${VULKAN_SDK}/Bin/glslc.exe"
else
    # Linux
    GLSLC="${VULKAN_SDK}/bin/glslc"
fi

if [ -z "$VULKAN_SDK" ]; then
    echo "Error: VULKAN_SDK environment variable is not set!"
    echo "Please set it to your Vulkan SDK installation directory."
    exit 1
fi

if [ ! -f "$GLSLC" ]; then
    echo "Error: glslc compiler not found at $GLSLC"
    echo "Make sure Vulkan SDK is properly installed."
    exit 1
fi

SHADER_DIR="assets/shaders_vk"
OUTPUT_DIR="$SHADER_DIR/compiled"
mkdir -p "$OUTPUT_DIR"

echo "Compiling shaders..."
for shader in "$SHADER_DIR"/*.vert "$SHADER_DIR"/*.frag "$SHADER_DIR"/*.comp "$SHADER_DIR"/*.geom "$SHADER_DIR"/*.tesc "$SHADER_DIR"/*.tese; do
    if [ -f "$shader" ]; then
        filename=$(basename "$shader")
        output="$OUTPUT_DIR/$filename.spv"
        echo "Compiling: $filename"
        "$GLSLC" -o "$output" "$shader"
        if [ $? -eq 0 ]; then
            echo "  Success: $output"
        else
            echo "  Failed to compile $filename"
        fi
    fi
done

echo "Shader compilation complete!"