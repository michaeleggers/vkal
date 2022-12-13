/* Michael Eggers, 9/20/2020
   Rendering to two different textures and blending thme together in one last step.
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <GLFW/glfw3.h>

#include <vkal.h>

#include "platform.h"
#include "tr_math.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

static GLFWwindow * window;

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

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
    window = glfwCreateWindow(1920, 1080, "Vulkan Sobel", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    //glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    //glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
}

Image load_image_file(char const * file)
{
    char exe_path[256];
    get_exe_path(exe_path, 256 * sizeof(char));
    char abs_path[256];
    memcpy(abs_path, exe_path, 256);
    strcat(abs_path, file);

    Image image = { };
    int tw, th, tn;
    image.data = stbi_load(abs_path, &tw, &th, &tn, 4);
    assert(image.data != NULL);
    image.width = tw;
    image.height = th;
    image.channels = tn;

    return image;
}

int main(int argc, char ** argv)
{
    init_window();
   
    char * device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME
//	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME /* is core already in Vulkan 1.2, not necessary */
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
	"VK_LAYER_KHRONOS_validation", //"VK_LAYER_LUNARG_standard_validation", <- deprecated!
	"VK_LAYER_LUNARG_monitor"
    };
    uint32_t instance_layer_count = 0;
#ifdef _DEBUG
    instance_layer_count = sizeof(instance_layers) / sizeof(*instance_layers);    
#endif
   
    vkal_create_instance_glfw(window,
			 instance_extensions, instance_extension_count,
 			 instance_layers, instance_layer_count);
    
    VkalPhysicalDevice * devices = 0;
    uint32_t device_count;
    vkal_find_suitable_devices(device_extensions, device_extension_count,
			       &devices, &device_count);
    assert(device_count > 0);
    printf("Suitable Devices:\n");
    for (uint32_t i = 0; i < device_count; ++i) {
	printf("    Phyiscal Device %d: %s\n", i, devices[i].property.deviceName);
    }
    vkal_select_physical_device(&devices[0]);
    VkalInfo * vkal_info =  vkal_init(device_extensions, device_extension_count);
    
    /* Shader Setup */
    uint8_t * vertex_byte_code = 0;
    int vertex_code_size;
    read_file("../../src/examples/assets/shaders/rendertexture_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    read_file("../../src/examples/assets/shaders/rendertexture_frag.spv", &fragment_byte_code, &fragment_code_size);
    /* Render to Texture Pass */
    ShaderStageSetup shader_setup = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);

    /* Composite Pass */
    read_file("../../src/examples/assets/shaders/rendertexture_composite_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup_composite = vkal_create_shaders(
	vertex_byte_code, vertex_code_size,
	fragment_byte_code, fragment_code_size);
    
    /* Vertex Input Assembly */
    VkVertexInputBindingDescription vertex_input_bindings[] =
	{
	    { 0, 2*sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
	};
    
    VkVertexInputAttributeDescription vertex_attributes[] =
	{
	    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },               // pos
	    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3) },    // color
	    { 2, 0, VK_FORMAT_R32G32_SFLOAT,    2*sizeof(vec3) },  // UV 
	};
    uint32_t vertex_attribute_count = sizeof(vertex_attributes)/sizeof(*vertex_attributes);

    /* Descriptor Sets */
    /* Render to Texture Pass */
    VkDescriptorSetLayoutBinding set_layout[] = {
	{
	    0,
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	    1, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 1);
    

    /* Composite Descriptor Set */
    VkDescriptorSetLayoutBinding set_layout_composite[] =
    {
	{
	    0,
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	    1, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	},
	{
	    1,
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	    1, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout_composite = vkal_create_descriptor_set_layout(set_layout_composite, 2);

    /* Build the descriptor Sets */
    VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout,
	descriptor_set_layout,
	descriptor_set_layout_composite
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);
    VkDescriptorSet * descriptor_sets = (VkDescriptorSet*)malloc(descriptor_set_layout_count * sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_sets);
	
    /* Pipelines */
    /* Render Texture */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	&layouts[0], 2, 
	NULL, 0);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_to_image_render_pass, pipeline_layout);
    /* Composite */
    VkPipelineLayout pipeline_layout_composite = vkal_create_pipeline_layout(
	&layouts[2], 1,
	NULL, 0);
    VkPipeline graphics_pipeline_composite = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup_composite, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout_composite);
    
    /* Render Image (Should this be called render-texture?) */
    RenderImage render_image = create_render_image(1920, 1080);
    VKAL_DBG_IMAGE_NAME(vkal_info->device, get_image(render_image.color_image.image), "Render Image 1920x1080");
    VkSampler sampler = create_sampler(
		VK_FILTER_NEAREST, VK_FILTER_NEAREST, 
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, 
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    vkal_update_descriptor_set_render_image(
		descriptor_sets[2], 0,
		get_image_view(render_image.color_image.image_view), sampler);
    
    RenderImage render_image2 = create_render_image(2048, 2048);
    VKAL_DBG_IMAGE_NAME(vkal_info->device, get_image(render_image2.color_image.image), "Render Image 1920x1080");
    vkal_update_descriptor_set_render_image(
		descriptor_sets[2], 1,
		get_image_view(render_image2.color_image.image_view), sampler);
    
    /* Model Data */
    float rectvertices[] = {
		// Pos            // Color        // UV
		-1.0,  1.0, 1.0,  1.0, 0.0, 0.0,  0.0, 0.0,
		 1.0,  1.0, 1.0,  0.0, 1.0, 0.0,  1.0, 0.0,
		-1.0, -1.0, 1.0,  0.0, 0.0, 1.0,  0.0, 1.0,
    	 1.0, -1.0, 1.0,  1.0, 1.0, 0.0,  1.0, 1.0
    };
    uint32_t vertex_count = sizeof(rectvertices)/sizeof(*rectvertices) / 8;
    
    uint16_t rectindices[] = {
	    // front
 	    0, 1, 2,
	    2, 1, 3
    };
    uint32_t index_count = sizeof(rectindices)/sizeof(*rectindices);
  
    uint32_t offset_vertices = vkal_vertex_buffer_add(rectvertices, 2*sizeof(vec3) + sizeof(vec2), vertex_count);
    uint32_t offset_indices  = vkal_index_buffer_add(rectindices, index_count);

    /* Texture Data */
    Image image = load_image_file("../../src/examples/assets/textures/hk.jpg");
    VkalTexture texture = vkal_create_texture(0, image.data, image.width, image.height, 4, 0,
					  VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
					  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    free(image.data);
    Image image2 = load_image_file("../../src/examples/assets/textures/brucelee.jpg");
    VkalTexture texture2 = vkal_create_texture(0, image2.data, image2.width, image2.height, 4, 0,
					   VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
					   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    
    free(image2.data);    
    vkal_update_descriptor_set_texture(descriptor_sets[0], texture);
    vkal_update_descriptor_set_texture(descriptor_sets[1], texture2);
	
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();

	{
	    uint32_t image_id = vkal_get_image();

	    vkal_begin_command_buffer(image_id);

	    vkal_begin_render_to_image_render_pass(image_id, vkal_info->default_command_buffers[image_id],
						   vkal_info->render_to_image_render_pass, render_image);
	    vkal_viewport(vkal_info->default_command_buffers[image_id], 0, 0, render_image.width, render_image.height);
	    vkal_scissor(vkal_info->default_command_buffers[image_id], 0, 0, render_image.width, render_image.height);
	    vkal_bind_descriptor_set(image_id, &descriptor_sets[0], pipeline_layout);
	    vkal_draw_indexed(image_id, graphics_pipeline,
			      offset_indices, index_count,
			      offset_vertices);
	    vkal_end_renderpass(image_id);

	    vkal_begin_render_to_image_render_pass(image_id, vkal_info->default_command_buffers[image_id],
						   vkal_info->render_to_image_render_pass, render_image2);
	    vkal_viewport(vkal_info->default_command_buffers[image_id], 0, 0, render_image2.width, render_image2.height);
	    vkal_scissor(vkal_info->default_command_buffers[image_id], 0, 0, render_image2.width, render_image2.height);
	    vkal_bind_descriptor_set(image_id, &descriptor_sets[1], pipeline_layout);
	    vkal_draw_indexed(image_id, graphics_pipeline,
			      offset_indices, index_count,
			      offset_vertices);
	    vkal_end_renderpass(image_id);

	    vkal_begin_render_pass(image_id, vkal_info->render_pass);
	    vkal_viewport(vkal_info->default_command_buffers[image_id], 0, 0,
			  2*image2.width, 2*image2.height);
	    vkal_scissor(vkal_info->default_command_buffers[image_id], 0, 0,
			 vkal_info->swapchain_extent.width, vkal_info->swapchain_extent.height);
	    vkal_bind_descriptor_set(image_id, &descriptor_sets[2], pipeline_layout_composite);
	    vkal_draw_indexed(image_id, graphics_pipeline_composite,
			      offset_indices, index_count,
			      offset_vertices);
	    vkal_end_renderpass(image_id);
	    
	    vkal_end_command_buffer(image_id);
	    VkCommandBuffer command_buffers1[] = { vkal_info->default_command_buffers[image_id] };
	    vkal_queue_submit(command_buffers1, 1);

	    vkal_present(image_id);
	}
    }
    
    vkal_cleanup();

    glfwDestroyWindow(window);
 
    return 0;
}
