/* Michael Eggers, 06/24/2022

   Use Dear ImGUI to draw some UI.
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <vector>

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include <vkal.h>

#include "platform.h"
#include "glslcompile.h"
#include "common.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768

static GLFWwindow* window;
static int width, height; /* current framebuffer width/height */

typedef struct Camera
{
	glm::vec3 pos;
	glm::vec3 center;
	glm::vec3 up;
	glm::vec3 right;
} Camera;

typedef struct ViewProjection
{
    glm::mat4 view;
    glm::mat4 proj;
} ViewProjection;

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

// GLFW callbacks
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        printf("escape key pressed\n");
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "VKAL Example: hello_triangle.cpp", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;
}


void init_imgui(VkalInfo * vkal_info)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    if (ImGui_ImplGlfw_InitForVulkan(window, true)) {
        printf("ImGui initialized.\n");
    }
    else {
        printf("Could not initialize ImGui!\n");
        exit(-1);
    }

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vkal_info->instance;
    init_info.PhysicalDevice = vkal_info->physical_device;
    init_info.Device = vkal_info->device;
    QueueFamilyIndicies qf = find_queue_families(vkal_info->physical_device, vkal_info->surface);
    init_info.QueueFamily = qf.graphics_family;
    init_info.Queue = vkal_info->graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = vkal_info->default_descriptor_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = vkal_info->swapchain_image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = NULL;
    init_info.CheckVkResultFn = check_vk_result;
    if (ImGui_ImplVulkan_Init(&init_info, vkal_info->render_pass)) {
        printf("ImGui Vulkan part initialized.\n");
    }
    else {
        printf("ImGui Vulkan part NOT initialized!\n");
        exit(-1);
    }

    // Upload Fonts
    {
        VkResult err;
        VkCommandBuffer font_cmd_buffer = vkal_create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
        ImGui_ImplVulkan_CreateFontsTexture(font_cmd_buffer);
        err = vkEndCommandBuffer(font_cmd_buffer);
        check_vk_result(err);
        VkSubmitInfo submit_info = {  };
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1; // vkal_info.default_command_buffer_count;
        submit_info.pCommandBuffers = &font_cmd_buffer;  //vkal_info.default_command_buffers;
        err = vkQueueSubmit(vkal_info->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
        check_vk_result(err);
        err = vkDeviceWaitIdle(vkal_info->device);
        check_vk_result(err);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void deinit_imgui(VkalInfo* vkal_info)
{
    VkResult err = vkDeviceWaitIdle(vkal_info->device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

int main(int argc, char** argv)
{
    init_window();

    char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME
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
    assert(device_count > 0);
    printf("Suitable Devices:\n");
    for (uint32_t i = 0; i < device_count; ++i) {
        printf("    Phyiscal Device %d: %s\n", i, devices[i].property.deviceName);
    }
    vkal_select_physical_device(&devices[0]);
    VkalInfo* vkal_info = vkal_init(device_extensions, device_extension_count);

    init_imgui(vkal_info);

    /* Compile Shaders at runtime */
    uint8_t * vertex_byte_code = 0;
    int vertex_code_size = 0;
    load_glsl_and_compile("../../src/examples/assets/shaders/hello_triangle.vert", &vertex_byte_code, &vertex_code_size, SHADER_TYPE_VERTEX);

    uint8_t * fragment_byte_code = 0;
    int fragment_code_size = 0;
    load_glsl_and_compile("../../src/examples/assets/shaders/hello_triangle.frag", &fragment_byte_code, &fragment_code_size, SHADER_TYPE_FRAGMENT);

    /* Shader Setup */
#if 0
    uint8_t* vertex_byte_code = 0;
    int vertex_code_size;
    read_file("/../../src/examples/assets/shaders/hello_triangle_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t* fragment_byte_code = 0;
    int fragment_code_size;
    read_file("/../../src/examples/assets/shaders/hello_triangle_frag.spv", &fragment_byte_code, &fragment_code_size);
#endif
    
    ShaderStageSetup shader_setup = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);
    free(vertex_byte_code);
    free(fragment_byte_code);

    /* Vertex Input Assembly */
    VkVertexInputBindingDescription vertex_input_bindings[] =
    {
        { 0, 2 * sizeof(glm::vec3) + sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    VkVertexInputAttributeDescription vertex_attributes[] =
    {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },					  // pos
        { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3) },      // color
        { 2, 0, VK_FORMAT_R32G32_SFLOAT,    2 * sizeof(glm::vec3) },  // UV 
    };
    uint32_t vertex_attribute_count = sizeof(vertex_attributes) / sizeof(*vertex_attributes);

    /* Descriptor Sets */
    VkDescriptorSetLayoutBinding set_layout[] = 
    {
        {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            0
        }
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 1);

    VkDescriptorSetLayout layouts[] = {
        descriptor_set_layout
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts) / sizeof(*layouts);
    VkDescriptorSet* descriptor_sets = (VkDescriptorSet*)malloc(descriptor_set_layout_count * sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_sets);

    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(layouts, descriptor_set_layout_count, NULL, 0);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
        vertex_input_bindings, 1,
        vertex_attributes, vertex_attribute_count,
        shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FRONT_FACE_CLOCKWISE,
        vkal_info->render_pass, pipeline_layout);

    /* Model Data */
    float rect_vertices[] = 
    {
        // Pos      // Color        // UV
        -1, -1, 0,  1.0, 0.0, 0.0,  0.0, 0.0,
         0,  1, 0,  0.0, 1.0, 0.0,  1.0, 0.0,
         1, -1, 0,  0.0, 0.0, 1.0,  0.0, 1.0
    };
    uint32_t vertex_count = sizeof(rect_vertices) / sizeof(*rect_vertices) / 8;

    float rect_vertices_2[] =
    {
        // Pos      // Color        // UV
        -2, -2, 0,  1.0, 0.0, 0.0,  0.0, 0.0,
         0,  2, 0,  0.0, 1.0, 0.0,  1.0, 0.0,
         1, -3, 0,  0.0, 0.0, 1.0,  0.0, 1.0
    };

    uint16_t rect_indices[] = 
    {
        0, 1, 2,
    };
    uint32_t index_count = sizeof(rect_indices) / sizeof(*rect_indices);

    // Upload Model Data to GPU
    uint64_t offset_vertices = vkal_vertex_buffer_add(rect_vertices, 3 * sizeof(glm::vec3), vertex_count);
	uint64_t offset_indices = vkal_index_buffer_add(rect_indices, index_count);

	// Setup the camera and setup storage for Uniform Buffer
	Camera camera;
	camera.pos = glm::vec3(0.0f, 0.0f, 3.f);
	camera.center = glm::vec3(0.0f);
	camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	ViewProjection view_proj_data;
	view_proj_data.view = glm::lookAt(camera.pos, camera.center, camera.up);

    // Uniform Buffer for View-Projection Matrix
    UniformBuffer view_proj_ub = vkal_create_uniform_buffer(sizeof(ViewProjection), 1, 0);
	vkal_update_descriptor_set_uniform(descriptor_sets[0], view_proj_ub, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	vkal_update_uniform(&view_proj_ub, &view_proj_data);

    // Main Loop
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool toggleVertexBuffer = false;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

		int width, height;
        glfwGetFramebufferSize(window, &width, &height);
		view_proj_data.proj =  adjust_y_for_vulkan_ndc * glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
		vkal_update_uniform(&view_proj_ub, &view_proj_data);

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // 1. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, triangle!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            // Overwrite existing data in the default vertex buffer by updating the buffer's data at position 0.
            if (ImGui::Button("Update Vertex Buffer")) {
                toggleVertexBuffer = !toggleVertexBuffer;
                if (toggleVertexBuffer) {
                    vkal_vertex_buffer_update(rect_vertices_2, vertex_count, 3 * sizeof(glm::vec3), 0);
                }
                else {
                    vkal_vertex_buffer_update(rect_vertices, vertex_count, 3 * sizeof(glm::vec3), 0);
                }
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        {
            uint32_t image_id = vkal_get_image();

            vkal_begin_command_buffer(image_id);
            vkal_begin_render_pass(image_id, vkal_info->render_pass);

            vkal_viewport(vkal_info->default_command_buffers[image_id],
                0, 0,
                (float)width, (float)height);
            vkal_scissor(vkal_info->default_command_buffers[image_id],
                0, 0,
                (float)width, (float)height);
            vkal_bind_descriptor_set(image_id, &descriptor_sets[0], pipeline_layout);
            vkal_draw_indexed(image_id, graphics_pipeline,
                offset_indices, index_count,
                offset_vertices, 1);

            // Rendering ImGUI
            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();
            // Record dear imgui primitives into command buffer
            ImGui_ImplVulkan_RenderDrawData(draw_data, vkal_info->default_command_buffers[image_id]);

            // End renderpass and submit the buffer to graphics-queue.
            vkal_end_renderpass(image_id);
            vkal_end_command_buffer(image_id);
            VkCommandBuffer command_buffers1[] = { vkal_info->default_command_buffers[image_id] };
            vkal_queue_submit(command_buffers1, 1);

            vkal_present(image_id);
        }
    }

    // Cleanup

    deinit_imgui(vkal_info);

	free(descriptor_sets);

    vkal_cleanup();

    return 0;
}
