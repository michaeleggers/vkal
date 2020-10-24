/* Michael Eggers, 9/20/2020

   The model matrices of the entities are provided to the shader through a dynamic uniform buffer.
   Note that only two models are loaded but for each a dedicated draw-call is issued. This
   is not very efficient. Instanced drawing should be used instead.
   The models come from an obj not using indexed drawing and a hard coded rect which, on the other
   hand uses indexed vertex data.
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

typedef struct Camera
{
	vec3 pos;
	vec3 center;
	vec3 up;
	vec3 right;
} Camera;

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

//#pragma pack(push, 1)
typedef struct Vertex
{
    vec3     pos;
    vec3     normal;
    vec3     color;
    uint32_t bone_indices[4];
    float    bone_weights[4];
} Vertex;

typedef struct MdMeshHeader
{
    uint32_t magic_number;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t bone_count;
    uint32_t node_count;
    
} MdMeshHeader;
//#pragma pack(pop)

#define MAX_BONE_NAME_LENGTH 64
typedef struct Bone
{
    char name[MAX_BONE_NAME_LENGTH];
    mat4 offset_matrix;
    uint32_t num_weights;
} Bone;

typedef struct Node
{
    uint32_t bone_index;
    int      parent_index;
    char     name[MAX_BONE_NAME_LENGTH];
} Node;

typedef struct MdMesh
{
    Vertex * vertices;
    uint32_t vertex_count;
    uint64_t vertex_buffer_offset;
    uint16_t * indices;
    uint32_t index_count;
    uint64_t index_buffer_offset;
    Bone *   bones;
    uint32_t bone_count;
    mat4 *   animation_matrices;
    mat4 *   tmp_matrices;
    Node *   skeleton_nodes;
    uint32_t node_count;
} MdMesh;

static GLFWwindow * window;
Platform p;

MdMesh load_md_mesh(char * filename)
{
    MdMesh result = {0 };
    
    FILE * fp = fopen( filename, "rb" );
    fseek( fp, 0L, SEEK_END );
    size_t file_size = ftell( fp );
    uint8_t * file_data = (uint8_t*)malloc(file_size*sizeof(uint8_t));
    rewind( fp );
    fread( file_data, file_size, 1, fp );
    fclose( fp );

    MdMeshHeader * header = (MdMeshHeader*)file_data;
    assert(0xAABBCCDD == header->magic_number);
#if 1
    printf("MD MESH HEADER DATA\n");
    printf("    Vertex Count: %d\n", header->vertex_count);
    printf("    Index Count:  %d\n", header->index_count);
    printf("    Bone Count:   %d\n", header->bone_count);
#endif
    result.vertex_count = header->vertex_count;
    result.index_count  = header->index_count;
    result.bone_count   = header->bone_count;
    result.node_count   = header->node_count;
    
    result.vertices           = (Vertex*)malloc(result.vertex_count * sizeof(Vertex));
    result.indices            = (uint16_t*)malloc(result.index_count * sizeof(uint16_t));
    result.bones              = (Bone*)malloc(result.bone_count * sizeof(Bone));
    result.animation_matrices = (mat4*)malloc(result.bone_count * sizeof(mat4));
    result.tmp_matrices       = (mat4*)malloc(result.bone_count * sizeof(mat4));
    result.skeleton_nodes     = (Node*)malloc(result.node_count * sizeof(Node));
    Vertex * vertex_data = (Vertex*)((MdMeshHeader*)file_data + 1);
    memcpy(result.vertices, vertex_data, result.vertex_count * sizeof(Vertex));
    uint16_t * index_data = (uint16_t*)(vertex_data + result.vertex_count);
    memcpy(result.indices, index_data, result.index_count * sizeof(uint16_t));
    Bone * bones = (Bone*)(index_data + result.index_count);
    memcpy(result.bones, bones, result.bone_count * sizeof(Bone));
    for (uint32_t i = 0; i < result.bone_count; ++i) {
	result.animation_matrices[i] = mat4_identity();
	result.tmp_matrices[i] = mat4_identity();
    }
    Node * skeleton_nodes = (Node*)(bones + result.bone_count);
    memcpy(result.skeleton_nodes, skeleton_nodes, result.node_count * sizeof(Node));
    
    return result;
}

void update_skeleton(MdMesh * mesh)
{
    Node * skeleton_nodes = mesh->skeleton_nodes;
    uint32_t node_count = mesh->node_count;
    for (uint32_t i = 0; i < node_count; ++i) {
	Node * node = &(skeleton_nodes[i]);
	int parent_index = node->parent_index;
	uint32_t bone_index = node->bone_index;
	if (parent_index >= 0) {
	    uint32_t parent_bone_index = skeleton_nodes[parent_index].bone_index;
	    mat4 parent_mat = mesh->tmp_matrices[parent_bone_index];
	    mat4 offset_mat = mesh->bones[bone_index].offset_matrix;
	    mat4 parent_offset_mat = mesh->bones[parent_index].offset_matrix;
	    mesh->tmp_matrices[bone_index] = mat4_x_mat4( parent_mat, mesh->animation_matrices[bone_index] );
	}
	else {
	    mesh->tmp_matrices[bone_index] = mat4_x_mat4( mat4_identity(), mesh->animation_matrices[bone_index] );
	}
    }

    for (uint32_t i = 0; i < node_count; ++i) {
	Node * node = &(skeleton_nodes[i]);
	int parent_index = node->parent_index;
//	if (parent_index >= 0) {
	    uint32_t bone_index = node->bone_index;
	    mat4 offset_mat = mesh->bones[bone_index].offset_matrix;
//	    mesh->tmp_matrices[bone_index] = mat4_x_mat4( mat4_inverse(offset_mat), mat4_x_mat4( mesh->tmp_matrices[bone_index], offset_mat ) );
//	}
    }
}

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
    vkal_select_physical_device(&devices[1]);
    VkalInfo * vkal_info =  vkal_init(device_extensions, device_extension_count);
    
    /* Shader Setup */
    uint8_t * vertex_byte_code = 0;
    int vertex_code_size;
    p.rfb("../src/examples/assets/shaders/model_loading_md_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    p.rfb("../src/examples/assets/shaders/model_loading_md_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);

    /* Vertex Input Assembly */
    VkVertexInputBindingDescription vertex_input_bindings[] =
	{
	    { 0, 3*sizeof(vec3) + 4*sizeof(uint32_t) + 4*sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX }
	};
    
    VkVertexInputAttributeDescription vertex_attributes[] =
	{
	    { 0, 0,  VK_FORMAT_R32G32B32_SFLOAT, 0 },               // pos
	    { 1, 0,  VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3) },    // normal
	    { 2, 0,  VK_FORMAT_R32G32B32_SFLOAT, 2*sizeof(vec3) },  // color
	    { 3, 0,  VK_FORMAT_R32_UINT, 3*sizeof(vec3) + 0*sizeof(uint32_t) }, // bone indices 0
	    { 4, 0,  VK_FORMAT_R32_UINT, 3*sizeof(vec3) + 1*sizeof(uint32_t) }, // bone indices 1
	    { 5, 0,  VK_FORMAT_R32_UINT, 3*sizeof(vec3) + 2*sizeof(uint32_t) }, // bone indices 2
	    { 6, 0,  VK_FORMAT_R32_UINT, 3*sizeof(vec3) + 3*sizeof(uint32_t) }, // bone indices 3
	    { 7, 0,  VK_FORMAT_R32_SFLOAT, 3*sizeof(vec3) + 4*sizeof(uint32_t) + 0*sizeof(float) }, // bone weights 0
	    { 8, 0,  VK_FORMAT_R32_SFLOAT, 3*sizeof(vec3) + 4*sizeof(uint32_t) + 1*sizeof(float) }, // bone weights 1
	    { 9, 0,  VK_FORMAT_R32_SFLOAT, 3*sizeof(vec3) + 4*sizeof(uint32_t) + 2*sizeof(float) }, // bone weights 2
	    { 10, 0, VK_FORMAT_R32_SFLOAT, 3*sizeof(vec3) + 4*sizeof(uint32_t) + 3*sizeof(float) }, // bone weights 3
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

    VkDescriptorSetLayoutBinding set_layout_storage[] = {
	{
	    0,
	    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	    1, /* Number bone matrices. TODO: Remove this magic-number! */
	    VK_SHADER_STAGE_VERTEX_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout_storage = vkal_create_descriptor_set_layout(set_layout_storage, 1);
    
    VkDescriptorSetLayout layouts[4];
    layouts[0] = descriptor_set_layout;
    layouts[1] = descriptor_set_layout_dynamic;
    layouts[2] = descriptor_set_layout_storage;
    layouts[3] = descriptor_set_layout_storage;
    
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
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_COUNTER_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout);

    /* Model Data */
    float rect_vertices[] = {
	// Pos            // Normal       // Color       
	-1.0,  1.0, 1.0,  0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  
	 1.0,  1.0, 1.0,  0.0, 0.0, 1.0,  0.0, 1.0, 0.0,  
	-1.0, -1.0, 1.0,  0.0, 0.0, 1.0,  0.0, 0.0, 1.0,  
    	 1.0, -1.0, 1.0,  0.0, 0.0, 1.0,  1.0, 1.0, 0.0, 
    };
    uint32_t vertex_count = sizeof(rect_vertices)/sizeof(*rect_vertices);
    
    uint16_t rect_indices[] = {
 	0, 1, 2,
	2, 1, 3
    };
    uint32_t index_count = sizeof(rect_indices)/sizeof(*rect_indices);
  
    uint64_t offset_vertices = vkal_vertex_buffer_add(rect_vertices, 2*sizeof(vec3) + sizeof(vec2), 4);
    uint64_t offset_indices  = vkal_index_buffer_add(rect_indices, index_count);
    Model rect_model = { 0 };
    rect_model.is_indexed = 1;
    rect_model.vertex_buffer_offset = offset_vertices;
    rect_model.vertex_count = vertex_count;
    rect_model.index_buffer_offset = offset_indices;
    rect_model.index_count = index_count;
    
    Model model = {0};
    model.is_indexed = 0;
    float bmin[3], bmax[3];
    load_obj(bmin, bmax, "../src/examples/assets/models/lego.obj", &model);
    model.vertex_buffer_offset = vkal_vertex_buffer_add(model.vertices, 9*sizeof(float), model.vertex_count);
    clear_model(&model);

    MdMesh md_mesh = load_md_mesh("../src/examples/assets/models/modeldata.md"); 
    md_mesh.vertex_buffer_offset = vkal_vertex_buffer_add(md_mesh.vertices, sizeof(Vertex), md_mesh.vertex_count);
    md_mesh.index_buffer_offset  = vkal_index_buffer_add(md_mesh.indices, md_mesh.index_count);
    Model md_model = { 0 };
    md_model.is_indexed = 1;
    md_model.vertex_buffer_offset = md_mesh.vertex_buffer_offset;
    md_model.vertex_count = md_mesh.vertex_count;
    md_model.index_buffer_offset = md_mesh.index_buffer_offset;
    md_model.index_count = md_mesh.index_count;
    
#define NUM_ENTITIES 2
    /* Entities */
    Entity entities[NUM_ENTITIES];
    vec3 pos = { 0, 0, 0.f };
    vec3 rot = { tr_radians(-90.f), 0.f, 0.f };
    vec3 scale = (vec3){ 2, 2, 2 };
    entities[0].model       = md_model;
    entities[0].position    = pos;
    entities[0].orientation = rot;
    entities[0].scale       = scale;
#if 1
    entities[1].model       = md_model;
    entities[1].position    = (vec3){ 0.f, 0.f, 3.f };
    entities[1].orientation = (vec3){ tr_radians(-90.f), 0.f, 0.f };
    entities[1].scale       = (vec3){ 1, 1, 1 };
#endif
    
    /* View Projection */
    Camera camera;
    camera.pos = (vec3){ 0, 0.f, 5.f };
    camera.center = (vec3){ 0 };
    camera.up = (vec3){ 0, 1, 0 };
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    view_proj_data.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f );

    /* Uniform Buffers */
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);
    ViewportData viewport_data;
    viewport_data.dimensions = (vec2){ SCREEN_WIDTH, SCREEN_HEIGHT };
    UniformBuffer viewport_ubo = vkal_create_uniform_buffer(sizeof(ViewportData), 1, 1);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], viewport_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&viewport_ubo, &viewport_data);

    /* Dynamic Uniform Buffers */
    UniformBuffer model_ubo = vkal_create_uniform_buffer(sizeof(ModelData), NUM_ENTITIES, 0);
    ModelData model_data[NUM_ENTITIES];
    mat4 model_mat = mat4_identity();
    model_mat = translate(model_mat, entities[0].position);
    for (uint32_t i = 0; i < NUM_ENTITIES; ++i) {
	((ModelData*)((uint8_t*)model_data + i*model_ubo.alignment))->model_mat = model_mat;
    }
    vkal_update_descriptor_set_uniform(descriptor_sets[1], model_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vkal_update_uniform(&model_ubo, model_data);

    /* Storage Buffer for bone-matrices */
    DeviceMemory offset_matrices_mem = vkal_allocate_devicememory(10*1024*1024,
								 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
								 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
								 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffer storage_buffer_bone_matrices = vkal_create_buffer(md_mesh.bone_count * sizeof(mat4),
					       &offset_matrices_mem,
					       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkal_dbg_buffer_name(storage_buffer_bone_matrices, "Storage Buffer Offset Matrices");
    map_memory(&storage_buffer_bone_matrices, md_mesh.bone_count * sizeof(mat4), 0);
    for (uint32_t i = 0; i < md_mesh.bone_count; ++i) {
	printf("%s\n", md_mesh.bones[i].name);
	memcpy( (void*)&((mat4*)storage_buffer_bone_matrices.mapped)[i], (void*)&(md_mesh.bones[i].offset_matrix), sizeof(mat4) );
    }
    unmap_memory(&storage_buffer_bone_matrices);	
    vkal_update_descriptor_set_bufferarray(descriptor_sets[2], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 0, storage_buffer_bone_matrices);

    /* Storage Buffer Skeleton (for now just the offset-matrices. Later on will use channels from key-frame animation) */
    DeviceMemory skeleton_matrices_mem = vkal_allocate_devicememory(10*1024*1024,
								    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
								    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
								    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffer storage_buffer_skeleton_matrices = vkal_create_buffer(md_mesh.bone_count * sizeof(mat4),
								 &skeleton_matrices_mem,
								 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkal_dbg_buffer_name(storage_buffer_skeleton_matrices, "Storage Buffer Skeleton Matrices");
    map_memory(&storage_buffer_skeleton_matrices, md_mesh.bone_count * sizeof(mat4), 0);
    for (uint32_t i = 0; i < md_mesh.bone_count; ++i) {
	memcpy( (void*)&((mat4*)storage_buffer_skeleton_matrices.mapped)[i], (void*)&(md_mesh.animation_matrices[i]), sizeof(mat4) );
    }
    // keep this storage's buffers memory mapped because we need to update the matrices in it every(?) frame.
    vkal_update_descriptor_set_bufferarray(descriptor_sets[3], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 0, storage_buffer_skeleton_matrices);

    float arm_r_rot_x = 0.0f;

    
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
	viewport_data.dimensions.x = (float)width;
	viewport_data.dimensions.y = (float)height;
	vkal_update_uniform(&viewport_ubo, &viewport_data);

        /* Update Model Matrices */
	static float d = .001f;
	d += .0005f;
	static float r = .001f;
	for (int i = 0; i < NUM_ENTITIES - 1; ++i) {
	    mat4 model_mat = mat4_identity();
	    d += 0.00001f;
//	    entities[i].position.x += sinf(d);
//	    entities[i].orientation.x += r;
//	    entities[i].orientation.y += r;
//	    entities[i].orientation.z += r;
	    model_mat = translate(model_mat, entities[i].position);
	    model_mat = tr_scale(model_mat, entities[i].scale);
	    mat4 rot_z = rotate_z( entities[i].orientation.z );
	    mat4 rot_y = rotate_y( entities[i].orientation.y );
	    mat4 rot_x = rotate_x( entities[i].orientation.x );
	    model_mat = mat4_x_mat4(model_mat, rot_z);
	    model_mat = mat4_x_mat4(model_mat, rot_y);
	    model_mat = mat4_x_mat4(model_mat, rot_x);

	    mat4 inv = mat4_inverse(model_mat);
	    mat4 iden = mat4_x_mat4(inv, model_mat);
	    mat4 iden2 = mat4_x_mat4(model_mat, inv);
	    
	    ((ModelData*)((uint8_t*)model_data + i*model_ubo.alignment))->model_mat = model_mat;
	}
	vkal_update_uniform(&model_ubo, model_data);

	/* update skeleton's upper right arm (Lego Model Index 14) */	
	arm_r_rot_x += 0.001f;
	static float dings = 0.0f;
	dings += 0.0001f;
	mat4 arm_rot_x = rotate_x (arm_r_rot_x );
	mat4 arm_rot_y = rotate_y (arm_r_rot_x );
	mat4 arm_rot_z = rotate_z (arm_r_rot_x );
	mat4 arm_rot = mat4_identity();
//	arm_rot = mat4_x_mat4( arm_rot, arm_rot_z );
	arm_rot = mat4_x_mat4( arm_rot, arm_rot_y );
//	arm_rot = mat4_x_mat4( arm_rot, arm_rot_x );
	mat4 arm_offset = md_mesh.bones[14].offset_matrix;
	mat4 trans = mat4_identity();
	trans = translate(trans, (vec3){0, 0.5f*fabs( sinf(dings) ), 0 });
	trans = mat4_x_mat4(trans, arm_rot);
	md_mesh.animation_matrices[14] = trans; //mat4_x_mat4(mat4_inverse(arm_offset), mat4_x_mat4(trans, arm_offset) );
	update_skeleton( &md_mesh );
	memcpy( storage_buffer_skeleton_matrices.mapped, md_mesh.tmp_matrices, md_mesh.bone_count * sizeof(mat4) );
	
	{
	    uint32_t image_id = vkal_get_image();

	    vkal_begin_command_buffer(image_id);

	    vkal_begin_render_pass(image_id, vkal_info->render_pass);
	    vkal_viewport(vkal_info->command_buffers[image_id],
			  0, 0,
			  (float)width, (float)height);
	    vkal_scissor(vkal_info->command_buffers[image_id],
			 0, 0,
			 (float)width, (float)height);
	    for (int i = 0; i < NUM_ENTITIES - 1; ++i) {
		uint32_t dynamic_offset = (uint32_t)(i*model_ubo.alignment);
		vkal_bind_descriptor_sets(image_id, descriptor_sets, descriptor_set_layout_count,
					  &dynamic_offset, 1,
					  pipeline_layout);
		Model model_to_draw = entities[i].model;
		if (model_to_draw.is_indexed) {
		    vkal_draw_indexed(image_id, graphics_pipeline,
				      model_to_draw.index_buffer_offset, model_to_draw.index_count,
				      model_to_draw.vertex_buffer_offset);
		}
		else {
		    vkal_draw(image_id, graphics_pipeline,
			      model_to_draw.vertex_buffer_offset, model_to_draw.vertex_count);
		}
	    }

	    vkal_end_renderpass(image_id);

	    vkal_end_command_buffer(image_id);
	    VkCommandBuffer command_buffers[1];
	    command_buffers[0] = vkal_info->command_buffers[image_id];
	    vkal_queue_submit(command_buffers, 1);

	    vkal_present(image_id);
	}
    }
    
    vkDeviceWaitIdle(vkal_info->device);
    
#if 1
    vkDestroyBuffer(vkal_info->device, storage_buffer_skeleton_matrices.buffer, NULL);
    vkFreeMemory(vkal_info->device, skeleton_matrices_mem.vk_device_memory, NULL);
#endif
    
    vkDestroyBuffer(vkal_info->device, storage_buffer_bone_matrices.buffer, NULL);
    vkFreeMemory(vkal_info->device, offset_matrices_mem.vk_device_memory, NULL);

    vkal_cleanup();

    glfwDestroyWindow(window);
 
    glfwTerminate();
    
    return 0;
}
