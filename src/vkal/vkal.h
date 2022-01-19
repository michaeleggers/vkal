#ifndef VKAL_H
#define VKAL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#if defined (VKAL_WIN32)
	#define VK_USE_PLATFORM_WIN32_KHR
#endif



#include <vulkan/vulkan.h>

#if defined (VKAL_GLFW)
    #include <GLFW/glfw3.h>
#elif defined (VKAL_WIN32)
	#include <Windows.h>
#elif defined (VKAL_SDL)
    #include <SDL.h>
    #include <SDL_vulkan.h>
#endif

#define VKAL_NULL                       0

#define VKAL_MB								(1024 * 1024)
#define STAGING_BUFFER_SIZE				(64 * VKAL_MB)
#define UNIFORM_BUFFER_SIZE				(64 * VKAL_MB)
#define VERTEX_BUFFER_SIZE				(64 * VKAL_MB)
#define INDEX_BUFFER_SIZE				(64 * VKAL_MB)

#define VKAL_MAX_SWAPCHAIN_IMAGES		4
#define VKAL_MAX_IMAGES_IN_FLIGHT		4
#define VKAL_MAX_DESCRIPTOR_SETS		10
#define VKAL_MAX_COMMAND_POOLS			2
#define VKAL_MAX_VKDEVICEMEMORY			128
#define VKAL_MAX_VKIMAGE				128
#define VKAL_MAX_VKIMAGEVIEW			128
#define VKAL_MAX_VKSHADERMODULE			64
#define VKAL_MAX_VKPIPELINELAYOUT		64
#define VKAL_MAX_VKDESCRIPTORSETLAYOUT	128
#define VKAL_MAX_VKPIPELINE				64
#define VKAL_MAX_VKSAMPLER				128
#define VKAL_MAX_TEXTURES				10
#define VKAL_MAX_VKFRAMEBUFFER			64
#define VKAL_VSYNC_ON					1
#define VKAL_SHADOW_MAP_DIMENSION		2048

#define VKAL_ASSERT(result)	                	\
	assert(VK_SUCCESS == result);			    \

#define VKAL_MALLOC(pointer, count) pointer = malloc(count * sizeof(*pointer))

#define VKAL_FREE(pointer) free(pointer)					

#define VKAL_ARRAY_LENGTH(arr)		\
	sizeof(arr) / sizeof(arr[0])	\

#define VKAL_MIN(a, b) (a < b ? a : b)
#define VKAL_MAX(a, b) (a > b ? a : b)

						/* Debug extensions */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
	PFN_vkSetDebugUtilsObjectNameEXT                       vkSetDebugUtilsObjectName;
#endif 

#if _DEBUG																	
#define VKAL_DBG_BUFFER_NAME(vkal_info_macro, vkal_buffer, macro_name) 												\
		{																												\
		VkDebugUtilsObjectNameInfoEXT obj_info;																			\
		obj_info.pNext = VKAL_NULL;																						\
		obj_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;											\
		obj_info.objectType = VK_OBJECT_TYPE_BUFFER;																	\
		obj_info.objectHandle = (uint64_t)vkal_buffer.buffer;															\
		obj_info.pObjectName = macro_name;																				\
		VKAL_ASSERT(vkSetDebugUtilsObjectName(vkal_info_macro, &obj_info),	"Failed to create debug name for Buffer");	\
		}

#define VKAL_DBG_IMAGE_NAME(vkal_info_macro, vkal_dbg_image_macro, name)											\
		{																												\
		VkDebugUtilsObjectNameInfoEXT obj_info = { 0 };																	\
		obj_info.pNext = VKAL_NULL;																						\
		obj_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;											\
		obj_info.objectType = VK_OBJECT_TYPE_IMAGE;																		\
		obj_info.objectHandle = (uint64_t)vkal_dbg_image_macro;															\
		obj_info.pObjectName = name;																					\
		VKAL_ASSERT(vkSetDebugUtilsObjectName(vkal_info_macro, &obj_info), "Failed to create debug name for Buffer");	\
		}
#else
	#define VKAL_DBG_BUFFER_NAME(vkal_info_macro, vkal_buffer, macro_name)
	#define VKAL_DBG_IMAGE_NAME(vkal_info_macro, vkal_dbg_image_macro, name)
#endif																					

#ifdef __cplusplus
}
#endif


typedef struct VkalTexture
{
    uint32_t  device_memory_id;
    VkSampler sampler;
    uint32_t  image;
    uint32_t  image_view;
    uint32_t  width;
    uint32_t  height;
    uint32_t  channels;
    uint32_t  binding;
    char      texture_file[64];
} VkalTexture;

typedef struct DeviceMemory
{
    VkDeviceMemory vk_device_memory;
    VkDeviceSize   size;
    VkDeviceSize   alignment;
    VkDeviceSize   free;
    uint32_t       mem_type_index;
} DeviceMemory;

typedef struct Buffer
{
    VkBuffer			buffer;
    uint64_t			size;
    uint64_t			offset;
    VkDeviceMemory		device_memory;
    VkBufferUsageFlags	usage;
    void				* mapped;
} Buffer;

typedef struct VkalImage
{
    uint32_t      image;
    uint32_t      image_view;
    VkSampler     sampler;
    uint32_t      device_memory;
    uint32_t      width, height;
} VkalImage;

typedef struct RenderImage
{
    uint32_t      width, height;
	VkalImage     color_image;
    VkalImage     depth_image;
    uint32_t      framebuffers[VKAL_MAX_SWAPCHAIN_IMAGES];
} RenderImage; 

typedef struct UniformBuffer
{
    uint64_t size;
    uint64_t offset;
    uint32_t binding;
    uint64_t alignment;
} UniformBuffer;

typedef struct DescriptorSetLayout
{
    uint32_t				binding;
    VkDescriptorSetLayout	layout;
} DescriptorSetLayout;

typedef struct VkalDeviceMemoryHandle {
    VkDeviceMemory device_memory;
    uint8_t        used;
} VkalDeviceMemoryHandle;

typedef struct VkalImageHandle {
    VkImage image;
    uint8_t used;
} VkalImageHandle;

typedef struct VkalImageViewHandle {
    VkImageView image_view;
    uint8_t     used;
    uint32_t    offset;
} VkalImageViewHandle;

typedef struct VkalShaderModuleHandle {
    VkShaderModule shader_module;
    uint8_t        used;
} VkalShaderModuleHandle;

typedef struct VkalPipelineLayoutHandle {
    VkPipelineLayout pipeline_layout;
    uint8_t          used;
} VkalPipelineLayoutHandle;

typedef struct VkalDescriptorSetLayoutHande {
    VkDescriptorSetLayout descriptor_set_layout;
    uint8_t               used;
} VkalDescriptorSetLayoutHande;

typedef struct VkalPipelineHandle {
    VkPipeline pipeline;
    uint8_t    used;
} VkalPipelineHandle;

typedef struct VkalSamplerHandle {
    VkSampler sampler;
    uint8_t   used;
} VkalSamplerHandle;

typedef struct VkalFramebufferHandle {
    VkFramebuffer framebuffer;
    uint8_t       used;
} VkalFramebufferHandle;

typedef struct OffscreenPass {
    uint32_t				width, height;
    uint32_t				framebuffer;
    uint32_t				image;
    uint32_t				image_view;
    uint32_t				device_memory;
    VkRenderPass			render_pass;
    VkSampler				depth_sampler;
    VkDescriptorImageInfo	image_descriptor;
    VkDescriptorSet			* descriptor_sets;
    VkCommandBuffer			* command_buffers;
    uint32_t				command_buffer_count;
} OffscreenPass;

typedef struct VkalPhysicalDevice
{
    VkPhysicalDeviceProperties	property; 
    VkPhysicalDevice			device;
}VkalPhysicalDevice;

typedef struct VkalInfo
{
    
#if defined (VKAL_GLFW)
    GLFWwindow * window;
#elif defined (VKAL_WIN32)
	HWND window;
#elif defined (VKAL_SDL)
    SDL_Window* window;
#endif

    VkInstance instance;

    VkExtensionProperties			* available_instance_extensions;
    uint32_t						available_instance_extension_count;

    VkLayerProperties				* available_instance_layers;
    uint32_t						available_instance_layer_count;
    int								enable_instance_layers;

    VkPhysicalDevice				* physical_devices;
    VkPhysicalDeviceProperties		* physical_devices_properties;
    uint32_t						physical_device_count;
    VkalPhysicalDevice				* suitable_devices;
    uint32_t						suitable_device_count;
    
    /* Active Physical Device */
    VkPhysicalDevice				physical_device;
    VkPhysicalDeviceProperties		physical_device_properties;
    
    VkalDeviceMemoryHandle			user_device_memory[VKAL_MAX_VKDEVICEMEMORY];

    VkalImageHandle					user_images[VKAL_MAX_VKIMAGE];
    uint32_t						user_image_count;

    VkalImageViewHandle				user_image_views[VKAL_MAX_VKIMAGEVIEW];
    VkalShaderModuleHandle			user_shader_modules[VKAL_MAX_VKSHADERMODULE];
    VkalPipelineLayoutHandle		user_pipeline_layouts[VKAL_MAX_VKPIPELINELAYOUT];
    VkalDescriptorSetLayoutHande	user_descriptor_set_layouts[VKAL_MAX_VKDESCRIPTORSETLAYOUT];
    VkalPipelineHandle				user_pipelines[VKAL_MAX_VKPIPELINE];
    VkalSamplerHandle				user_samplers[VKAL_MAX_VKSAMPLER];
    VkalFramebufferHandle			user_framebuffers[VKAL_MAX_VKFRAMEBUFFER];

    VkDevice	 device; 
    VkQueue		 graphics_queue;
    VkQueue      present_queue;
    VkSurfaceKHR surface;

    VkSwapchainKHR	swapchain;
    uint32_t		should_recreate_swapchain; // = 0;
    VkImage			swapchain_images[VKAL_MAX_SWAPCHAIN_IMAGES];
    uint32_t		swapchain_image_count;
    VkFormat		swapchain_image_format;
    VkExtent2D		swapchain_extent;
    VkImageView		swapchain_image_views[VKAL_MAX_SWAPCHAIN_IMAGES];
    uint32_t		depth_stencil_image;
    uint32_t		depth_stencil_image_view;
    uint32_t		device_memory_depth_stencil;

    VkDeviceMemory	device_memory_staging;
    Buffer			staging_buffer;

    VkRenderPass		render_pass;
    VkRenderPass		render_to_image_render_pass;
    VkFramebuffer		* framebuffers;
    uint32_t			framebuffer_count;
    VkPipelineLayout	pipeline_layout;
    VkPipeline			graphics_pipeline;
    VkClearColorValue   clear_color_value;

    VkCommandPool		default_command_pools[VKAL_MAX_COMMAND_POOLS];
    uint32_t			default_commandpool_count;
    VkCommandBuffer		* default_command_buffers;
    uint32_t			default_command_buffer_count;
    
    VkSemaphore			image_available_semaphores[VKAL_MAX_IMAGES_IN_FLIGHT];
    VkSemaphore			render_finished_semaphores[VKAL_MAX_IMAGES_IN_FLIGHT];
    VkFence				in_flight_fences[VKAL_MAX_IMAGES_IN_FLIGHT];
    uint32_t			frames_rendered;
    //uint32_t current_frame;
    
	Buffer			default_uniform_buffer;
    Buffer			default_vertex_buffer;
    Buffer			default_index_buffer;
    VkDeviceMemory	default_device_memory_uniform;
    VkDeviceMemory	default_device_memory_vertex;
    VkDeviceMemory	default_device_memory_index;
    uint64_t		default_uniform_buffer_offset;
    uint64_t		default_vertex_buffer_offset;
    uint64_t		default_index_buffer_offset;

    VkDescriptorPool default_descriptor_pool;
} VkalInfo;

typedef struct QueueFamilyIndicies {
    int has_graphics_family;
    uint32_t graphics_family;
    int has_present_family;
    uint32_t present_family;
} QueueFamilyIndicies;

typedef struct ShaderStageSetup
{
    VkPipelineShaderStageCreateInfo vertex_shader_create_info;
    VkPipelineShaderStageCreateInfo fragment_shader_create_info;
    uint32_t vert_shader_module;
    uint32_t frag_shader_module;
} ShaderStageSetup;

#define VKAL_MAX_SURFACE_FORMATS	176
#define VKAL_MAX_PRESENT_MODES		9

typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR formats[VKAL_MAX_SURFACE_FORMATS];
    uint32_t format_count;
    VkPresentModeKHR present_modes[VKAL_MAX_PRESENT_MODES];
    uint32_t present_mode_count;
} SwapChainSupportDetails;


#ifdef __cplusplus
extern "C"{
#endif 

VkalInfo * vkal_init(char ** extensions, uint32_t extension_count);

#if defined (VKAL_GLFW)
	void vkal_create_instance_glfw(
		GLFWwindow * window,
		char ** instance_extensions, uint32_t instance_extension_count,
		char ** instance_layers, uint32_t instance_layer_count);
#elif defined (VKAL_WIN32)
	void vkal_create_instance_win32(
		HWND window, HINSTANCE hInstance,
		char ** instance_extensions, uint32_t instance_extension_count,
		char ** instance_layers, uint32_t instance_layer_count);
#elif defined (VKAL_SDL)
void vkal_create_instance_sdl(
    SDL_Window * window,
    char** instance_extensions, uint32_t instance_extension_count,
    char** instance_layers, uint32_t instance_layer_count);
#endif

void vkal_find_suitable_devices(
	char ** extensions, uint32_t extension_count,
	VkalPhysicalDevice ** out_devices, uint32_t * out_device_count);
void vkal_select_physical_device(VkalPhysicalDevice * physical_device);
int check_instance_layer_support(
	char const * requested_layer,
	VkLayerProperties * available_layers, uint32_t available_layer_count);
int check_instance_extension_support(
	char const * requested_extension,
	VkExtensionProperties * available_extensions, uint32_t available_extension_count);
void create_logical_device(char ** extensions, uint32_t extension_count);
QueueFamilyIndicies find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);
void create_swapchain(void);
void create_image_views(void);
void recreate_swapchain(void);
void create_default_framebuffers(void);
void internal_create_framebuffer(VkFramebufferCreateInfo create_info, uint32_t * out_framebuffer);
VkFramebuffer get_framebuffer(uint32_t id);
void destroy_framebuffer(uint32_t id);
uint32_t create_render_image_framebuffer(RenderImage render_image, uint32_t width, uint32_t height);
RenderImage recreate_render_image(RenderImage render_image, uint32_t width, uint32_t height);
VkalImage create_vkal_image(
	uint32_t width, uint32_t height, 
	VkFormat format,
	VkImageUsageFlagBits usage_flags, VkImageAspectFlags aspect_bits,
	VkImageLayout layout);
void create_default_command_buffers(void);
VkCommandBuffer vkal_create_command_buffer(VkCommandBufferLevel cmd_buffer_level, uint32_t begin);
void create_default_render_pass(void);
void create_render_to_image_render_pass(void);

VkPipeline vkal_create_graphics_pipeline(
	VkVertexInputBindingDescription * vertex_input_bindings,
	uint32_t vertex_input_binding_count,
	VkVertexInputAttributeDescription * vertex_attributes,
	uint32_t vertex_attribute_count,
	ShaderStageSetup shader_setup, 
    VkBool32 depth_test_enable, VkCompareOp depth_compare_op, 
    VkCullModeFlags cull_mode,
    VkPolygonMode polygon_mode,
    VkPrimitiveTopology primitive_topology,
    VkFrontFace face_winding, VkRenderPass render_pass,
	VkPipelineLayout pipeline_layout);
void create_graphics_pipeline(VkGraphicsPipelineCreateInfo create_info, uint32_t * out_graphics_pipeline);
VkPipeline get_graphics_pipeline(uint32_t id);
void destroy_graphics_pipeline(uint32_t id);
    
void create_default_depth_buffer(void);
void create_default_descriptor_pool(void);
void create_default_command_pool(void);
void allocate_default_device_memory_uniform(void);
void allocate_default_device_memory_vertex(void);
void allocate_default_device_memory_index(void);
DeviceMemory vkal_allocate_devicememory(uint32_t size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags);
void create_default_uniform_buffer(uint32_t size);
void create_default_vertex_buffer(uint32_t size);
void create_default_index_buffer(uint32_t size);
void create_default_semaphores(void);
void vkal_cleanup(void);
void flush_to_memory(VkDeviceMemory device_memory, void * dst_memory, void * src_memory, uint32_t size, uint32_t offset);
uint64_t vkal_vertex_buffer_add(void * vertices, uint32_t vertex_size, uint32_t vertex_count);
uint64_t vkal_index_buffer_add(uint16_t * indices, uint32_t index_count);

#if defined (VKAL_GLFW)
	void create_glfw_surface(void);
#elif defined (VKAL_WIN32)
	void create_win32_surface(HINSTANCE hInstance);
#elif defined (VKAL_SDL)
    void create_sdl_surface(void);
#endif

UniformBuffer vkal_create_uniform_buffer(uint32_t size, uint32_t elements, uint32_t binding);
void vkal_update_descriptor_set_uniform(
	VkDescriptorSet descriptor_set, UniformBuffer uniform_buffer,
	VkDescriptorType descriptor_type);
void vkal_update_descriptor_set_bufferarray(VkDescriptorSet descriptor_set, VkDescriptorType descriptor_type, uint32_t binding, uint32_t array_element, Buffer buffer);
void vkal_update_descriptor_set_texturearray(
	VkDescriptorSet descriptor_set,
	VkDescriptorType descriptor_type,
	uint32_t array_element, VkalTexture texture);
void vkal_update_uniform(UniformBuffer * uniform_buffer, void * data);
uint32_t check_memory_type_index(uint32_t const memory_requirement_bits, VkMemoryPropertyFlags const wanted_property);
void upload_texture(VkImage const image, uint32_t w, uint32_t h, uint32_t n, uint32_t array_layer_count, unsigned char * texture_data);
void create_staging_buffer(uint32_t size);
Buffer create_buffer(uint32_t size, VkBufferUsageFlags usage);
Buffer vkal_create_buffer(VkDeviceSize size, DeviceMemory * device_memory, VkBufferUsageFlags buffer_usage_flags);
VkResult map_memory(Buffer * buffer, VkDeviceSize size, VkDeviceSize offset);
void unmap_memory(Buffer * buffer);
void vkal_update_buffer(Buffer buffer, uint8_t* data);
static void create_image(
	uint32_t width, uint32_t height, uint32_t mip_levels, uint32_t array_layers,
	VkImageCreateFlags flags, VkFormat format, VkImageUsageFlags usage_flags, uint32_t * out_image_id);
static void destroy_image(uint32_t id);
VkImage get_image(uint32_t id);
static void create_image_view(
	VkImage image,
	VkImageViewType view_type, VkFormat format, VkImageAspectFlags aspect_flags,
	uint32_t base_mip_level, uint32_t mip_level_count,
	uint32_t base_array_layer, uint32_t array_layer_count,
	uint32_t * out_image_view);
static void destroy_image_view(uint32_t id);
VkImageView get_image_view(uint32_t id);
VkSampler create_sampler(
	VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode u,
	VkSamplerAddressMode v, VkSamplerAddressMode w);
static void internal_create_sampler(VkSamplerCreateInfo create_info, uint32_t * out_sampler);
static VkSampler get_sampler(uint32_t id);
void destroy_sampler(uint32_t id);
VkPipelineLayout vkal_create_pipeline_layout(
    VkDescriptorSetLayout * descriptor_set_layouts, uint32_t descriptor_set_layout_count,
    VkPushConstantRange * push_constant_ranges, uint32_t push_constant_range_count);
void create_pipeline_layout(
    VkDescriptorSetLayout * descriptor_set_layouts, uint32_t descriptor_set_layout_count,
    VkPushConstantRange * push_constant_ranges, uint32_t push_constant_range_count,
    uint32_t * out_pipeline_layout);
void destroy_pipeline_layout(uint32_t id);
VkPipelineLayout get_pipeline_layout(uint32_t id);
VkDeviceMemory allocate_memory(VkDeviceSize size, uint32_t mem_type_bits);
void create_device_memory(VkDeviceSize size, uint32_t mem_type_bits, uint32_t * out_memory_id);
uint32_t destroy_device_memory(uint32_t id);
VkDeviceMemory get_device_memory(uint32_t id);
VkWriteDescriptorSet create_write_descriptor_set_image(
	VkDescriptorSet dst_descriptor_set, uint32_t dst_binding,
	uint32_t count, VkDescriptorType type,
	VkDescriptorImageInfo * image_info);
VkWriteDescriptorSet create_write_descriptor_set_image2(
	VkDescriptorSet dst_descriptor_set, uint32_t dst_binding,
	uint32_t array_element, uint32_t count,
	VkDescriptorType type, VkDescriptorImageInfo * image_info);
VkWriteDescriptorSet create_write_descriptor_set_buffer(
	VkDescriptorSet dst_descriptor_set, uint32_t dst_binding,
	uint32_t count, VkDescriptorType type,
	VkDescriptorBufferInfo * buffer_info);
VkWriteDescriptorSet create_write_descriptor_set_buffer2(
	VkDescriptorSet dst_descriptor_set, uint32_t dst_binding,
	uint32_t dst_array_element,
	uint32_t count, VkDescriptorType type, VkDescriptorBufferInfo * buffer_info);
void vkal_allocate_descriptor_sets(VkDescriptorPool pool, VkDescriptorSetLayout * layout, uint32_t layout_count, VkDescriptorSet ** out_descriptor_set);
VkalTexture vkal_create_texture(
	uint32_t binding,
    unsigned char * texture_data, uint32_t width, uint32_t height, uint32_t channels, 
	VkImageCreateFlags flags, VkImageViewType view_type, VkFormat format,
    uint32_t base_mip_level, uint32_t mip_level_count, 
	uint32_t base_array_layer, uint32_t array_layer_count,
    VkFilter min_filter, VkFilter mag_filter,
	VkSamplerAddressMode sampler_u, VkSamplerAddressMode sampler_v, VkSamplerAddressMode sampler_w);
RenderImage create_render_image(uint32_t width, uint32_t height);
void vkal_update_descriptor_set_texture(VkDescriptorSet descriptor_set, VkalTexture texture);
void vkal_update_descriptor_set_render_image(
	VkDescriptorSet descriptor_set, uint32_t binding,
	VkImageView image_view, VkSampler sampler);
ShaderStageSetup vkal_create_shaders(const uint8_t * vertex_shader_code, uint32_t vertex_shader_code_size, const uint8_t * fragment_shader_code, uint32_t fragment_shader_code_size);
VkPipelineShaderStageCreateInfo create_shader_stage_info(VkShaderModule module, VkShaderStageFlagBits shader_stage_flag_bits);
void create_shader_module(uint8_t const * shader_byte_code, int size, uint32_t * out_shader_module);
VkShaderModule get_shader_module(uint32_t id);
void destroy_shader_module(uint32_t id);
uint32_t vkal_get_image(void);
void vkal_viewport(VkCommandBuffer command_buffer, float x, float y, float width, float height);
void vkal_scissor(VkCommandBuffer command_buffer, float offset_x, float offset_y, float extent_x, float extent_y);
void vkal_draw_indexed(
    uint32_t image_id, VkPipeline pipeline,
    VkDeviceSize index_buffer_offset, uint32_t index_count,
    VkDeviceSize vertex_buffer_offset);
void vkal_draw_indexed_from_buffers(
    Buffer index_buffer, uint64_t index_buffer_offset, uint32_t index_count, Buffer vertex_buffer, uint64_t vertex_buffer_offset,
    uint32_t image_id, VkPipeline pipeline);
void vkal_draw(
    uint32_t image_id, VkPipeline pipeline,
    VkDeviceSize vertex_buffer_offset, uint32_t vertex_count);
void vkal_draw_from_buffers(
    Buffer vertex_buffer,
    uint32_t image_id, VkPipeline pipeline,
    VkDeviceSize vertex_buffer_offset, uint32_t vertex_count);
void vkal_draw_indexed2(
    VkCommandBuffer command_buffer, VkPipeline pipeline,
    VkDeviceSize index_buffer_offset, uint32_t index_count,
    VkDeviceSize vertex_buffer_offset);
void vkal_bind_descriptor_set(
	uint32_t image_id,
	VkDescriptorSet * descriptor_sets,
	VkPipelineLayout pipeline_layout);
void vkal_bind_descriptor_sets(
	uint32_t image_id,
	VkDescriptorSet * descriptor_sets, uint32_t descriptor_set_count,
	uint32_t * dynamic_offsets, uint32_t dynamic_offset_count,
	VkPipelineLayout pipeline_layout);
void vkal_bind_descriptor_set_dynamic(
	uint32_t image_id,
	VkDescriptorSet * descriptor_sets,
	VkPipelineLayout pipeline_layout, uint32_t dynamic_offset);
void vkal_bind_descriptor_set2(
	VkCommandBuffer command_buffer,
	uint32_t first_set, VkDescriptorSet * descriptor_sets, uint32_t descriptor_set_count,
	VkPipelineLayout pipeline_layout);
void vkal_begin_command_buffer(uint32_t image_id);
void vkal_begin(uint32_t image_id, VkCommandBuffer command_buffer, VkRenderPass render_pass);
void vkal_begin_render_to_image_render_pass(
	uint32_t image_id, VkCommandBuffer command_buffer,
	VkRenderPass render_pass, RenderImage render_image);
void vkal_begin_render_pass(uint32_t image_id, VkRenderPass render_pass);
void vkal_end(VkCommandBuffer command_buffer);
void vkal_end_command_buffer(uint32_t image_id);
void vkal_end_renderpass(uint32_t image_id);
void vkal_queue_submit(VkCommandBuffer * command_buffers, uint32_t command_buffer_count);
void vkal_present(uint32_t image_id);
VkDescriptorSetLayout vkal_create_descriptor_set_layout(VkDescriptorSetLayoutBinding * layout, uint32_t binding_count);
void create_descriptor_set_layout(VkDescriptorSetLayoutBinding * layout, uint32_t binding_count, uint32_t * out_descriptor_set_layout);
VkDescriptorSetLayout get_descriptor_set_layout(uint32_t id);
void destroy_descriptor_set_layout(uint32_t id);
QueueFamilyIndicies find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);
void vkal_destroy_graphics_pipeline(VkPipeline pipeline);
void vkal_set_clear_color(VkClearColorValue value);

void set_image_layout(
    VkCommandBuffer         command_buffer,
    VkImage                 image,
    VkImageLayout           old_layout,
    VkImageLayout           new_layout,
    VkImageSubresourceRange subresource_range,
    VkPipelineStageFlags    src_mask, // = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags    dst_mask); // = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
void flush_command_buffer(VkCommandBuffer command_buffer, VkQueue queue, int free);

#ifdef __cplusplus
}
#endif


#endif
