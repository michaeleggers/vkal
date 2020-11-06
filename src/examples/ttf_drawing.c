/* Michael Eggers, 10/22/2020

   Loading and rasterizing TTF using stb_truetype.
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

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb/stb_truetype.h"

#define TRM_NDC_ZERO_TO_ONE
#include "utils/tr_math.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768

static GLFWwindow * window;
static Platform p;

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct TTFInfo
{
    Texture          texture;
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
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Michi's TTF rasterizer", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
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
    tl.color = (vec3){0,1,0};
    bl.color = (vec3){0,1,0};
    tr.color = (vec3){0,1,0};
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
    p.rfb("../src/examples/assets/shaders/texture_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    p.rfb("../src/examples/assets/shaders/texture_frag.spv", &fragment_byte_code, &fragment_code_size);
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
    uint32_t vertex_count = sizeof(rect_vertices)/sizeof(*rect_vertices);
    
    uint16_t rect_indices[] = {
 	0, 1, 2,
	2, 1, 3
    };
    uint32_t index_count = sizeof(rect_indices)/sizeof(*rect_indices);
  
    uint32_t offset_vertices = vkal_vertex_buffer_add(rect_vertices, 2*sizeof(vec3) + sizeof(vec2), 4);
    uint32_t offset_indices  = vkal_index_buffer_add(rect_indices, index_count);

    /* Uniform Buffer for view projection matrices */
    Camera camera;
    camera.pos = (vec3){ 0, 0.f, 5.f };
    camera.center = (vec3){ 0 };
    camera.up = (vec3){ 0, 1, 0 };
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    view_proj_data.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f );
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_set[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);

    /* TTF loading */
    uint8_t * ttf = NULL;
    int file_size = 0;
    p.rfb("../src/examples/assets/fonts/ProggyClean.ttf", &ttf, &file_size);
    
    // stb_truetype texture baking API
    stbtt_fontinfo font;
    stbtt_InitFont(&font, (uint8_t*)ttf, 0);

    stbtt_pack_context spc;
    unsigned char * pixels = (unsigned char *)malloc(sizeof(unsigned char)*1024*1024);
    if (!stbtt_PackBegin(&spc, pixels, 1024, 1024, 0, 1, 0))
        printf("failed to create packing context\n");

    //float font_size = stbtt_ScaleForMappingEmToPixels(&font, 100);
    //stbtt_PackSetOversampling(&spc, 2, 2);
    stbtt_packedchar chardata['~'-' '];
    stbtt_PackFontRange(&spc, (unsigned char*)ttf, 0, 100,
                        ' ', '~'-' ', chardata);
    stbtt_PackEnd(&spc);
    Texture font_texture = vkal_create_texture(1, pixels, 1024, 1024, 1, 0,
					       VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM,
					       0, 1, 0, 1,
					       VK_FILTER_NEAREST, VK_FILTER_NEAREST);
    free(pixels);

    TTFInfo ttf_info;
    ttf_info.texture = font_texture;
    ttf_info.size = 13.f;
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
    
    Batch batch = { 0 };
    batch.indices = (uint16_t*)malloc(PRIMITIVES_INDEX_BUFFER_SIZE);
    batch.vertices = (Vertex*)malloc(PRIMITIVES_VERTEX_BUFFER_SIZE);

    
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
	    vkal_viewport(vkal_info->command_buffers[image_id],
			  0, 0,
			  width, height);
	    vkal_scissor(vkal_info->command_buffers[image_id],
			 0, 0,
			 width, height);
	    vkal_bind_descriptor_set(image_id, &descriptor_set[0], pipeline_layout);
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