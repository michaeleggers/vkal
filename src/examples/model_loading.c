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

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "external/tinyobj_loader_c.h"

#define FLT_MAX       100.f;
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

typedef struct Model
{
    float *  vertices;
    uint32_t vertex_count;
    uint32_t vertex_buffer_offset;
    uint16_t * indices;
    uint32_t index_count;
    uint32_t index_buffer_offset;
} Model;

static GLFWwindow * window;
static Platform p;
static Model g_model;

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

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
    float v10[3];
    float v20[3];
    float len2;

    v10[0] = v1[0] - v0[0];
    v10[1] = v1[1] - v0[1];
    v10[2] = v1[2] - v0[2];

    v20[0] = v2[0] - v0[0];
    v20[1] = v2[1] - v0[1];
    v20[2] = v2[2] - v0[2];

    N[0] = v20[1] * v10[2] - v20[2] * v10[1];
    N[1] = v20[2] * v10[0] - v20[0] * v10[2];
    N[2] = v20[0] * v10[1] - v20[1] * v10[0];

    len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
    if (len2 > 0.0f) {
	float len = (float)sqrt((double)len2);

	N[0] /= len;
	N[1] /= len;
    }
}

static void get_file_data(char const * filename, char ** data, size_t * len)
{
    p.rfb(filename, (uint8_t**)data, (int*)len);
}

static int load_obj(float bmin[3], float bmax[3], char const * filename)
{
    tinyobj_attrib_t     attrib;
    tinyobj_shape_t*     shapes = NULL;
    size_t               num_shapes;
    tinyobj_material_t*  materials = NULL;
    size_t               num_materials;

    
    {
	unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
	int ret = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
				    &num_materials, filename, get_file_data, flags);
	if (ret != TINYOBJ_SUCCESS) {
	    return 0;
	}
	printf("# of shapes    = %d\n", (int)num_shapes);
	printf("# of materials = %d\n", (int)num_materials);
    }
    
    size_t stride      = 9; /* 9 = pos(3float), normal(3float), color(3float) */
    size_t num_triangles = attrib.num_face_num_verts;
    float * vb = (float*)malloc(sizeof(float) * stride * num_triangles * 3);
    uint16_t * indices = (uint16_t*)malloc(sizeof(uint16_t) * attrib.num_faces);
    
    bmin[0] = bmin[1] = bmin[2] = FLT_MAX;
    bmax[0] = bmax[1] = bmax[2] = -FLT_MAX;

    {
	size_t face_offset = 0;
	size_t i;

	for (i = 0; i < attrib.num_face_num_verts; ++i) {
	    size_t f;
	    assert(attrib.face_num_verts[i] % 3 == 0); /* each face must be a triangle */
	    for (f = 0; f < (size_t)attrib.face_num_verts[i] / 3; ++f) {
		size_t k;
		float v[3][3];
		float n[3][3];
		float c[3];
		float len2;

		tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 3 * f + 0];
		tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 3 * f + 1];
		tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 3 * f + 2];

		for (k = 0; k < 3; ++k) {
		    int f0 = idx0.v_idx;
		    int f1 = idx1.v_idx;
		    int f2 = idx2.v_idx;
		    assert(f0 >= 0);
		    assert(f1 >= 0);
		    assert(f2 >= 0);

		    v[0][k] = attrib.vertices[3 * (size_t)f0 + k];
		    v[1][k] = attrib.vertices[3 * (size_t)f1 + k];
		    v[2][k] = attrib.vertices[3 * (size_t)f2 + k];
		    bmin[k] = (v[0][k] < bmin[k]) ? v[0][k] : bmin[k];
		    bmin[k] = (v[1][k] < bmin[k]) ? v[1][k] : bmin[k];
		    bmin[k] = (v[2][k] < bmin[k]) ? v[2][k] : bmin[k];
		    bmax[k] = (v[0][k] > bmax[k]) ? v[0][k] : bmax[k];
		    bmax[k] = (v[1][k] > bmax[k]) ? v[1][k] : bmax[k];
		    bmax[k] = (v[2][k] > bmax[k]) ? v[2][k] : bmax[k];		       
		}

		if (attrib.num_normals > 0) {
		    int f0 = idx0.vn_idx;
		    int f1 = idx1.vn_idx;
		    int f2 = idx2.vn_idx;
		    if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
			assert(f0 < (int)attrib.num_normals);
			assert(f1 < (int)attrib.num_normals);
			assert(f2 < (int)attrib.num_normals);
			for (k = 0; k < 3; k++) {
			    n[0][k] = attrib.normals[3 * (size_t)f0 + k];
			    n[1][k] = attrib.normals[3 * (size_t)f1 + k];
			    n[2][k] = attrib.normals[3 * (size_t)f2 + k];
			}
		    } else { /* normal index is not defined for this face */
			/* compute geometric normal */
			CalcNormal(n[0], v[0], v[1], v[2]);
			n[1][0] = n[0][0];
			n[1][1] = n[0][1];
			n[1][2] = n[0][2];
			n[2][0] = n[0][0];
			n[2][1] = n[0][1];
			n[2][2] = n[0][2];
		    }
		} else {
		    /* compute geometric normal */
		    CalcNormal(n[0], v[0], v[1], v[2]);
		    n[1][0] = n[0][0];
		    n[1][1] = n[0][1];
		    n[1][2] = n[0][2];
		    n[2][0] = n[0][0];
		    n[2][1] = n[0][1];
		    n[2][2] = n[0][2];
		}

		for (k = 0; k < 3; k++) {
		    vb[(3 * i + k) * stride + 0] = v[k][0];
		    vb[(3 * i + k) * stride + 1] = v[k][1];
		    vb[(3 * i + k) * stride + 2] = v[k][2];
		    vb[(3 * i + k) * stride + 3] = n[k][0];
		    vb[(3 * i + k) * stride + 4] = n[k][1];
		    vb[(3 * i + k) * stride + 5] = n[k][2];

		    /* Use normal as color. */
		    c[0] = n[k][0];
		    c[1] = n[k][1];
		    c[2] = n[k][2];
		    len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
		    if (len2 > 0.0f) {
			float len = (float)sqrt((double)len2);

			c[0] /= len;
			c[1] /= len;
			c[2] /= len;
		    }

		    vb[(3 * i + k) * stride + 6] = (c[0] * 0.5f + 0.5f);
		    vb[(3 * i + k) * stride + 7] = (c[1] * 0.5f + 0.5f);
		    vb[(3 * i + k) * stride + 8] = (c[2] * 0.5f + 0.5f);
		}
	    }
	    /* You can access per-face material through attrib.material_ids[i] */

	    face_offset += (size_t)attrib.face_num_verts[i];	    
	}	
    }
  
    g_model.vertices = vb;
    g_model.vertex_count = 3*num_triangles;

    printf("bmin = %f, %f, %f\n", (double)bmin[0], (double)bmin[1],
	   (double)bmin[2]);
    printf("bmax = %f, %f, %f\n", (double)bmax[0], (double)bmax[1],
	   (double)bmax[2]);

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
  
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
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 1);

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

    float bmin[3], bmax[3];
    load_obj(bmin, bmax, "../src/examples/assets/models/egg_deformed.obj");
    g_model.vertex_buffer_offset = vkal_vertex_buffer_add(g_model.vertices, 9*sizeof(float), g_model.vertex_count);

    /* View Projection */
    mat4 view = mat4_identity();
    view = translate(view, (vec3){ 0.f, 0.f, -5.f });
    ViewProjection view_proj_data = {
	.view = view,
	.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f )
    };

    /* Uniform Buffers */
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);

    /* Dynamic Uniform Buffers */
    UniformBuffer model_ubo = vkal_create_uniform_buffer(sizeof(ModelData), 2, 0);
    ModelData * model_data = (ModelData*)malloc(2*model_ubo.alignment);
    model_data[0].model_mat = mat4_identity();
    mat4 model_mat_2 = mat4_identity();
    model_mat_2 = translate(model_mat_2, (vec3){ 1.f, 0.f, 0.f });
    ((ModelData*)((uint8_t*)model_data + model_ubo.alignment))->model_mat = model_mat_2;
    vkal_update_descriptor_set_uniform(descriptor_sets[1], model_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vkal_update_uniform(&model_ubo, model_data);
    uint32_t dynamic_offsets[] = { 0, model_ubo.alignment };
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	/* Update View Projection Matrices */
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	view_proj_data.proj = perspective( tr_radians(45.f), (float)width/(float)height, 0.1f, 100.f );
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);
	    
	glfwPollEvents();

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
	    vkal_bind_descriptor_sets(image_id, descriptor_sets, descriptor_set_layout_count,
				      &dynamic_offsets[0], 1,
				      pipeline_layout);
	    vkal_draw(image_id, graphics_pipeline, g_model.vertex_buffer_offset, g_model.vertex_count);
	    vkal_bind_descriptor_sets(image_id, descriptor_sets, descriptor_set_layout_count,
				      &dynamic_offsets[1], 1,
				      pipeline_layout);
	    vkal_draw(image_id, graphics_pipeline, g_model.vertex_buffer_offset, g_model.vertex_count);
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
