#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <GLFW/glfw3.h>

#include "vkal.h"
#include "platform.h"

static GLFWwindow * window;
static Platform p;


// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
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
    window = glfwCreateWindow(VKAL_SCREEN_WIDTH, VKAL_SCREEN_HEIGHT, "Vulkan", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    //glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    //glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
}

int main(int argc, char ** argv)
{
    init_window();
    init_platform(&p);
    
    char * device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
    uint32_t device_extension_count = sizeof(device_extensions) / sizeof(*device_extensions);

    char * instance_extensions[] = {
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#ifdef _DEBUG
	,VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };
    uint32_t instance_extension_count = sizeof(instance_extensions) / sizeof(*instance_extensions);

    char * instance_layers[] = {
	"VK_LAYER_LUNARG_standard_validation",
	"VK_LAYER_LUNARG_monitor"
    };
    uint32_t instance_layer_count = 0;
#ifdef _DEBUG
    instance_layer_count = sizeof(instance_layers) / sizeof(*instance_layers);    
#endif
   
    vkal_create_instance(window,
			 instance_extensions, instance_extension_count,
 			 instance_layers, instance_layer_count);
    
    VkalPhysicalDevice * devices = 0;
    uint32_t device_count;
    vkal_find_suitable_devices(device_extensions, device_extension_count,
			       &devices, &device_count);
    assert(device_count > 0);
    printf("Suitable Devices:\n");
    for (uint32_t i = 0; i < device_count; ++i) {
	printf("Phyiscal Device %d: %s\n", i, devices[i].property.deviceName);
    }
    vkal_select_physical_device(&devices[0]);
    VkalInfo * vkal_info =  vkal_init(device_extensions, device_extension_count);

    
    /* Shader Setup */
    uint8_t * vertex_byte_code = 0;
    int vertex_code_size;
    p.rfb("../assets/shaders/vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    p.rfb("../assets/shaders/frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);

    /* Vertex Input Assembly */
    VkVertexInputBindingDescription vertex_input_bindings[] =
	{
	    { 0, sizeof(vec3), VK_VERTEX_INPUT_RATE_VERTEX }
	};
    
    VkVertexInputAttributeDescription vertex_attributes[] =
	{
	    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }
	};

    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	NULL, 0, 
	NULL, 0);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, 1,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_COUNTER_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout);

    /* Model Data */
    vec3 cube_vertices[] = {
	// front
	{-1.0, -1.0,  1.0},
	{0.0, 1.0,  1.0},
	{1.0,  -1.0,  1.0}
    };
    uint32_t vertex_count = sizeof(cube_vertices)/sizeof(*cube_vertices);
    
    uint16_t cube_indices[] = {
	// front
	0, 1, 2
    };
    uint32_t index_count = sizeof(cube_indices)/sizeof(*cube_indices);
  
    uint32_t offset_vertices = vkal_vertex_buffer_add2(cube_vertices, sizeof(vec3), vertex_count);
    uint32_t offset_indices  = vkal_index_buffer_add(cube_indices, index_count);
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();

	{
	    uint32_t image_id = vkal_get_image();

	    vkal_begin_command_buffer(vkal_info->command_buffers[image_id]);
	    vkal_begin_render_pass(image_id, vkal_info->command_buffers[image_id], vkal_info->render_pass);
// Do draw calls here
	    vkal_draw_indexed(image_id, graphics_pipeline,
			      offset_indices, index_count,
			      offset_vertices);
	    vkal_end_renderpass(vkal_info->command_buffers[image_id]);
	    vkal_end_command_buffer(vkal_info->command_buffers[image_id]);
			
	    VkCommandBuffer command_buffers[] = { vkal_info->command_buffers[image_id] };
	    vkal_queue_submit(command_buffers, 1);

	    vkal_present(image_id);
	}
    }

    
    vkal_cleanup();

    glfwDestroyWindow(window);
 
    glfwTerminate();
    
    return 0;
}
