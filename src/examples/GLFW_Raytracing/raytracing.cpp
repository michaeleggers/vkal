// TODOs:
// - load_image appears in multiple project -> Utility file
// - Fix VKAL so that a buffer can stay mapped and updated at different offsets in a loop.
// - Currently the AssImp model loading function assigns one index to one individual vertex which kind of defeats their purpose.
// - Memory: Storage Buffers are fixed at 10MB. Maybe make this more dynamic.
// - On RTX 2070 MaxQ (Laptop version of 2070) check the memory alignment for (storage) buffers.



#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <string>

#include <vkal.h>

#define GLM_FORCE_RADIANS
// necessary so that glm::perspective produces z-clip coordinates 0<z_clip<w_clip
// see: https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
// see: https://twitter.com/pythno/status/1230478042096709632
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include <camera.h>
#include <model_v2.h>
#include <common.h>
#include <platform.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"


#define SCREEN_WIDTH        1280
#define SCREEN_HEIGHT       768
#define CAM_SPEED			0.1f
#define CAM_SPEED_SLOW      (CAM_SPEED*0.1f)
#define MOUSE_SPEED			0.007f


typedef struct ViewProjection
{
    glm::mat4 view;
    glm::mat4 proj;
} ViewProjection;

struct ModelBuffers
{
    VkalBuffer vertexBuffer;
    VkalBuffer indexBuffer;
};

struct PipelineInfo {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups;
    VkDescriptorSetLayout descriptor_set_layout;
};

struct ASInfo{
    VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info;
    VkAccelerationStructureGeometryKHR       as_geometry;
};

struct MouseState
{
    double xpos, ypos;
    double xpos_old, ypos_old;
    uint32_t left_button_down;
    uint32_t right_button_down;
};

/* GLOBALS */
static GLFWwindow* window;
static int keys[GLFW_KEY_LAST];
static MouseState mouse_state;
/* current framebuffer width/height */
static int width = SCREEN_WIDTH;
static int height = SCREEN_HEIGHT;

static UniformBuffer    g_view_proj_ub;
static VkalImage        g_storage_image;
static VkDescriptorSet  g_descriptor_set;
static VkalBuffer       g_raygen_shader_binding_table;
static VkalBuffer       g_miss_shader_binding_table;
static VkalBuffer       g_hit_shader_binding_table;

// TLAS
static DeviceMemory                 g_instance_device_memory;
static VkalBuffer                   g_instance_buffer;
static DeviceMemory                 g_tlas_device_memory;
static DeviceMemory                 g_tlas_scratch_memory;
static VkalBuffer                   g_tlas_scratch_buffer;
static VkCommandBuffer              g_tlas_one_time_command_buffer;
static VkalAccelerationStructure    g_top_level_acceleration_structure;

Image load_image(const char* file)
{
    Image image = {};

    char exe_path[256];
    get_exe_path(exe_path, 256 * sizeof(char));
    std::string finalPath = concat_paths(std::string(exe_path), std::string(file));

 
    int width, height, channels;
    unsigned char* data = stbi_load(finalPath.c_str(), &width, &height, &channels, 4);
    if (stbi_failure_reason() != NULL) {
        printf("[STB-Image] %s file: %s\n", stbi_failure_reason(), file);
    }
    image.width = uint32_t(width);
    image.height = uint32_t(height);
    image.channels = 4; // force this, even if less than 4. 

    size_t imageSize = size_t(image.width * image.height * image.channels);
    image.data = (unsigned char*)malloc(imageSize);
    memcpy(image.data, data, imageSize);
    stbi_image_free(data);

    return image;
}

void create_storage_image(VkalInfo* vkal_info)
{
    vkDeviceWaitIdle(vkal_info->device);

    // If the view port size has changed, we need to recreate the storage image
    vkal_destroy_image_view(g_storage_image.image_view);
    vkal_destroy_image(g_storage_image.image);
    vkal_destroy_device_memory(g_storage_image.device_memory);

    g_storage_image = create_vkal_image(width, height,
        VK_FORMAT_B8G8R8A8_UNORM, // TODO: Why is it in raytracing shaders BGR not RGB?
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_GENERAL);

    // The descriptor also needs to be updated to reference the new image
    VkDescriptorImageInfo image_descriptor{};
    image_descriptor.imageView = get_image_view(g_storage_image.image_view);
    image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    VkWriteDescriptorSet result_image_write = create_write_descriptor_set_image(g_descriptor_set, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &image_descriptor);
    vkUpdateDescriptorSets(vkal_info->device, 1, &result_image_write, 0, VK_NULL_HANDLE);

    vkDeviceWaitIdle(vkal_info->device);
}

// GLFW callbacks
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        printf("escape key pressed\n");
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_W) {
            keys[GLFW_KEY_W] = 1;
        }
        if (key == GLFW_KEY_S) {
            keys[GLFW_KEY_S] = 1;
        }
        if (key == GLFW_KEY_A) {
            keys[GLFW_KEY_A] = 1;
        }
        if (key == GLFW_KEY_D) {
            keys[GLFW_KEY_D] = 1;
        }
        if (key == GLFW_KEY_LEFT_ALT) {
            keys[GLFW_KEY_LEFT_ALT] = 1;
        }
        if (key == GLFW_KEY_LEFT_SHIFT) {
            keys[GLFW_KEY_LEFT_SHIFT] = 1;
        }
    }
    else if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_W) {
            keys[GLFW_KEY_W] = 0;
        }
        if (key == GLFW_KEY_S) {
            keys[GLFW_KEY_S] = 0;
        }
        if (key == GLFW_KEY_A) {
            keys[GLFW_KEY_A] = 0;
        }
        if (key == GLFW_KEY_D) {
            keys[GLFW_KEY_D] = 0;
        }
        if (key == GLFW_KEY_LEFT_ALT) {
            keys[GLFW_KEY_LEFT_ALT] = 0;
        }
        if (key == GLFW_KEY_LEFT_SHIFT) {
            keys[GLFW_KEY_LEFT_SHIFT] = 0;
        }
    }
}

static void glfw_mouse_button_callback(GLFWwindow* window, int mouse_button, int action, int mods)
{
    if (mouse_button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mouse_state.left_button_down = 1;
    }
    if (mouse_button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mouse_state.left_button_down = 0;
    }
    if (mouse_button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        mouse_state.right_button_down = 1;
    }
    if (mouse_button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        mouse_state.right_button_down = 0;
    }
}

static void update_camera(Camera& camera, float dt)
{
    float camera_dolly_speed = CAM_SPEED;
    if (keys[GLFW_KEY_LEFT_SHIFT]) {
        camera_dolly_speed = CAM_SPEED_SLOW;
    }

    if (keys[GLFW_KEY_W]) {
        camera.MoveForward(camera_dolly_speed * dt);
    }
    if (keys[GLFW_KEY_S]) {
        camera.MoveForward(-camera_dolly_speed * dt);
    }
    if (keys[GLFW_KEY_A]) {
        camera.MoveSide(camera_dolly_speed * dt);
    }
    if (keys[GLFW_KEY_D]) {
        camera.MoveSide(-camera_dolly_speed * dt);
    }
}

void update_camera_mouse_look(Camera& camera, float dt)
{
    if (mouse_state.left_button_down || mouse_state.right_button_down) {
        mouse_state.xpos_old = mouse_state.xpos;
        mouse_state.ypos_old = mouse_state.ypos;
        glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
        double dx = mouse_state.xpos - mouse_state.xpos_old;
        double dy = mouse_state.ypos - mouse_state.ypos_old;
        if (mouse_state.left_button_down) {
            camera.RotateAroundSide(dy * MOUSE_SPEED);
            camera.RotateAroundUp(-dx * MOUSE_SPEED);
        }
    }
    else {
        glfwGetCursorPos(window, &mouse_state.xpos, &mouse_state.ypos);
        mouse_state.xpos_old = mouse_state.xpos;
        mouse_state.ypos_old = mouse_state.ypos;
    }
}

static void glfw_window_size_callback(GLFWwindow* window, int width, int height)
{
    
}

void init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raytracing", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetWindowSizeCallback(window, glfw_window_size_callback);    
    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;
}

std::vector<VkalBuffer> create_instance_metadata_buffers(uint32_t instance_count) {
    std::vector<VkalBuffer> buffers{};
    DeviceMemory per_instance_memory = vkal_allocate_devicememory(1024 * 1024 * 10,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    for (uint32_t i = 0; i < instance_count; i++) {
        VkalBuffer instance_metadata_buffer = vkal_create_buffer(instance_count * sizeof(Vertex), &per_instance_memory,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
        buffers.push_back(instance_metadata_buffer);
    }
    
    return buffers;
}

PipelineInfo create_ray_tracing_pipeline(VkalInfo * vkal_info, uint32_t model_count, uint32_t material_count, uint32_t texture_count)
{
    // Slot for binding top level acceleration structures to the ray generation shader
    VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
    acceleration_structure_layout_binding.binding = 0;
    acceleration_structure_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    acceleration_structure_layout_binding.descriptorCount = 1;
    acceleration_structure_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding result_image_layout_binding{};
    result_image_layout_binding.binding = 1;
    result_image_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    result_image_layout_binding.descriptorCount = 1;
    result_image_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding uniform_buffer_binding{};
    uniform_buffer_binding.binding = 2;
    uniform_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_binding.descriptorCount = 1;
    uniform_buffer_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding vertex_storage_buffer_binding{};
    vertex_storage_buffer_binding.binding = 3;
    vertex_storage_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertex_storage_buffer_binding.descriptorCount = model_count;
    vertex_storage_buffer_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding index_storage_buffer_binding{};
    index_storage_buffer_binding.binding = 4;
    index_storage_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    index_storage_buffer_binding.descriptorCount = model_count;
    index_storage_buffer_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding material_storage_buffer_binding{};
    material_storage_buffer_binding.binding = 5;
    material_storage_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    material_storage_buffer_binding.descriptorCount = material_count; 
    material_storage_buffer_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding instance_metadata_storage_buffer_binding{};
    instance_metadata_storage_buffer_binding.binding = 6;
    instance_metadata_storage_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    instance_metadata_storage_buffer_binding.descriptorCount = model_count;
    instance_metadata_storage_buffer_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding texture_array_binding{};
    texture_array_binding.binding = 7;
    texture_array_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_array_binding.descriptorCount = texture_count;
    texture_array_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        acceleration_structure_layout_binding,
        result_image_layout_binding,
        uniform_buffer_binding,
        vertex_storage_buffer_binding,
        index_storage_buffer_binding,
        material_storage_buffer_binding,
        instance_metadata_storage_buffer_binding,
        texture_array_binding
    };

    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(&bindings[0], bindings.size());
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(&descriptor_set_layout, 1, nullptr, 0);

    /*
        Setup ray tracing shader groups
        Each shader group points at the corresponding shader in the pipeline
    */
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};
    std::vector<SingleShaderStageSetup> shader_stages;

    uint8_t* shader_code = 0;
    int shader_size;

    /* Ray Generation Group */
    read_file("../../src/examples/assets/shaders/raygen.spv", &shader_code, &shader_size);
    shader_stages.push_back(vkal_create_shader(shader_code, shader_size, VK_SHADER_STAGE_RAYGEN_BIT_KHR));
    VkRayTracingShaderGroupCreateInfoKHR raygen_group_ci{};
    raygen_group_ci.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    raygen_group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raygen_group_ci.generalShader = static_cast<uint32_t>(shader_stages.size()) - 1;
    raygen_group_ci.closestHitShader = VK_SHADER_UNUSED_KHR;
    raygen_group_ci.anyHitShader = VK_SHADER_UNUSED_KHR;
    raygen_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
    shader_groups.push_back(raygen_group_ci);

    /* Ray Miss Group */
    read_file("../../src/examples/assets/shaders/raymiss.spv", &shader_code, &shader_size);
    shader_stages.push_back(vkal_create_shader(shader_code, shader_size, VK_SHADER_STAGE_MISS_BIT_KHR));
    VkRayTracingShaderGroupCreateInfoKHR miss_group_ci{};
    miss_group_ci.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    miss_group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    miss_group_ci.generalShader = static_cast<uint32_t>(shader_stages.size()) - 1;
    miss_group_ci.closestHitShader = VK_SHADER_UNUSED_KHR;
    miss_group_ci.anyHitShader = VK_SHADER_UNUSED_KHR;
    miss_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
    shader_groups.push_back(miss_group_ci);

    /* Shadow Ray misses the light */
    read_file("../../src/examples/assets/shaders/lightmiss.spv", &shader_code, &shader_size);
    shader_stages.push_back(vkal_create_shader(shader_code, shader_size, VK_SHADER_STAGE_MISS_BIT_KHR));
    VkRayTracingShaderGroupCreateInfoKHR shadowmiss_group_ci{};
    shadowmiss_group_ci.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shadowmiss_group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shadowmiss_group_ci.generalShader = static_cast<uint32_t>(shader_stages.size()) - 1;
    shadowmiss_group_ci.closestHitShader = VK_SHADER_UNUSED_KHR;
    shadowmiss_group_ci.anyHitShader = VK_SHADER_UNUSED_KHR;
    shadowmiss_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
    shader_groups.push_back(shadowmiss_group_ci);

    /* Closest Hit Group */
    /* Primary Ray hits something */
    read_file("../../src/examples/assets/shaders/closesthit.spv", &shader_code, &shader_size);
    shader_stages.push_back(vkal_create_shader(shader_code, shader_size, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
    VkRayTracingShaderGroupCreateInfoKHR closes_hit_group_ci{};
    closes_hit_group_ci.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    closes_hit_group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    closes_hit_group_ci.generalShader = VK_SHADER_UNUSED_KHR;
    closes_hit_group_ci.closestHitShader = static_cast<uint32_t>(shader_stages.size()) - 1;
    closes_hit_group_ci.anyHitShader = VK_SHADER_UNUSED_KHR;
    closes_hit_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
    shader_groups.push_back(closes_hit_group_ci);

    /* Finally, create Ray Tracing Pipeline */
    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos;
    for (auto& shader_stage : shader_stages) {
        shader_stage_create_infos.push_back(shader_stage.create_info);
    }
    VkRayTracingPipelineCreateInfoKHR raytracing_pipeline_create_info{};
    raytracing_pipeline_create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    raytracing_pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
    raytracing_pipeline_create_info.pStages = shader_stage_create_infos.data();
    raytracing_pipeline_create_info.groupCount = static_cast<uint32_t>(shader_groups.size());
    raytracing_pipeline_create_info.pGroups = shader_groups.data();
    raytracing_pipeline_create_info.maxPipelineRayRecursionDepth = 1;
    raytracing_pipeline_create_info.layout = pipeline_layout;
    VkPipeline pipeline{};
    vkCreateRayTracingPipelinesKHR(vkal_info->device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracing_pipeline_create_info, nullptr, &pipeline);

    return { pipeline, pipeline_layout, shader_groups, descriptor_set_layout } ;
}

VkalAccelerationStructure create_blas(VkalInfo * vkal_info, VkalBuffer model_buffer, uint32_t vertex_count)
{
    /* BLAS */
    VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
    acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    acceleration_structure_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    acceleration_structure_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    VkDeviceOrHostAddressConstKHR vertex_data_device_address{};
    vertex_data_device_address.deviceAddress = vkal_get_buffer_device_address(model_buffer.buffer);
    acceleration_structure_geometry.geometry.triangles.vertexData = vertex_data_device_address;
    acceleration_structure_geometry.geometry.triangles.maxVertex = vertex_count;
    acceleration_structure_geometry.geometry.triangles.vertexStride = sizeof(Vertex);
    acceleration_structure_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

    /* Get size requrements */
    VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
    acceleration_structure_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    acceleration_structure_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    acceleration_structure_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    acceleration_structure_build_geometry_info.geometryCount = 1;
    acceleration_structure_build_geometry_info.pGeometries = &acceleration_structure_geometry;

    const uint32_t primitive_count = vertex_count / 3;

    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
    acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        vkal_info->device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &acceleration_structure_build_geometry_info,
        &primitive_count,
        &acceleration_structure_build_sizes_info
    );

    // Create a buffer to hold the acceleration structure
    VkalAccelerationStructure bottom_level_acceleration_structure{};
    DeviceMemory blas_device_memory = vkal_allocate_devicememory(
        acceleration_structure_build_sizes_info.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    );
    bottom_level_acceleration_structure.buffer = vkal_create_buffer(
        acceleration_structure_build_sizes_info.accelerationStructureSize,
        &blas_device_memory,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    );

    // Create the acceleration structure
    VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
    acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    acceleration_structure_create_info.buffer = bottom_level_acceleration_structure.buffer.buffer;
    acceleration_structure_create_info.size = acceleration_structure_build_sizes_info.accelerationStructureSize;
    acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(vkal_info->device,
        &acceleration_structure_create_info,
        nullptr,
        &bottom_level_acceleration_structure.handle
    );

    // The actual build process starts here

    // Create a scratch buffer as a temporary storage for the acceleration structure build
    DeviceMemory scratch_memory = vkal_allocate_devicememory(
        acceleration_structure_build_sizes_info.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    );
    VkalBuffer scratch_buffer = vkal_create_buffer(
        acceleration_structure_build_sizes_info.buildScratchSize,
        &scratch_memory,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    );

    VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
    acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    acceleration_build_geometry_info.dstAccelerationStructure = bottom_level_acceleration_structure.handle;
    acceleration_build_geometry_info.geometryCount = 1;
    acceleration_build_geometry_info.pGeometries = &acceleration_structure_geometry;
    acceleration_build_geometry_info.scratchData.deviceAddress = vkal_get_buffer_device_address(scratch_buffer.buffer);

    VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info{};
    acceleration_structure_build_range_info.primitiveCount = primitive_count;
    acceleration_structure_build_range_info.primitiveOffset = 0;
    acceleration_structure_build_range_info.firstVertex = 0;
    acceleration_structure_build_range_info.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> acceleration_build_structure_range_infos = { &acceleration_structure_build_range_info };

    // Build the acceleration structure on the device via a one-time command buffer submission
    // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
    VkCommandBuffer one_time_command_buffer = vkal_create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    vkCmdBuildAccelerationStructuresKHR(
        one_time_command_buffer,
        1,
        &acceleration_build_geometry_info,
        acceleration_build_structure_range_infos.data()
    );
    vkal_flush_command_buffer(one_time_command_buffer, vkal_info->graphics_queue, 1);

    // TODO: Delete scratch buffer!
   

    // Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
    VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
    acceleration_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    acceleration_device_address_info.accelerationStructure = bottom_level_acceleration_structure.handle;
    bottom_level_acceleration_structure.device_address = vkGetAccelerationStructureDeviceAddressKHR(vkal_info->device, &acceleration_device_address_info);

    return bottom_level_acceleration_structure;
}

void create_descriptor_sets(VkalInfo * vkal_info, VkDescriptorSetLayout descriptor_set_layout, VkalAccelerationStructure tlas, 
    std::vector<ModelBuffers> model_buffers, 
    std::vector<VkalBuffer> material_buffers, 
    std::vector<VkalBuffer> instance_metadata_buffers,
    std::vector<VkalTexture> textures)
{
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
    };
    
    VkDescriptorPoolCreateInfo descriptor_pool_info = { };
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_info.maxSets = 20; // NOTE: This is an arbitrary number at the moment. We don't _have_ to use all of them.
    descriptor_pool_info.poolSizeCount = pool_sizes.size();
    descriptor_pool_info.pPoolSizes = pool_sizes.data();

    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VKAL_ASSERT(vkCreateDescriptorPool(vkal_info->device, &descriptor_pool_info, nullptr, &descriptor_pool));

    VkDescriptorSetAllocateInfo allocate_info = { };
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.pSetLayouts = &descriptor_set_layout;
    allocate_info.descriptorSetCount = 1;
    g_descriptor_set = VK_NULL_HANDLE;
    VKAL_ASSERT(vkAllocateDescriptorSets(vkal_info->device, &allocate_info, &g_descriptor_set));

    // Setup the descriptor for binding our top level acceleration structure to the ray tracing shaders
    VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
    descriptor_acceleration_structure_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptor_acceleration_structure_info.accelerationStructureCount = 1;
    descriptor_acceleration_structure_info.pAccelerationStructures = &tlas.handle;

    VkWriteDescriptorSet acceleration_structure_write{};
    acceleration_structure_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    acceleration_structure_write.dstSet = g_descriptor_set;
    acceleration_structure_write.dstBinding = 0;
    acceleration_structure_write.descriptorCount = 1;
    acceleration_structure_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    // The acceleration structure descriptor has to be chained via pNext
    acceleration_structure_write.pNext = &descriptor_acceleration_structure_info;

    VkDescriptorImageInfo image_descriptor{};
    g_storage_image = create_vkal_image(width, height,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_GENERAL);

    //vkal_update_descriptor_set_render_image // Cannot use this because it takes combined image sampler as descriptor type by default!
    image_descriptor.imageView = get_image_view(g_storage_image.image_view);
    image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    VkWriteDescriptorSet result_image_write = create_write_descriptor_set_image(g_descriptor_set, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &image_descriptor);
    
    // TODO: Maybe we cannot use the default uniform buffer memory for Raytracing! (NOT SURE THOUGH...)
    //DeviceMemory uniform_device_memory = vkal_allocate_devicememory(1024 * 1024, 
    //    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    //VkalBuffer uniform_buffer = vkal_create_buffer(sizeof(ViewProjection), &uniform_device_memory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    g_view_proj_ub = vkal_create_uniform_buffer(sizeof(ViewProjection), 1, 2);
    vkal_update_descriptor_set_uniform(g_descriptor_set, g_view_proj_ub, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
        acceleration_structure_write,
        result_image_write
    };

    // Activate all models in the storage buffer array (vertices and indices)
    for (size_t i = 0; i < model_buffers.size(); i++) {
        vkal_update_descriptor_set_bufferarray(g_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, i, model_buffers[i].vertexBuffer);
        vkal_update_descriptor_set_bufferarray(g_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, i, model_buffers[i].indexBuffer);
    }

    // Activate all materials in the storage buffer array
    for (size_t i = 0; i < material_buffers.size(); i++) {
        vkal_update_descriptor_set_bufferarray(g_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, i, material_buffers[i]);
    }

    // Activate the instance metadata buffer so that we can get the material for an instance
    for (size_t i = 0; i < instance_metadata_buffers.size(); i++) {
        vkal_update_descriptor_set_bufferarray(g_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, i, instance_metadata_buffers[i]);
    }

    // Activate textures
    for (size_t i = 0; i < textures.size(); i++) {
        vkal_update_descriptor_set_texturearray(g_descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, i, textures[i]);
    }


    /* TODO: Create Texture Array Descriptor Set */
    /*Image texture = load_image("../../assets/textures/sand_diffuse.jpg");
    VkalTexture vkalTexture = vkal_create_texture(3, texture.data, texture.width, texture.height, 4, 0,
        VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);*/

    vkUpdateDescriptorSets(vkal_info->device, write_descriptor_sets.size(), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
}

ASInfo get_acceleration_structure_info(VkalInfo* vkal_info, std::vector<VkalAccelerationStructure> blases, std::vector<Model> models)
{
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    size_t i = 0;
    vkal_map_buffer(&g_instance_buffer);
    for (VkalAccelerationStructure& blas : blases) {
        glm::mat4 model_matrix = models[i].model_matrix;
        VkTransformMatrixKHR transform_matrix = { // NOTE: Row Major
            1.0f, 0.0f, 0.0f, model_matrix[3].x,
            0.0f, 1.0f, 0.0f, model_matrix[3].y,
            0.0f, 0.0f, 1.0f, model_matrix[3].z 
        };

        VkAccelerationStructureInstanceKHR acceleration_structure_instance{};
        acceleration_structure_instance.transform = transform_matrix;
        acceleration_structure_instance.instanceCustomIndex = i;
        acceleration_structure_instance.mask = 0xFF;
        acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
        acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        acceleration_structure_instance.accelerationStructureReference = blas.device_address;

        //vkal_update_buffer_offset(&instance_buffer,
        //    (uint8_t*)&acceleration_structure_instance,
        //    sizeof(VkAccelerationStructureInstanceKHR),
        //    i * sizeof(VkAccelerationStructureInstanceKHR));

        memcpy((uint8_t*)(g_instance_buffer.mapped) + i * sizeof(VkAccelerationStructureInstanceKHR), (uint8_t*)&acceleration_structure_instance, sizeof(VkAccelerationStructureInstanceKHR));


        instances.push_back(acceleration_structure_instance);
        i++;
    }
    vkal_unmap_buffer(&g_instance_buffer);

    VkDeviceOrHostAddressConstKHR instance_data_device_address{};
    instance_data_device_address.deviceAddress = vkal_get_buffer_device_address(g_instance_buffer.buffer);

    // The top level acceleration structure contains (bottom level) instance as the input geometry
    VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
    acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    acceleration_structure_geometry.geometry.instances.data = instance_data_device_address;

    std::vector<VkAccelerationStructureGeometryKHR> acceleration_structure_geometries = {
        acceleration_structure_geometry
    };

    // Get the size requirements for buffers involved in the acceleration structure build process
    VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
    acceleration_structure_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    acceleration_structure_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    acceleration_structure_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    acceleration_structure_build_geometry_info.geometryCount = 1;
    acceleration_structure_build_geometry_info.pGeometries = &acceleration_structure_geometry;

    uint32_t primitive_count = 0;
    for (auto& model : models) {
        primitive_count += model.vertex_count / 3;
    }

    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
    acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        vkal_info->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &acceleration_structure_build_geometry_info,
        &primitive_count,
        &acceleration_structure_build_sizes_info);

    return { acceleration_structure_build_sizes_info, acceleration_structure_geometry };
}

void create_tlas_handle(VkalInfo* vkal_info, ASInfo as_info)
{
    VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
    acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    acceleration_structure_create_info.buffer = g_top_level_acceleration_structure.buffer.buffer;
    acceleration_structure_create_info.size = as_info.as_build_sizes_info.accelerationStructureSize; // THIS IS THE PROBLEM!!!!!!!!
    acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(vkal_info->device, &acceleration_structure_create_info, nullptr, &g_top_level_acceleration_structure.handle);
}

void build_tlas(VkalInfo* vkal_info, ASInfo as_info, std::vector< VkalAccelerationStructure> blases, bool update)
{
    VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
    acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    acceleration_build_geometry_info.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    acceleration_build_geometry_info.srcAccelerationStructure = update ? g_top_level_acceleration_structure.handle : VK_NULL_HANDLE;
    acceleration_build_geometry_info.dstAccelerationStructure = g_top_level_acceleration_structure.handle;
    acceleration_build_geometry_info.geometryCount = 1;
    acceleration_build_geometry_info.pGeometries = &as_info.as_geometry;
    acceleration_build_geometry_info.scratchData.deviceAddress = vkal_get_buffer_device_address(g_tlas_scratch_buffer.buffer);
    VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info{};
    acceleration_structure_build_range_info.primitiveCount = blases.size();
    acceleration_structure_build_range_info.primitiveOffset = 0;
    acceleration_structure_build_range_info.firstVertex = 0;
    acceleration_structure_build_range_info.transformOffset = 0;

    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> acceleration_build_geometry_infos = {
        acceleration_build_geometry_info
    };
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> acceleration_build_structure_range_infos = {
        &acceleration_structure_build_range_info
    };

    // Build the acceleration structure on the device via a one-time command buffer submission
    // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), 
    // but we prefer device builds    
    g_tlas_one_time_command_buffer = vkal_create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true); // Used by create_tlas
    vkCmdBuildAccelerationStructuresKHR(
        g_tlas_one_time_command_buffer,
        1,
        acceleration_build_geometry_infos.data(),
        acceleration_build_structure_range_infos.data()
    );
    vkal_flush_command_buffer(g_tlas_one_time_command_buffer, vkal_info->graphics_queue, 1); // 0 = Don't free command buffer

    // TODO: Delete scratch buffer

    // Get the top acceleration structure's handle, which will be used to setup it's descriptor
    VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
    acceleration_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    acceleration_device_address_info.accelerationStructure = g_top_level_acceleration_structure.handle;
    g_top_level_acceleration_structure.device_address = vkGetAccelerationStructureDeviceAddressKHR(vkal_info->device, &acceleration_device_address_info);
}

void update_tlas(VkalInfo* vkal_info, std::vector<VkalAccelerationStructure> blases, std::vector<Model> models)
{
    ASInfo as_info = get_acceleration_structure_info(vkal_info, blases, models);
    build_tlas(vkal_info, as_info, blases, true);
}

// TODO: Fix Buffer mapping issues in VKAL!
VkalAccelerationStructure create_tlas(VkalInfo* vkal_info, std::vector<VkalAccelerationStructure> blases, std::vector<Model> models)
{
    g_instance_device_memory = vkal_allocate_devicememory(
        blases.size() * sizeof(VkAccelerationStructureInstanceKHR),
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

    g_instance_buffer = vkal_create_buffer(
        blases.size() * sizeof(VkAccelerationStructureInstanceKHR),
        &g_instance_device_memory,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
 
    ASInfo as_info = get_acceleration_structure_info(vkal_info, blases, models);

    // Create a buffer to hold the acceleration structure
    g_tlas_device_memory = vkal_allocate_devicememory(
        as_info.as_build_sizes_info.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    );

    g_top_level_acceleration_structure = {};
    g_top_level_acceleration_structure.buffer = vkal_create_buffer(
        as_info.as_build_sizes_info.accelerationStructureSize,
        &g_tlas_device_memory,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

    // Create the acceleration structure
    create_tlas_handle(vkal_info, as_info);
    
    // The actual build process starts here

    // Create a scratch buffer as a temporary storage for the acceleration structure build
    g_tlas_scratch_memory = vkal_allocate_devicememory(
        as_info.as_build_sizes_info.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    );
    g_tlas_scratch_buffer = vkal_create_buffer(
        as_info.as_build_sizes_info.buildScratchSize,
        &g_tlas_scratch_memory,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    );

    build_tlas(vkal_info, as_info, blases, false);

    return g_top_level_acceleration_structure;
}

VkPhysicalDeviceRayTracingPipelinePropertiesKHR get_ray_tracing_pipeline_properties(VkalInfo* vkal_info)
{
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties = {};
    ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 device_properties{};
    device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    device_properties.pNext = &ray_tracing_pipeline_properties;
    vkGetPhysicalDeviceProperties2(vkal_info->physical_device, &device_properties);

    return ray_tracing_pipeline_properties;
}

// Connect Ray Tracing Pipeline's shader programs with the TLAS through the shader binding table.
void create_shader_binding_tables(VkalInfo* vkal_info, VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties, PipelineInfo pipeline_info)
{
    const uint32_t           handle_size = ray_tracing_pipeline_properties.shaderGroupHandleSize;
    const uint32_t           handle_size_aligned = vkal_aligned_size(handle_size, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);
    const uint32_t           group_count = static_cast<uint32_t>(pipeline_info.shader_groups.size());
    const uint32_t           sbt_size = group_count * handle_size_aligned;
    const VkBufferUsageFlags sbt_buffer_usage_flags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    //const VmaMemoryUsage     sbt_memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    
    
    // Create binding table buffers for each shader type

    // TODO: Only use one device memory for all shader binding tables
    DeviceMemory shader_binding_table_device_memory_raygen = vkal_allocate_devicememory(1024 * 1024, sbt_buffer_usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    DeviceMemory shader_binding_table_device_memory_miss = vkal_allocate_devicememory(1024 * 1024, sbt_buffer_usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    DeviceMemory shader_binding_table_device_memory_hit = vkal_allocate_devicememory(1024 * 1024, sbt_buffer_usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

    g_raygen_shader_binding_table = vkal_create_buffer(handle_size, &shader_binding_table_device_memory_raygen, sbt_buffer_usage_flags);
    g_miss_shader_binding_table = vkal_create_buffer(2*handle_size, &shader_binding_table_device_memory_miss, sbt_buffer_usage_flags);
    g_hit_shader_binding_table = vkal_create_buffer(handle_size, &shader_binding_table_device_memory_hit, sbt_buffer_usage_flags);
    //raygen_shader_binding_table = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usafge_flags, sbt_memory_usage, 0);
    //miss_shader_binding_table = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usafge_flags, sbt_memory_usage, 0);
    //hit_shader_binding_table = std::make_unique<vkb::core::Buffer>(get_device(), handle_size, sbt_buffer_usafge_flags, sbt_memory_usage, 0);

    // Copy the pipeline's shader handles into a host buffer
    std::vector<uint8_t> shader_handle_storage(sbt_size);
    vkGetRayTracingShaderGroupHandlesKHR(vkal_info->device, pipeline_info.pipeline, 0, group_count, sbt_size, shader_handle_storage.data());

    // Copy the shader handles from the host buffer to the binding tables
    vkal_map_buffer(&g_raygen_shader_binding_table);
    uint8_t* data = static_cast<uint8_t*>(g_raygen_shader_binding_table.mapped);
    memcpy(data, shader_handle_storage.data(), handle_size);
    vkal_unmap_buffer(&g_raygen_shader_binding_table);

    vkal_map_buffer(&g_miss_shader_binding_table);
    data = static_cast<uint8_t*>(g_miss_shader_binding_table.mapped);
    memcpy(data, shader_handle_storage.data() + handle_size_aligned, 2*handle_size);
    vkal_unmap_buffer(&g_miss_shader_binding_table);

    vkal_map_buffer(&g_hit_shader_binding_table);
    data = static_cast<uint8_t*>(g_hit_shader_binding_table.mapped);
    memcpy(data, shader_handle_storage.data() + handle_size_aligned * 3, handle_size);
    vkal_unmap_buffer(&g_hit_shader_binding_table);
}

void build_command_buffers(VkalInfo * vkal_info, PipelineInfo pipeline_info, VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties)
{    
    vkDeviceWaitIdle(vkal_info->device);

    // VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info(); // TODO: Do a VKAL version of this?
    VkCommandBufferBeginInfo command_buffer_begin_info = { };
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    for (int32_t i = 0; i < vkal_info->default_command_buffer_count; ++i)
    {
        VKAL_ASSERT(vkBeginCommandBuffer(vkal_info->default_command_buffers[i], &command_buffer_begin_info));

        /*
            Setup the strided device address regions pointing at the shader identifiers in the shader binding table
        */

        const uint32_t handle_size_aligned = vkal_aligned_size(ray_tracing_pipeline_properties.shaderGroupHandleSize, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);

        VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
        raygen_shader_sbt_entry.deviceAddress = vkal_get_buffer_device_address(g_raygen_shader_binding_table.buffer);
        raygen_shader_sbt_entry.stride = handle_size_aligned;
        raygen_shader_sbt_entry.size = handle_size_aligned;

        VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
        miss_shader_sbt_entry.deviceAddress = vkal_get_buffer_device_address(g_miss_shader_binding_table.buffer);
        miss_shader_sbt_entry.stride = handle_size_aligned;
        miss_shader_sbt_entry.size = 2*handle_size_aligned;

        VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
        hit_shader_sbt_entry.deviceAddress = vkal_get_buffer_device_address(g_hit_shader_binding_table.buffer);
        hit_shader_sbt_entry.stride = handle_size_aligned;
        hit_shader_sbt_entry.size = handle_size_aligned;

        VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

        /*
            Dispatch the ray tracing commands
        */
        vkCmdBindPipeline(vkal_info->default_command_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_info.pipeline);
        vkCmdBindDescriptorSets(vkal_info->default_command_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_info.pipeline_layout, 0, 1, &g_descriptor_set, 0, 0);

        vkCmdTraceRaysKHR(
            vkal_info->default_command_buffers[i],
            &raygen_shader_sbt_entry,
            &miss_shader_sbt_entry,
            &hit_shader_sbt_entry,
            &callable_shader_sbt_entry,
            width,
            height,
            1);

        /*
            Copy ray tracing output to swap chain image
        */

        // Prepare current swap chain image as transfer destination
        set_image_layout(
            vkal_info->default_command_buffers[i],
            vkal_info->swapchain_images[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        // Prepare ray tracing output image as transfer source
        set_image_layout(
            vkal_info->default_command_buffers[i],
            get_image(g_storage_image.image),
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        VkImageCopy copy_region{};
        copy_region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy_region.srcOffset = { 0, 0, 0 };
        copy_region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy_region.dstOffset = { 0, 0, 0 };
        copy_region.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        vkCmdCopyImage(vkal_info->default_command_buffers[i], get_image(g_storage_image.image), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vkal_info->swapchain_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

        // Transition swap chain image back for presentation
        set_image_layout(
            vkal_info->default_command_buffers[i],
            vkal_info->swapchain_images[i],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        // Transition ray tracing output image back to general layout
        set_image_layout(
            vkal_info->default_command_buffers[i],
            get_image(g_storage_image.image),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        /*
            Start a new render pass to draw the UI overlay on top of the ray traced image
        */
        //VkClearValue clear_values[2];
        //clear_values[0].color = { {0.0f, 0.0f, 0.033f, 0.0f} };
        //clear_values[1].depthStencil = { 0.0f, 0 };

        //VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
        //render_pass_begin_info.renderPass = render_pass;
        //render_pass_begin_info.framebuffer = framebuffers[i];
        //render_pass_begin_info.renderArea.extent.width = width;
        //render_pass_begin_info.renderArea.extent.height = height;
        //render_pass_begin_info.clearValueCount = 2;
        //render_pass_begin_info.pClearValues = clear_values;

        //vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        //draw_ui(draw_cmd_buffers[i]);
        //vkCmdEndRenderPass(draw_cmd_buffers[i]);

        VKAL_ASSERT(vkEndCommandBuffer(vkal_info->default_command_buffers[i]));
    }
}

std::vector<VkalBuffer> upload_materials(std::vector<Material> materials)
{   
    std::vector<VkalBuffer> material_buffers{};

    DeviceMemory material_memory = vkal_allocate_devicememory(1024 * 1024 * 10,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    for (size_t i = 0; i < materials.size(); i++) {
        VkalBuffer material_buffer = vkal_create_buffer(sizeof(Material), &material_memory,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
        //vkal_update_buffer(&material_buffer, (uint8_t*)&materials[i], sizeof(Material));
        vkal_map_buffer(&material_buffer);
        memcpy(material_buffer.mapped, (uint8_t*)&materials[i], sizeof(Material));
        vkal_unmap_buffer(&material_buffer);
        material_buffers.push_back(material_buffer);
    }
    
    return material_buffers;
};

ModelBuffers upload_model(Model model)
{   
    ModelBuffers buffers{};

    DeviceMemory vertex_memory = vkal_allocate_devicememory(1024 * 1024 * 10,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    VkalBuffer vertex_buffer = vkal_create_buffer(model.vertex_count * sizeof(Vertex), &vertex_memory,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    //vkal_update_buffer(&vertex_buffer, (uint8_t*)(model.vertices), model.vertex_count * sizeof(Vertex));
    vkal_map_buffer(&vertex_buffer);
    memcpy(vertex_buffer.mapped, model.vertices, model.vertex_count * sizeof(Vertex));
    vkal_unmap_buffer(&vertex_buffer);
    buffers.vertexBuffer = vertex_buffer;

    DeviceMemory index_memory = vkal_allocate_devicememory(1024 * 1024 * 10,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    VkalBuffer index_buffer = vkal_create_buffer(model.index_count * sizeof(uint32_t), &index_memory,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    //vkal_update_buffer(&index_buffer, (uint8_t*)(model.indices), model.index_count * sizeof(uint32_t));
    vkal_map_buffer(&index_buffer);
    memcpy(index_buffer.mapped, model.indices, model.index_count * sizeof(uint32_t));
    vkal_unmap_buffer(&index_buffer);
    buffers.indexBuffer = index_buffer;

    return buffers;
}

int main(int argc, char** argv)
{
    init_window();

    char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,

        // Ray Tracing support in Vulkan API
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME
    };
    uint32_t device_extension_count = sizeof(device_extensions) / sizeof(*device_extensions);

    char* instance_extensions[] = {
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#ifdef _DEBUG
    ,VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };
    uint32_t instance_extension_count = sizeof(instance_extensions) / sizeof(*instance_extensions);

    char* instance_layers[] = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_monitor"
    };
    uint32_t instance_layer_count = 0;
#ifdef _DEBUG
    instance_layer_count = sizeof(instance_layers) / sizeof(*instance_layers);
#endif

    vkal_create_instance_glfw(window, instance_extensions, instance_extension_count, instance_layers, instance_layer_count);

    VkalPhysicalDevice* devices = 0;
    uint32_t device_count;
    vkal_find_suitable_devices(device_extensions, device_extension_count, &devices, &device_count);
    assert(device_count > 0 && "No suitable device found!");
    printf("Suitable Devices:\n");
    for (uint32_t i = 0; i < device_count; ++i) {
        printf("    Phyiscal Device %d: %s\n", i, devices[i].property.deviceName);
    }
    vkal_select_physical_device(&devices[0]);

    VkalWantedFeatures wanted_features{};
    wanted_features.features12.runtimeDescriptorArray = VK_TRUE;
    wanted_features.features12.descriptorIndexing = VK_TRUE;
    wanted_features.features12.bufferDeviceAddress = VK_TRUE;
    wanted_features.features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    wanted_features.features12.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    wanted_features.rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    wanted_features.accelerationStructureFeatures.accelerationStructure = VK_TRUE;
    
    vkal_init_raytracing();

    VkalInfo* vkal_info = vkal_init(device_extensions, device_extension_count, wanted_features, VK_INDEX_TYPE_UINT32);    

    /* Textures */
    Image sandImage = load_image("../../src/examples/assets/textures/sand_diffuse.jpg");
    Image knightImage = load_image("../../src/examples/assets/textures/knight.png");

    VkalTexture sandTexture = vkal_create_texture(7, sandImage.data, sandImage.width, sandImage.height, 4, 0,
        VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    VkalTexture knightTexture = vkal_create_texture(7, knightImage.data, knightImage.width, knightImage.height, 4, 0,
        VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    std::vector<VkalTexture> textures = {
        sandTexture,
        knightTexture
    };

    /* Materials */
    Material goldMat{};
    goldMat.diffuse = glm::vec4(1.0, 0.85, 0.0, 1.0);
    goldMat.is_textured = 1;
    goldMat.texture_id = 1;

    Material redMat{};
    redMat.diffuse = glm::vec4(1.0, 0.0, 0.0, 1.0);
    redMat.is_textured = 1;
    redMat.texture_id = 0;

    Material greenMat{};
    greenMat.diffuse = glm::vec4(0.1, 0.6, 0.1, 1.0);
    greenMat.is_textured = 0;
    greenMat.texture_id = 0;

    std::vector<Material> materials = {
        goldMat,
        redMat,
        greenMat
    };  
    std::vector<VkalBuffer> material_buffers = upload_materials(materials);

    /* Geometry */
    Model palmtree = create_model_from_file_indexed("../../src/examples/assets/models/palmtree.obj");
    palmtree.pos = glm::vec3(0, 2.7, 0);
    palmtree.model_matrix = glm::translate(glm::mat4(1), palmtree.pos);
    ModelBuffers palmtree_buffer = upload_model(palmtree);
    /*glm::vec3 model[3] = {
        glm::vec3(-1, -1, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(1, -1, 0)
    };*/

    Model plane = create_model_from_file_indexed("../../src/examples/assets/models/plane.obj");        
    plane.pos = glm::vec3(0, 0, 0);
    plane.model_matrix = glm::translate(glm::mat4(1), plane.pos);
    ModelBuffers plane_buffer = upload_model(plane);

    Model knight = create_model_from_file_indexed("../../src/examples/assets/models/pknight_small.obj");
    knight.pos = glm::vec3(0);
    knight.model_matrix = glm::translate(glm::mat4(1), knight.pos);
    ModelBuffers knight_buffer = upload_model(knight);
    std::vector<Model> models = {
        palmtree,
        plane,
        knight
    };
    std::vector<ModelBuffers> model_buffers = {
        palmtree_buffer,
        plane_buffer,
        knight_buffer
    };
    std::vector<VkalBuffer> metadata_buffers = create_instance_metadata_buffers(model_buffers.size());
    uint32_t material_info[] = { 2, 1, 0 };
    for (size_t i = 0; i < metadata_buffers.size(); i++) {
        vkal_map_buffer(&metadata_buffers[i]);
        //vkal_update_buffer(&metadata_buffers[i], (uint8_t*)&(material_info[i]), sizeof(uint32_t));
        memcpy(metadata_buffers[i].mapped, (uint8_t*)&(material_info[i]), sizeof(uint32_t));
        vkal_unmap_buffer(&metadata_buffers[i]);
    }

    /* Build acceleration structures */
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties = get_ray_tracing_pipeline_properties(vkal_info);
    VkalAccelerationStructure palmtree_blas = create_blas(vkal_info, palmtree_buffer.vertexBuffer, palmtree.vertex_count);
    VkalAccelerationStructure plane_blas = create_blas(vkal_info, plane_buffer.vertexBuffer, plane.vertex_count);
    VkalAccelerationStructure knight_blas = create_blas(vkal_info, knight_buffer.vertexBuffer, knight.vertex_count);
    std::vector<VkalAccelerationStructure> blases = {
        palmtree_blas,
        plane_blas,
        knight_blas
    };

    VkalAccelerationStructure tlas = create_tlas(vkal_info, blases, models);
    PipelineInfo pipeline_info = create_ray_tracing_pipeline(vkal_info, blases.size(), materials.size(), textures.size());
    create_shader_binding_tables(vkal_info, ray_tracing_pipeline_properties, pipeline_info);
    create_descriptor_sets(vkal_info, pipeline_info.descriptor_set_layout, tlas, model_buffers, material_buffers, metadata_buffers, textures);
    create_storage_image(vkal_info);
    build_command_buffers(vkal_info, pipeline_info, ray_tracing_pipeline_properties);

	// Setup the camera and setup storage for Uniform Buffer
    Camera camera = Camera(glm::vec3(0.0f, 5.0f, 10.0f));
    ViewProjection view_proj_data = { };
	view_proj_data.view = glm::inverse(glm::lookAt(camera.m_Pos, camera.m_Center, camera.m_Up));

    // Uniform Buffer for View-Projection Matrix
	vkal_update_uniform(&g_view_proj_ub, &view_proj_data);

    // Main Loop
    double dt = 0;
    float foo = 1.0f;
    while (!glfwWindowShouldClose(window))
    {
        double start_time = glfwGetTime();

        glfwPollEvents();

        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) continue; // TODO: This is hacky?

        update_camera(camera, dt*1000.0f);
        update_camera_mouse_look(camera, dt*1000.0f);

        models[2].pos.x = 3.0*sinf(foo);
        models[2].pos.z = 3.0*cosf(foo);
        models[2].model_matrix = glm::translate(glm::mat4(1), models[2].pos);
        foo += 1.0f*dt;
        
        // create_tlas(vkal_info, blases, models);
        update_tlas(vkal_info, blases, models);

        view_proj_data.proj = adjust_y_for_vulkan_ndc * glm::inverse(glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 1000.0f));
        view_proj_data.view = glm::inverse(glm::lookAt(camera.m_Pos, camera.m_Center, camera.m_Up));
		vkal_update_uniform(&g_view_proj_ub, &view_proj_data);    

        {
            uint32_t image_id = vkal_get_image();
         
            VkCommandBuffer command_buffers1[] = { vkal_info->default_command_buffers[image_id] };
            vkal_queue_submit(command_buffers1, 1);

            vkal_present(image_id);
        }

        if (width != g_storage_image.width || height != g_storage_image.height)
        {
            create_storage_image(vkal_info);
            build_command_buffers(vkal_info, pipeline_info, ray_tracing_pipeline_properties);
        }

        double end_time = glfwGetTime();
        dt = end_time - start_time;        
    }

    vkal_cleanup();


    return 0;
}
