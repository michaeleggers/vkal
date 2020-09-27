/* Michael Eggers, 9/20/2020

   This example uses vkal_bind_descriptor_set_dynamic to send the texture index to
   the fragment shader. These Dynamic Descriptor Sets are explained well here:
   https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
   
   This way the index which is used to lookup the correct texture in the
   descriptor-array for samplers can be passed through a single descriptor.
   when binding, the offset within the buffer is provided.
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <GLFW/glfw3.h>

#include "../vkal.h"
#include "../platform.h"
#include "utils/tr_math.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct MaterialUniform
{
    uint32_t * index;
} MaterialUniform;

static GLFWwindow * window;
static Platform p;
static MaterialUniform material_data;

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
    window = glfwCreateWindow(800, 800, "Vulkan", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    //glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    //glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
}

Image load_image_file(char const * file)
{
    Image image = (Image){0};
    int tw, th, tn;
    image.data = stbi_load(file, &tw, &th, &tn, 4);
    assert(image.data != NULL);
    image.width = tw;
    image.height = th;
    image.channels = tn;

    return image;
}

int main(int argc, char ** argv)
{
    init_window();
    init_platform(&p);
    
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
	printf("    Phyiscal Device %d: %s\n", i, devices[i].property.deviceName);
    }
    vkal_select_physical_device(&devices[0]);
    VkalInfo * vkal_info =  vkal_init(device_extensions, device_extension_count);
    
    /* Shader Setup */
    uint8_t * vertex_byte_code = 0;
    int vertex_code_size;
    p.rfb("../src/examples/assets/shaders/textures_descriptorarray_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    p.rfb("../src/examples/assets/shaders/textures_descriptorarray_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup = vkal_create_shaders(
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
    VkDescriptorSetLayoutBinding set_layout[] = {
	{
	    0,
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	    VKAL_MAX_TEXTURES, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	},
	{
	    1,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
	    1, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	},
	{
	    2,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    1, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	},
	{
	    3,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    1, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 4);
    
    VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);

    VkDescriptorSet * descriptor_sets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, 1, &descriptor_sets);
	
    /* HACK: Update Texture Slots so validation layer won't complain */
    Image yakult_image = load_image_file("../src/examples/assets/textures/yakult.png");
    Texture yakult_texture = vkal_create_texture(
	0, yakult_image.data, yakult_image.width, yakult_image.height, yakult_image.channels, 0,
	VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
    for (uint32_t i = 0; i < VKAL_MAX_TEXTURES; ++i) {
	vkal_update_descriptor_set_texturearray(
	    descriptor_sets[0], 
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
	    i, /* texture-id (index into array) */
	    yakult_texture);
    }

    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	layouts, descriptor_set_layout_count, 
	NULL, 0);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout);

    /* Model Data */
    float cube_vertices[] = {
	// Pos            // Color        // UV
	-1.0,  1.0, 1.0,  1.0, 0.0, 0.0,  0.0, 0.0,
	 1.0,  1.0, 1.0,  0.0, 1.0, 0.0,  1.0, 0.0,
	-1.0, -1.0, 1.0,  0.0, 0.0, 1.0,  0.0, 1.0,
    	 1.0, -1.0, 1.0,  1.0, 1.0, 0.0,  1.0, 1.0
    };
    uint32_t vertex_count = sizeof(cube_vertices)/sizeof(*cube_vertices);
    
    uint16_t cube_indices[] = {
	// front
 	0, 1, 2,
	2, 1, 3
    };
    uint32_t index_count = sizeof(cube_indices)/sizeof(*cube_indices);
  
    uint32_t offset_vertices = vkal_vertex_buffer_add(cube_vertices, 2*sizeof(vec3) + sizeof(vec2), 4);
    uint32_t offset_indices  = vkal_index_buffer_add(cube_indices, index_count);

    /* Texture Data */
    Image image2 = load_image_file("../src/examples/assets/textures/mario.jpg");
    Texture mario_texture = vkal_create_texture(0, image2.data, image2.width, image2.height, image2.channels, 0,
					   VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
    Image image = load_image_file("../src/examples/assets/textures/indy1.jpg");
    Texture indy_texture = vkal_create_texture(0, image.data, image.width, image.height, image.channels, 0,
					  VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
    free(image.data);
    free(image2.data);
    
    vkal_update_descriptor_set_texturearray(
	descriptor_sets[0], 
	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
	0, /* texture-id (index into array) */
	indy_texture);
    vkal_update_descriptor_set_texturearray(
	descriptor_sets[0], 
	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
	1, /* texture-id (index into array) */
	mario_texture);

    /* Material Uniform using Dynamic Descriptors */
    UniformBuffer material_ubo = vkal_create_uniform_buffer(sizeof(uint32_t), 2, 1);
    /* NOTE: Wasting a bit of memory here. The alignment will be 64bytes (probably) but we are filling
       those two 64 byte-chunks with only 4 bytes each (for uint32_t).
    */
    material_data.index = (uint32_t*)malloc(2*sizeof(material_ubo.alignment));
    *material_data.index = 0;
    *(uint32_t*)((uint8_t*)material_data.index + material_ubo.alignment) = 1;
    vkal_update_descriptor_set_uniform(descriptor_sets[0], material_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vkal_update_uniform(&material_ubo, material_data.index);

    UniformBuffer dummy_ubo = vkal_create_uniform_buffer(sizeof(uint64_t), 1, 2);
    UniformBuffer dummy_ubo_large = vkal_create_uniform_buffer(100, 1, 3);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], dummy_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], dummy_ubo_large, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    uint32_t dummy_data[2] = { 998, 999 };
    vkal_update_uniform(&dummy_ubo, dummy_data);
    uint32_t dummy_data_large[4] = { 42, 1811, 666, 3008 };
    vkal_update_uniform(&dummy_ubo_large, dummy_data_large);
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();
	
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	{
	    uint32_t image_id = vkal_get_image();

	    vkal_begin_command_buffer(image_id);
	    vkal_begin_render_pass(image_id, vkal_info->render_pass);
	    vkal_viewport(vkal_info->command_buffers[image_id],
			  0, 0,
			  width, height);
	    vkal_scissor(vkal_info->command_buffers[image_id],
			 0, 0,
			 width, height);
	    vkal_bind_descriptor_set_dynamic(image_id, &descriptor_sets[0], pipeline_layout, 0);
	    vkal_draw_indexed(image_id, graphics_pipeline,
			      offset_indices, index_count,
			      offset_vertices);
	    vkal_bind_descriptor_set_dynamic(image_id, &descriptor_sets[0], pipeline_layout, material_ubo.alignment);
	    vkal_draw_indexed(image_id, graphics_pipeline,
			      offset_indices, index_count,
			      offset_vertices);
	    vkal_end_renderpass(image_id);
	    
	    vkal_end_command_buffer(image_id);
	    VkCommandBuffer command_buffers1[] = { vkal_info->command_buffers[image_id] };
	    vkal_queue_submit(command_buffers1, 1);

	    vkal_present(image_id);
	}
    }
    
    vkal_cleanup();

    glfwDestroyWindow(window);
 
    glfwTerminate();
    
    return 0;
}
