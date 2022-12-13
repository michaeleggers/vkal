/* Michael Eggers, 10/22/2020

   Loading and rasterizing TTF using stb_truetype.
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

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define TRM_NDC_ZERO_TO_ONE
#include "tr_math.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768

static GLFWwindow * window;

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct TTFInfo
{
    VkalTexture          texture;
    float            size;
    uint32_t         num_chars_in_range;
    uint32_t         first_char;
    stbtt_packedchar chardata['~'-' '];
} TTFInfo;

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
} Camera;

static char g_textbuffer[1024];		  
static int g_textbuffer_pos;

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if ( (action == GLFW_REPEAT) || (action == GLFW_PRESS) ) {
	if (key == GLFW_KEY_ESCAPE) {
	    printf("escape key pressed\n");
	    glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_A ) {
	    g_textbuffer[g_textbuffer_pos++] = 'a';
	}
	else if (key == GLFW_KEY_B ) {
	    g_textbuffer[g_textbuffer_pos++] = 'b';
	}
	else if (key == GLFW_KEY_C ) {
	    g_textbuffer[g_textbuffer_pos++] = 'c';
	}
	else if (key == GLFW_KEY_D ) {
	    g_textbuffer[g_textbuffer_pos++] = 'd';
	}
	else if (key == GLFW_KEY_E ) {
	    g_textbuffer[g_textbuffer_pos++] = 'e';
	}
	else if (key == GLFW_KEY_F ) {
	    g_textbuffer[g_textbuffer_pos++] = 'f';
	}
	else if (key == GLFW_KEY_G ) {
	    g_textbuffer[g_textbuffer_pos++] = 'g';
	}
	else if (key == GLFW_KEY_H ) {
	    g_textbuffer[g_textbuffer_pos++] = 'h';
	}
	else if (key == GLFW_KEY_I ) {
	    g_textbuffer[g_textbuffer_pos++] = 'i';
	}
	else if (key == GLFW_KEY_J ) {
	    g_textbuffer[g_textbuffer_pos++] = 'j';
	}
	else if (key == GLFW_KEY_K ) {
	    g_textbuffer[g_textbuffer_pos++] = 'k';
	}
	else if (key == GLFW_KEY_L ) {
	    g_textbuffer[g_textbuffer_pos++] = 'l';
	}
	else if (key == GLFW_KEY_M ) {
	    g_textbuffer[g_textbuffer_pos++] = 'm';
	}
	else if (key == GLFW_KEY_N ) {
	    g_textbuffer[g_textbuffer_pos++] = 'n';
	}
	else if (key == GLFW_KEY_O ) {
	    g_textbuffer[g_textbuffer_pos++] = 'o';
	}
	else if (key == GLFW_KEY_P ) {
	    g_textbuffer[g_textbuffer_pos++] = 'p';
	}
	else if (key == GLFW_KEY_Q ) {
	    g_textbuffer[g_textbuffer_pos++] = 'q';
	}
	else if (key == GLFW_KEY_R ) {
	    g_textbuffer[g_textbuffer_pos++] = 'r';
	}
	else if (key == GLFW_KEY_S ) {
	    g_textbuffer[g_textbuffer_pos++] = 's';
	}
	else if (key == GLFW_KEY_T ) {
	    g_textbuffer[g_textbuffer_pos++] = 't';
	}
	else if (key == GLFW_KEY_U ) {
	    g_textbuffer[g_textbuffer_pos++] = 'u';
	}
	else if (key == GLFW_KEY_V ) {
	    g_textbuffer[g_textbuffer_pos++] = 'v';
	}
	else if (key == GLFW_KEY_W ) {
	    g_textbuffer[g_textbuffer_pos++] = 'w';
	}
	else if (key == GLFW_KEY_X ) {
	    g_textbuffer[g_textbuffer_pos++] = 'x';
	}
	else if (key == GLFW_KEY_Y ) {
	    g_textbuffer[g_textbuffer_pos++] = 'y';
	}
	else if (key == GLFW_KEY_Z ) {
	    g_textbuffer[g_textbuffer_pos++] = 'z';
	}
	else if (key == GLFW_KEY_SPACE ) {
	    g_textbuffer[g_textbuffer_pos++] = ' ';
	}
	else if (key == GLFW_KEY_PERIOD ) {
	    g_textbuffer[g_textbuffer_pos++] = '.';
	}
	else if (key == GLFW_KEY_BACKSPACE) {
	    g_textbuffer[g_textbuffer_pos] = ' ';
	    if (g_textbuffer_pos > 0) g_textbuffer_pos--;
	}
    }
    g_textbuffer[g_textbuffer_pos] = '_';
}

void init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Michi's TTF rasterizer", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
}

Image load_image_file(char const * file)
{
    Image image = { };
    int tw, th, tn;
    image.data = stbi_load(file, &tw, &th, &tn, 4);
    assert(image.data != NULL);
    image.width = tw;
    image.height = th;
    image.channels = tn;

    return image;
}

void fill_rect(Batch * batch, float x, float y, float width, float height,
	       vec2 tl_uv, vec2 bl_uv, vec2 tr_uv, vec2 br_uv)
{
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;

    tl.pos = {x,y,-1};
    bl.pos = {x, y+height,-1};
    tr.pos = {x+width, y, -1};
    br.pos = {x+width, y+height, -1};
    tl.color = {0,1,0};
    bl.color = {0,1,0};
    tr.color = {0,1,0};
    br.color = {1,1,0};
    tl.uv = tl_uv;
    bl.uv = bl_uv;
    tr.uv = tr_uv;
    br.uv = br_uv;

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

void draw_text(Batch * batch, TTFInfo info, char * text)
{
    stbtt_aligned_quad quad = { 0 };
    char * current_char = text;
    float x_offset = 0;
    float y_offset = info.size;
    int i = 0;
    while (*current_char != '\0') {
	int chardata_index = (int)*current_char;
	stbtt_GetPackedQuad(info.chardata, 
                            info.texture.width, info.texture.height,  
                            *current_char - info.first_char,             // character to display
                            &x_offset, &y_offset,
                            // pointers to current position in screen pixel space
                            &quad,      // output: quad to draw
                            1);

	stbtt_packedchar chardata  = info.chardata[chardata_index];
	vec2 tl_uv;
	vec2 bl_uv;
	vec2 tr_uv;
	vec2 br_uv;
	tl_uv.x = quad.s0;
	tl_uv.y = quad.t0;
	bl_uv.x = quad.s0;
	bl_uv.y = quad.t1;
	tr_uv.x = quad.s1;
	tr_uv.y = quad.t0;
	br_uv.x = quad.s1;
	br_uv.y = quad.t1;

	float x = quad.x0;
	float y = quad.y0;
	float w = quad.x1 - quad.x0;
	float h = quad.y1 - quad.y0;
	
	fill_rect(batch,
		  x, y,
		  w, h,
		  tl_uv, bl_uv, tr_uv, br_uv);

	current_char++; i++;
    }
}

void reset_batch(Batch * batch)
{
    batch->index_count = 0;
    batch->vertex_count = 0;
    batch->rect_count = 0;
}

int main(int argc, char ** argv)
{
    init_window();
    
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
    read_file("../../dependencies/vkal/src/examples/assets/shaders/ttf_drawing_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    read_file("../../dependencies/vkal/src/examples/assets/shaders/ttf_drawing_frag.spv", &fragment_byte_code, &fragment_code_size);
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
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_set);
    
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

    /* Uniform Buffer for view projection matrices */
    Camera camera;
    camera.pos = { 0.0f, 0.0f, 0.0f };
    camera.center = { 0 };
    camera.up = { 0, 1, 0 };
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_set[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);

    /* TTF loading */
    uint8_t * ttf = NULL;
    int file_size = 0;
    read_file("../../dependencies/vkal/src/examples/assets/fonts/efmi.ttf", &ttf, &file_size);
    
    // stb_truetype texture baking API
    stbtt_fontinfo font;
    stbtt_InitFont(&font, (uint8_t*)ttf, 0);

    stbtt_pack_context spc;
    unsigned char * pixels = (unsigned char *)malloc(sizeof(unsigned char)*1024*1024);
    if (!stbtt_PackBegin(&spc, pixels, 1024, 1024, 0, 1, 0))
        printf("failed to create packing context\n");

    //float font_size = stbtt_ScaleForMappingEmToPixels(&font, 100);
    //float scale = stbtt_ScaleForPixelHeight(&font, 100);
    //stbtt_PackSetOversampling(&spc, 4, 4);
    stbtt_packedchar chardata['~'-' '];
    stbtt_PackFontRange(&spc, (unsigned char*)ttf, 0, 72,
                        ' ', '~'-' ', chardata);
    stbtt_PackEnd(&spc);
    VkalTexture font_texture = vkal_create_texture(1, pixels, 1024, 1024, 1, 0,
					       VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM,
					       0, 1, 0, 1,
					       VK_FILTER_NEAREST, VK_FILTER_NEAREST,
					       VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
 
    unsigned char * output = (unsigned char *)malloc(1024*1024*sizeof(unsigned char));
    memcpy(output, pixels, 1024*1024*sizeof(unsigned char));
    stbi_write_png("font_texture.png", 1024, 1024, 1, output, 0);
    free(pixels);
    free(output);
    
    TTFInfo ttf_info;
    ttf_info.texture = font_texture;
    ttf_info.size = 72;
    ttf_info.num_chars_in_range = '~' - ' ';
    ttf_info.first_char = ' ';
    memcpy( ttf_info.chardata, chardata, ('~' - ' ')*sizeof(stbtt_packedchar) );
    
    vkal_update_descriptor_set_texture(descriptor_set[0], font_texture);
    view_proj_data.image_aspect = (float)font_texture.width/(float)font_texture.height;

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
    VKAL_DBG_BUFFER_NAME(vkal_info->device, index_buffer, "Index Buffer");
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
	VKAL_DBG_BUFFER_NAME(vkal_info->device, vertex_buffer, "Vertex Buffer");
    map_memory(&vertex_buffer, PRIMITIVES_VERTEX_BUFFER_SIZE, 0);
    
    Batch batch = { 0 };
    batch.indices = (uint16_t*)malloc(PRIMITIVES_INDEX_BUFFER_SIZE);
    batch.vertices = (Vertex*)malloc(PRIMITIVES_VERTEX_BUFFER_SIZE);

    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	view_proj_data.proj = ortho(0, width, height, 0, 0.1f, 2.f);
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);

	reset_batch(&batch);
	draw_text(&batch, ttf_info, g_textbuffer);
	memcpy(index_buffer.mapped, batch.indices, PRIMITIVES_INDEX_BUFFER_SIZE);
	memcpy(vertex_buffer.mapped, batch.vertices, PRIMITIVES_VERTEX_BUFFER_SIZE);

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
	    vkal_bind_descriptor_set(image_id, &descriptor_set[0], pipeline_layout);
	    vkal_draw_indexed_from_buffers(index_buffer, 0, batch.index_count,
					   vertex_buffer, 0,
					   image_id, graphics_pipeline);
	    vkal_end_renderpass(image_id);
	    vkal_end_command_buffer(image_id);
	    VkCommandBuffer command_buffers1[] = { vkal_info->default_command_buffers[image_id] };
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
 
    return 0;
}
