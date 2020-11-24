/* Michael Eggers, 10/22/2020
   
   Simple example showing how to draw a rect (two triangles) and mapping
   a texture on it.
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <GLFW/glfw3.h>
#include <physfs.h>

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

#define MAX_MAP_TEXTURES 1024

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
    vec3     pos;
    vec3     normal;
    vec2     uv;
    uint32_t texture_id;
} Vertex;

typedef struct MapTexture
{
    Texture texture;
    char    name[32];
} MapTexture;

typedef enum KeyCmd
{
    W,
    A,
    S,
    D,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    MAX_KEYS
} KeyCmd;

void camera_dolly(Camera * camera, vec3 translate);
void camera_yaw(Camera * camera, float angle);
void camera_pitch(Camera * camera, float angle);

static GLFWwindow * g_window;
static Platform p;
static Camera camera;
static int g_keys[MAX_KEYS];
static MapTexture g_map_textures[MAX_MAP_TEXTURES];
static uint32_t g_map_texture_count;

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
	if (key == GLFW_KEY_ESCAPE) {
	    printf("escape key pressed\n");
	    glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
    }
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
	if (key == GLFW_KEY_W) {
	    g_keys[W] = 1;
	}
	if (key == GLFW_KEY_S) {
	    g_keys[S] = 1;
	}
	if (key == GLFW_KEY_A) {
	    g_keys[A] = 1;
	}
	if (key == GLFW_KEY_D) {
	    g_keys[D] = 1;
	}
	
	if (key == GLFW_KEY_UP) {
	    g_keys[UP] = 1;
	}	
	if (key == GLFW_KEY_DOWN) {
	    g_keys[DOWN] = 1;
	}
	if (key == GLFW_KEY_LEFT) {
	    g_keys[LEFT] = 1;
	}	
	if (key == GLFW_KEY_RIGHT) {
	    g_keys[RIGHT] = 1;
	}
    }
    else if (action == GLFW_RELEASE) {
	if (key == GLFW_KEY_W) {
	    g_keys[W] = 0;
	}
	if (key == GLFW_KEY_S) {
	    g_keys[S] = 0;
	}
	if (key == GLFW_KEY_A) {
	    g_keys[A] = 0;
	}
	if (key == GLFW_KEY_D) {
	    g_keys[D] = 0;
	}

	if (key == GLFW_KEY_UP) {
	    g_keys[UP] = 0;
	}	
	if (key == GLFW_KEY_DOWN) {
	    g_keys[DOWN] = 0;
	}
	if (key == GLFW_KEY_LEFT) {
	    g_keys[LEFT] = 0;
	}	
	if (key == GLFW_KEY_RIGHT) {
	    g_keys[RIGHT] = 0;
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

int string_length(char * string)
{
    int len = 0;
    char * c = string;
    while (*c != '\0') { c++; len++; }
    return len;
}

Image load_image_file_from_dir(char * dir, char * file)
{
    Image image = (Image){ 0 };

    /* Build search path within archive */
    char searchpath[64] = { '\0' };
    int dir_len = string_length(dir);
    strcpy(searchpath, dir);
    searchpath[dir_len++] = '/';
    int file_name_len = string_length(file);
    strcpy(searchpath + dir_len, file);
    strcpy(searchpath + dir_len + file_name_len, ".tga");

    /* Use PhysFS to load file data */
    PHYSFS_File * phys_file = PHYSFS_openRead(searchpath);
    if ( !phys_file ) {
	printf("PHYSFS_openRead() failed!\n  reason: %s.\n", PHYSFS_getLastError());
    }    
    uint64_t file_length = PHYSFS_fileLength(phys_file);
    void * buffer = malloc(file_length);
    PHYSFS_readBytes(phys_file, buffer, file_length);

    /* Finally, load the image from the memory buffer */
    int x, y, n;
    image.data = stbi_load_from_memory(buffer, (int)file_length, &x, &y, &n, 4);    
    assert(image.data != NULL);
    image.width    = x;
    image.height   = y;
    image.channels = n;

    free(buffer);
    
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

void init_physfs(char const * argv0)
{
    if (!PHYSFS_init(argv0)) {
        printf("PHYSFS_init() failed!\n  reason: %s.\n", PHYSFS_getLastError());
        exit(-1);
    }
    
    if ( !PHYSFS_mount("q2_textures.zip", "/", 0) ) {
	printf("PHYSFS_mount() failed!\n  reason: %s.\n", PHYSFS_getLastError());
    }
}

void deinit_physfs()
{
    if (!PHYSFS_deinit())
        printf("PHYSFS_deinit() failed!\n  reason: %s.\n", PHYSFS_getLastError());
}

int main(int argc, char ** argv)
{
    init_physfs(argv[0]);
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
	    { 0, 2*sizeof(vec3) + sizeof(vec2) + sizeof(uint32_t), VK_VERTEX_INPUT_RATE_VERTEX }
	};
    
    VkVertexInputAttributeDescription vertex_attributes[] =
	{
	    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },               // pos
	    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3) },    // color
	    { 2, 0, VK_FORMAT_R32G32_SFLOAT,    2*sizeof(vec3) },  // UV
	    { 3, 0, VK_FORMAT_A8B8G8R8_UINT_PACK32 ,    2*sizeof(vec3) + sizeof(vec2) },  // TEXTURE ID
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
	    108,
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

    /* Load Quake 2 BSP map */
    uint8_t * bsp_data = NULL;
    int bsp_data_size;
    p.rfb("../src/examples/assets/maps/base1.bsp", &bsp_data, &bsp_data_size);
    assert(bsp_data != NULL);
    Q2Bsp bsp = q2bsp_init(bsp_data);
    Vertex * map_vertices = (Vertex*)malloc(3*1024*1024*sizeof(Vertex));
    uint16_t * map_indices = (uint16_t*)malloc(1024*1024*sizeof(uint16_t));
    uint32_t map_vertex_count = 0;
    uint32_t map_index_count = 0;
    for (uint32_t i = 0; i < bsp.face_count; ++i) {
	int16_t texinfo_idx = bsp.faces[i].texture_info; // TODO: can it be negative??
	BspTexinfo texinfo = bsp.texinfos[ texinfo_idx ];	
	char texture_name[32];
	memcpy(texture_name, texinfo.texture_name, 32*sizeof(char));
	uint32_t texture_idx = 0;
	for ( ; texture_idx < g_map_texture_count+1; ++texture_idx) {
	    if (!strcmp(g_map_textures[ texture_idx ].name, texture_name)) { 
		goto check_against_texture_count;
	    }
	}
   check_against_texture_count:	
	if (texture_idx > g_map_texture_count) {
	    memcpy(g_map_textures[g_map_texture_count].name, texture_name, 32*sizeof(char));
	    Image image = load_image_file_from_dir("textures", texture_name);
	    g_map_textures[g_map_texture_count].texture = vkal_create_texture(1, image.data, image.width, image.height, 4, 0,
									      VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
									      0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	    vkal_update_descriptor_set_texturearray(descriptor_set[0],
						    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						    g_map_texture_count, g_map_textures[ g_map_texture_count ].texture);
	    printf("%s\n", texture_name);
	    g_map_texture_count++;	
	}
	
        Q2Tri face_verts = q2bsp_triangulateFace(&bsp, bsp.faces[i]);
        for (uint32_t i = 0; i < face_verts.vert_count; ++i) {
	    Vertex vert = { 0 };
	    float x = -face_verts.verts[i].x;
	    float y =  face_verts.verts[i].z;
	    float z =  face_verts.verts[i].y;
	    vert.pos.x = x; 
	    vert.pos.y = y;
	    vert.pos.z = z;
	    vert.normal.x = -face_verts.normal.x;
	    vert.normal.y = face_verts.normal.z;
	    vert.normal.z = face_verts.normal.y;
	    vert.uv.x = -x * texinfo.u_axis.x + y * texinfo.u_axis.y + z * texinfo.u_axis.z + texinfo.u_offset;
	    vert.uv.y = -x * texinfo.v_axis.x + y * texinfo.v_axis.y + z * texinfo.v_axis.z + texinfo.v_offset;
	    vert.texture_id = g_map_texture_count;
	    map_vertices[map_vertex_count++] = vert;
	}
	for (uint32_t i = 0; i < face_verts.idx_count; ++i) {
	    map_indices[map_index_count++] = face_verts.indices[i];
	}
    }
    printf("MAP TEXTURE-COUNT: %d\n", g_map_texture_count);
    uint32_t offset_vertices = vkal_vertex_buffer_add(map_vertices, 2*sizeof(vec3) + sizeof(vec2), map_vertex_count);
    uint32_t offset_indices  = vkal_index_buffer_add(map_indices, map_index_count);

    /* Uniform Buffer for view projection matrices */
    camera.right = (vec3){ 1.0, 0.0, 0.0 };
    camera.pos = (vec3){ 0, 1000.f, 1000.f };
    camera.center = (vec3){ 0 };
    camera.up = (vec3){ 0, 1, 0 };
    camera.velocity = 10.f;
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

	if (g_keys[W]) {
	    vec3 forward = vec3_normalize( vec3_sub(camera.center, camera.pos) );
	    camera_dolly(&camera, forward);
	}
	if (g_keys[S]) {
	    vec3 forward = vec3_normalize( vec3_sub(camera.center, camera.pos) );
	    camera_dolly(&camera, vec3_mul(-1, forward) );
	}
	if (g_keys[A]) {
	    camera_dolly(&camera, vec3_mul(-1, camera.right) );
	}
	if (g_keys[D]) {
	    camera_dolly(&camera, camera.right);
	}
	if (g_keys[UP]) {
	    camera_yaw(&camera, tr_radians(-2.0f) );	    
	}
	if (g_keys[DOWN]) {
	    camera_yaw(&camera, tr_radians(2.0f) );
	}
	if (g_keys[LEFT]) {
	    camera_pitch(&camera, tr_radians(2.0f) );
	}
	if (g_keys[RIGHT]) {
	    camera_pitch(&camera, tr_radians(-2.0f) );
	}
	
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

    deinit_physfs();
    
    return 0;
}
