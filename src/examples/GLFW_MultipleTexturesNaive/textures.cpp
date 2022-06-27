/* Michael Eggers, 9/20/2020

   This example uses distinct descriptor-sets to change the texture being used in the
   shader. This means, however, that for every different texture we need to have
   another descriptor set (as a descriptor set cannot be updated while a command
   buffer is in flight). Those sets essentially are the same in that they have
   all the same layout. With many textures this results in many descriptor-sets
   just for the textures. This problem can be solved through dynamic uniform buffers
   and descriptor-arrays for the textures. This is shown in textures_descriptorarray.c.
   COOL!
   Actually, a push constant is used here to transmit the model's position where the texture
   will be mapped on. Also, the textures aspect ratio is transmitted via this push constant
   to scale it correctly in the vertex-shader.
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

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768

typedef struct ViewProjection
{
    mat4  view;
    mat4  proj;
} ViewProjection;

typedef struct ModelData
{
    vec3  position;
    float image_aspect;
} ModelData;

typedef struct Camera
{
	vec3 pos;
	vec3 center;
	vec3 up;
	vec3 right;
} Camera;

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
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vulkan", 0, 0);
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

    Image image = (Image){0};
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
   
    vkal_create_instance_glfw(
		window,
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
    read_file("../../src/examples/assets/shaders/textures_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    read_file("../../src/examples/assets/shaders/textures_frag.spv", &fragment_byte_code, &fragment_code_size);
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
	    1, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	},
	{
	    1,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    1, 
	    VK_SHADER_STAGE_VERTEX_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 2);
    
    VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);
    
    VkDescriptorSet * descriptor_set_tex_1 = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
    VkDescriptorSet * descriptor_set_tex_2 = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, 1, &descriptor_set_tex_1);
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, 1, &descriptor_set_tex_2);

    /* Push Constants */	
    VkPushConstantRange push_constant_ranges[] =
	{
	    { 
		VK_SHADER_STAGE_VERTEX_BIT,
		0, 
		sizeof(ModelData)
	    }
	};
    uint32_t push_constant_range_count = sizeof(push_constant_ranges) / sizeof(*push_constant_ranges);
    
    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	layouts, descriptor_set_layout_count, 
	push_constant_ranges, push_constant_range_count);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout);

    /* Model Data */
    float rect_vertices[] = {
	// Pos            // Color        // UV
	    -1.0,  1.0, 1.0,  1.0, 0.0, 0.0,  0.0, 0.0,
	     1.0,  1.0, 1.0,  0.0, 1.0, 0.0,  1.0, 0.0,
	    -1.0, -1.0, 1.0,  0.0, 0.0, 1.0,  0.0, 1.0,
    	 1.0, -1.0, 1.0,  1.0, 1.0, 0.0,  1.0, 1.0
    };
    uint32_t vertex_count = sizeof(rect_vertices)/sizeof(*rect_vertices) / 8;
    
    uint16_t rect_indices[] = {
	    // front
 	    0, 1, 2,
	    2, 1, 3
    };
    uint32_t index_count = sizeof(rect_indices)/sizeof(*rect_indices);
  
    uint32_t offset_vertices = vkal_vertex_buffer_add(rect_vertices, 2*sizeof(vec3) + sizeof(vec2), vertex_count);
    uint32_t offset_indices  = vkal_index_buffer_add(rect_indices, index_count);

    /* Texture Data */
    Image image = load_image_file("../../src/examples/assets/textures/vklogo.jpg");
    VkalTexture texture = vkal_create_texture(0, image.data, image.width, image.height, 4, 0,
					  VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
					  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    Image image2 = load_image_file("../../src/examples/assets/textures/hk.jpg");
    VkalTexture texture2 = vkal_create_texture(0, image2.data, image2.width, image2.height, 4, 0,
					   VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
					   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    free(image.data);
    free(image2.data);

    /* Position and aspect ratio for textures */
    ModelData model_data_tex_1;
    model_data_tex_1.position = (vec3){ -1, 0, 0 };
    model_data_tex_1.image_aspect = (float)image.width/(float)image.height;
    ModelData model_data_tex_2;
    model_data_tex_2.position = (vec3){ 1, 0, 0 };
    model_data_tex_2.image_aspect = (float)image2.width/(float)image2.height;
    
    vkal_update_descriptor_set_texture(descriptor_set_tex_1[0], texture);
    vkal_update_descriptor_set_texture(descriptor_set_tex_2[0], texture2);				       

    /* View Projection Uniform */
    Camera camera;
    camera.pos = (vec3){ 0, 0.f, 10.f };
    camera.center = (vec3){ 0 };
    camera.up = (vec3){ 0, 1, 0 };
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    view_proj_data.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f );
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(ViewProjection), 1, 1);
    vkal_update_descriptor_set_uniform(descriptor_set_tex_1[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_descriptor_set_uniform(descriptor_set_tex_2[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	view_proj_data.proj = perspective( tr_radians(45.f), (float)width/(float)height, 0.1f, 100.f );
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);

		{
			uint32_t image_id = vkal_get_image();

			vkal_begin_command_buffer(image_id);
			vkal_begin_render_pass(image_id, vkal_info->render_pass);
			vkal_viewport(vkal_info->default_command_buffers[image_id],
				  0, 0,
				  width, height);
			vkal_scissor(vkal_info->default_command_buffers[image_id],
				 0, 0,
				 width, height);
			vkal_bind_descriptor_set(image_id, &descriptor_set_tex_1[0], pipeline_layout);
			vkCmdPushConstants(vkal_info->default_command_buffers[image_id], pipeline_layout,
					   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelData), (void*)&model_data_tex_1);
			vkal_draw_indexed(image_id, graphics_pipeline,
					  offset_indices, index_count,
					  offset_vertices);
			vkal_bind_descriptor_set(image_id, &descriptor_set_tex_2[0], pipeline_layout);
			vkCmdPushConstants(vkal_info->default_command_buffers[image_id], pipeline_layout,
					   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelData), (void*)&model_data_tex_2);
			vkal_draw_indexed(image_id, graphics_pipeline,
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
 
    glfwTerminate();
    
    return 0;
}
