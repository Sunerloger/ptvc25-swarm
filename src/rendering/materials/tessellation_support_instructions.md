# Extending the Material System for Tessellation Shaders

The material system we've designed can be extended to support tessellation shaders for terrain or other specialized rendering techniques. Here's how you would extend it:

## 1. Extend MaterialProperties

First, extend the `MaterialProperties` struct to include tessellation-related properties:

```cpp
struct MaterialProperties {
    // Existing properties
    bool depthWriteEnable = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    
    // Shader paths
    std::string vertShaderPath = "texture_shader.vert";
    std::string fragShaderPath = "texture_shader.frag";
    
    // New tessellation shader paths (optional)
    std::string tessControlShaderPath = "";  // Empty means not used
    std::string tessEvalShaderPath = "";     // Empty means not used
    
    // Tessellation parameters
    uint32_t patchControlPoints = 3;  // Default for triangular patches
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    // Additional tessellation state
    float tessellationFactor = 1.0f;  // Default tessellation level
    bool wireframe = false;           // Wireframe rendering
};
```

## 2. Create a TerrainMaterial Class

Create a specialized material class for terrain:

```cpp
class TerrainMaterial : public Material {
public:
    TerrainMaterial(Device& device, 
                   const std::string& heightmapPath,
                   const std::string& diffuseTexturePath);
    ~TerrainMaterial() override;
    
    VkDescriptorSet getDescriptorSet() const override { return descriptorSet; }
    VkDescriptorSetLayout getDescriptorSetLayout() const override { return descriptorSetLayout; }
    
    static VkDescriptorSetLayout descriptorSetLayout;
    static std::unique_ptr<DescriptorPool> descriptorPool;
    
    void setTessellationFactor(float factor) { 
        properties.tessellationFactor = factor; 
    }
    
    void setWireframe(bool enabled) { 
        properties.wireframe = enabled; 
    }
    
    static void cleanupResources(Device& device);
    
private:
    void createHeightmap(const std::string& heightmapPath);
    void createDiffuseTexture(const std::string& diffuseTexturePath);
    void createTextureImageViews();
    void createTextureSamplers();
    void createDescriptorSet();
    
    static void createDescriptorSetLayoutIfNeeded(Device& device);
    
    // Heightmap texture
    VkImage heightmapImage = VK_NULL_HANDLE;
    VkDeviceMemory heightmapImageMemory = VK_NULL_HANDLE;
    VkImageView heightmapImageView = VK_NULL_HANDLE;
    VkSampler heightmapSampler = VK_NULL_HANDLE;
    
    // Diffuse texture
    VkImage diffuseImage = VK_NULL_HANDLE;
    VkDeviceMemory diffuseImageMemory = VK_NULL_HANDLE;
    VkImageView diffuseImageView = VK_NULL_HANDLE;
    VkSampler diffuseSampler = VK_NULL_HANDLE;
    
    // Descriptor set
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};
```

## 3. Modify Pipeline Creation

Update the pipeline creation to support tessellation shaders:

```cpp
// In TextureRenderSystem::createPipeline
PipelineInfo TextureRenderSystem::createPipeline(
    const MaterialProperties& properties) {
    
    std::string key = getPipelineKey(properties);
    
    // Check if pipeline already exists
    auto it = pipelineCache.find(key);
    if (it != pipelineCache.end()) {
        return it->second;
    }
    
    // Create new pipeline
    PipelineInfo pipelineInfo{};
    
    // Get descriptor set layout based on material type
    VkDescriptorSetLayout materialSetLayout = TextureMaterial::descriptorSetLayout;
    if (properties.vertShaderPath == "skybox_shader.vert") {
        materialSetLayout = CubemapMaterial::descriptorSetLayout;
    } else if (properties.tessControlShaderPath != "") {
        materialSetLayout = TerrainMaterial::descriptorSetLayout;
    }
    
    createPipelineLayout(globalSetLayout, materialSetLayout, pipelineInfo.pipelineLayout);
    
    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    
    // Apply material properties
    pipelineConfig.depthStencilInfo.depthWriteEnable = properties.depthWriteEnable ? VK_TRUE : VK_FALSE;
    pipelineConfig.depthStencilInfo.depthCompareOp = properties.depthCompareOp;
    pipelineConfig.rasterizationInfo.cullMode = properties.cullMode;
    
    // Set wireframe mode if enabled
    if (properties.wireframe) {
        pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
    }
    
    // Configure for tessellation if tessellation shaders are specified
    if (!properties.tessControlShaderPath.empty() && !properties.tessEvalShaderPath.empty()) {
        // Enable tessellation
        pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        pipelineConfig.tessellationInfo.patchControlPoints = properties.patchControlPoints;
        
        // Create pipeline with tessellation shaders
        pipelineInfo.pipeline = std::make_unique<Pipeline>(
            device, 
            properties.vertShaderPath,
            properties.tessControlShaderPath,
            properties.tessEvalShaderPath,
            properties.fragShaderPath,
            pipelineConfig
        );
    } else {
        // Create regular pipeline
        pipelineInfo.pipeline = std::make_unique<Pipeline>(
            device, 
            properties.vertShaderPath, 
            properties.fragShaderPath, 
            pipelineConfig
        );
    }
    
    // Cache and return
    pipelineCache[key] = pipelineInfo;
    return pipelineInfo;
}
```

## 4. Extend the Pipeline Class

Extend the Pipeline class to support tessellation shaders:

```cpp
// In vk_pipeline.h
class Pipeline {
public:
    // Existing constructor
    Pipeline(Device& device, 
             const std::string& vertFilepath, 
             const std::string& fragFilepath, 
             const PipelineConfigInfo& configInfo);
    
    // New constructor for tessellation pipelines
    Pipeline(Device& device, 
             const std::string& vertFilepath,
             const std::string& tessControlFilepath,
             const std::string& tessEvalFilepath,
             const std::string& fragFilepath, 
             const PipelineConfigInfo& configInfo);
    
    // Rest of the class...
};

// In vk_pipeline.cpp
Pipeline::Pipeline(Device& device, 
                  const std::string& vertFilepath,
                  const std::string& tessControlFilepath,
                  const std::string& tessEvalFilepath,
                  const std::string& fragFilepath, 
                  const PipelineConfigInfo& configInfo) 
    : device{device} {
    
    createGraphicsPipeline(vertFilepath, tessControlFilepath, tessEvalFilepath, fragFilepath, configInfo);
}

void Pipeline::createGraphicsPipeline(
    const std::string& vertFilepath,
    const std::string& tessControlFilepath,
    const std::string& tessEvalFilepath,
    const std::string& fragFilepath,
    const PipelineConfigInfo& configInfo) {
    
    // Load shader modules
    auto vertCode = readFile(vertFilepath);
    auto fragCode = readFile(fragFilepath);
    auto tessControlCode = readFile(tessControlFilepath);
    auto tessEvalCode = readFile(tessEvalFilepath);
    
    createShaderModule(vertCode, &vertShaderModule);
    createShaderModule(fragCode, &fragShaderModule);
    createShaderModule(tessControlCode, &tessControlShaderModule);
    createShaderModule(tessEvalCode, &tessEvalShaderModule);
    
    // Create shader stages
    VkPipelineShaderStageCreateInfo shaderStages[4];
    
    // Vertex shader stage
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;
    
    // Tessellation control shader stage
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    shaderStages[1].module = tessControlShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;
    
    // Tessellation evaluation shader stage
    shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[2].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    shaderStages[2].module = tessEvalShaderModule;
    shaderStages[2].pName = "main";
    shaderStages[2].flags = 0;
    shaderStages[2].pNext = nullptr;
    shaderStages[2].pSpecializationInfo = nullptr;
    
    // Fragment shader stage
    shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[3].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[3].module = fragShaderModule;
    shaderStages[3].pName = "main";
    shaderStages[3].flags = 0;
    shaderStages[3].pNext = nullptr;
    shaderStages[3].pSpecializationInfo = nullptr;
    
    // Create pipeline with all shader stages
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 4;
    pipelineInfo.pStages = shaderStages;
    // ... rest of pipeline creation
}
```

## 5. Create Tessellation Shaders for Terrain

```glsl
// terrain_shader.vert
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    int hasTexture;
    float tessellationFactor;
} push;

layout(location = 0) out vec2 outUV;

void main() {
    outUV = uv;
    // No transformation here - will be done in tessellation evaluation shader
}
```

```glsl
// terrain_shader.tesc
#version 450
#extension GL_ARB_tessellation_shader : enable

layout(vertices = 3) out;

layout(location = 0) in vec2 inUV[];
layout(location = 0) out vec2 outUV[];

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    int hasTexture;
    float tessellationFactor;
} push;

void main() {
    // Pass attributes through
    outUV[gl_InvocationID] = inUV[gl_InvocationID];
    
    // Set tessellation levels
    if (gl_InvocationID == 0) {
        // Outer tessellation levels
        gl_TessLevelOuter[0] = push.tessellationFactor;
        gl_TessLevelOuter[1] = push.tessellationFactor;
        gl_TessLevelOuter[2] = push.tessellationFactor;
        
        // Inner tessellation level
        gl_TessLevelInner[0] = push.tessellationFactor;
    }
    
    // Pass position through
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
```

```glsl
// terrain_shader.tese
#version 450
#extension GL_ARB_tessellation_shader : enable

layout(triangles, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    int hasTexture;
    float tessellationFactor;
} push;

layout(set = 1, binding = 0) uniform sampler2D heightmapSampler;
layout(set = 1, binding = 1) uniform sampler2D diffuseSampler;

layout(location = 0) in vec2 inUV[];
layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;

void main() {
    // Interpolate UVs
    vec2 uv = gl_TessCoord.x * inUV[0] + 
              gl_TessCoord.y * inUV[1] + 
              gl_TessCoord.z * inUV[2];
    
    // Interpolate position
    vec4 pos = gl_TessCoord.x * gl_in[0].gl_Position + 
               gl_TessCoord.y * gl_in[1].gl_Position + 
               gl_TessCoord.z * gl_in[2].gl_Position;
    
    // Sample heightmap and displace vertex
    float height = texture(heightmapSampler, uv).r * 10.0; // Scale as needed
    pos.y += height;
    
    // Transform position
    gl_Position = ubo.projection * ubo.view * push.modelMatrix * pos;
    
    // Calculate normal
    vec2 texelSize = 1.0 / textureSize(heightmapSampler, 0);
    float heightL = texture(heightmapSampler, uv - vec2(texelSize.x, 0)).r * 10.0;
    float heightR = texture(heightmapSampler, uv + vec2(texelSize.x, 0)).r * 10.0;
    float heightD = texture(heightmapSampler, uv - vec2(0, texelSize.y)).r * 10.0;
    float heightU = texture(heightmapSampler, uv + vec2(0, texelSize.y)).r * 10.0;
    
    vec3 normal = normalize(vec3(
        heightL - heightR,
        2.0, // Scale for steepness
        heightD - heightU
    ));
    
    // Output to fragment shader
    fragUV = uv;
    fragColor = texture(diffuseSampler, uv).rgb;
    fragNormal = mat3(push.normalMatrix) * normal;
}
```

```glsl
// terrain_shader.frag
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
} ubo;

void main() {
    // Simple lighting calculation
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diffuse = max(dot(normalize(fragNormal), lightDir), 0.0);
    
    vec3 ambient = ubo.ambientLightColor.rgb * ubo.ambientLightColor.a;
    vec3 diffuseLight = vec3(1.0) * diffuse;
    
    outColor = vec4(fragColor * (ambient + diffuseLight), 1.0);
}
```

## 6. Using the Terrain Material

```cpp
// Create a terrain model
Model::Builder terrainBuilder{};
// Create a grid of vertices for the terrain
// ...

auto terrainModel = std::make_unique<Model>(device, terrainBuilder);

// Create a terrain material
auto terrainMaterial = std::make_shared<TerrainMaterial>(
    device,
    "textures:terrain/heightmap.png",
    "textures:terrain/diffuse.png"
);

// Configure tessellation properties
terrainMaterial->setProperty("tessControlShaderPath", "terrain_shader.tesc");
terrainMaterial->setProperty("tessEvalShaderPath", "terrain_shader.tese");
terrainMaterial->setProperty("vertShaderPath", "terrain_shader.vert");
terrainMaterial->setProperty("fragShaderPath", "terrain_shader.frag");
terrainMaterial->setProperty("patchControlPoints", 3);
terrainMaterial->setProperty("topology", VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
terrainMaterial->setTessellationFactor(16.0f); // Higher values = more detail

// Assign material to model
terrainModel->setMaterial(terrainMaterial);

// Create terrain game object
auto terrain = std::make_unique<GameObject>();
terrain->setModel(std::move(terrainModel));

// Add to scene
sceneManager->addManagedPhysicsEntity(std::move(terrain));
```

## Key Points About This Extension

1. **Material-Driven Approach**: The material system is extended to support tessellation shaders, maintaining the material-driven design.

2. **Flexible Pipeline Creation**: The pipeline creation is modified to support different shader stages based on material properties.

3. **Specialized Material Class**: A specialized TerrainMaterial class is created to handle terrain-specific properties and resources.

4. **Dynamic Tessellation Control**: Tessellation factors can be adjusted dynamically through push constants.

5. **Heightmap-Based Displacement**: The terrain is displaced based on a heightmap texture, creating realistic terrain geometry.

This approach maintains the clean material-based design while extending it to support more advanced rendering techniques like tessellation. The same pattern can be used to support other specialized rendering techniques (geometry shaders, compute shaders, etc.) by extending the material system and pipeline creation accordingly.