/*
 * Copyright 2023 TU Wien, Institute of Visual Computing & Human-Centered Technology.
 * This file is part of the GCG Lab Framework and must not be redistributed.
 *
 * Original version created by Lukas Gersthofer and Bernhard Steiner.
 * Vulkan edition created by Johannes Unterguggenberger (junt@cg.tuwien.ac.at).
 */
#include "PathUtils.h"
#include "Utils.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <array>

#include <sstream>
#include "camera/Camera.h"
#include "Geometry.h"

#undef min
#undef max

/* --------------------------------------------- */
// Helper Function Declarations
/* --------------------------------------------- */

/*!
 *	Add extension_name to the target vector ref_vector if the extension is supported on this system.
 *	@param	extension_name		The instance extension that shall be added to ref_vector
 *	@param	ref_vector			Reference to the vector that the instance extension shall be added to, if supported
 */
void addInstanceExtensionToVectorIfSupported(const char* extension_name, std::vector<const char*>& ref_vector);

/*!
 *	Add validation_layer_name to the target vector ref_vector if the validation layer is supported on this system:
 *	@param	validation_layer_name	The validation layer name that shall be added to ref_vector
 *	@param	ref_vector				Reference to the vector that the validation layer name shall be added to, if
 *supported
 */
void addValidationLayerNameToVectorIfSupported(const char* validation_layer_name, std::vector<const char*>& ref_vector);

/*!
 *	Add extension_name to the target vector ref_vector if the extension is supported by the given physical device.
 *	@param	extension_name		The device extension that shall be added to ref_vector
 *	@param	physical_device		The physical device handle which must support the given extension
 *	@param	ref_vector			Reference to the vector that the device extension shall be added to, if supported
 */
void addDeviceExtensionToVectorIfSupported(const char* extension_name, VkPhysicalDevice physical_device, std::vector<const char*>& ref_vector);
/*!
 *	From the given list of physical devices, select the first one that satisfies all requirements.
 *	@param		physical_devices		A pointer which points to contiguous memory of #physical_device_count sequentially
                                        stored VkPhysicalDevice handles is expected. The handles can (or should) be those
 *										that are returned from vkEnumeratePhysicalDevices.
 *	@param		physical_device_count	The number of consecutive physical device handles there are at the memory location
 *										that is pointed to by the physical_devices parameter.
 *	@param		surface					A valid VkSurfaceKHR handle, which is used to determine if a certain
 *										physical device supports presenting images to the given surface.
 *	@return		The index of the physical device that satisfies all requirements is returned.
 */
uint32_t selectPhysicalDeviceIndex(const VkPhysicalDevice* physical_devices, uint32_t physical_device_count, VkSurfaceKHR surface);

/*!
 *	From the given list of physical devices, select the first one that satisfies all requirements.
 *	@param		physical_devices	A vector containing all available VkPhysicalDevice handles, like those
 *									that are returned from vkEnumeratePhysicalDevices.
 *	@param		surface				A valid VkSurfaceKHR handle, which is used to determine if a certain
 *									physical device supports presenting images to the given surface.
 *	@return		The index of the physical device that satisfies all requirements is returned.
 */
uint32_t selectPhysicalDeviceIndex(const std::vector<VkPhysicalDevice>& physical_devices, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, select a queue family which supports both,
 *	graphics and presentation to the given surface. Return the INDEX of an appropriate queue family!
 *	@return		The index of a queue family which supports the required features shall be returned.
 */
uint32_t selectQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, a the physical device's surface capabilites are read and returned.
 *	@return		VkSurfaceCapabilitiesKHR data
 */
VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, a supported surface image format
 *	which can be used for the framebuffer's attachment formats is searched and returned.
 *	@return		A supported format is returned.
 */
VkSurfaceFormatKHR getSurfaceImageFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, return its surface transform flag.
 *	This can be used to set the swap chain to the same configuration as the surface's current transform.
 *	@return		The surface capabilities' currentTransform value is returned, which is suitable for swap chain config.
 */
VkSurfaceTransformFlagBitsKHR getSurfaceTransform(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	It matches the definition and sizes of the corresponding GPU-side struct exactly, which is used in shaders.
 */
struct UniformBuffer {
    /*! A color values, stored as four floats, to properly align to 16 byte boundaries. */
    glm::vec4 color;


    /*! Storage for the model matrix, consisting of 16 float values (inherently aligned to 16 bytes) */
    glm::mat4 modelMatrix;

    /*! Storage for the model matrix suitable for normals transformation, consisting of 16 float values (inherently aligned to 16 bytes) */
    glm::mat4 modelMatrixForNormals;

    /*! Storage for the view-projection matrix, consisting of 16 float values (inherently aligned to 16 bytes) */
    glm::mat4 viewProjMatrix;

    /*! Storage for the camera's world space position (aligned to 16 bytes) */
    glm::vec4 cameraPosition;

    /*! Illumination properties ka, kd, ks, alpha (in that order)
     *	First three are material coefficients, the last one is specular alpha. */
    glm::vec4 materialProperties;

    /*! stores user input as magic numbers */
    glm::ivec4 userInput;
};

/*!
 *	This struct contains the data of a directional light.
 *	It matches the definition and sizes of the corresponding GPU-side struct exactly, which is used in shaders.
 */
struct DirectionalLight {
    /*! Light color of this light source */
    glm::vec4 color;

    /*! Light direction of this directional light source */
    glm::vec4 direction;
};

/*!
 *	This struct contains the data of a point light.
 *	It matches the definition and sizes of the corresponding GPU-side struct exactly, which is used in shaders.
 */
struct PointLight {
    /*! Light color of this light source */
    glm::vec4 color;

    /*! Position of this light source in world space */
    glm::vec4 position;

    /*! Attenuation properties of this light source */
    glm::vec4 attenuation;
};

/*!
 *	Allocates a new descriptor set of the given layout from the given descriptor pool.
 *	It is not required to cleanup the returned descriptor set explicitly, it will be cleaned up when the descriptor pool is destroyed.
 *	@param	device					Valid handle to the logical device
 *	@param	descriptor_pool			Valid handle to a descriptor pool
 *	@param	descriptor_set_layout	Valid handle to a descriptor set layout
 */
VkDescriptorSet allocDescriptorSet(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout);

/*!
 *	Writes the descriptor information to a given descriptor set which describes one uniform buffer at binding = 0.
 *	The given descriptor set must have been created from a descriptor set layout according to this structure.
 *	@param	device					Valid handle to the logical device
 *	@param	descriptor_set			Valid handle to a descriptor set, which concrete descriptor information will be written to.
 *	@param	uniform_buffer			A descriptor for this uniform buffer will be written to binding = 0
 */
void writeDescriptorSet(VkDevice device, VkDescriptorSet descriptor_set, VkBuffer uniform_buffer);

/*!
 *	Writes the descriptor information to a given descriptor set which describes one uniform buffer at binding = 0,
 *	another uniform buffer at binding = 1, and yet another uniform buffer at binding = 2.
 *	The given descriptor set must have been created from a descriptor set layout according to this structure.
 *	@param	device					Valid handle to the logical device
 *	@param	descriptor_set			Valid handle to a descriptor set, which concrete descriptor information will be written to.
 *	@param	object_data				A descriptor for this uniform buffer will be written to binding = 0
 *	@param	directional_light_data	A descriptor for this uniform buffer will be written to binding = 1
 *	@param	point_light_data		A descriptor for this uniform buffer will be written to binding = 2
 */
void writeDescriptorSet(VkDevice device, VkDescriptorSet descriptor_set, VkBuffer object_data, VkBuffer directional_light_data, VkBuffer point_light_data);

/*!
 *	Writes the descriptor information to a given descriptor set which describes one uniform buffer at binding = 0,
 *	another uniform buffer at binding = 1, and yet another uniform buffer at binding = 2, furthermore,
 *	a combined image sampler descriptor is written for binding = 3
 *	The given descriptor set must have been created from a descriptor set layout according to this structure.
 *	@param	device					Valid handle to the logical device
 *	@param	descriptor_set			Valid handle to a descriptor set, which concrete descriptor information will be written to.
 *	@param	object_data				A descriptor for this uniform buffer will be written to binding = 0
 *	@param	directional_light_data	A descriptor for this uniform buffer will be written to binding = 1
 *	@param	point_light_data		A descriptor for this uniform buffer will be written to binding = 2
 *	@param	image_view				A combined descriptor together with sampler will be written to binding = 3
 *	@param	sampler					A combined descriptor together with image_view will be written to binding = 3
 */
void writeDescriptorSet(VkDevice device, VkDescriptorSet descriptor_set, VkBuffer object_data, VkBuffer directional_light_data, VkBuffer point_light_data,
    VkImageView image_view, VkSampler sampler);

/*!
 *	This callback function gets invoked by GLFW whenever a GLFW error occured.
 */
void errorCallbackFromGlfw(int error, const char* description) { std::cout << "GLFW error " << error << ": " << description << std::endl; }

static bool g_dragging = false;
static bool g_strafing = false;
static float g_zoom = 5.0f;

//FPV
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static double lastX = 800, lastY = 800;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = lastX -xpos;
    double yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f; // Change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    camera->yaw += xoffset;
    camera->pitch += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (camera->pitch > 89.0f)
        camera->pitch = 89.0f;
    if (camera->pitch < -89.0f)
        camera->pitch = -89.0f;

    camera->updateCameraVectors();
}


/*!
 *	This callback function gets invoked by GLFW during glfwPollEvents() if there was
 *	mouse scroll input that can be processed by our application.
 */
void scrollCallbackFromGlfw(GLFWwindow* glfw_window, double xoffset, double yoffset) { g_zoom -= static_cast<float>(yoffset) * 0.5f; }


/*!
 *	0 ... fill polygons
 *	1 ... wireframe mode
 */
static int g_polygon_mode_index = 0;

/*!
 *	0 ... no culling
 *	1 ... cull back faces
 *	2 ... cull front faces
 */
static int g_culling_index = 0;

/*!
 *	Bind the given descriptor se to use the material it represents for subsequent draw calls
 *	with the given pipeline, and render the given geometry (using its vertex and index buffers).
 *	Record everything into the current command buffer as provided by the framework.
 *	@param	pipeline		Valid handle to a given pipeline which shall be used for drawing.
 *	@param	geometry		Reference to a geometry object containing the buffers to be used for drawing.
 *	@param	material		Valid handle to a descriptor set that refers to resources that contain material properties.
 *	@param	num_instances	How many instances to draw of the given geometry. Default = one single instance.
 */
void drawGeometryWithMaterial(VkPipeline pipeline, const Geometry& geometry, VkDescriptorSet material, uint32_t num_instances = 1u);

static bool g_draw_normals = false;
static bool g_draw_texcoords = false;

/*!
 *	A flag that will be set during initialization code.
 *	If set to true, it will indicate that Synchronization2 is supported and can be used.
 */
static bool g_synchronization2_supported = false;

/*!
 *	Function pointer to the implementation of vkCmdPipelineBarrier2KHR() which is not
 *	loaded by default since it is an extension. We have to get the pointer manually,
 *	and we do that only in the case where Synchronization2 is supported.
 */
PFN_vkCmdPipelineBarrier2KHR g_vkCmdPipelineBarrier2KHR;

/*!
 *	Creates an image view for the given image and returns its handle.
 *	The returned handle is supposed to be destroyed with vkDestroyImageView
 */
VkImageView createImageViewForImage(VkDevice device, VkImage image, VkImageViewType view_type, VkFormat format);

/*!
 *	Struct used as the return value for loadImage
 */
struct ImageAndView {
    /*! Contains the created image handle */
    VkImage image;

    /*! Contains the created image view handle */
    VkImageView view;
};

/*!
 *	Copies the data from file into a VkBuffer, and then copies it from that buffer into the given image.
 *	Waits until the operation has finished on the GPU.
 */
ImageAndView loadImage(VkDevice device, VkQueue queue, VkCommandPool command_pool, std::string image_file_name);

/* --------------------------------------------- */
// Main
/* --------------------------------------------- */

int main(int argc, char** argv) {
    VKL_LOG(":::::: WELCOME TO GCG 2023 ::::::");

    CMDLineArgs cmdline_args;
    gcgParseArgs(cmdline_args, argc, argv);

    /* --------------------------------------------- */
    // Subtask 1.1: Load Settings From File
    /* --------------------------------------------- */

    int window_width = 800;
    int window_height = 800;
    bool fullscreen = false;
    std::string window_title = "Task 0";
    INIReader window_reader("assets/settings/window.ini");

    window_width = window_reader.GetInteger("window", "width", 800);
    window_height = window_reader.GetInteger("window", "height", 800);
    fullscreen = window_reader.GetBoolean("window", "fullscreen", false);
    window_title = window_reader.Get("window", "title", "GCG 2023");
    std::string init_camera_filepath = "assets/settings/camera_front.ini";
    if (cmdline_args.init_camera) {
        init_camera_filepath = cmdline_args.init_camera_filepath;
    }
    INIReader camera_reader(init_camera_filepath);

    float field_of_view = static_cast<float>(camera_reader.GetReal("camera", "fov", 60.0f));
    float near_plane_distance = static_cast<float>(camera_reader.GetReal("camera", "near", 0.1f));
    float far_plane_distance = static_cast<float>(camera_reader.GetReal("camera", "far", 100.0f));
    float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);
    float camera_yaw = static_cast<float>(camera_reader.GetReal("camera", "yaw", 0.0f));
    float camera_pitch = static_cast<float>(camera_reader.GetReal("camera", "pitch", 0.0f));
    std::string init_renderer_filepath = "assets/settings/renderer_standard.ini";
    if (cmdline_args.init_renderer) {
        init_renderer_filepath = cmdline_args.init_renderer_filepath;
    }
    INIReader renderer_reader(init_renderer_filepath);
    bool as_wireframe = renderer_reader.GetBoolean("renderer", "wireframe", false);
    if (as_wireframe) {
        g_polygon_mode_index = 1;
    }
    bool with_backface_culling = renderer_reader.GetBoolean("renderer", "backface_culling", false);
    if (with_backface_culling) {
        g_culling_index = 1;
    }
    g_draw_normals = renderer_reader.GetBoolean("renderer", "normals", false);
    g_draw_texcoords = renderer_reader.GetBoolean("renderer", "texcoords", false);
    bool depthtest = renderer_reader.GetBoolean("renderer", "depthtest", true);

    // Install a callback function, which gets invoked whenever a GLFW error occurred.
    glfwSetErrorCallback(errorCallbackFromGlfw);

    /* --------------------------------------------- */
    // Subtask 1.2: Create a Window with GLFW
    /* --------------------------------------------- */
    if (!glfwInit()) {
        VKL_EXIT_WITH_ERROR("Failed to init GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No need to create a graphics context for Vulkan
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Use a monitor if we'd like to open the window in fullscreen mode:
    GLFWmonitor* monitor = nullptr;
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
    }

    GLFWwindow* window = nullptr;
    window = glfwCreateWindow(window_width, window_height, window_title.c_str(), monitor, nullptr);

    if (!window) {
        VKL_LOG("If your program reaches this point, that means two things:");
        VKL_LOG("1) Project setup was successful. Everything is working fine.");
        VKL_LOG("2) You haven't implemented Subtask 1.2, which is creating a window with GLFW.");
        VKL_EXIT_WITH_ERROR("No GLFW window created.");
    }
    VKL_LOG("Subtask 1.2 done.");

    VkResult result;
    VkInstance vk_instance = VK_NULL_HANDLE;              // To be set during Subtask 1.3
    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;             // To be set during Subtask 1.4
    VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE; // To be set during Subtask 1.5
    VkDevice vk_device = VK_NULL_HANDLE;                  // To be set during Subtask 1.7
    VkQueue vk_queue = VK_NULL_HANDLE;                    // To be set during Subtask 1.7
    VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;         // To be set during Subtask 1.8

    /* --------------------------------------------- */
    // Subtask 1.3: Create a Vulkan Instance
    /* --------------------------------------------- */
    VkApplicationInfo application_info = {};                     // Zero-initialize every member
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Set this struct instance's type
    application_info.pEngineName = "GCG_VK_Library";             // Set some properties...
    application_info.engineVersion = VK_MAKE_API_VERSION(0, 2023, 9, 1);
    application_info.pApplicationName = "GCG_VK_Solution";
    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 2023, 9, 19);
    application_info.apiVersion = VK_API_VERSION_1_1;            // Your system needs to support this Vulkan API version.

    VkInstanceCreateInfo instance_create_info = {};                      // Zero-initialize every member
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Set the struct's type
    instance_create_info.pApplicationInfo = &application_info;

    // A vector to hold all our requested instance extensions:
    std::vector<const char*> instance_extensions;

    // Query extensions which are required by GLFW:
    uint32_t glfw_instance_extensions_count;
    const char** glfw_instance_extensions_names = glfwGetRequiredInstanceExtensions(&glfw_instance_extensions_count);
    // And add them all to our vector:
    for (uint32_t i = 0; i < glfw_instance_extensions_count; ++i) {
        addInstanceExtensionToVectorIfSupported(glfw_instance_extensions_names[i], instance_extensions);
    }

    // Query extensions which are required by the framework:
    uint32_t framework_instance_extensions_count;
    const char** framework_instance_extensions_names = vklGetRequiredInstanceExtensions(&framework_instance_extensions_count);
    // And add them all to our vector:
    for (uint32_t i = 0; i < framework_instance_extensions_count; ++i) {
        addInstanceExtensionToVectorIfSupported(framework_instance_extensions_names[i], instance_extensions);
    }
#ifdef __APPLE__
    addInstanceExtensionToVectorIfSupported(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, instance_extensions);
#endif

    // Set info in the VkInstanceCreateInfo struct:
    instance_create_info.enabledExtensionCount = instance_extensions.size();
    instance_create_info.ppEnabledExtensionNames = instance_extensions.data();

    // A vector to hold all our requested validation layers, add standard validation, and set info in the VkInstanceCreateInfo struct:
    std::vector<const char*> enabled_layer_names;
    addValidationLayerNameToVectorIfSupported("VK_LAYER_KHRONOS_validation", enabled_layer_names);
    instance_create_info.enabledLayerCount = enabled_layer_names.size();
    instance_create_info.ppEnabledLayerNames = enabled_layer_names.data();
#ifdef __APPLE__
    instance_create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // Create the instance
    result = vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    VKL_CHECK_VULKAN_RESULT(result);
    if (!vk_instance) {
        VKL_EXIT_WITH_ERROR("No VkInstance created or handle not assigned.");
    }
    VKL_LOG("Subtask 1.3 done.");

    /* --------------------------------------------- */
    // Subtask 1.4: Create a Vulkan Window Surface
    /* --------------------------------------------- */
    result = glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface);
    VKL_CHECK_VULKAN_RESULT(result);
    if (!vk_surface) {
        VKL_EXIT_WITH_ERROR("No VkSurfaceKHR created or handle not assigned.");
    }
    VKL_LOG("Subtask 1.4 done.");

    /* --------------------------------------------- */
    // Subtask 1.5: Pick a Physical Device
    /* --------------------------------------------- */
    // Query the number of physical devices:
    uint32_t physical_devices_count;
    vkEnumeratePhysicalDevices(vk_instance, &physical_devices_count, nullptr);

    if (physical_devices_count == 0) {
        VKL_EXIT_WITH_ERROR("Vulkan does not recognize any physical devices.");
    }

    std::vector<VkPhysicalDevice> physical_devices(physical_devices_count);
    vkEnumeratePhysicalDevices(vk_instance, &physical_devices_count, physical_devices.data());

    uint32_t selected_physical_device_index = selectPhysicalDeviceIndex(physical_devices, vk_surface);
    vk_physical_device = physical_devices[selected_physical_device_index];
    if (!vk_physical_device) {
        VKL_EXIT_WITH_ERROR("No VkPhysicalDevice selected or handle not assigned.");
    }
    VKL_LOG("Subtask 1.5 done.");

    /* --------------------------------------------- */
    // Subtask 1.6: Select a Queue Family
    /* --------------------------------------------- */
    std::array<float, 1> queue_priorities = {1.0f};

    uint32_t selected_queue_family_index = selectQueueFamilyIndex(vk_physical_device, vk_surface);
    VkDeviceQueueCreateInfo device_queue_create_info = {};
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = selected_queue_family_index;
    device_queue_create_info.queueCount = 1u;
    device_queue_create_info.pQueuePriorities = queue_priorities.data();

    // Sanity check if we have selected a valid queue family index:
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr);
    if (selected_queue_family_index >= queue_family_count) {
        VKL_EXIT_WITH_ERROR("Invalid queue family index selected.");
    }
    VKL_LOG("Subtask 1.6 done.");

    /* --------------------------------------------- */
    // Subtask 1.7: Create a Logical Device and Get Queue
    /* --------------------------------------------- */
    std::vector<const char*> device_extensions;
    addDeviceExtensionToVectorIfSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, vk_physical_device, device_extensions);
    addDeviceExtensionToVectorIfSupported(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, vk_physical_device, device_extensions);
    // Looks like SDK 1.2.170 also requires
    addDeviceExtensionToVectorIfSupported(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, vk_physical_device, device_extensions);
    // See if this device supports Synchronization2, and if so, set a flag to indicate that we are going to use Synchronization2:
    if (std::find(std::begin(device_extensions), std::end(device_extensions), std::string(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)) !=
        std::end(device_extensions)) {
        g_synchronization2_supported = true;
    }
#ifdef __APPLE__
    addDeviceExtensionToVectorIfSupported("VK_KHR_portability_subset", vk_physical_device, device_extensions);
#endif

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // Add information about queues to be created, and extensions to be enabled:
    device_create_info.queueCreateInfoCount = 1u;
    device_create_info.pQueueCreateInfos = &device_queue_create_info;
    device_create_info.enabledExtensionCount = device_extensions.size();
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    VkPhysicalDeviceFeatures enabled_physical_device_features = {};
    enabled_physical_device_features.fillModeNonSolid = VK_TRUE;
    device_create_info.pEnabledFeatures = &enabled_physical_device_features;

    // Enable Synchronization2 and hook it into the pNext chain:
    VkPhysicalDeviceSynchronization2FeaturesKHR physical_device_sync2_features = {};
    physical_device_sync2_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    physical_device_sync2_features.synchronization2 = VK_TRUE;
    if (g_synchronization2_supported) {
        device_create_info.pNext = &physical_device_sync2_features;
    }

    // Create the logical device
    result = vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &vk_device);
    VKL_CHECK_VULKAN_RESULT(result);

    if (g_synchronization2_supported) {
        auto* procAddr = vkGetDeviceProcAddr(vk_device, "vkCmdPipelineBarrier2KHR");
        if (procAddr == nullptr) {
            // Couldn't get the function pointer to vkCmdPipelineBarrier2KHR
            g_synchronization2_supported = false;
        } else {
            g_vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(procAddr);
        }
    }
    if (!vk_device) {
        VKL_EXIT_WITH_ERROR("No VkDevice created or handle not assigned.");
    }

    // Get the handle of the queue that was requested:
    vkGetDeviceQueue(vk_device, selected_queue_family_index, 0u, &vk_queue);
    if (!vk_queue) {
        VKL_EXIT_WITH_ERROR("No VkQueue selected or handle not assigned.");
    }
    VKL_LOG("Subtask 1.7 done.");

    /* --------------------------------------------- */
    // Subtask 1.8: Create a Swapchain
    /* --------------------------------------------- */
    uint32_t queueFamilyIndexCount = 0u;
    std::vector<uint32_t> queueFamilyIndices;
    VkSurfaceFormatKHR surface_format = getSurfaceImageFormat(vk_physical_device, vk_surface);
    queueFamilyIndices.push_back(selected_queue_family_index);
    queueFamilyIndexCount = 1u;
    VkSurfaceCapabilitiesKHR surface_capabilities = getPhysicalDeviceSurfaceCapabilities(vk_physical_device, vk_surface);
    // Build the swapchain config struct:
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vk_surface;
    swapchain_create_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_create_info.imageArrayLayers = 1u;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    } else {
        std::cout << "Warning: Automatic Testing might fail, VK_IMAGE_USAGE_TRANSFER_SRC_BIT image usage is not supported" << std::endl;
    }
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = queueFamilyIndexCount;
    swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices.data();
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = VkExtent2D{static_cast<uint32_t>(window_width), static_cast<uint32_t>(window_height)};
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Create the swapchain:
    result = vkCreateSwapchainKHR(vk_device, &swapchain_create_info, nullptr, &vk_swapchain);
    VKL_CHECK_VULKAN_RESULT(result);
    if (!vk_swapchain) {
        VKL_EXIT_WITH_ERROR("No VkSwapchainKHR created or handle not assigned.");
    }

    // Query how many swapchain images we got:
    uint32_t swapchain_image_count;
    vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchain_image_count, nullptr);

    // Retrieve the swapchain images:
    std::vector<VkImage> swapchain_image_handles(swapchain_image_count);
    vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchain_image_count, swapchain_image_handles.data());
    VKL_LOG("Subtask 1.8 done.");

    /* --------------------------------------------- */
    // Subtask 2.7: Depth Test
    /* --------------------------------------------- */
    VkImage depth_buffer = vklCreateDeviceLocalImageWithBackingMemory(
        vk_physical_device, vk_device, window_width, window_height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VkClearValue depth_clear_value;
    depth_clear_value.depthStencil.depth = 1.0f;
    depth_clear_value.depthStencil.stencil = 0u;

    /* --------------------------------------------- */
    // Subtask 1.9: Init GCG Framework
    /* --------------------------------------------- */

    // Gather swapchain config as required by the framework:
    VklSwapchainConfig swapchain_config = {};

    VkClearValue color_clear_value;
    color_clear_value.color.float32[0] = 0.8f;
    color_clear_value.color.float32[1] = 1.0f;
    color_clear_value.color.float32[2] = 1.0f;
    color_clear_value.color.float32[3] = 1.0f;

    swapchain_config.swapchainHandle = vk_swapchain;
    swapchain_config.imageExtent = swapchain_create_info.imageExtent;
    for (const VkImage& img : swapchain_image_handles) {
        VklSwapchainFramebufferComposition framebufferComposition;
        framebufferComposition.colorAttachmentImageDetails.imageHandle = img;
        framebufferComposition.colorAttachmentImageDetails.imageFormat = swapchain_create_info.imageFormat;
        framebufferComposition.colorAttachmentImageDetails.imageUsage = swapchain_create_info.imageUsage;
        framebufferComposition.colorAttachmentImageDetails.clearValue = color_clear_value;
        if (depthtest) {
            // If we also set the data of the depth buffer, our framebuffer will consist of two images:
            framebufferComposition.depthAttachmentImageDetails.imageHandle = depth_buffer;
            framebufferComposition.depthAttachmentImageDetails.imageFormat = VK_FORMAT_D32_SFLOAT;
            framebufferComposition.depthAttachmentImageDetails.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            framebufferComposition.depthAttachmentImageDetails.clearValue = depth_clear_value;
        }
        swapchain_config.swapchainImages.push_back(framebufferComposition);
    }

    // Init the framework:
    if (!gcgInitFramework(vk_instance, vk_surface, vk_physical_device, vk_device, vk_queue, swapchain_config)) {
        VKL_EXIT_WITH_ERROR("Failed to init framework");
    }
    VKL_LOG("Subtask 1.9 done.");

    /* --------------------------------------------- */
    // Subtask 2.1: Create a Custom Graphics Pipeline
    /* --------------------------------------------- */

    //HUD
    std::string vertexShaderPathHUD = gcgLoadShaderFilePath("assets/shaders_vk/crosshair/crosshair.vert");
    std::string fragmentShaderPathHUD = gcgLoadShaderFilePath("assets/shaders_vk/crosshair/crosshair.frag");
    VkPipeline HUD_pipeline = vklCreateGraphicsPipeline(
            VklGraphicsPipelineConfig {
                    vertexShaderPathHUD.c_str(),
                    fragmentShaderPathHUD.c_str(),
                    {
                            VkVertexInputBindingDescription{0u, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX}, // Correct stride for 2D vertices
                    },
                    {
                            VkVertexInputAttributeDescription{0u, 0u, VK_FORMAT_R32G32B32_SFLOAT, 0u}, // Correct format for 2D coordinates
                    },
                    VK_POLYGON_MODE_FILL,
                    VK_CULL_MODE_NONE,
                    {} // No dynamic data needed
            }
    );

    float crosshairSize = 0.5f; // Adjust based on your needs
    float crosshairThickness = 0.5f; // Adjust based on your needs
    GeometryData crosshairData = createCrosshairGeometry(crosshairSize, crosshairThickness);

    // Assuming `crosshairData` contains your vertex and index data as per `createCrosshairGeometry`
    size_t vertexDataSize = sizeof(glm::vec3) * crosshairData.positions.size();
    // Create vertex buffer
    VkBuffer vertexBuffer = vklCreateHostCoherentBufferWithBackingMemory(vertexDataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    // Copy vertex data into the buffer
    vklCopyDataIntoHostCoherentBuffer(vertexBuffer, crosshairData.positions.data(), vertexDataSize);

    size_t indexDataSize = sizeof(uint32_t) * crosshairData.indices.size();
    // Create index buffer
    VkBuffer indexBuffer = vklCreateHostCoherentBufferWithBackingMemory(indexDataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    // Copy index data into the buffer
    vklCopyDataIntoHostCoherentBuffer(indexBuffer, crosshairData.indices.data(), indexDataSize);




    // clang-format off
    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings = {
        VkDescriptorSetLayoutBinding{0u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
      , VkDescriptorSetLayoutBinding{1u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
      , VkDescriptorSetLayoutBinding{2u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
        /* --------------------------------------------- */
        // Subtask 5.8: Use the Textures in Shaders
        /* --------------------------------------------- */
      , VkDescriptorSetLayoutBinding{3u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    // clang-format on

    /* --------------------------------------------- */
    // Subtask 3.3: Interaction
    /* --------------------------------------------- */
    const size_t POLYMODES = 2;
    const size_t CULLMODES = 3;
    VkPipeline cornell_pipelines[POLYMODES][CULLMODES];
    const size_t ILLUMODES = 1;
    VkPipeline custom_pipelines[POLYMODES][CULLMODES][ILLUMODES]; // Prepare pipelines for all the different combinations of configurations
    VkPolygonMode polygon_modes[POLYMODES] = {VK_POLYGON_MODE_FILL, VK_POLYGON_MODE_LINE};
    VkCullModeFlags cull_modes[CULLMODES] = {VK_CULL_MODE_NONE, VK_CULL_MODE_BACK_BIT, VK_CULL_MODE_FRONT_BIT};
    const char* shaders[ILLUMODES][2] = {{"assets/shaders_vk/texture.vert", "assets/shaders_vk/texture.frag"}};
    // prepare cornell box pipeline
    std::string vertexShaderPath = gcgLoadShaderFilePath("assets/shaders_vk/cornellGouraud.vert");
    std::string fragmentShaderPath = gcgLoadShaderFilePath("assets/shaders_vk/cornellGouraud.frag");
    for (size_t i = 0; i < POLYMODES; ++i) {
        for (size_t j = 0; j < CULLMODES; ++j) {
            // clang-format off
            cornell_pipelines[i][j] = vklCreateGraphicsPipeline(
                VklGraphicsPipelineConfig {
                    vertexShaderPath.c_str(),
                    fragmentShaderPath.c_str(),
                    {
                        VkVertexInputBindingDescription{0u, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX},
                        {VkVertexInputBindingDescription{1u, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX}},
                        {VkVertexInputBindingDescription{2u, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX}}
                    },
                    {
                        VkVertexInputAttributeDescription{0u, 0u, VK_FORMAT_R32G32B32_SFLOAT, 0u},
                        {VkVertexInputAttributeDescription{1u, 1u, VK_FORMAT_R32G32B32_SFLOAT, 0u}},
                        {VkVertexInputAttributeDescription{2u, 2u, VK_FORMAT_R32G32B32_SFLOAT, 0u}}
                    },
                    /* --------------------------------------------- */
                    // Subtask 3.1: Wireframe Mode
                    /* --------------------------------------------- */
                    polygon_modes[i],
                    /* --------------------------------------------- */
                    // Subtask 3.2: Back-face Culling
                    /* --------------------------------------------- */
                    cull_modes[j],
                    descriptor_set_layout_bindings
                }
            );
            // clang-format on
        }
    }

    // Prepare one pipeline for every combination of cull mode and polygon mode:
    for (size_t i = 0; i < POLYMODES; ++i) {
        for (size_t j = 0; j < CULLMODES; ++j) {
            std::vector<std::vector<std::string>> allShaderPaths = gcgFindAllShaderFiles<ILLUMODES, 2>(shaders);
            for (size_t k = 0; k < ILLUMODES; ++k) {
                // clang-format off
                custom_pipelines[i][j][k] = vklCreateGraphicsPipeline(
                    VklGraphicsPipelineConfig {
                        allShaderPaths[k][0].c_str(),
                        allShaderPaths[k][1].c_str(),
                        {
                            VkVertexInputBindingDescription{0u, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX},
                            /* --------------------------------------------- */
                            // Subtask 4.4: Normals As Additional Vertex Attributes
                            /* --------------------------------------------- */
                            VkVertexInputBindingDescription{1u, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX},
                            /* --------------------------------------------- */
                            // Subtask 5.4: Pass UV Coordinates As Vertex Attributes
                            /* --------------------------------------------- */
                            VkVertexInputBindingDescription{2u, sizeof(float) * 2, VK_VERTEX_INPUT_RATE_VERTEX},
                        },
                        {
                            VkVertexInputAttributeDescription{0u, 0u, VK_FORMAT_R32G32B32_SFLOAT, 0u},
                            /* --------------------------------------------- */
                            // Subtask 4.4: Normals As Additional Vertex Attributes
                            /* --------------------------------------------- */
                            VkVertexInputAttributeDescription{1u, 1u, VK_FORMAT_R32G32B32_SFLOAT, 0u},
                            /* --------------------------------------------- */
                            // Subtask 5.4: Pass UV Coordinates As Vertex Attributes
                            /* --------------------------------------------- */
                            VkVertexInputAttributeDescription{2u, 2u, VK_FORMAT_R32G32_SFLOAT, 0u}
                        },
                        /* --------------------------------------------- */
                        // Subtask 3.1: Wireframe Mode
                        /* --------------------------------------------- */
                        polygon_modes[i],
                        /* --------------------------------------------- */
                        // Subtask 3.2: Back-face Culling
                        /* --------------------------------------------- */
                        cull_modes[j],
                        descriptor_set_layout_bindings
                    }
                );
                // clang-format on
            }
        }
    }

    /* --------------------------------------------- */
    // Subtask 2.3: Allocate and Write Descriptors
    /* --------------------------------------------- */
    // clang-format off
    std::vector<VkDescriptorPoolSize> pool_sizes{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 24u}
      , VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12u}
    };
    // clang-format on

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 8u;
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes = pool_sizes.data();

    VkDescriptorPool vk_descriptor_pool;
    result = vkCreateDescriptorPool(vk_device, &descriptor_pool_create_info, nullptr, &vk_descriptor_pool);
    VKL_CHECK_VULKAN_RESULT(result);

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // We can reuse the same layout description that we have passed to graphics pipeline creation:
    descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();

    VkDescriptorSetLayout vk_descriptor_set_layout;
    result = vkCreateDescriptorSetLayout(vk_device, &descriptor_set_layout_create_info, nullptr, &vk_descriptor_set_layout);
    VKL_CHECK_VULKAN_RESULT(result);

    /* --------------------------------------------- */
    // Subtasks 3.5 to 3.7: Geometric Objects
    // Subtask 4.5: Create Uniform Buffers for Lights
    // Subtask 5.5: Load DDS Textures into Images
    /* --------------------------------------------- */

    // Create buffers for the light sources
    VkDeviceSize num_dirlights = 1;
    VkBuffer ub_dirlight = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(DirectionalLight) * num_dirlights, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    DirectionalLight directional_light = {glm::vec4{0.8f, 0.8f, 0.8f, 0.0f}, glm::normalize(glm::vec4{0.0f, -1.0f, -1.0f, 0.0f})};
    vklCopyDataIntoHostCoherentBuffer(ub_dirlight, &directional_light, sizeof(DirectionalLight));

    VkDeviceSize num_pointlights = 1;
    VkBuffer ub_pointlight = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(PointLight) * num_pointlights, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    PointLight point_light = {glm::vec4{1.0f, 1.0f, 1.0f, 0.0f}, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{1.0f, 0.4f, 0.1f, 0.0f}};
    vklCopyDataIntoHostCoherentBuffer(ub_pointlight, &point_light, sizeof(PointLight));

    /* --------------------------------------------- */
    // Subtask 5.5: Load DDS Textures into Images
    /* --------------------------------------------- */
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = selected_queue_family_index;

    VkCommandPool command_pool;
    vkCreateCommandPool(vk_device, &command_pool_create_info, nullptr, &command_pool);

    ImageAndView wood_texture = loadImage(vk_device, vk_queue, command_pool, "assets/textures/wood_texture.dds");
    ImageAndView tiles_diffuse = loadImage(vk_device, vk_queue, command_pool, "assets/textures/tiles_diffuse.dds");

    /* --------------------------------------------- */
    // Subtask 5.7: Create a Sampler
    /* --------------------------------------------- */
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
    VkSampler sampler;
    result = vkCreateSampler(vk_device, &sampler_create_info, nullptr, &sampler);
    VKL_CHECK_VULKAN_RESULT(result);


    // cornell box geometry and material
    Geometry cornell_geometry = createAndUploadIntoGpuMemory(createCornellBoxGeometry(3.0f, 3.0f, 3.0f));
    VkBuffer ub_cornell = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(UniformBuffer), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    VkDescriptorSet ds_cornell = allocDescriptorSet(vk_device, vk_descriptor_pool, vk_descriptor_set_layout);
    writeDescriptorSet(vk_device, ds_cornell, ub_cornell, ub_dirlight, ub_pointlight);

    // Box geometry and material:
    // clang-format off
    Geometry box_geometry = createAndUploadIntoGpuMemory(
        createBoxGeometry(0.34f, 0.34f, 0.34f)

    );
    // clang-format on
    VkBuffer ub_box = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(UniformBuffer), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    VkDescriptorSet ds_box = allocDescriptorSet(vk_device, vk_descriptor_pool, vk_descriptor_set_layout);
    writeDescriptorSet(vk_device, ds_box, ub_box, ub_dirlight, ub_pointlight, wood_texture.view, sampler);
    // Cylinder Material:
    Geometry cylinder_geometry = createAndUploadIntoGpuMemory(createCylinderGeometry(18, 1.5f, 0.2f));
    VkBuffer ub_cylinder = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(UniformBuffer), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    VkDescriptorSet ds_cylinder = allocDescriptorSet(vk_device, vk_descriptor_pool, vk_descriptor_set_layout);
    writeDescriptorSet(vk_device, ds_cylinder, ub_cylinder, ub_dirlight, ub_pointlight, wood_texture.view, sampler);
    // Cylinder Bezier Material:
    std::vector<glm::vec3> controlPoints = {
        glm::vec3(-0.3f, 0.6f, 0.0f),
        glm::vec3(0.0f, 1.6f, 0.0f),
        glm::vec3(1.4f, 0.3f, 0.0f),
        glm::vec3(0.0f, 0.3f, 0.0f),
        glm::vec3(0.0f, -0.5f, 0.0f),
    };
    int numSegments = 42;
    Geometry bezier_cylinder_geometry = createAndUploadIntoGpuMemory(createBezierCylinderGeometry(18, controlPoints, numSegments, 0.2));
    VkBuffer ub_bezier_cylinder = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(UniformBuffer), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    VkDescriptorSet ds_bezier_cylinder = allocDescriptorSet(vk_device, vk_descriptor_pool, vk_descriptor_set_layout);
    writeDescriptorSet(vk_device, ds_bezier_cylinder, ub_bezier_cylinder, ub_dirlight, ub_pointlight, tiles_diffuse.view, sampler);

    // Sphere Material:
    Geometry sphere_geometry = createAndUploadIntoGpuMemory(createSphereGeometry(32, 16, 0.24f));
    VkBuffer ub_sphere = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(UniformBuffer), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    VkDescriptorSet ds_sphere = allocDescriptorSet(vk_device, vk_descriptor_pool, vk_descriptor_set_layout);
    writeDescriptorSet(vk_device, ds_sphere, ub_sphere, ub_dirlight, ub_pointlight, tiles_diffuse.view, sampler);

    /* --------------------------------------------- */
    // Subtask 2.6: Orbit Camera
    /* --------------------------------------------- */

    // Initialize Camera
    Camera camera(field_of_view, aspect_ratio, near_plane_distance, far_plane_distance);
    camera.setYaw(camera_yaw);
    camera.setPitch(camera_pitch);
    static bool isMovingForward = false;
    static bool isMovingBackward = false;
    static bool isMovingLeft = false;
    static bool isMovingRight = false;

    glfwSetWindowUserPointer(window, &camera);


    // Establish a callback function for handling mouse scroll events:
    glfwSetScrollCallback(window, scrollCallbackFromGlfw);
    /* --------------------------------------------- */
    // Subtask 1.10: Set-up the Render Loop
    // Subtask 1.11: Register a Key Callback
    /* --------------------------------------------- */

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));

        if (key == GLFW_KEY_UP) {
            isMovingForward = (action != GLFW_RELEASE);
        }
        if (key == GLFW_KEY_DOWN) {
            isMovingBackward = (action != GLFW_RELEASE);
        }
        if (key == GLFW_KEY_LEFT) {
            isMovingLeft = (action != GLFW_RELEASE);
        }
        if (key == GLFW_KEY_RIGHT) {
            isMovingRight = (action != GLFW_RELEASE);
        }

        if (key == GLFW_KEY_F1) {
            g_polygon_mode_index = 1 - g_polygon_mode_index;
        }
        if (key == GLFW_KEY_F2) {
            g_culling_index = (g_culling_index + 1) % 3;
        }
        if (key == GLFW_KEY_N) {
            g_draw_normals = !g_draw_normals;
        }
        if (key == GLFW_KEY_T) {
            g_draw_texcoords = !g_draw_texcoords;
        }
    });

    double mouse_x, mouse_y;

    // FPV
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);


    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);
    while (!glfwWindowShouldClose(window)) {
        // Handle user input:
        glfwPollEvents();
        glfwGetCursorPos(window, &mouse_x, &mouse_y);







        // Handle continuous movement
        const float cameraSpeed = 0.05f; // Adjust as needed
        if (isMovingForward) {
            camera.moveForward(cameraSpeed);
        }
        if (isMovingBackward) {
            camera.moveBackward(cameraSpeed);
        }
        if (isMovingLeft) {
            camera.moveLeft(cameraSpeed);
        }
        if (isMovingRight) {
            camera.moveRight(cameraSpeed);
        }







        UniformBuffer ub_data;
        ub_data.userInput[0] = g_draw_normals ? 1 : 0;
        ub_data.userInput[1] = g_draw_texcoords ? 1 : 0;






        ub_data.viewProjMatrix = camera.getViewProjMatrix();
        ub_data.cameraPosition = glm::vec4{camera.getPosition(), 1.0f};








        ub_data.color = {1.f, 1.f, 1.f, 1.f};
        // Update cornell box:
        ub_data.color = {0.7f, 0.1f, 0.2f, 1.0f};
        ub_data.modelMatrix = glm::mat4{1.0f};
        ub_data.modelMatrixForNormals = glm::mat4{1.0f};
        ub_data.materialProperties = {0.1f, 0.9f, 0.3f, 10.0f};
        vklCopyDataIntoHostCoherentBuffer(ub_cornell, &ub_data, sizeof(UniformBuffer));
        // Update box:
        ub_data.modelMatrix = glm::translate(glm::mat4{1.0f}, glm::vec3{-0.5f, -0.8f, 0.0f})
                              * glm::rotate(glm::mat4{1.0f}, glm::radians(45.0f), glm::vec3{0.0f, 1.0f, 0.0f});
        ub_data.modelMatrixForNormals = glm::transpose(glm::inverse(ub_data.modelMatrix));
        ub_data.materialProperties = {0.1f, 0.7f, 0.1f, 2.0f};
        vklCopyDataIntoHostCoherentBuffer(ub_box, &ub_data, sizeof(UniformBuffer));

        // Update sphere
        ub_data.modelMatrix = glm::translate(glm::mat4{1.0f}, glm::vec3{0.5f, -0.8f, 0.0f});
        ub_data.modelMatrixForNormals = glm::transpose(glm::inverse(ub_data.modelMatrix));
        ub_data.materialProperties = {0.1f, 0.7f, 0.3f, 8.0f};
        vklCopyDataIntoHostCoherentBuffer(ub_sphere, &ub_data, sizeof(UniformBuffer));

        // Update cylinder:
        ub_data.modelMatrix = glm::translate(glm::mat4{1.0f}, glm::vec3{-0.5f, 0.3f, 0.0f});
        ub_data.modelMatrixForNormals = glm::transpose(glm::inverse(ub_data.modelMatrix));
        ub_data.materialProperties = {0.1f, 0.7f, 0.1f, 2.0f};
        vklCopyDataIntoHostCoherentBuffer(ub_cylinder, &ub_data, sizeof(UniformBuffer));

        // Update bezier cylinder:
        ub_data.modelMatrix = glm::translate(glm::mat4{1.0f}, glm::vec3{0.5f, 0.0f, 0.0f});
        ub_data.modelMatrixForNormals = glm::transpose(glm::inverse(ub_data.modelMatrix));
        ub_data.materialProperties = {0.1f, 0.7f, 0.3f, 8.0f};
        vklCopyDataIntoHostCoherentBuffer(ub_bezier_cylinder, &ub_data, sizeof(UniformBuffer));

        // Wait until we get an image from the swapchain to render into:
        vklWaitForNextSwapchainImage();
        vklStartRecordingCommands();


        VkPipeline selected_cornell_pipeline = cornell_pipelines[g_polygon_mode_index][g_culling_index];
        drawGeometryWithMaterial(selected_cornell_pipeline, cornell_geometry, ds_cornell);
        drawGeometryWithMaterial(custom_pipelines[g_polygon_mode_index][g_culling_index][0], box_geometry, ds_box);
        drawGeometryWithMaterial(custom_pipelines[g_polygon_mode_index][g_culling_index][0], cylinder_geometry, ds_cylinder);
        drawGeometryWithMaterial(custom_pipelines[g_polygon_mode_index][g_culling_index][0], bezier_cylinder_geometry, ds_bezier_cylinder);
        drawGeometryWithMaterial(custom_pipelines[g_polygon_mode_index][g_culling_index][0], sphere_geometry, ds_sphere);

        //HUD
        VkDeviceSize offsets[] = {0};
        vkCmdBindPipeline(vklGetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, HUD_pipeline); // Use your actual command buffer variable
        // Bind the vertex buffer for the crosshair
        vkCmdBindVertexBuffers(vklGetCurrentCommandBuffer(), 0, 1, &vertexBuffer, offsets);
        // If you have an index buffer for the crosshair
        vkCmdBindIndexBuffer(vklGetCurrentCommandBuffer(), indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        // With an index buffer
        vkCmdDrawIndexed(vklGetCurrentCommandBuffer(), crosshairData.indices.size(), 1, 0, 0, 0);





        vklEndRecordingCommands();
        // Present rendered image to the screen:
        vklPresentCurrentSwapchainImage();

        if (cmdline_args.run_headless) {
            uint32_t idx = vklGetCurrentSwapChainImageIndex();
            std::string screenshot_filename = "screenshot";
            if (cmdline_args.set_filename) {
                screenshot_filename = cmdline_args.filename;
            }
            gcgSaveScreenshot(screenshot_filename, swapchain_image_handles[idx], window_width, window_height, surface_format.format, vk_device, vk_physical_device,
                vk_queue, selected_queue_family_index);
            break;
        }
    }

    // Wait for all GPU work to finish before cleaning up:
    vkDeviceWaitIdle(vk_device);

    // Cleanup:
    vklDestroyDeviceLocalImageAndItsBackingMemory(depth_buffer);
    vkDestroyDescriptorSetLayout(vk_device, vk_descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(vk_device, vk_descriptor_pool, nullptr);
    vklDestroyHostCoherentBufferAndItsBackingMemory(ub_cornell);
    destroyGeometryGpuMemory(cornell_geometry);
    vklDestroyHostCoherentBufferAndItsBackingMemory(ub_sphere);
    destroyGeometryGpuMemory(sphere_geometry);
    vklDestroyHostCoherentBufferAndItsBackingMemory(ub_bezier_cylinder);
    destroyGeometryGpuMemory(bezier_cylinder_geometry);
    vklDestroyHostCoherentBufferAndItsBackingMemory(ub_cylinder);
    destroyGeometryGpuMemory(cylinder_geometry);
    vklDestroyHostCoherentBufferAndItsBackingMemory(ub_box);
    destroyGeometryGpuMemory(box_geometry);

    //HUD
    vklDestroyHostCoherentBufferAndItsBackingMemory(vertexBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(indexBuffer);
    vkDestroyPipeline(vk_device, HUD_pipeline, nullptr);

    vkDestroySampler(vk_device, sampler, nullptr);
    vklDestroyHostCoherentBufferAndItsBackingMemory(ub_pointlight);
    vklDestroyHostCoherentBufferAndItsBackingMemory(ub_dirlight);
    vkDestroyImageView(vk_device, tiles_diffuse.view, nullptr);
    vklDestroyDeviceLocalImageAndItsBackingMemory(tiles_diffuse.image);
    vkDestroyImageView(vk_device, wood_texture.view, nullptr);
    vklDestroyDeviceLocalImageAndItsBackingMemory(wood_texture.image);
    vkDestroyCommandPool(vk_device, command_pool, nullptr);

    for (size_t i = 0; i < POLYMODES; ++i) {
        for (size_t j = 0; j < CULLMODES; ++j) {
            vklDestroyGraphicsPipeline(cornell_pipelines[i][j]);
            for (size_t k = 0; k < ILLUMODES; ++k) {
                vklDestroyGraphicsPipeline(custom_pipelines[i][j][k]);
            }
        }
    }

    /* --------------------------------------------- */
    // Subtask 1.12: Cleanup
    /* --------------------------------------------- */
    gcgDestroyFramework();
    vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);
    vkDestroyDevice(vk_device, nullptr);
    vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
    vkDestroyInstance(vk_instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

/* --------------------------------------------- */
// Helper Function Definitions
/* --------------------------------------------- */

void addInstanceExtensionToVectorIfSupported(const char* extension_name, std::vector<const char*>& ref_vector) {
    VkResult result;

    // Query how many instance extensions there are:
    uint32_t instance_extension_count;
    result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(result);
    VKL_RETURN_ON_ERROR(result);

    // Get all the instance extension names/properties there are:
    std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data());
    VKL_CHECK_VULKAN_ERROR(result);
    VKL_RETURN_ON_ERROR(result);

    for (const VkExtensionProperties& available_extension : available_instance_extensions) {
        if (strcmp(available_extension.extensionName, extension_name) == 0) {
            // Found the extension => Add it to the vector:
            ref_vector.push_back(extension_name);
            return;
        }
    }
}

void addValidationLayerNameToVectorIfSupported(const char* validation_layer_name, std::vector<const char*>& ref_vector) {
    VkResult result;

    // Query how many validation layers there are:
    uint32_t layer_count;
    result = vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(result);
    VKL_RETURN_ON_ERROR(result);

    // Get all the validation layer names/properties there are:
    std::vector<VkLayerProperties> available_layers(layer_count);
    result = vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    VKL_CHECK_VULKAN_ERROR(result);
    VKL_RETURN_ON_ERROR(result);

    for (const VkLayerProperties& available_layer : available_layers) {
        if (strcmp(available_layer.layerName, validation_layer_name) == 0) {
            // Found the validation layer => Add it to the vector:
            ref_vector.push_back(validation_layer_name);
            return;
        }
    }
}

void addDeviceExtensionToVectorIfSupported(const char* extension_name, VkPhysicalDevice physical_device, std::vector<const char*>& ref_vector) {
    VkResult result;

    // Query how many device extensions there are:
    uint32_t extensions_count;
    result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(result);
    VKL_RETURN_ON_ERROR(result);

    // Get all the device extensions:
    std::vector<VkExtensionProperties> available_extensions(extensions_count);
    result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, available_extensions.data());
    VKL_CHECK_VULKAN_ERROR(result);
    VKL_RETURN_ON_ERROR(result);

    for (const VkExtensionProperties& available_extension : available_extensions) {
        if (strcmp(available_extension.extensionName, extension_name) == 0) {
            // Found the extension => Add it to the vector:
            ref_vector.push_back(extension_name);
            return;
        }
    }
}

uint32_t selectPhysicalDeviceIndex(const VkPhysicalDevice* physical_devices, uint32_t physical_device_count, VkSurfaceKHR surface) {
    // Iterate over all the physical devices and select one that satisfies all our requirements.
    // Our requirements are:
    //  - Must support a queue that must have both, graphics and presentation capabilities
    for (uint32_t physical_device_index = 0u; physical_device_index < physical_device_count; ++physical_device_index) {
        // Check if fillModeNonSolid is supported
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[physical_device_index], &features);
        if (VK_TRUE != features.fillModeNonSolid) {
            continue; // This physical device does not support it => look for a different one
        }

        // Get the number of different queue families:
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[physical_device_index], &queue_family_count, nullptr);

        // Get the queue families' data:
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[physical_device_index], &queue_family_count, queue_families.data());

        for (uint32_t queue_family_index = 0u; queue_family_index < queue_family_count; ++queue_family_index) {
            // If this physical device supports a queue family which supports both, graphics and presentation
            //  => select this physical device
            if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                // This queue supports graphics! Let's see if it also supports presentation:
                VkBool32 presentation_supported;
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[physical_device_index], queue_family_index, surface, &presentation_supported);

                if (VK_TRUE == presentation_supported) {
                    // We've found a suitable physical device
                    return physical_device_index;
                }
            }
        }
    }
    VKL_EXIT_WITH_ERROR("Unable to find a suitable physical device that supports graphics and presentation on the same queue.");
}

uint32_t selectPhysicalDeviceIndex(const std::vector<VkPhysicalDevice>& physical_devices, VkSurfaceKHR surface) {
    return selectPhysicalDeviceIndex(physical_devices.data(), static_cast<uint32_t>(physical_devices.size()), surface);
}

uint32_t selectQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    // Get the number of different queue families for the given physical device:
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    // Get the queue families' data:
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    for (uint32_t queue_family_index = 0u; queue_family_index < queue_family_count; ++queue_family_index) {
        if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            // This queue supports graphics! Let's see if it also supports presentation:
            VkBool32 presentation_supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, surface, &presentation_supported);

            if (VK_TRUE == presentation_supported) {
                // We've found a suitable queue family on the given physical device and for the given surface
                //  => return its INDEX:
                return queue_family_index;
            }
        }
    }
    VKL_EXIT_WITH_ERROR("Unable to find a suitable queue family that supports graphics and presentation on the same queue.");
}

VkSurfaceFormatKHR getSurfaceImageFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkResult result;

    uint32_t surface_format_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(result);

    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data());
    VKL_CHECK_VULKAN_ERROR(result);

    if (surface_formats.empty()) {
        VKL_EXIT_WITH_ERROR("Unable to find supported surface formats.");
    }

    // Prefer a RGB8/sRGB format; If we are unable to find such, just return any:
    for (const VkSurfaceFormatKHR& f : surface_formats) {
        if ((f.format == VK_FORMAT_B8G8R8A8_SRGB || f.format == VK_FORMAT_R8G8B8A8_SRGB) && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }

    return surface_formats[0];
}

VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    VKL_CHECK_VULKAN_ERROR(result);
    return surface_capabilities;
}

VkSurfaceTransformFlagBitsKHR getSurfaceTransform(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    return getPhysicalDeviceSurfaceCapabilities(physical_device, surface).currentTransform;
}


VkDescriptorSet allocDescriptorSet(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout) {
    VkResult result;

    // Prepare allocation info and allocate:
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1u;
    descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;

    VkDescriptorSet descriptor_set;
    result = vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set);
    VKL_CHECK_VULKAN_RESULT(result);

    if (result < VK_SUCCESS) {
        VKL_EXIT_WITH_ERROR("Allocating a new descriptor set from the given pool and of the given layout failed.");
    }

    return descriptor_set;
}

void writeDescriptorSet(VkDevice device, VkDescriptorSet descriptor_set, VkBuffer uniform_buffer) {
    // Prepare write info and write:
    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = uniform_buffer;
    descriptor_buffer_info.offset = static_cast<VkDeviceSize>(0);
    descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = 0u;
    write_descriptor_set.dstArrayElement = 0u;
    write_descriptor_set.descriptorCount = 1u;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;

    vkUpdateDescriptorSets(device, 1u, &write_descriptor_set, 0u, nullptr);
}

void writeDescriptorSet(VkDevice device, VkDescriptorSet descriptor_set, VkBuffer object_data, VkBuffer directional_light_data, VkBuffer point_light_data) {
    // Write object data descriptor first:
    writeDescriptorSet(device, descriptor_set, object_data);

    // Then write the rest:

    // Prepare info about directional light data:
    VkDescriptorBufferInfo dirlight_buffer_info = {};
    dirlight_buffer_info.buffer = directional_light_data;
    dirlight_buffer_info.offset = static_cast<VkDeviceSize>(0);
    dirlight_buffer_info.range = VK_WHOLE_SIZE;

    // Prepare info about point light data:
    VkDescriptorBufferInfo pointlight_buffer_info = {};
    pointlight_buffer_info.buffer = point_light_data;
    pointlight_buffer_info.offset = static_cast<VkDeviceSize>(0);
    pointlight_buffer_info.range = VK_WHOLE_SIZE;

    std::vector<VkWriteDescriptorSet> writes = {
        VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptor_set,
            /* dstBinding: */ 1u, 0u, 1u,
            /* descriptorType: */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr,
            /* pBufferInfo: */ &dirlight_buffer_info, nullptr},
        VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptor_set,
            /* dstBinding: */ 2u, 0u, 1u,
            /* descriptorType: */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr,
            /* pBufferInfo: */ &pointlight_buffer_info, nullptr},
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0u, nullptr);
}
void writeDescriptorSet(VkDevice device, VkDescriptorSet descriptor_set, VkBuffer object_data, VkBuffer directional_light_data, VkBuffer point_light_data,
    VkImageView image_view, VkSampler sampler) {
    // Write object data, and light source dat first:
    writeDescriptorSet(device, descriptor_set, object_data, directional_light_data, point_light_data);

    // Then write the rest:

    // Prepare info about the texture:
    VkDescriptorImageInfo texture_info = {};
    texture_info.imageView = image_view;
    texture_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texture_info.sampler = sampler;

    /* --------------------------------------------- */
    // Subtask 5.8: Use the Textures in Shaders
    /* --------------------------------------------- */
    std::vector<VkWriteDescriptorSet> writes = {VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptor_set,
        /* dstBinding: */ 3u, 0u, 1u,
        /* descriptorType: */ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        /* pImageInfo: */ &texture_info, nullptr, nullptr}};

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0u, nullptr);
}

void drawGeometryWithMaterial(VkPipeline pipeline, const Geometry& geometry, VkDescriptorSet material, uint32_t num_instances) {
    /* --------------------------------------------- */
    // Subtask 3.4: Command Buffer Recording
    /* --------------------------------------------- */

    // Get the current command buffer:
    VkCommandBuffer cb = vklGetCurrentCommandBuffer();

    // Record binding the descriptor set for subsequent draw calls into the command buffer:
    VkPipelineLayout pipeline_layout = vklGetLayoutForPipeline(pipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0u, 1u, &material, 0u, nullptr);

    // Record the draw call into the command buffer, which uses vertex and index buffers of the geometry:
    vklCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    if (VK_NULL_HANDLE == geometry.colorsBuffer) {
        VkBuffer vertex_buffers[3] = {geometry.positionsBuffer, geometry.normalsBuffer, geometry.textureCoordinatesBuffer};
        VkDeviceSize offsets[3] = {0, 0, 0};
        vkCmdBindVertexBuffers(cb, 0u, 3u, vertex_buffers, offsets);
    } else {
        VkBuffer vertex_buffers[3] = {geometry.positionsBuffer, geometry.normalsBuffer, geometry.colorsBuffer};
        VkDeviceSize offsets[3] = {0, 0, 0};
        vkCmdBindVertexBuffers(cb, 0u, 3u, vertex_buffers, offsets);
    }

    vkCmdBindIndexBuffer(cb, geometry.indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cb, geometry.numberOfIndices, num_instances, 0u, 0u, 0u);
}



VkImageView createImageViewForImage(VkDevice device, VkImage image, VkImageViewType view_type, VkFormat format) {
    /* --------------------------------------------- */
    // Subtask 5.6: Create an Image View for each Image
    /* --------------------------------------------- */
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = view_type;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0u;
    image_view_create_info.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    image_view_create_info.subresourceRange.baseArrayLayer = 0u;
    image_view_create_info.subresourceRange.layerCount = VK_IMAGE_VIEW_TYPE_CUBE == view_type ? 6u : 1u;

    VkImageView vk_image_view;
    VkResult result = vkCreateImageView(device, &image_view_create_info, nullptr, &vk_image_view);
    VKL_CHECK_VULKAN_RESULT(result);

    return vk_image_view;
}

ImageAndView loadImage(VkDevice device, VkQueue queue, VkCommandPool command_pool, std::string image_file_name) {
    std::vector<std::string> image_file_paths = gcgFindTextureFiles({image_file_name});

    /* --------------------------------------------- */
    // Subtask 5.5: Load DDS Textures into Images
    /* --------------------------------------------- */
    VklImageInfo image_info = vklGetDdsImageInfo(image_file_paths[0].c_str());
    const uint32_t image_layers = static_cast<uint32_t>(image_file_paths.size());
    assert(image_layers == 1u || image_layers == 6u);
    VkImageCreateFlags image_flags = image_layers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : VkImageCreateFlags{};
    VkResult result;

    // 0. Create the image:
    VkImage image = vklCreateDeviceLocalImageWithBackingMemory(image_info.extent.width, image_info.extent.height, image_info.imageFormat,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, image_layers, image_flags);

    // 1. Create a command buffer and start recording
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1u;

    VkCommandBuffer command_buffer;
    result = vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer);
    VKL_CHECK_VULKAN_RESULT(result);

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    // Data structures for Synchronization2:
    VkImageMemoryBarrier2KHR image_memory_barrier2 = {}; // <-- Used only in the g_synchronization2_supported case
    image_memory_barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
    image_memory_barrier2.image = image;
    image_memory_barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    VkDependencyInfoKHR dependency_info = {};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
    dependency_info.imageMemoryBarrierCount = 1u;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier2;

    // Data structures for classical synchronization:
    VkImageMemoryBarrier image_memory_barrier = {}; // <-- Used only in the !g_synchronization2_supported case
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.image = image;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    /* --------------------------------------------- */
    // Subtask 5.10: Enable Mipmapping
    /* --------------------------------------------- */
    std::vector<VkBuffer> staging_buffers;
    uint32_t num_levels = static_cast<uint32_t>(std::log2(std::max(image_info.extent.width, image_info.extent.height))) + 1u;

    for (uint32_t layer = 0u; layer < image_layers; ++layer) {
        image_info = vklGetDdsImageInfo(image_file_paths[layer].c_str());
        uint32_t mipWidth = image_info.extent.width;
        uint32_t mipHeight = image_info.extent.height;
        for (uint32_t level = 0u; level < num_levels; ++level) {
            image_info = vklGetDdsImageLevelInfo(image_file_paths[layer].c_str(), level);
            // Sanity check to see we are loading an image with the correct size
            if (mipWidth != image_info.extent.width || mipHeight != image_info.extent.height)
                VKL_EXIT_WITH_ERROR("vklGetDdsImageLevelInfo for level " + std::to_string(level) +
                    " returned an image with width=" + std::to_string(image_info.extent.width) + ", height=" + std::to_string(image_info.extent.height) +
                    " instead of the expected width=" + std::to_string(mipWidth) + ", height=" + std::to_string(mipHeight) + ".");
            mipWidth = mipWidth > 1u ? mipWidth / 2u : 1u;
            mipHeight = mipHeight > 1u ? mipHeight / 2u : 1u;
            // Store in vector to keep alive until they are no longer needed, which is after the fence has been signaled:
            staging_buffers.push_back(vklLoadDdsImageLevelIntoHostCoherentBuffer(image_file_paths[layer].c_str(), level));

            if (g_synchronization2_supported) {
                // 2. Record an image layout transition into a format that is optimal for the image to get data copied into it:
                image_memory_barrier2.srcStageMask = VK_PIPELINE_STAGE_2_NONE_KHR;       // No need to wait on anything
                image_memory_barrier2.srcAccessMask = VK_ACCESS_2_NONE_KHR;              // Nothing required thanks to implicit memory guaranteed with host writes
                image_memory_barrier2.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;   // The subsequent command (which must wait) is a COPY command.
                image_memory_barrier2.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT_KHR; // Copy reads from the buffer. The layout transition must be available
                                                                                         // to that type of access.
                image_memory_barrier2.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                image_memory_barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_memory_barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_memory_barrier2.subresourceRange.baseMipLevel = level;
                image_memory_barrier2.subresourceRange.levelCount = 1u;
                image_memory_barrier2.subresourceRange.baseArrayLayer = layer;
                image_memory_barrier2.subresourceRange.layerCount = 1u;
                g_vkCmdPipelineBarrier2KHR(command_buffer, &dependency_info);
            } else { // => Synchronization2 is NOT supported => use oldschool style:
                // 2. Record an image layout transition into a format that is optimal for the image to get data copied into it:
                image_memory_barrier.srcAccessMask = {};
                image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_memory_barrier.subresourceRange.baseMipLevel = level;
                image_memory_barrier.subresourceRange.levelCount = 1u;
                image_memory_barrier.subresourceRange.baseArrayLayer = layer;
                image_memory_barrier.subresourceRange.layerCount = 1u;
                vkCmdPipelineBarrier(command_buffer, 
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, /* --> */ VK_PIPELINE_STAGE_TRANSFER_BIT, {},
                    /* Memory barriers:        */ 0u, nullptr,
                    /* Buffer memory barriers: */ 0u, nullptr,
                    /* Image memory barriers:  */ 1u, &image_memory_barrier);
            }

            // 3. Record copying data from the (host-coherent) buffer into the (device-local) image:
            VkBufferImageCopy buffer_image_copy = {};
            buffer_image_copy.bufferOffset = 0;
            buffer_image_copy.bufferRowLength = 0u;
            buffer_image_copy.bufferImageHeight = 0u;
            buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            buffer_image_copy.imageSubresource.mipLevel = level;
            buffer_image_copy.imageSubresource.baseArrayLayer = layer;
            buffer_image_copy.imageSubresource.layerCount = 1u;
            buffer_image_copy.imageOffset.x = 0u;
            buffer_image_copy.imageOffset.y = 0u;
            buffer_image_copy.imageOffset.z = 0u;
            buffer_image_copy.imageExtent.width = image_info.extent.width;
            buffer_image_copy.imageExtent.height = image_info.extent.height;
            buffer_image_copy.imageExtent.depth = 1u;
            vkCmdCopyBufferToImage(command_buffer, staging_buffers.back(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &buffer_image_copy);

            if (g_synchronization2_supported) {
                // 4. Record an image layout transition into a format that is optimal for rendering:
                // Re-use the VkImageMemoryBarrier from above, but modify a few parameters:
                image_memory_barrier2.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;    // It is the preceding COPY which must have completed before moving on.
                image_memory_barrier2.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR; // The writes of that COPY must have completed.
                image_memory_barrier2.dstStageMask =
                    VK_PIPELINE_STAGE_2_NONE_KHR; // No dst stage required in the barrier, because afterwards we sync with the fence.
                image_memory_barrier2.dstAccessMask = VK_ACCESS_2_NONE_KHR;
                image_memory_barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_memory_barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Transform into right format for being sampled from in shaders.
                g_vkCmdPipelineBarrier2KHR(command_buffer, &dependency_info);
            } else { // => Synchronization2 is NOT supported => use oldschool style:
                // 4. Record an image layout transition into a format that is optimal for rendering:
                // Re-use the VkImageMemoryBarrier from above, but modify a few parameters:
                image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                image_memory_barrier.dstAccessMask = {};
                image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                vkCmdPipelineBarrier(command_buffer, 
                    VK_PIPELINE_STAGE_TRANSFER_BIT, /* --> */ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {},
                    /* Memory barriers:        */ 0u, nullptr,
                    /* Buffer memory barriers: */ 0u, nullptr,
                    /* Image memory barriers:  */ 1u, &image_memory_barrier);
            }
        }
    }

    // 5. End recording and submit to queue; use a fence to wait for the operation to complete:
    vkEndCommandBuffer(command_buffer);

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence;
    result = vkCreateFence(device, &fence_create_info, nullptr, &fence);
    VKL_CHECK_VULKAN_RESULT(result);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0u;
    submit_info.commandBufferCount = 1u;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 0u;
    result = vkQueueSubmit(queue, 1u, &submit_info, fence);
    VKL_CHECK_VULKAN_RESULT(result);

    // 6. Wait for the operation to complete, then clean up:
    result = vkWaitForFences(device, 1u, &fence, VK_TRUE, UINT64_MAX);
    VKL_CHECK_VULKAN_RESULT(result);

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, command_pool, 1u, &command_buffer);
    for (auto staging_buffer : staging_buffers) {
        vklDestroyHostCoherentBufferAndItsBackingMemory(staging_buffer);
    }

    return ImageAndView{
        image, createImageViewForImage(device, image, image_layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D, image_info.imageFormat)
    };
}
