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

#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

#define MAX_MAP_TEXTURES 1024
#define MAX_MAP_VERTS    65536 
#define	MAX_MAP_FACES	 65536

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

typedef enum MapFaceType
{
    REGULAR,
    SKY
} MapFaceType;

typedef struct MapFace
{
    MapFaceType type;
    uint32_t vertex_buffer_offset;
    uint32_t vertex_count;
    uint32_t texture_id;
} MapFace;

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

typedef struct VertexBuffer
{
    DeviceMemory memory;
    Buffer       buffer;
} VertexBuffer;

void camera_dolly(Camera * camera, vec3 translate);
void camera_yaw(Camera * camera, float angle);
void camera_pitch(Camera * camera, float angle);

static GLFWwindow * g_window;
static Platform p;
static Camera camera;
static int g_keys[MAX_KEYS];
static MapTexture g_map_textures[MAX_MAP_TEXTURES];
static uint32_t g_map_texture_count;
static MapTexture g_sky_textures[MAX_MAP_TEXTURES];
static uint32_t g_sky_texture_count;
static MapFace  g_map_faces[MAX_MAP_FACES];
static uint32_t g_map_face_count;

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
    g_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Michi's Quake 2 BSP Vulkan Renderer ;)", 0, 0);
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

uint32_t register_texture(VkDescriptorSet descriptor_set, char * texture_name, MapFaceType type)
{
    uint32_t i = 0;
    for ( ; i <= g_map_texture_count; ++i) {
	if ( !strcmp(texture_name, g_map_textures[ i ].name) ) {
	    break;
	}
    }
    if (i > g_map_texture_count) {
	i = g_map_texture_count;
	Image image = load_image_file_from_dir("textures", texture_name);
	/* NOTE: Since the texture coordinates are created in worldspace, mapping the vertex onto a uvw plane
	   we need to make sure the coordinate wraps around the texture (MODE_REPEAT) as the coordinates
	   will be (most of the time) outside the range [0,1]!
	*/
	if (type == REGULAR) {
	    Texture texture = vkal_create_texture(1, image.data, image.width, image.height, 4, 0,
						  VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
						  0, 1, 0, 1, VK_FILTER_NEAREST, VK_FILTER_NEAREST,
						  VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	    vkal_update_descriptor_set_texturearray(
		descriptor_set, 
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
		i,
		texture);
	    strcpy( g_map_textures[ g_map_texture_count ].name, texture_name );
	    g_map_textures[ g_map_texture_count ].texture = texture;
	    g_map_texture_count++;
	}
	else if (type == SKY) { /* Cubemap Texture */
	    Texture texture = vkal_create_texture(
		2,
		image.data,
		image.width, image.height, 4,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_IMAGE_VIEW_TYPE_CUBE,
		VK_FORMAT_R8G8B8A8_UNORM,
		0, 1,
		0, 6,
		VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	    vkal_update_descriptor_set_texture(descriptor_set, texture);
	    strcpy( g_sky_textures[ g_sky_texture_count ].name, texture_name );
	    g_sky_textures[ g_sky_texture_count ].texture = texture;
	    g_sky_texture_count++;
	}
    }

    return i;
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

void create_transient_vertex_buffer(VertexBuffer * vertex_buf)
{
    vertex_buf->memory = vkal_allocate_devicememory(MAX_MAP_VERTS*sizeof(Vertex),
						    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
						    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
						    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |			 
						    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vertex_buf->buffer = vkal_create_buffer(MAX_MAP_VERTS*sizeof(Vertex),
					    &vertex_buf->memory,
					    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkal_dbg_buffer_name(vertex_buf->buffer, "Vertex Buffer");
    map_memory(&vertex_buf->buffer, MAX_MAP_VERTS*sizeof(Vertex), 0);
}

void update_transient_vertex_buffer(VertexBuffer * vertex_buf, uint32_t offset, Vertex * vertices, uint32_t vertex_count)
{
    memcpy( ((Vertex*)(vertex_buf->buffer.mapped)) + offset, vertices, vertex_count*sizeof(Vertex) );      
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

    VkDescriptorSetLayoutBinding set_layout_sky[] = {
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
    VkDescriptorSetLayout descriptor_set_layout_sky = vkal_create_descriptor_set_layout(set_layout_sky, 2);

    VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout,
	descriptor_set_layout_sky
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);
    VkDescriptorSet * descriptor_set = (VkDescriptorSet*)malloc(descriptor_set_layout_count*sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_set);
    
    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	layouts, 1, 
	NULL, 0);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout);

    VkPipelineLayout pipeline_layout_sky = vkal_create_pipeline_layout(
	&layouts[1], 1, 
	NULL, 0);
    VkPipeline graphics_pipeline_sky = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout_sky);
    
    /* Load Quake 2 BSP map */
    VertexBuffer transient_vertex_buffer;
    create_transient_vertex_buffer(&transient_vertex_buffer);
    VertexBuffer transient_vertex_buffer_sky;
    create_transient_vertex_buffer(&transient_vertex_buffer_sky);

    uint8_t * bsp_data = NULL;
    int bsp_data_size;
    p.rfb("../src/examples/assets/maps/base1.bsp", &bsp_data, &bsp_data_size);
    assert(bsp_data != NULL);
    Q2Bsp bsp = q2bsp_init(bsp_data);

    Vertex * map_vertices = (Vertex*)malloc(MAX_MAP_VERTS*sizeof(Vertex));
    uint16_t * map_indices = (uint16_t*)malloc(1024*1024*sizeof(uint16_t));
    uint32_t map_vertex_count = 0;
    uint32_t map_index_count = 0;
    
    for (uint32_t face_idx = 0; face_idx < bsp.face_count; ++face_idx) {
	BspFace face = bsp.faces[ face_idx ];
	BspTexinfo texinfo = bsp.texinfos[ face.texture_info ];
	uint32_t texture_flags = texinfo.flags;

	uint32_t texture_id;
	if ( (texture_flags & SURF_SKY) == SURF_SKY) { /* Skybox Face*/
	    g_map_faces[ g_map_face_count ].type = SKY;
//	    texture_id = register_texture(descriptor_set[0], texinfo.texture_name, SKY);
	    texture_id = register_texture(descriptor_set[0], texinfo.texture_name, REGULAR);
	}
	else { /* Ordinary Face */
	    g_map_faces[ g_map_face_count ].type = REGULAR;
	    texture_id = register_texture(descriptor_set[0], texinfo.texture_name, REGULAR);
	}

	uint32_t tex_width  = g_map_textures[ texture_id ].texture.width;
	uint32_t tex_height = g_map_textures[ texture_id ].texture.height;

	uint32_t prev_map_vertex_count = map_vertex_count;
	Q2Tri   tris = q2bsp_triangulateFace(&bsp, face);
	for (uint32_t idx = 0; idx < tris.idx_count; ++idx) {
	    uint16_t vert_index = tris.indices[ idx ];
	    vec3 pos            = bsp.vertices[ vert_index ];
	    vec3 normal         = tris.normal;
	    Vertex v = (Vertex){ 0 };
	    float x = -pos.x;
	    float y =  pos.z;
	    float z =  pos.y;
	    v.pos.x = x;
	    v.pos.y = y;
	    v.pos.z = z;
	    v.normal.x = -normal.x;
	    v.normal.y =  normal.z;
	    v.normal.z =  normal.y;
	    v.uv.x = (-x * texinfo.u_axis.x + y * texinfo.u_axis.z + z * texinfo.u_axis.y + texinfo.u_offset)/(float)(0.25*(float)tex_width);
	    v.uv.y = (-x * texinfo.v_axis.x + y * texinfo.v_axis.z + z * texinfo.v_axis.y + texinfo.v_offset)/(float)(0.25*(float)tex_height);
	    v.texture_id = texture_id;
	    map_vertices[ map_vertex_count++ ] = v;
	}

	g_map_faces[ g_map_face_count ].vertex_buffer_offset = prev_map_vertex_count;
	g_map_faces[ g_map_face_count ].vertex_count = tris.idx_count;
	g_map_face_count++;
    }



    uint32_t offset_vertices = vkal_vertex_buffer_add(map_vertices, sizeof(Vertex), map_vertex_count);
//    uint32_t offset_indices  = vkal_index_buffer_add(map_indices, map_index_count);
    
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

        /* Update the vertex buffer */
	uint32_t vertex_count = 0;
	uint32_t vertex_count_sky = 0;
	uint32_t offset = 0;
	uint32_t offset_sky = 0;
	if ( 1 ) {
	    for (uint32_t face_idx = 0; face_idx < g_map_face_count; ++face_idx) {
		MapFace * face = &g_map_faces[ face_idx ];
		uint32_t map_verts_offset = face->vertex_buffer_offset;
		if (face->type == SKY) {
		    vertex_count_sky += face->vertex_count;
		    update_transient_vertex_buffer(&transient_vertex_buffer_sky, offset_sky, map_vertices + map_verts_offset, face->vertex_count);
		}
		else {
		    vertex_count += face->vertex_count;
		    update_transient_vertex_buffer(&transient_vertex_buffer, offset, map_vertices + map_verts_offset, face->vertex_count);
		}
		offset = vertex_count;
		offset_sky = vertex_count_sky;
	    }
	}

	
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
//	    vkal_draw(image_id, graphics_pipeline, offset_vertices, map_vertex_count);
	    vkal_draw_from_buffers(transient_vertex_buffer.buffer,
				   image_id, graphics_pipeline,
				   0, vertex_count);		
//	    vkal_draw_from_buffers(transient_vertex_buffer_sky.buffer,
//				   image_id, graphics_pipeline,
//				   0, vertex_count_sky);		
	   
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
