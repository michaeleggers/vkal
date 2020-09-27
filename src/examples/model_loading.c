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
#define TRM_NDC_ZERO_TO_ONE
#include "utils/tr_math.h"
#include "utils/model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 800

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct ViewProjection
{
    mat4 view;
    mat4 proj;
} ViewProjection;

typedef struct ModelData
{
    mat4 model_mat;
} ModelData;

typedef struct ViewportData
{
    vec2 dimensions;
} ViewportData;

typedef struct Entity
{
    Model model;
    vec3 position;
    vec3 orientation;
    vec3 scale;
} Entity;

static GLFWwindow * window;
Platform p;

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
    p.rfb("../src/examples/assets/shaders/model_loading_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    p.rfb("../src/examples/assets/shaders/model_loading_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);

    /* Vertex Input Assembly */
    VkVertexInputBindingDescription vertex_input_bindings[] =
	{
	    { 0, 3*sizeof(vec3), VK_VERTEX_INPUT_RATE_VERTEX }
	};
    
    VkVertexInputAttributeDescription vertex_attributes[] =
	{
	    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },               // pos
	    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3) },    // normal
	    { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 2*sizeof(vec3) },  // color
	};
    uint32_t vertex_attribute_count = sizeof(vertex_attributes)/sizeof(*vertex_attributes);

    /* Descriptor Sets */
    VkDescriptorSetLayoutBinding set_layout[] = {
	{
	    0,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    1,
	    VK_SHADER_STAGE_VERTEX_BIT,
	    0
	},
	{
	    1,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    1,
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 2);

    VkDescriptorSetLayoutBinding set_layout_dynamic[] = {
	{
	    0,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
	    1,
	    VK_SHADER_STAGE_VERTEX_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout_dynamic = vkal_create_descriptor_set_layout(set_layout_dynamic, 1);
    
    VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout,
	descriptor_set_layout_dynamic	
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);
    VkDescriptorSet * descriptor_sets = (VkDescriptorSet*)malloc(descriptor_set_layout_count*sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_sets);

    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	layouts, descriptor_set_layout_count, 
	NULL, 0);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
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

    Model model = {0};
    float bmin[3], bmax[3];
    load_obj(bmin, bmax, "../src/examples/assets/models/lego.obj", &model);
    model.vertex_buffer_offset = vkal_vertex_buffer_add(model.vertices, 9*sizeof(float), model.vertex_count);
    clear_model(&model);

#define NUM_ENTITIES 100
    /* Entities */
    Entity entities[NUM_ENTITIES];
    for (int i = 0; i < NUM_ENTITIES; ++i) {
    	vec3 pos   = (vec3){ rand_between(-25.f, 25.f), rand_between(-15.f, 15.f), rand_between(-15.f, 15.f) };
	vec3 rot   = (vec3){ rand_between(-15.f, 15.f), rand_between(-15.f, 15.f), rand_between(-15.f, 15.f) };
	float scale_xyz = rand_between(.1f, 3.f);
	vec3 scale = (vec3){ scale_xyz, scale_xyz, scale_xyz };
	entities[i] = (Entity){ model, pos, rot, scale };
    }
    
    /* View Projection */
    mat4 view = mat4_identity();
    view = translate(view, (vec3){ 0.f, 0.f, -50.f });
    ViewProjection view_proj_data = {
	.view = view,
	.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f )
    };

    /* Uniform Buffers */
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);
    ViewportData viewport_data = { .dimensions = (vec2){ SCREEN_WIDTH, SCREEN_HEIGHT } };
    UniformBuffer viewport_ubo = vkal_create_uniform_buffer(sizeof(ViewportData), 1, 1);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], viewport_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&viewport_ubo, &viewport_data);

    /* Dynamic Uniform Buffers */
    uint32_t transformation_count = NUM_ENTITIES;
    UniformBuffer model_ubo = vkal_create_uniform_buffer(sizeof(ModelData), transformation_count, 0);
    ModelData * model_data = (ModelData*)malloc(transformation_count*model_ubo.alignment);
    for (int i = 0; i < transformation_count; ++i) {
	mat4 model_mat = mat4_identity();
	model_mat = translate(model_mat, entities[i].position);
	((ModelData*)((uint8_t*)model_data + i*model_ubo.alignment))->model_mat = model_mat;
    }
    vkal_update_descriptor_set_uniform(descriptor_sets[1], model_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vkal_update_uniform(&model_ubo, model_data);
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();
		
	/* Update View Projection Matrices */
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	view_proj_data.proj = perspective( tr_radians(45.f), (float)width/(float)height, 0.1f, 100.f );
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);

        /* Update Info about screen */
	viewport_data.dimensions = (vec2){ (float)width, (float)height };
	vkal_update_uniform(&viewport_ubo, &viewport_data);

        /* Update Model Matrices */
	for (int i = 0; i < transformation_count; ++i) {
	    mat4 model_mat = mat4_identity();
	    static float d = 1.f;
	    static float r = .0001f;
	    d += 0.00001f;
//	    entities[i].position.x += sinf(d);
	    entities[i].orientation.x += r;
	    entities[i].orientation.y += r;
	    entities[i].orientation.z += r;
	    model_mat = translate(model_mat, entities[i].position);
	    model_mat = tr_scale(model_mat, entities[i].scale);
	    mat4 rot_x = rotate_x( entities[i].orientation.x );
	    mat4 rot_y = rotate_y( entities[i].orientation.y );
	    mat4 rot_z = rotate_z( entities[i].orientation.z );
	    model_mat = mat4_x_mat4(model_mat, rot_x);
	    model_mat = mat4_x_mat4(model_mat, rot_y);
	    model_mat = mat4_x_mat4(model_mat, rot_z);
	    ((ModelData*)((uint8_t*)model_data + i*model_ubo.alignment))->model_mat = model_mat;
	}
	vkal_update_uniform(&model_ubo, model_data);	
	
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
	    for (int i = 0; i < transformation_count; ++i) {
		uint32_t dynamic_offset = i*model_ubo.alignment;
		vkal_bind_descriptor_sets(image_id, descriptor_sets, descriptor_set_layout_count,
					  &dynamic_offset, 1,
					  pipeline_layout);
		vkal_draw(image_id, graphics_pipeline, model.vertex_buffer_offset, model.vertex_count);		
	    }

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
