cmake_minimum_required(VERSION 3.22)

project(Swarm_Shaders)

# This CMakeLists.txt file can be used to compile shaders independent of the main build

find_package(Vulkan REQUIRED)

# Find glslc compiler from Vulkan SDK
if(WIN32)
    find_program(GLSLC_EXECUTABLE NAMES glslc PATHS $ENV{VULKAN_SDK}/Bin)
elseif(APPLE)
    find_program(GLSLC_EXECUTABLE NAMES glslc PATHS $ENV{VULKAN_SDK}/macOS/bin)
else() # Linux
    find_program(GLSLC_EXECUTABLE NAMES glslc PATHS $ENV{VULKAN_SDK}/bin)
endif()

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc shader compiler not found! Make sure Vulkan SDK is installed.")
endif()

set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled")

# Make sure output directory exists
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# Find all shader files
file(GLOB SHADER_SOURCE_FILES
    "*.vert"
    "*.frag"
    "*.comp"
    "*.geom"
    "*.tesc"
    "*.tese"
)

message(STATUS "Found ${CMAKE_ELEMENTS_LENGTH(SHADER_SOURCE_FILES)} shaders to compile:")
foreach(SHADER ${SHADER_SOURCE_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    message(STATUS "  - ${SHADER_NAME}")
endforeach()

# Shader compilation
foreach(SHADER_SOURCE_FILE ${SHADER_SOURCE_FILES})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE_FILE} NAME)
    set(SHADER_OUTPUT_FILE "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_FILE}
        COMMAND ${GLSLC_EXECUTABLE} -o ${SHADER_OUTPUT_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_NAME}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_NAME}
        COMMENT "Compiling shader: ${SHADER_NAME}"
        VERBATIM
    )
    
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT_FILE})
endforeach()

# Create a custom target for shader compilation
add_custom_target(
    shaders ALL
    DEPENDS ${SHADER_OUTPUTS}
)

# Installation rules
install(
    DIRECTORY "${SHADER_OUTPUT_DIR}/"
    DESTINATION "compiled"
    FILES_MATCHING PATTERN "*.spv"
)

message(STATUS "Shader output directory: ${SHADER_OUTPUT_DIR}")
message(STATUS "Using glslc from: ${GLSLC_EXECUTABLE}")