cmake_minimum_required(VERSION 3.22)

project(Swarm)

option(USE_STATIC_MSVC_RUNTIME_LIBRARY "Link Swarm and all FetchContent deps with the static CRT" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64")
    if (USE_STATIC_MSVC_RUNTIME_LIBRARY)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()

    add_compile_options(/FS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Qpar")

    if (GENERATE_DEBUG_SYMBOLS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    endif()

    string(REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")

    if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "/GS /Od /Ob0 /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "/GS- /Gy /O2 /Oi /Ot")
    set(CMAKE_CXX_FLAGS_DISTRIBUTION "/GS- /Gy /O2 /Oi /Ot")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        if (CROSS_PLATFORM_DETERMINISTIC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /showFilenames")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
    endif()
else()
    # Unix/Mac compilers
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    if (GENERATE_DEBUG_SYMBOLS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    endif()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-stringop-overflow -ffp-contract=off")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-model=precise")
        if (CMAKE_CXX_COMPILER_VERSION LESS 14 OR CROSS_PLATFORM_DETERMINISTIC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-contract=off")
        endif()
    endif()

    # Disable error for missing override
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=inconsistent-missing-override")
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    message(STATUS "Using ccache for faster builds")
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

if(APPLE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
  add_compile_options("-isysroot" "${CMAKE_OSX_SYSROOT}")
  add_link_options   ("-isysroot" "${CMAKE_OSX_SYSROOT}")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Distribution")

# Distribution build flags match release
set(CMAKE_EXE_LINKER_FLAGS_DISTRIBUTION "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

# on allows for much bigger worlds
set(DOUBLE_PRECISION OFF)

# When turning this option on, the library will be compiled with debug symbols
set(GENERATE_DEBUG_SYMBOLS ON)

# When turning this option on, the library will be compiled in such a way to attempt to keep the simulation deterministic across platforms
set(CROSS_PLATFORM_DETERMINISTIC ON)

# When turning this on, in Debug and Release mode, the library will emit extra code to ensure that the 4th component of a 3-vector is kept the same as the 3rd component
# and will enable floating point exceptions during simulation to detect divisions by zero.
# Note that this currently only works using MSVC. Clang turns Float2 into a SIMD vector sometimes causing floating point exceptions (the option is ignored).
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)

# Number of bits to use in ObjectLayer. Can be 16 or 32.
set(OBJECT_LAYER_BITS 16)

# Select X86 processor features to use, by default the library compiles with AVX2, if everything is off it will be SSE2 compatible.
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)

find_package(Vulkan REQUIRED)

# Find glslc compiler from Vulkan SDK
if(WIN32)
    find_program(GLSLC_EXECUTABLE NAMES glslc PATHS $ENV{VULKAN_SDK}/Bin REQUIRED)
elseif(APPLE)
    find_program(GLSLC_EXECUTABLE NAMES glslc PATHS $ENV{VULKAN_SDK}/macOS/bin REQUIRED)
else() # Linux
    find_program(GLSLC_EXECUTABLE NAMES glslc PATHS $ENV{VULKAN_SDK}/bin REQUIRED)
endif()

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc shader compiler not found! Make sure Vulkan SDK is installed.")
endif()

include(FetchContent)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
  GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        master
  GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  inih
  GIT_REPOSITORY https://github.com/benhoyt/inih.git
  GIT_TAG r60
  GIT_SHALLOW ON
)

# no cmake support in inih
FetchContent_Populate(inih)
add_library(inih STATIC
  ${inih_SOURCE_DIR}/ini.c
  ${inih_SOURCE_DIR}/cpp/INIReader.cpp
)

FetchContent_Declare(
  soloud
  GIT_REPOSITORY https://github.com/jarikomppa/soloud.git
  GIT_TAG        master
  GIT_SHALLOW    ON
)
FetchContent_GetProperties(soloud)
if(NOT soloud_POPULATED)
  FetchContent_Populate(soloud)
endif()

set(SOLOUD_SOURCES
  ${soloud_SOURCE_DIR}/src/core/soloud.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_audiosource.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_bus.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_core_3d.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_core_basicops.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_core_faderops.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_core_filterops.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_core_getters.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_core_setters.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_core_voicegroup.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_core_voiceops.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_fader.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_fft.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_fft_lut.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_file.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_filter.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_misc.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_queue.cpp
  ${soloud_SOURCE_DIR}/src/core/soloud_thread.cpp
  ${soloud_SOURCE_DIR}/src/audiosource/wav/soloud_wav.cpp
  ${soloud_SOURCE_DIR}/src/audiosource/wav/soloud_wavstream.cpp
  ${soloud_SOURCE_DIR}/src/audiosource/wav/stb_vorbis.c
  ${soloud_SOURCE_DIR}/src/backend/miniaudio/soloud_miniaudio.cpp
)

if(WIN32)
  list(APPEND SOLOUD_SOURCES ${soloud_SOURCE_DIR}/src/backend/winmm/soloud_winmm.cpp)
  add_definitions(-DWITH_WINMM)
elseif(APPLE)
  list(APPEND SOLOUD_SOURCES ${soloud_SOURCE_DIR}/src/backend/coreaudio/soloud_coreaudio.cpp)
  add_definitions(-DWITH_COREAUDIO)
else()
  list(APPEND SOLOUD_SOURCES ${soloud_SOURCE_DIR}/src/backend/alsa/soloud_alsa.cpp)
  add_definitions(-DWITH_ALSA)
endif()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dr_libs_impl.c
"#define DR_MP3_IMPLEMENTATION
#include \"${soloud_SOURCE_DIR}/src/audiosource/wav/dr_mp3.h\"
#define DR_WAV_IMPLEMENTATION
#include \"${soloud_SOURCE_DIR}/src/audiosource/wav/dr_wav.h\"
#define DR_FLAC_IMPLEMENTATION
#include \"${soloud_SOURCE_DIR}/src/audiosource/wav/dr_flac.h\"
")

list(APPEND SOLOUD_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dr_libs_impl.c)
list(APPEND SOLOUD_SOURCES ${soloud_SOURCE_DIR}/src/audiosource/wav/dr_impl.cpp)

add_library(soloud STATIC ${SOLOUD_SOURCES})
target_include_directories(soloud PUBLIC
  ${soloud_SOURCE_DIR}/include
  ${soloud_SOURCE_DIR}/src/audiosource/wav
)

set(JPH_BUILD_SHARED_LIBRARY ON CACHE BOOL "Build JoltPhysics as a DLL" FORCE)
set(JPH_SHARED_LIBRARY ON CACHE BOOL "Use JoltPhysics as a shared library" FORCE)
FetchContent_Declare(
    Jolt
    GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
    GIT_TAG "v5.2.0"
    SOURCE_SUBDIR "Build"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(Jolt)

if(MSVC)
  target_compile_options(Jolt PRIVATE
    $<$<CONFIG:Release>:/WX->         # disable “warnings as errors” in Release for Jolt
  )
endif()

set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY "https://github.com/syoyo/tinygltf"
    GIT_TAG "v2.9.5"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(tinygltf)

FetchContent_Declare(
    glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG "0.9.9.8"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(glm)

set(GLFW_BUILD_DOCS      OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw.git"
    GIT_TAG "3.3.8"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY "https://github.com/tinyobjloader/tinyobjloader.git"
    GIT_TAG "v1.0.6"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(tinyobjloader)

# Platform-specific pthread linking
if(UNIX AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(UNIX AND NOT APPLE)
    set(LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux" ".")
elseif(UNIX AND APPLE)
    set(LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos" ".")
elseif(WIN32)
    set(LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows" ".")
endif()

# Set up libraries to link
set(LINK_LIBRARIES
    Vulkan::Vulkan
    Vulkan::Headers
    glfw
    glm
    Jolt
    tinygltf
    tinyobjloader
    fmt
    inih
    soloud
    $<$<BOOL:${APPLE}>:
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework IOKit"
        "-framework AppKit"
    >
)

# Windows-specific libraries if needed
if (WIN32)
    list(APPEND LINK_LIBRARIES
    # debug VulkanLaunchpadd optimized VulkanLaunchpad
    # debug glslangd optimized glslang
    # debug MachineIndependentd optimized MachineIndependent
    # debug GenericCodeGend optimized GenericCodeGen
    # debug OSDependentd optimized OSDependent
    # debug SPIRVd optimized SPIRV
    # debug glfw3d optimized glfw3
    # debug OGLCompilerd optimized OGLCompiler
    # debug GCG_VK_Lib_Debug optimized GCG_VK_Lib_Release
    )
endif()

message(LINK_LIBRARIES=${LINK_LIBRARIES})

if (WIN32)
    # Enable link time optimization in Release and Distribution mode if requested and available
    SET_INTERPROCEDURAL_OPTIMIZATION()
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

add_definitions(-DPROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_include_directories(inih PUBLIC
  ${inih_SOURCE_DIR}
  ${inih_SOURCE_DIR}/cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${stb_SOURCE_DIR}
    ${soloud_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LINK_LIBRARIES}
)

# Setup shader compilation
# Define shader directories
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders_vk")
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders_vk/compiled")

# Make sure the output directory exists
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# Find all shader files
file(GLOB SHADER_SOURCE_FILES
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.comp"
    "${SHADER_SOURCE_DIR}/*.geom"
    "${SHADER_SOURCE_DIR}/*.tesc"
    "${SHADER_SOURCE_DIR}/*.tese"
)

# Setup custom commands for shader compilation
foreach(SHADER_SOURCE_FILE ${SHADER_SOURCE_FILES})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE_FILE} NAME)
    set(SHADER_OUTPUT_FILE "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_FILE}
        COMMAND ${GLSLC_EXECUTABLE} -o ${SHADER_OUTPUT_FILE} ${SHADER_SOURCE_FILE}
        DEPENDS ${SHADER_SOURCE_FILE}
        COMMENT "Compiling shader: ${SHADER_SOURCE_FILE}"
        VERBATIM
    )

    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT_FILE})
endforeach()

# Create a custom target for shader compilation
add_custom_target(
    shaders ALL
    DEPENDS ${SHADER_OUTPUTS}
)

# Make the main target depend on shaders
add_dependencies(${PROJECT_NAME} shaders)

# Copy compiled shaders to binary directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${SHADER_OUTPUT_DIR}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders_vk/compiled
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/models
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/textures
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/settings/
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/settings
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/audio
)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
   set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
   set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
elseif(CMAKE_GENERATOR MATCHES "Xcode")
   set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_GENERATE_SCHEME TRUE CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
   set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY XCODE_STARTUP_PROJECT ${PROJECT_NAME})
endif()

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
endif()

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "."
    CONFIGURATIONS Debug Release
)
install(
    RUNTIME_DEPENDENCY_SET runtime
    CONFIGURATIONS Debug Release
    DESTINATION $<CONFIG>
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

install(
    DIRECTORY "${SHADER_OUTPUT_DIR}/"
    DESTINATION "assets/shaders_vk/compiled"
    FILES_MATCHING PATTERN "*.spv"
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/"
    DESTINATION "assets/models"
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/"
    DESTINATION "assets/textures"
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/settings/"
    DESTINATION "assets/settings"
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/"
    DESTINATION "assets/audio"
)