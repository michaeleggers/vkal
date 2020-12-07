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
#define MAX_MAP_LEAVES   65536

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct ViewProjection
{
    mat4  view;
    mat4  proj;
    vec3  cam_pos;
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
    uint32_t surface_flags;
} Vertex;

typedef enum MapFaceType
{
    REGULAR,
    LIGHT,
    SKY,
    TRANS33,
    TRANS66,
    NODRAW
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

typedef struct m_BspLeaf
{
    uint32_t   brush_or;          // ?
	
    int16_t    cluster;           // -1 for cluster indicates no visibility information
    uint16_t   area;              // ?

    vec3_16i   bbox_min;          // bounding box minimums
    vec3_16i   bbox_max;          // bounding box maximums

    uint16_t   first_leaf_face;   // index of the first face (in the face leaf array)
    uint16_t   num_leaf_faces;    // number of consecutive faces (in the face leaf array)

    uint16_t   first_leaf_brush;  // ?
    uint16_t   num_leaf_brushes;  // ?

    int        visframe;
} m_BspLeaf;

typedef struct MapModel
{
    BspNode * nodes;
    uint32_t node_count;

    m_BspLeaf * leaves;
    uint32_t leaf_count;
} MapModel;

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
static Vertex * map_vertices;

/* Reset those every frame */
static uint32_t vertex_count = 0;
static uint32_t vertex_count_sky = 0;
static uint32_t vertex_count_lights = 0;
static uint32_t vertex_count_trans = 0;
static uint32_t offset = 0;
static uint32_t offset_sky = 0;
static uint32_t offset_lights = 0;
static uint32_t offset_trans = 0;

/* Renderer current frame */
static int r_framecount;

static VertexBuffer transient_vertex_buffer;
static VertexBuffer transient_vertex_buffer_sky;
static VertexBuffer transient_vertex_buffer_lights;
static VertexBuffer transient_vertex_buffer_sub_models;
static VertexBuffer transient_vertex_buffer_trans;

static Vertex g_skybox_verts[8] =
{
    { .pos = { -1, 1, 1 } },
    { .pos =  { 1, 1, 1 } },
    { .pos = { 1, -1, 1 } },
    { .pos = { -1, -1, 1} },
    { .pos = { -1, 1, -1 } },
    { .pos = { 1, 1, -1 } },
    { .pos = { 1, -1, -1 } },
    { .pos = { -1, -1, -1 } }
};

static uint16_t g_skybox_indices[36] =
{
    // front
    0, 1, 2,
    2, 3, 0,
    // right
    1, 5, 6,
    6, 2, 1,
    // back
    5, 4, 7,
    7, 6, 5,
    // left
    4, 0, 3,
    3, 7, 4,
    // top
    4, 5, 1,
    1, 0, 4,
    // bottom
    3, 2, 6,
    6, 7, 3    
};

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
	printf("Warning: Image File not found: %s\n", searchpath);
	getchar();
	exit(-1);
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

uint32_t register_texture(VkDescriptorSet descriptor_set, char * texture_name)
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

    return i;
}

uint32_t register_sky_texture(VkDescriptorSet descriptor_set, char * texture_name)
{
    uint32_t i = 0;
    for ( ; i <= g_sky_texture_count; ++i) {
	if ( !strcmp(texture_name, g_sky_textures[ i ].name) ) {
	    break;
	}
    }
    if (i > g_sky_texture_count) {
	i = g_sky_texture_count;
	Image image = load_image_file_from_dir("textures", texture_name);
	uint32_t image_size = image.width*image.height*4;
	unsigned char * cubemaps = (unsigned char*)malloc(image_size*6);
	unsigned char * current = cubemaps;
	for (uint32_t i = 0; i < 6; ++i) {
	    memcpy(current + i*image_size, image.data, image.width*image.height*4);	    
	}
	Texture texture = vkal_create_texture(
	    1,
	    cubemaps,
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

	free(cubemaps);
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
    float fwd_dot_up = vec3_dot( vec3_normalize(forward), (vec3){0, 1, 0} );
    if ( fabs(fwd_dot_up) > 0.9999 ) return;
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

int isVisible(uint8_t * pvs, int i)
{
		
    // i>>3 = i/8    
    // i&7  = i%8
	    
    return pvs[i>>3] & (1<<(i&7));	    
}

uint8_t * Mod_DecompressVis (uint8_t * in, Q2Bsp * bsp)
{
    static uint8_t	decompressed[MAX_MAP_LEAVES/8];
    int		c;
    uint8_t	* out;
    int		row;

    row = (bsp->vis->numclusters+7)>>3;	
    out = decompressed;

    if (!in)
    {	// no vis info, so make all visible
	while (row)
	{
	    *out++ = 0xff;
	    row--;
	}
	return decompressed;		
    }

    do
    {
	if (*in)
	{
	    *out++ = *in++;
	    continue;
	}
	
	c = in[1];
	in += 2;
	while (c)
	{
	    *out++ = 0;
	    c--;
	}
    } while (out - decompressed < row);
	
    return decompressed;
}

int point_in_leaf(Q2Bsp bsp, vec3 pos)
{
    BspNode * node = bsp.nodes;
    BspLeaf current_leaf;
    while (1) {
	BspPlane plane = bsp.planes[ node->plane ];
	vec3 plane_abc = (vec3){ -plane.normal.x, plane.normal.z, plane.normal.y };
	float front_or_back = vec3_dot( pos, plane_abc ) - plane.distance;
	if (front_or_back > 0) {
	    int32_t front_child = node->front_child;
	    if (front_child < 0) {
		current_leaf = bsp.leaves[ -(front_child + 1) ];
		break;
	    }
	    node = &bsp.nodes[ node->front_child ];
	}
	else {
	    int32_t back_child = node->back_child;
	    if (back_child < 0) {
		current_leaf = bsp.leaves[ -(back_child + 1) ];
		break;
	    }
	    node = &bsp.nodes[ node->back_child ];
	}
    }
    return current_leaf.cluster;
}


void draw_leaf(Q2Bsp bsp, BspLeaf leaf)
{
    for (uint32_t leaf_face_idx = leaf.first_leaf_face; leaf_face_idx < (leaf.first_leaf_face + leaf.num_leaf_faces); ++leaf_face_idx) {
	uint32_t face_idx = bsp.leaf_face_table[ leaf_face_idx ];
	MapFace * face = &g_map_faces[ face_idx ];
	uint32_t map_verts_offset = face->vertex_buffer_offset;
	if (face->type == SKY) {
	    vertex_count_sky += face->vertex_count;
	    update_transient_vertex_buffer(&transient_vertex_buffer_sky, offset_sky, map_vertices + map_verts_offset, face->vertex_count);
	    offset_sky = vertex_count_sky;
	}
	else if (face->type == REGULAR) {
	    vertex_count += face->vertex_count;
	    update_transient_vertex_buffer(&transient_vertex_buffer, offset, map_vertices + map_verts_offset, face->vertex_count);
	    offset = vertex_count;
	}
	else if (face->type == LIGHT) {
	    vertex_count_lights += face->vertex_count;
	    update_transient_vertex_buffer(&transient_vertex_buffer_lights, offset_lights, map_vertices + map_verts_offset,
					   face->vertex_count);
	    offset_lights = vertex_count_lights;
	}
	else if (face->type == TRANS66) {
	    vertex_count_trans += face->vertex_count;
	    update_transient_vertex_buffer(&transient_vertex_buffer_trans, offset_trans, map_vertices + map_verts_offset,
					   face->vertex_count);
	    offset_trans = vertex_count_trans;
	}
    }
}

void draw_marked_leaves(Q2Bsp bsp, BspNode * node, uint8_t * pvs)
{
    int front_child = node->front_child;
    int back_child  = node->back_child;
    
    if (front_child < 0) { // leaf
	BspLeaf leaf = bsp.leaves[ -(front_child + 1) ];
	if (isVisible(pvs, leaf.cluster)) {
	    draw_leaf(bsp, leaf);
	}
    }
    else {
	draw_marked_leaves( bsp, &bsp.nodes[ front_child ], pvs );
    }

    if (back_child < 0) { // leaf
	BspLeaf leaf = bsp.leaves[ -(back_child + 1) ];
	if (isVisible(pvs, leaf.cluster)) {
	    draw_leaf(bsp, leaf);
	}
    }
    else {
	draw_marked_leaves( bsp, &bsp.nodes[ back_child ], pvs );
    }
}

void begin_drawing(void)
{
     vertex_count = 0;
     vertex_count_sky = 0;
     vertex_count_lights = 0;
     vertex_count_trans = 0;
     offset = 0;
     offset_sky = 0;
     offset_lights = 0;
     offset_trans = 0;
}

MapModel init_mapmodel(Q2Bsp bsp)
{
    MapModel map_model = { 0 };
    map_model.nodes = (BspNode*)malloc(bsp.node_count * sizeof(BspNode));
    map_model.node_count = bsp.node_count;
    memcpy(map_model.nodes, bsp.nodes, bsp.node_count * sizeof(BspNode));
    
    map_model.leaves = (m_BspLeaf*)malloc(bsp.leaf_count * sizeof(m_BspLeaf));
    map_model.leaf_count = bsp.leaf_count;
    m_BspLeaf * m_leaf = map_model.leaves;
    BspLeaf   * d_leaf = bsp.leaves;
    for (uint32_t i = 0; i < bsp.leaf_count; ++i) {
	*( (BspLeaf*)m_leaf ) = *d_leaf;
	m_leaf->visframe = -1;
	m_leaf++; d_leaf++;
    }
    return map_model;
}

void deinit_mapmodel(MapModel map_model)
{
    assert(map_model.nodes != NULL);
    free(map_model.nodes);

    assert(map_model.leaves != NULL);
    free(map_model.leaves);
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

    p.rfb("../src/examples/assets/shaders/q2bsp_sky_vert.spv", &vertex_byte_code, &vertex_code_size);
    p.rfb("../src/examples/assets/shaders/q2bsp_sky_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup_sky = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);

    p.rfb("../src/examples/assets/shaders/q2bsp_trans_vert.spv", &vertex_byte_code, &vertex_code_size);
    p.rfb("../src/examples/assets/shaders/q2bsp_trans_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup_trans = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);
    
    /* Vertex Input Assembly */
    VkVertexInputBindingDescription vertex_input_bindings[] =
	{
	    { 0, 2*sizeof(vec3) + sizeof(vec2) + sizeof(uint32_t) + sizeof(uint32_t), VK_VERTEX_INPUT_RATE_VERTEX }
	};
    
    VkVertexInputAttributeDescription vertex_attributes[] =
	{
	    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },               // pos
	    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3) },    // color
	    { 2, 0, VK_FORMAT_R32G32_SFLOAT,    2*sizeof(vec3) },  // UV
	    { 3, 0, VK_FORMAT_A8B8G8R8_UINT_PACK32 ,    2*sizeof(vec3) + sizeof(vec2) },                     // TEXTURE ID
	    { 4, 0, VK_FORMAT_A8B8G8R8_UINT_PACK32 ,    2*sizeof(vec3) + sizeof(vec2) + sizeof(uint32_t) },  // SURFACE FLAGS
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
	    1024, /* Max Textures */
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

    /* Create dummy Texture to make sure every descriptor is initialized */

    Image dummy_img = load_image_file_from_dir("textures", "michi");
    Texture dummy_texture = vkal_create_texture(1, dummy_img.data, dummy_img.width, dummy_img.height, 4, 0,
						VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
						0, 1, 0, 1, VK_FILTER_NEAREST, VK_FILTER_NEAREST,
						VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    for (uint32_t i = 0; i < 1024; ++i) {
	vkal_update_descriptor_set_texturearray(
	    descriptor_set[0], 
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
	    i,
	    dummy_texture);

    }
    
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

    /* Pipeline for Transparent surfaces */
    VkPipelineLayout pipeline_layout_trans = vkal_create_pipeline_layout(
	layouts, 1, 
	NULL, 0);
    VkPipeline graphics_pipeline_trans = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup_trans, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout_trans);

    /* Pipeline for Skybox */
    VkPipelineLayout pipeline_layout_sky = vkal_create_pipeline_layout(
	&layouts[1], 1, 
	NULL, 0);
    VkPipeline graphics_pipeline_sky = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup_sky, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout_sky);
    
    /* Load Quake 2 BSP map */
    create_transient_vertex_buffer(&transient_vertex_buffer);
    create_transient_vertex_buffer(&transient_vertex_buffer_sky);
    create_transient_vertex_buffer(&transient_vertex_buffer_lights);
    create_transient_vertex_buffer(&transient_vertex_buffer_sub_models);
    create_transient_vertex_buffer(&transient_vertex_buffer_trans);
    
    uint8_t * bsp_data = NULL;
    int bsp_data_size;
    p.rfb("../src/examples/assets/maps/michi2.bsp", &bsp_data, &bsp_data_size);
    assert(bsp_data != NULL);
    Q2Bsp bsp = q2bsp_init(bsp_data);
    MapModel mapmodel = init_mapmodel(bsp);
    FILE * f_map_entities = fopen("entities.txt", "w");
    fprintf(f_map_entities, "%s", bsp.entities);
    fclose(f_map_entities);
    printf("BSP Cluster Count: %d\n", bsp.vis->numclusters);
    printf("BSP Brush Model Count: %d\n", bsp.sub_model_count);
    
    map_vertices = (Vertex*)malloc(MAX_MAP_VERTS*sizeof(Vertex));
    uint16_t * map_indices = (uint16_t*)malloc(1024*1024*sizeof(uint16_t));
    uint32_t map_vertex_count = 0;
    uint32_t map_index_count = 0;

    /* Precompute all faces so they are ready to be sent to GPU */
    for (uint32_t face_idx = 0; face_idx < bsp.face_count; ++face_idx) {
	BspFace face = bsp.faces[ face_idx ];
	BspTexinfo texinfo = bsp.texinfos[ face.texture_info ];
	uint32_t texture_flags = texinfo.flags;

	uint32_t texture_id;
	uint32_t tex_width, tex_height;
	if ( (texture_flags & SURF_SKY) == SURF_SKY) { /* Skybox Face*/
	    g_map_faces[ g_map_face_count ].type = SKY;
	    texture_id = register_sky_texture(descriptor_set[1], texinfo.texture_name);
	    tex_width  = g_sky_textures[ texture_id ].texture.width;
	    tex_height = g_sky_textures[ texture_id ].texture.height;
	}
	else if ( (texture_flags & SURF_LIGHT) == SURF_LIGHT) { /* Light Face*/
	    g_map_faces[ g_map_face_count ].type = LIGHT;
	    texture_id = register_texture(descriptor_set[0], texinfo.texture_name);
	    tex_width  = g_map_textures[ texture_id ].texture.width;
	    tex_height = g_map_textures[ texture_id ].texture.height;
	}
	else if ( (texture_flags & SURF_NODRAW) == SURF_NODRAW) { /* Don't draw. Probably Trigger boxes, etc. */
	    g_map_faces[ g_map_face_count ].type = NODRAW;
	    texture_id = register_texture(descriptor_set[0], texinfo.texture_name);
	    tex_width  = g_map_textures[ texture_id ].texture.width;
	    tex_height = g_map_textures[ texture_id ].texture.height;
	}
	else if ( (texture_flags & SURF_TRANS66) == SURF_TRANS66) { /* 66% transparent surface */
	    g_map_faces[ g_map_face_count ].type = TRANS66;
	    texture_id = register_texture(descriptor_set[0], texinfo.texture_name);
	    tex_width  = g_map_textures[ texture_id ].texture.width;
	    tex_height = g_map_textures[ texture_id ].texture.height;
	}
	else {
	    g_map_faces[ g_map_face_count ].type = REGULAR;
	    texture_id = register_texture(descriptor_set[0], texinfo.texture_name);
	    tex_width  = g_map_textures[ texture_id ].texture.width;
	    tex_height = g_map_textures[ texture_id ].texture.height;
	}

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
	    v.surface_flags = texture_flags;
	    map_vertices[ map_vertex_count++ ] = v;
	}

	g_map_faces[ g_map_face_count ].vertex_buffer_offset = prev_map_vertex_count;
	g_map_faces[ g_map_face_count ].vertex_count = tris.idx_count;
	g_map_face_count++;
	} // end for face_idx

    uint32_t offset_vertices = vkal_vertex_buffer_add(map_vertices, sizeof(Vertex), map_vertex_count);
//    uint32_t offset_indices  = vkal_index_buffer_add(map_indices, map_index_count);

    uint32_t offset_sky_vertices = vkal_vertex_buffer_add(g_skybox_verts, sizeof(Vertex), sizeof(g_skybox_verts)/sizeof(*g_skybox_verts));
    uint32_t offset_sky_indices  = vkal_index_buffer_add(g_skybox_indices, sizeof(g_skybox_indices)/sizeof(*g_skybox_indices));
    
    /* Uniform Buffer for view projection matrices */
    camera.pos = (vec3){ 2, 46, 42 };
    camera.center = (vec3){ 0 };
    vec3 f = vec3_normalize(vec3_sub(camera.center, camera.pos));
    camera.up = (vec3){ 0, 1, 0 };
    camera.right = vec3_normalize(vec3_cross(f, camera.up));
    camera.velocity = 10.f;
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    view_proj_data.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f );
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);

    vkal_update_descriptor_set_uniform(descriptor_set[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_descriptor_set_uniform(descriptor_set[1], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
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
	view_proj_data.view    = look_at(camera.pos, camera.center, camera.up);
	view_proj_data.proj    = perspective( tr_radians(90.f), (float)width/(float)height, 0.1f, 10000.f );
	view_proj_data.cam_pos = camera.pos;
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);



        /* Walk BSP to check in which cluster we are */
	int cluster_id = point_in_leaf(bsp, camera.pos);
//	printf("%d\n", cluster_id);

	begin_drawing();
	
	/* Update the vertex buffer */
	if (cluster_id >= 0) {
	    uint32_t c_pvs_idx = bsp.vis_offsets[ cluster_id ].pvs;
	    uint8_t * c_pvs = ((uint8_t*)(bsp.vis)) + c_pvs_idx;
	    uint8_t * pvs = Mod_DecompressVis(c_pvs, &bsp);
	    draw_marked_leaves(bsp, bsp.nodes, pvs);
	}
	else {
	    vertex_count = map_vertex_count;
	    update_transient_vertex_buffer(&transient_vertex_buffer, 0, map_vertices, vertex_count);
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
	    /* Draw Map */
	    vkal_draw_from_buffers(transient_vertex_buffer.buffer,
				   image_id, graphics_pipeline,
				   0, vertex_count);
	   
	    /* Draw Lights */
	    vkal_draw_from_buffers(transient_vertex_buffer_lights.buffer,
				   image_id, graphics_pipeline,
				   0, vertex_count_lights);

	    /* Draw Skybox */
	    vkal_bind_descriptor_set(image_id, &descriptor_set[1], pipeline_layout_sky);
	    vkal_draw_from_buffers(transient_vertex_buffer_sky.buffer,
				   image_id, graphics_pipeline_sky,
				   0, vertex_count_sky);

	    /* Draw Transparent */
	    vkal_bind_descriptor_set(image_id, &descriptor_set[0], pipeline_layout);
	    vkal_draw_from_buffers(transient_vertex_buffer_trans.buffer,
				   image_id, graphics_pipeline,
				   0, vertex_count_trans);

//	    vkal_draw_indexed_from_buffers(
//		vkal_info->index_buffer, offset_sky_indices, 36, transient_vertex_buffer_sky.buffer, 0,
//		image_id, graphics_pipeline_sky);

	   
	    vkal_end_renderpass(image_id);	    
	    vkal_end_command_buffer(image_id);
	    VkCommandBuffer command_buffers1[] = { vkal_info->command_buffers[image_id] };
	    vkal_queue_submit(command_buffers1, 1);

	    vkal_present(image_id);

	    r_framecount++;
	}
    }

    deinit_mapmodel(mapmodel);
    
    vkal_cleanup();

    glfwDestroyWindow(g_window);
 
    glfwTerminate();

    deinit_physfs();

    return 0;
}
