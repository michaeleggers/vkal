/* Michael Eggers, 10/22/2020
   
   Simple example showing how to draw a rect (two triangles) and mapping
   a texture on it.
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <GLFW/glfw3.h>

#include "../vkal.h"
#include "../platform.h"
#include "utils/tr_math.h"
#include "utils/q2bsp.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"
#define TRM_NDC_ZERO_TO_ONE
#include "utils/tr_math.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768


typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct ViewProjection
{
    mat4  view;
    mat4  proj;
    float image_aspect;
} ViewProjection;

typedef struct Camera
{
    vec3 pos;
    vec3 center;
    vec3 up;
    vec3 right;
    float velocity;
} Camera;

typedef struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec2 uv;
} Vertex;


void camera_dolly(Camera * camera, vec3 translate);
void camera_yaw(Camera * camera, float angle);
void camera_pitch(Camera * camera, float angle);

static GLFWwindow * g_window;
static Platform p;
static Camera camera;

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    float yaw_angle = 0.0f;
    float pitch_angle = 0.0f;
    if (action == GLFW_PRESS) {
	if (key == GLFW_KEY_ESCAPE) {
	    printf("escape key pressed\n");
	    glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
    }
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
	if (key == GLFW_KEY_W) {
	    vec3 forward = vec3_normalize( vec3_sub(camera.center, camera.pos) );
	    camera_dolly(&camera, forward);
	}
	if (key == GLFW_KEY_S) {
	    vec3 forward = vec3_normalize( vec3_sub(camera.center, camera.pos) );
	    camera_dolly(&camera, vec3_mul(-1, forward) );
	}
	if (key == GLFW_KEY_A) {
	    camera_dolly(&camera, vec3_mul(-1, camera.right) );
	}
	if (key == GLFW_KEY_D) {
	    camera_dolly(&camera, camera.right);
	}
	if (key == GLFW_KEY_UP) {
	    yaw_angle = -5.0f;
	    camera_yaw(&camera, tr_radians(yaw_angle) );
	}	
	if (key == GLFW_KEY_DOWN) {
	    yaw_angle = 5.0f;
	    camera_yaw(&camera, tr_radians(yaw_angle) );
	}
	if (key == GLFW_KEY_LEFT) {
	    pitch_angle = 5.0f;
	    camera_pitch(&camera, tr_radians(pitch_angle) );
	}	
	if (key == GLFW_KEY_RIGHT) {
	    pitch_angle = -5.0f;
	    camera_pitch(&camera, tr_radians(pitch_angle) );
	}
    }
}

void init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    g_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "VKAL Example: texture.c", 0, 0);
    glfwSetKeyCallback(g_window, glfw_key_callback);
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

void camera_dolly(Camera * camera, vec3 translate)
{
    camera->pos = vec3_add(camera->pos, vec3_mul(camera->velocity, translate) );
    camera->center = vec3_add(camera->center, vec3_mul(camera->velocity, translate));
}

void camera_yaw(Camera * camera, float angle)
{
    vec3 forward = vec3_sub(camera->center, camera->pos);
    mat4 rot = rotate(camera->right, angle);
    forward = vec4_as_vec3( mat4_x_vec4(rot, vec3_to_vec4(forward, 1.0)) );
    camera->center = vec3_add( camera->pos, forward );
//    camera->up = vec3_normalize( vec3_cross(camera->right, new_forward3) );    
}

void camera_pitch(Camera * camera, float angle)
{
    mat4 rot_y   = rotate_y(angle);
    vec3 forward = vec3_sub(camera->center, camera->pos);
    forward        = vec4_as_vec3( mat4_x_vec4( rot_y, vec3_to_vec4(forward, 1.0) ) );
    camera->right  = vec4_as_vec3( mat4_x_vec4( rot_y, vec3_to_vec4(camera->right, 1.0) ) );
    camera->center = vec3_add(camera->pos, forward);
}

int main(int argc, char ** argv)
{
    init_window();
    init_platform(&p);
    
    char * device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME
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
	"VK_LAYER_KHRONOS_validation",
	"VK_LAYER_LUNARG_monitor"
    };
    uint32_t instance_layer_count = 0;
#ifdef _DEBUG
    instance_layer_count = sizeof(instance_layers) / sizeof(*instance_layers);    
#endif
   
    vkal_create_instance(g_window,
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
    p.rfb("../src/examples/assets/shaders/q2bsp_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    p.rfb("../src/examples/assets/shaders/q2bsp_frag.spv", &fragment_byte_code, &fragment_code_size);
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
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    1,
	    VK_SHADER_STAGE_VERTEX_BIT,
	    0
	},
	{
	    1,
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	    1,
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 2);
    
    VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);
    VkDescriptorSet * descriptor_set = (VkDescriptorSet*)malloc(descriptor_set_layout_count*sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_set);
    
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

    /* Load Quake 2 BSP map */
    uint8_t * bsp_data = NULL;
    int bsp_data_size;
    p.rfb("../src/examples/assets/maps/base1.bsp", &bsp_data, &bsp_data_size);
    assert(bsp_data != NULL);
    Q2Bsp bsp = q2bsp_init(bsp_data);
    Vertex * map_vertices = (Vertex*)malloc(3*1024*1024);
    uint16_t * map_indices = (uint16_t*)malloc(1024*1024);
    uint32_t map_vertex_count = 0;
    uint32_t map_index_count = 0;

    for (uint32_t i = 0; i < bsp.face_count; ++i) {
        Q2Tri face_verts = q2bsp_triangulateFace(&bsp, bsp.faces[i]);
        for (uint32_t i = 0; i < face_verts.vert_count; ++i) {
	    Vertex vert = { 0 };
	    vert.pos.x = face_verts.verts[i].x;
	    vert.pos.y = face_verts.verts[i].z;
	    vert.pos.z = face_verts.verts[i].y;
	    vert.normal.x = face_verts.normal.x;
	    vert.normal.y = face_verts.normal.z;
	    vert.normal.z = face_verts.normal.y;
	    map_vertices[map_vertex_count++] = vert;
	}
	for (uint32_t i = 0; i < face_verts.idx_count; ++i) {
	    map_indices[map_index_count++] = face_verts.indices[i];
	}
    }
      
    uint32_t offset_vertices = vkal_vertex_buffer_add(map_vertices, 2*sizeof(vec3) + sizeof(vec2), map_vertex_count);
    uint32_t offset_indices  = vkal_index_buffer_add(map_indices, map_index_count);

    /* Uniform Buffer for view projection matrices */
    camera.right = (vec3){ 1.0, 0.0, 0.0 };
    camera.pos = (vec3){ 0, 1000.f, 1000.f };
    camera.center = (vec3){ 0 };
    camera.up = (vec3){ 0, 1, 0 };
    camera.velocity = 20.f;
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    view_proj_data.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f );
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_set[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);
    
    /* Texture Data */
    Image image = load_image_file("../src/examples/assets/textures/vklogo.jpg");
    Texture texture = vkal_create_texture(1, image.data, image.width, image.height, 4, 0,
					  VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
    free(image.data);
    vkal_update_descriptor_set_texture(descriptor_set[0], texture);
    view_proj_data.image_aspect = (float)texture.width/(float)texture.height;
    
    // Main Loop
    while (!glfwWindowShouldClose(g_window))
    {
	glfwPollEvents();

	int width, height;
	glfwGetFramebufferSize(g_window, &width, &height);
	view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
	view_proj_data.proj = perspective( tr_radians(90.f), (float)width/(float)height, 0.1f, 10000.f );
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);

	{
	    uint32_t image_id = vkal_get_image();

	    vkal_begin_command_buffer(image_id);
	    vkal_begin_render_pass(image_id, vkal_info->render_pass);
	    vkal_viewport(vkal_info->command_buffers[image_id],
			  0, 0,
			  (float)width, (float)height);
	    vkal_scissor(vkal_info->command_buffers[image_id],
			 0, 0,
			 width, height);
	    vkal_bind_descriptor_set(image_id, &descriptor_set[0], pipeline_layout);
//	    vkal_draw_indexed(image_id, graphics_pipeline,
//			      offset_indices, map_index_count,
//			      offset_vertices);
	    vkal_draw(image_id, graphics_pipeline, offset_vertices, map_vertex_count);
	    vkal_end_renderpass(image_id);	    
	    vkal_end_command_buffer(image_id);
	    VkCommandBuffer command_buffers1[] = { vkal_info->command_buffers[image_id] };
	    vkal_queue_submit(command_buffers1, 1);

	    vkal_present(image_id);
	}
    }
    
    vkal_cleanup();

    glfwDestroyWindow(g_window);
 
    glfwTerminate();
    
    return 0;
}
