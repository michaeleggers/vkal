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
    uint32_t poly3_count;
} Batch;


void line(Batch * batch, float x0, float y0, float x1, float y1, float thickness, vec3 color);
void fill_circle(Batch * batch, float x, float y, float r, uint32_t spans, vec3 color);

/* Globals */
static GLFWwindow * window;
static Platform p;
static int width, height; /* current framebuffer width/height */
static Batch batch = { 0 };




// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
	printf("escape key pressed\n");
	glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	double gridx_step = 50;
	double gridy_step = 50;
	double xposi = xpos/gridx_step;
	double yposi = ypos/gridy_step;
	if ( (xposi - (int)xposi) > 0.5 ) {
	    xpos = (int)xposi * 50 + 50;
	}
	else {
	    xpos = (int)xposi * 50;
	}
	if ( (yposi - (int)yposi) > 0.5 ) {
	    ypos = (int)yposi * 50 + 50;
	}
	else {
	    ypos = (int)yposi * 50;
	}
	
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
	    printf("right mouse button clicked at: %f, %f\n", xpos, ypos);
	}
	static int click_state = 0;
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
	    printf("left mouse button clicked at: %f, %f\n", xpos, ypos);
	    click_state++;
	    static double x_old;
	    static double y_old;
	    if (click_state == 1) {
		fill_circle( &batch, xpos, ypos, 10, 32, (vec3){1, 1, 0} );
	    }
	    else if (click_state == 2) {
		fill_circle( &batch, xpos, ypos, 10, 32, (vec3){1, 1, 0} );
		line( &batch, x_old, y_old, xpos, ypos, 2, (vec3){1, 1, 1});
		click_state = 0;
	    }
	    x_old = xpos;
	    y_old = ypos;
	}
    }
}

void init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "VKAL Example: primitives.c", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;
}

void reset_batch(Batch * batch)
{
    batch->index_count = 0;
    batch->vertex_count = 0;
    batch->rect_count = 0;
    batch->poly3_count = 0;
}

void fill_rect(Batch * batch, float x, float y, float width, float height, vec3 color)
{
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;

    tl.pos = (vec3){x,y,-1};
    bl.pos = (vec3){x, y+height,-1};
    tr.pos = (vec3){x+width, y, -1};
    br.pos = (vec3){x+width, y+height, -1};
    tl.color = color;
    bl.color = color;
    tr.color = color;
    br.color = color;
    
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 3;
    batch->rect_count++;
}

void line(Batch * batch, float x0, float y0, float x1, float y1, float thickness, vec3 color)
{
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;
    
    float dx = fabs(x1 - x0);
    float dy = fabs(y1 - y0);
    /* The lines' normals are (-dy, dx), (dy, =dx)
       but we only are interested in (dy, dx) because
       it is easier to reason about when applying the
       normal components as offsets.
    */
    vec2 normal = (vec2){dy, dx};
    normal = vec2_normalize(normal);
    float offset_x = thickness*normal.x/2.0;
    float offset_y = thickness*normal.y/2.0;
    if ( (x1 > x0) && (y1 > y0) ) { /* bottom right */
	tl.pos = (vec3){x0 + offset_x, y0 - offset_y, -1};
	bl.pos = (vec3){x0 - offset_x, y0 + offset_y, -1};
	tr.pos = (vec3){x1 + offset_x, y1 - offset_y, -1};
	br.pos = (vec3){x1 - offset_x, y1 + offset_y, -1};
    }
    else if ( (x1 > x0) && (y1 < y0) ) { /* top right */
	tl.pos = (vec3){x0 - offset_x, y0 - offset_y, -1};
	bl.pos = (vec3){x0 + offset_x, y0 + offset_y, -1};
	tr.pos = (vec3){x1 - offset_x, y1 - offset_y, -1};
	br.pos = (vec3){x1 + offset_x, y1 + offset_y, -1};
    }
    else if ( (x1 < x0) && (y1 < y0) ) { /* top left */
	tl.pos = (vec3){x0 - offset_x, y0 + offset_y, -1};
	bl.pos = (vec3){x0 + offset_x, y0 - offset_y, -1};
	tr.pos = (vec3){x1 - offset_x, y1 + offset_y, -1};
	br.pos = (vec3){x1 + offset_x, y1 - offset_y, -1};
    }
    else { /* bottom left*/
	tl.pos = (vec3){x0 + offset_x, y0 + offset_y, -1};
	bl.pos = (vec3){x0 - offset_x, y0 - offset_y, -1};
	tr.pos = (vec3){x1 + offset_x, y1 + offset_y, -1};
	br.pos = (vec3){x1 - offset_x, y1 - offset_y, -1};
    }

    tl.color = color;
    bl.color = color;
    tr.color = color;
    br.color = color;
    
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 3;
    batch->rect_count++;
}

void poly3(Batch * batch, vec2 xy1, vec2 xy2, vec2 xy3, vec3 color)
{
    vec3 xyz1;
    vec3 xyz2;
    vec3 xyz3;
    Vertex v1;
    Vertex v2;
    Vertex v3;
    xyz1.x = xy1.x; xyz1.y = xy1.y; xyz1.z = -1;
    xyz2.x = xy2.x; xyz2.y = xy2.y; xyz2.z = -1;
    xyz3.x = xy3.x; xyz3.y = xy3.y; xyz3.z = -1;
    v1.pos = xyz1; v2.pos = xyz2; v3.pos = xyz3;
    v1.color = color; v2.color = color; v3.color = color;

    batch->vertices[batch->vertex_count++] = v1;
    batch->vertices[batch->vertex_count++] = v2;    
    batch->vertices[batch->vertex_count++] = v3;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;

    batch->poly3_count++;
}

void circle(Batch * batch, float x, float y, float r, uint32_t spans, float thickness, vec3 color)
{
    float theta = 2*TR_PI/(float)spans;
    for (uint32_t i = 0; i < spans; ++i) {
	vec2 xy;
	vec2 xy2;
	xy.x = x + r * cosf( (i+1)*theta );
	xy.y = y + r * sinf( (i+1)*theta );
	xy2.x = x + r * cosf( (i+2)*theta );
	xy2.y = y + r * sinf( (i+2)*theta );
	line( batch, xy.x, xy.y, xy2.x, xy2.y, thickness, color );
    }
}

void fill_circle(Batch * batch, float x, float y, float r, uint32_t spans, vec3 color)
{
    float theta = 2*TR_PI/(float)spans;
    for (uint32_t i = 0; i < spans; ++i) {
	vec2 xy1;
	vec2 xy2;
	vec2 xy3;
	xy1.x = x;
	xy1.y = y;
	xy2.x = x + r * cosf( (i+1)*theta );
	xy2.y = y + r * sinf( (i+1)*theta );
	xy3.x = x + r * cosf( (i+2)*theta );
	xy3.y = y + r * sinf( (i+2)*theta );
	poly3(batch, xy1, xy2, xy3, color);
    }
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

    /* Create Buffers for indices and vertices that can get updated every frame */
#define MAX_PRIMITIVES                 8192
#define VERTICES_PER_PRIMITIVE         (4)
#define INDICES_PER_PRIMITVE           (6)
#define PRIMITIVES_VERTEX_BUFFER_SIZE  (MAX_PRIMITIVES * 4 * sizeof(Vertex))
#define PRIMITIVES_INDEX_BUFFER_SIZE   (MAX_PRIMITIVES * 6 * sizeof(uint16_t))
    DeviceMemory index_memory = vkal_allocate_devicememory(PRIMITIVES_INDEX_BUFFER_SIZE,
							   VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
							   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
							   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |			 
							   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffer index_buffer = vkal_create_buffer(PRIMITIVES_INDEX_BUFFER_SIZE,
					     &index_memory,
					     VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkal_dbg_buffer_name(index_buffer, "Index Buffer");
    map_memory(&index_buffer, PRIMITIVES_INDEX_BUFFER_SIZE, 0);

    DeviceMemory vertex_memory = vkal_allocate_devicememory(PRIMITIVES_VERTEX_BUFFER_SIZE,
							   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
							   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
							   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |			 
							   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffer vertex_buffer = vkal_create_buffer(PRIMITIVES_VERTEX_BUFFER_SIZE,
					     &vertex_memory,
					     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkal_dbg_buffer_name(vertex_buffer, "Vertex Buffer");
    map_memory(&vertex_buffer, PRIMITIVES_VERTEX_BUFFER_SIZE, 0);
    
    batch.indices = (uint16_t*)malloc(PRIMITIVES_INDEX_BUFFER_SIZE);
    batch.vertices = (Vertex*)malloc(PRIMITIVES_VERTEX_BUFFER_SIZE);


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


    fill_rect(&batch,  0, 0, width, height, (vec3){0, 0, 0});
    for (int i = 0; i < 200; ++i) {
	line( &batch, i*50, 0, i*50, height, 1, (vec3){.4, .4, .4});
	line( &batch, 0, i*50, width, i*50, 1, (vec3){.4, .4, .4});
    }
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();

	glfwGetFramebufferSize(window, &width, &height);
	view_proj_data.proj = ortho(0, width, height, 0, 0.1f, 2.f);
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);

#if 0
	/* Draw Some Primitives and update buffers */
	reset_batch(&batch);
	for (uint32_t i = 0; i < MAX_PRIMITIVES; ++i) {
	    vec3 color = (vec3){0, 0.5, 1.0};
	    float x0 = 0.0;
	    float y0 = 0.0f;
	    float x1 = width;
	    float y1 = height;
	    float thickness = 10.0;

	    color.x = rand_between(0.0, 1.0);
	    color.y = rand_between(0.0, 1.0);
	    color.z = rand_between(0.0, 1.0);
	     x0 = rand_between(0.0, width);
	     y0 = rand_between(0.0, height);
	     x1 = rand_between(0.0, width);
	     y1 = rand_between(0.0, height);
	     thickness = rand_between(1.0, 10.0);

	    line(&batch, x0, y0, x1, y1, thickness, color);
	}
#endif

	float radius = 100.f;
	static float pulse = 0.f;
	pulse += 0.1f;
	radius += 20*sinf( pulse );
	float theta = 2*TR_PI/64;
#if 0
	reset_batch( &batch );
	fill_circle( &batch, 300, 300, 100, 16, (vec3){1, .5, 1});
	fill_rect(&batch,  500, 500, 100, 100, (vec3){1, 0, 0});
	fill_circle( &batch, 400, 300, 30, 16, (vec3){1, 0, 0});
	fill_rect(&batch,  800, 500, 100, 100, (vec3){0, 0, 1});
	fill_circle( &batch, 500, 300, 100, 16, (vec3){1, 0, 1});      
	for (int i = 0; i < 64; ++i) {	   
	    circle( &batch, 500 + radius*cosf(i*theta), 500 + radius*sinf(i*theta), radius, 32, 2, (vec3){0, 0.5, 1.0});
	}
#endif
	
	memcpy(index_buffer.mapped, batch.indices, PRIMITIVES_INDEX_BUFFER_SIZE);
	memcpy(vertex_buffer.mapped, batch.vertices, PRIMITIVES_VERTEX_BUFFER_SIZE);

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
	    vkal_draw_indexed_from_buffers(index_buffer, batch.index_count,
					   vertex_buffer,
					   image_id, graphics_pipeline);
	    vkal_end_renderpass(image_id);
	    vkal_end_command_buffer(image_id);
	    VkCommandBuffer command_buffers1[] = { vkal_info->command_buffers[image_id] };
	    vkal_queue_submit(command_buffers1, 1);

	    vkal_present(image_id);
	}
    }

    vkDeviceWaitIdle(vkal_info->device);
    vkDestroyBuffer(vkal_info->device, index_buffer.buffer, NULL);
    vkDestroyBuffer(vkal_info->device, vertex_buffer.buffer, NULL);
    vkFreeMemory(vkal_info->device, index_memory.vk_device_memory, NULL);
    vkFreeMemory(vkal_info->device, vertex_memory.vk_device_memory, NULL);
    
    vkal_cleanup();

    glfwDestroyWindow(window);
 
    glfwTerminate();
    
    return 0;
}
