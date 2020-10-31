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

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"
#define TRM_NDC_ZERO_TO_ONE
#include "utils/tr_math.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768

static GLFWwindow * window;
static Platform p;
static int width, height; /* current framebuffer width/height */

typedef struct ViewProjection
{
    mat4  view;
    mat4  proj;
} ViewProjection;

typedef struct Camera
{
	vec3 pos;
	vec3 center;
	vec3 up;
	vec3 right;
} Camera;

typedef struct Vertex
{
    vec3 pos;
    vec3 color;
    vec2 uv;
} Vertex;

typedef struct Batch
{
    Vertex * vertices;
    uint32_t vertex_count;
    uint16_t * indices;
    uint32_t index_count;
    uint32_t rect_count;
} Batch;

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
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "VKAL Example: primitives.c", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;
}

void fill_rect(Batch * batch, float x, float y, float width, float height)
{
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;

    tl.pos = (vec3){x,y,-1};
    bl.pos = (vec3){x, y+height,-1};
    tr.pos = (vec3){x+width, y, -1};
    br.pos = (vec3){x+width, y+height, -1};
    tl.color = (vec3){1,0,0};
    bl.color = (vec3){0,1,0};
    tr.color = (vec3){0,0,1};
    br.color = (vec3){1,1,0};
    
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    batch->indices[batch->index_count++] = 0 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 2 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 1 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 1 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 2 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 3 + 4*batch->rect_count;
    batch->rect_count++;
}

void line(Batch * batch, float x0, float y0, float x1, float y1)
{
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;

    float pixel_width = 30.f;//10.f/(float)width;
    float pixel_height = 30.f;//10.f/(float)height;
    float half_thick = pixel_width/2.0;
    float dx = fabs(x1 - x0);
    float dy = fabs(y1 - y0);
    float slope = (dx > 0.0) && (dy > 0.0) ? (dy/dx) : 1.f;
    float offset = 1.0 - (1.0/slope);
    offset = offset < 0.0 ? 0.0 : offset;
    vec2 normal = (vec2){-dy, dx};
    normal = vec2_normalize(normal);
    
    if ( (x1 > x0) && (y1 > y0) ) { /* bottom right */
	tl.pos = (vec3){x0, y0, -1};
	bl.pos = (vec3){x0-offset, y0+pixel_height, -1};
	tr.pos = (vec3){x1, y1, -1};
	br.pos = (vec3){x1-offset, y1+pixel_height, -1};
    }
    else if ( (x1 > x0) && (y1 < y0) ) { /* top right */
	tl.pos = (vec3){x0, y0, -1};
	bl.pos = (vec3){x0+offset, y0+pixel_height, -1};
	tr.pos = (vec3){x1, y1, -1};
	br.pos = (vec3){x1+offset, y1+pixel_height, -1};
    }
    else if ( (x1 < x0) && (y1 < y0) ) { /* top left */
	tl.pos = (vec3){x0, y0, -1};
	bl.pos = (vec3){x0+offset, y0-pixel_height, -1};
	tr.pos = (vec3){x1, y1, -1};
	br.pos = (vec3){x1+offset, y1-pixel_height, -1};
    }
    else { /* bottom left*/
	tl.pos = (vec3){x0, y0, -1};
	bl.pos = (vec3){x0-offset, y0-pixel_height, -1};
	tr.pos = (vec3){x1, y1, -1};
	br.pos = (vec3){x1-offset, y1-pixel_height, -1};
    }
    
    tl.color = (vec3){1,0,0};
    bl.color = (vec3){1,0,0};
    tr.color = (vec3){1,0,0};
    br.color = (vec3){1,0,0};
    
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    batch->indices[batch->index_count++] = 0 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 2 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 1 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 1 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 2 + 4*batch->rect_count;
    batch->indices[batch->index_count++] = 3 + 4*batch->rect_count;
    batch->rect_count++;
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
    p.rfb("../src/examples/assets/shaders/primitives_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    p.rfb("../src/examples/assets/shaders/primitives_frag.spv", &fragment_byte_code, &fragment_code_size);
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
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 1);
    
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
    /* Vertices are defined CCW, but we flip y-axis in vertex-shader, so the
       winding of the polys appear CW after vertex-shader has run. That is
       why this graphics pipeline defines front-faces as CW, not CCW.

       Maybe, for primitive rendering we do not want to cull polys at all,
       but not sure yet...

       -> Update: Changed to CULL_MODE_NONE because that way line-drawing
       becomes easier. If drawn from eg bottom right to top left the
       winding is changed -> line would be culled!
    */
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE, 
	vkal_info->render_pass, pipeline_layout);

    /* Model Data */
    float rect_vertices[] = {
	// Pos                              // Color        // UV
	100,  100.f, -1.0,                  1.0, 0.0, 0.0,  0.0, 0.0,
	100,  SCREEN_HEIGHT, -1.0,          0.0, 1.0, 0.0,  1.0, 0.0,
	SCREEN_WIDTH, 100.f, -1.0,          0.0, 0.0, 1.0,  0.0, 1.0,
	SCREEN_WIDTH, SCREEN_HEIGHT, -1.0,  1.0, 1.0, 0.0,  1.0, 1.0
    };
    uint32_t vertex_count = sizeof(rect_vertices)/sizeof(*rect_vertices);
    
    uint16_t rect_indices[] = {
 	0, 2, 1,
	1, 2, 3
    };
    uint32_t index_count = sizeof(rect_indices)/sizeof(*rect_indices);

    Batch batch = { 0 };
    batch.vertices = (Vertex*)malloc(40000 * sizeof(Vertex));
    batch.indices = (uint16_t*)malloc(60000 * sizeof(uint16_t));
    float theta = 0.0f;
    float steps = 2*TR_PI/64.f;
    for (uint32_t i = 0; i < 1; ++i) {
//	line(&batch, rand_between(0, width), rand_between(0, height),
//	     rand_between(0, width), rand_between(0, height));
//	line(&batch, 500, 500, 500+400*cosf(theta), 500+400*sinf(theta));
	line(&batch, 500, 500, 600, 700);
	theta += steps;
//	fill_rect(&batch, rand_between(0, SCREEN_WIDTH), rand_between(0, SCREEN_HEIGHT),
//		  rand_between(100, 200), rand_between(100, 200));
    }
    uint32_t offset_vertices = vkal_vertex_buffer_add(batch.vertices, 2*sizeof(vec3) + sizeof(vec2),
						      batch.vertex_count);
    uint32_t offset_indices  = vkal_index_buffer_add(batch.indices, batch.index_count);
/*
    uint32_t offset_vertices = vkal_vertex_buffer_add(rect_vertices, 2*sizeof(vec3) + sizeof(vec2), 4);
    uint32_t offset_indices  = vkal_index_buffer_add(rect_indices, index_count);
*/
    /* Uniform Buffer for view projection matrices */
    Camera camera;
    camera.pos = (vec3){ 0, 0.f, 0.f };
    camera.center = (vec3){ 0 };
    camera.up = (vec3){ 0, 1, 0 };
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    view_proj_data.proj = ortho(0.f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.f, -1.f, 2.f);
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_set[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);    
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();

	glfwGetFramebufferSize(window, &width, &height);
	view_proj_data.proj = ortho(0, width, height, 0, 0.1f, 2.f);
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);

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
	    vkal_bind_descriptor_set(image_id, &descriptor_set[0], pipeline_layout);
	    vkal_draw_indexed(image_id, graphics_pipeline,
			      offset_indices, batch.index_count,
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
