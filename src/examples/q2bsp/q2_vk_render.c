
#include <assert.h>

#include "../../vkal.h"

#include "q2_common.h"
#include "q2_vk_render.h"
#include "q2_r_local.h"
#include "q2_io.h"

static VkalInfo *        vkal_info;

static VkDescriptorSet * r_descriptor_set;
static VkPipeline        r_graphics_pipeline;
static VkPipelineLayout  r_pipeline_layout;
static VkPipeline        r_graphics_pipeline_bb;
static VkPipelineLayout  r_pipeline_layout_bb;
static VkPipeline        r_graphics_pipeline_sky;
static VkPipelineLayout  r_pipeline_layout_sky;

/* Buffers */
static VertexBuffer      r_transient_vertex_buffer; // scratch memory gets updated every frame with faces depending on viewpos
static VertexBuffer      r_transient_vertex_buffer_sky;
static VertexBuffer      r_transient_vertex_buffer_bb;
static VertexBuffer      r_transient_vertex_buffer_trans;
static IndexBuffer       r_transient_index_buffer_bb;
static StorageBuffer     r_transient_material_buffer;
static uint32_t          r_light_count;
static VertexBuffer      r_transient_vertex_storage_buffer;

static UniformBuffer     r_view_proj_ubo;

static Q2Texture         r_textures[1024];
static uint32_t          r_texture_count;
static Texture           r_cubemap;

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

void create_transient_index_buffer(IndexBuffer * index_buf)
{
	index_buf->memory = vkal_allocate_devicememory(MAX_MAP_VERTS*sizeof(uint16_t),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	index_buf->buffer = vkal_create_buffer( MAX_MAP_VERTS*sizeof(uint16_t), &index_buf->memory,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT );

	map_memory( &index_buf->buffer, MAX_MAP_VERTS*sizeof(uint16_t), 0 );
}

void update_transient_index_buffer(IndexBuffer * index_buf, uint32_t offset, uint16_t * indices, uint32_t index_count)
{
	memcpy( ((uint16_t*)(index_buf->buffer.mapped)) + offset, indices, index_count*sizeof(uint16_t) );      
}

void create_transient_vertex_storage_buffer(VertexBuffer * vertex_buf)
{
	vertex_buf->memory = vkal_allocate_devicememory(MAX_MAP_VERTS*sizeof(Vertex),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vertex_buf->buffer = vkal_create_buffer(MAX_MAP_VERTS*sizeof(Vertex), &vertex_buf->memory,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	map_memory( &vertex_buf->buffer, MAX_MAP_VERTS*sizeof(Vertex), 0 );
}

void vk_init_render(void * window)
{
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
	vkal_info =  vkal_init(device_extensions, device_extension_count);

	/* Shader Setup */
	uint8_t * vertex_byte_code = 0;
	uint8_t * fragment_byte_code = 0;
	int vertex_code_size;
	int fragment_code_size;

	load_shader_from_dir("assets/shaders/", "q2bsp_vert.spv", &vertex_byte_code, &vertex_code_size);	
	load_shader_from_dir("assets/shaders/", "q2bsp_frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup = vkal_create_shaders(
		vertex_byte_code, vertex_code_size, 
		fragment_byte_code, fragment_code_size);

	load_shader_from_dir("assets/shaders/", "q2bsp_bb_vert.spv", &vertex_byte_code, &vertex_code_size);	
	load_shader_from_dir("assets/shaders/", "q2bsp_bb_frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup_bb = vkal_create_shaders(
		vertex_byte_code, vertex_code_size, 
		fragment_byte_code, fragment_code_size);

	load_shader_from_dir("assets/shaders/", "q2bsp_sky_vert.spv", &vertex_byte_code, &vertex_code_size);
	load_shader_from_dir("assets/shaders/", "q2bsp_sky_frag.spv", &fragment_byte_code, &fragment_code_size);
	ShaderStageSetup shader_setup_sky = vkal_create_shaders(
		vertex_byte_code, vertex_code_size, 
		fragment_byte_code, fragment_code_size);

	load_shader_from_dir("assets/shaders/", "q2bsp_trans_vert.spv", &vertex_byte_code, &vertex_code_size);
	load_shader_from_dir("assets/shaders/", "q2bsp_trans_frag.spv", &fragment_byte_code, &fragment_code_size);
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
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT,			0												 },  // pos
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT,			sizeof(vec3)									 },  // color
		{ 2, 0, VK_FORMAT_R32G32_SFLOAT,			2*sizeof(vec3)									 },  // UV
		{ 3, 0, VK_FORMAT_A8B8G8R8_UINT_PACK32 ,    2*sizeof(vec3) + sizeof(vec2)					 },  // TEXTURE ID
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
		},
		{
			2,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1, 
			VK_SHADER_STAGE_VERTEX_BIT,
			0
		}
	};
	VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 3);

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

	VkDescriptorSetLayoutBinding set_layout_bb[] = {
		{
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT,
			0
		}
	};
	VkDescriptorSetLayout descriptor_set_layout_bb = vkal_create_descriptor_set_layout(set_layout_bb, 1);


	VkDescriptorSetLayout layouts[] = {
		descriptor_set_layout,
		descriptor_set_layout_sky,
		descriptor_set_layout_bb
	};
	uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);
	r_descriptor_set = (VkDescriptorSet*)malloc(descriptor_set_layout_count*sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, descriptor_set_layout_count, &r_descriptor_set);

	/* Create dummy Texture to make sure every descriptor is initialized */
	Image dummy_img = load_image_file_from_dir("textures", "michi");
	Texture dummy_texture = vkal_create_texture(1, dummy_img.data, dummy_img.width, dummy_img.height, 4, 0,
		VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
		0, 1, 0, 1, VK_FILTER_NEAREST, VK_FILTER_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	for (uint32_t i = 0; i < 1024; ++i) {
		vkal_update_descriptor_set_texturearray(
			r_descriptor_set[0], 
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
			i,
			dummy_texture);

	} 

	/* Pipeline */
	r_pipeline_layout = vkal_create_pipeline_layout(
		layouts, 1, 
		NULL, 0);
	r_graphics_pipeline = vkal_create_graphics_pipeline(
		vertex_input_bindings, 1,
		vertex_attributes, vertex_attribute_count,
		shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FRONT_FACE_CLOCKWISE,
		vkal_info->render_pass, r_pipeline_layout);

	r_pipeline_layout_bb = vkal_create_pipeline_layout(
		&layouts[2], 1,
		NULL, 0);
	r_graphics_pipeline_bb = vkal_create_graphics_pipeline(
		vertex_input_bindings, 1,
		vertex_attributes, vertex_attribute_count,
		shader_setup_bb, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		VK_FRONT_FACE_CLOCKWISE,
		vkal_info->render_pass, r_pipeline_layout_bb);

	/* Pipeline for Transparent surfaces */
	VkPipelineLayout pipeline_layout_trans = vkal_create_pipeline_layout(
		layouts, 1, 
		NULL, 0);
	VkPipeline graphics_pipeline_trans = vkal_create_graphics_pipeline(
		vertex_input_bindings, 1,
		vertex_attributes, vertex_attribute_count,
		shader_setup_trans, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FRONT_FACE_CLOCKWISE,
		vkal_info->render_pass, pipeline_layout_trans);

	/* Pipeline for Skybox */
	r_pipeline_layout_sky = vkal_create_pipeline_layout(
		&layouts[1], 1, 
		NULL, 0);
	r_graphics_pipeline_sky = vkal_create_graphics_pipeline(
		vertex_input_bindings, 1,
		vertex_attributes, vertex_attribute_count,
		shader_setup_sky, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FRONT_FACE_CLOCKWISE,
		vkal_info->render_pass, r_pipeline_layout_sky);

	/* Buffers */
	create_transient_vertex_buffer(&r_transient_vertex_buffer);
	create_transient_vertex_buffer(&r_transient_vertex_buffer_sky);
	create_transient_vertex_buffer(&r_transient_vertex_buffer_bb);
	create_transient_vertex_buffer(&r_transient_vertex_buffer_trans);
	create_transient_index_buffer(&r_transient_index_buffer_bb);
	create_transient_vertex_storage_buffer( &r_transient_vertex_storage_buffer ); 

	/* Uniforms */
	r_view_proj_ubo = vkal_create_uniform_buffer(sizeof(r_view_proj_data), 1, 0);
	vkal_update_descriptor_set_uniform(r_descriptor_set[ 0 ], r_view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	vkal_update_descriptor_set_bufferarray( r_descriptor_set[ 0 ], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 0, r_transient_vertex_storage_buffer.buffer);
	vkal_update_descriptor_set_uniform(r_descriptor_set[ 1 ], r_view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	vkal_update_descriptor_set_uniform(r_descriptor_set[ 2 ], r_view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

uint32_t vk_register_texture(char * texture_name, uint32_t * out_width, uint32_t * out_height)
{
	uint32_t i = 0;
	for ( ; i <= r_texture_count; ++i) {
		if ( !strcmp(texture_name, r_textures[ i ].name) ) {
			break;
		}
	}
	if (i > r_texture_count) {
		i = r_texture_count;
		Image image = load_image_file_from_dir("textures", texture_name);
		Texture texture = vkal_create_texture(1, image.data, image.width, image.height, 4, 0,
			VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
			0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
		vkal_update_descriptor_set_texturearray(
			r_descriptor_set[0], 
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
			i,
			texture);
		strcpy( r_textures[ r_texture_count ].name, texture_name );
		r_textures[ r_texture_count ].vk_texture = texture;
		r_texture_count++;
	}

	*out_width  = r_textures[ i ].vk_texture.width;
	*out_height = r_textures[ i ].vk_texture.height;

	return i;
}

void vk_create_cubemap(unsigned char * data, uint32_t width, uint32_t height, uint32_t channels)
{
	r_cubemap = vkal_create_texture(
		1,
		data,
		width, height, channels,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_IMAGE_VIEW_TYPE_CUBE,
		VK_FORMAT_R8G8B8A8_UNORM,
		0, 1,
		0, 6,
		VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	vkal_update_descriptor_set_texture(r_descriptor_set[1], r_cubemap);
}

void vk_begin_frame(void)
{
	vkal_update_uniform(&r_view_proj_ubo, &r_view_proj_data);

	r_image_id = vkal_get_image();

	vkal_begin_command_buffer(r_image_id);
	vkal_begin_render_pass(r_image_id, vkal_info->render_pass);
	vkal_viewport(vkal_info->command_buffers[r_image_id],
		0, 0,
		(float)r_width, (float)r_height);
	vkal_scissor(vkal_info->command_buffers[r_image_id],
		0, 0,
		(float)r_width, (float)r_height);

}

void vk_end_frame(void)
{
	vkal_end_renderpass(r_image_id);	    
	vkal_end_command_buffer(r_image_id);
	VkCommandBuffer command_buffers1[] = { vkal_info->command_buffers[r_image_id] };
	vkal_queue_submit(command_buffers1, 1);

	vkal_present(r_image_id);	    

	vkDeviceWaitIdle(vkal_info->device);

	r_framecount++;
}

void vk_draw_static_geometry(Vertex * vertices, uint32_t vertex_count)
{
	vkal_bind_descriptor_set(r_image_id, &r_descriptor_set[0], r_pipeline_layout);
	memcpy( r_transient_vertex_buffer.buffer.mapped, vertices, vertex_count*sizeof(Vertex) );      
	vkal_draw_from_buffers( r_transient_vertex_buffer.buffer, r_image_id, r_graphics_pipeline, 0, vertex_count );
}

void vk_draw_bb(Vertex * vertices, uint32_t vertex_count, uint16_t * indices, uint32_t index_count)
{
	//vkal_bind_descriptor_set(r_image_id, &r_descriptor_set[2], r_pipeline_layout_bb);

	//memcpy( r_transient_vertex_buffer_bb.buffer.mapped, vertices, vertex_count*sizeof(Vertex) );
	//memcpy( r_transient_index_buffer_bb.buffer.mapped, indices, index_count*sizeof(uint16_t) );      

	//vkal_draw_indexed_from_buffers( 
	//	r_transient_index_buffer_bb.buffer, 0, index_count, 
	//	r_transient_vertex_buffer_bb.buffer, 0, r_image_id, r_graphics_pipeline_bb );
}

void vk_draw_sky(Vertex * vertices, uint32_t vertex_count)
{
	vkal_bind_descriptor_set(r_image_id, &r_descriptor_set[1], r_pipeline_layout_sky);
	memcpy( r_transient_vertex_buffer_sky.buffer.mapped, vertices, vertex_count*sizeof(Vertex) );      
	vkal_draw_from_buffers( r_transient_vertex_buffer_sky.buffer, r_image_id, r_graphics_pipeline_sky, 0, vertex_count );
}

void vk_draw_transluscent_chain(MapFace * transluscent_chain)
{
	vkal_bind_descriptor_set(r_image_id, &r_descriptor_set[0], r_pipeline_layout);
	MapFace * trans_surf = transluscent_chain;
	while ( trans_surf ) {
		vkal_draw(r_image_id, r_graphics_pipeline, trans_surf->vk_vertex_buffer_offset, trans_surf->vertex_count);
		trans_surf = trans_surf->transluscent_chain;
	}
}

void vk_draw_transluscent(Vertex * vertices, uint32_t vertex_count)
{
	vkal_bind_descriptor_set(r_image_id, &r_descriptor_set[0], r_pipeline_layout);
	memcpy( r_transient_vertex_buffer_trans.buffer.mapped, vertices, vertex_count*sizeof(Vertex) );      
	vkal_draw_from_buffers( r_transient_vertex_buffer_trans.buffer, r_image_id, r_graphics_pipeline, 0, vertex_count );
}

void vk_add_light(Vertex * vertex)
{
	memcpy( (Vertex*)r_transient_vertex_storage_buffer.buffer.mapped + r_light_count, vertex, sizeof(Vertex) );
	r_light_count++;
}