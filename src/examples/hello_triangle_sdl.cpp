/* Michael Eggers, 27/12/2021

   The Hello World of computer graphics
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "../vkal/vkal.h"

#include "utils/platform.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768

static SDL_Window * window;
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


void init_window()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow(
        "Hello Vulkan SDL",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;
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
        //"VK_LAYER_LUNARG_monitor"
    };
    uint32_t instance_layer_count = 0;
#ifdef _DEBUG
    instance_layer_count = sizeof(instance_layers) / sizeof(*instance_layers);
#endif

    vkal_create_instance_sdl(window, instance_extensions, instance_extension_count, instance_layers, instance_layer_count);

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

    /* Shader Setup */
    uint8_t* vertex_byte_code = 0;
    int vertex_code_size;
    read_file("/../../src/examples/assets/shaders/hello_triangle_vert.spv", &vertex_byte_code, &vertex_code_size);

    uint8_t* fragment_byte_code = 0;
    int fragment_code_size;
    read_file("/../../src/examples/assets/shaders/hello_triangle_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);
    
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
    uint32_t vertex_count = 3;

    uint16_t rect_indices[] = 
    {
        0, 1, 2,
    };
    uint32_t index_count = 3;

    // Upload Model Data to GPU
    uint64_t offset_vertices = vkal_vertex_buffer_add(rect_vertices, 2*sizeof(glm::vec3) + sizeof(glm::vec2), vertex_count);
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
    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                {
                    running = false;
                }
                break;
                }
            }
            break;
            }
        }

		int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);

		view_proj_data.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
		vkal_update_uniform(&view_proj_ub, &view_proj_data);

        {
            uint32_t image_id = vkal_get_image();
               
            VkCommandBuffer currentCmdBuffer = vkal_info->default_command_buffers[image_id];

            vkal_set_clear_color({0.2f, 0.2f, 0.2f, 1.0f});

            vkal_begin_command_buffer(image_id);
            vkal_begin_render_pass(image_id, vkal_info->render_pass);
            vkal_viewport(currentCmdBuffer,
                0, 0,
                (float)width, (float)height);
            vkal_scissor(currentCmdBuffer,
                0, 0,
                (float)width, (float)height);
            vkal_bind_descriptor_set(image_id, &descriptor_sets[0], pipeline_layout);
            vkal_draw_indexed(image_id, graphics_pipeline,
                offset_indices, index_count,
                offset_vertices);
            vkal_end_renderpass(image_id);
            vkal_end_command_buffer(image_id);

            vkal_queue_submit(&currentCmdBuffer, 1);

            vkal_present(image_id);
        }
    }

	free(descriptor_sets);

    vkal_cleanup();

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
