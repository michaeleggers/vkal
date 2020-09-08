#include <assert.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "vkal.h"
#include "platform.h"

static Platform p;
static void * vkal_memory;
static VkalInfo vkal_info;
static Texture g_texture;

#define MB                  (1024 * 1024)
#define STAGING_BUFFER_SIZE (128 * MB)
#define UNIFORM_BUFFER_SIZE (10 * MB)
#define VERTEX_BUFFER_SIZE  (128 * MB)
#define INDEX_BUFFER_SIZE   (128 * MB)

/* To make the NV extensions visible globally we must go through an indirection (_DEF). Otherwise the compiler will complain
about redefinition of the NV function! We have to load them during load time since vulkan.lib does not expose those
extensions to the linker!
*/
PFN_vkCreateAccelerationStructureNV                    vkCreateAccelerationStructureNV_DEF;
#define vkCreateAccelerationStructureNV                vkCreateAccelerationStructureNV_DEF

PFN_vkGetAccelerationStructureMemoryRequirementsNV     vkGetAccelerationStructureMemoryRequirementsNV_DEF;
#define vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV_DEF

PFN_vkBindAccelerationStructureMemoryNV                vkBindAccelerationStructureMemoryNV_DEF;
#define vkBindAccelerationStructureMemoryNV            vkBindAccelerationStructureMemoryNV_DEF

PFN_vkGetAccelerationStructureHandleNV                 vkGetAccelerationStructureHandleNV_DEF;
#define vkGetAccelerationStructureHandleNV             vkGetAccelerationStructureHandleNV_DEF

PFN_vkCmdBuildAccelerationStructureNV                  vkCmdBuildAccelerationStructureNV_DEF;
#define vkCmdBuildAccelerationStructureNV              vkCmdBuildAccelerationStructureNV_DEF

PFN_vkCreateRayTracingPipelinesNV                      vkCreateRayTracingPipelinesNV_DEF;
#define vkCreateRayTracingPipelinesNV	               vkCreateRayTracingPipelinesNV_DEF

PFN_vkGetRayTracingShaderGroupHandlesNV                vkGetRayTracingShaderGroupHandlesNV_DEF;
#define vkGetRayTracingShaderGroupHandlesNV            vkGetRayTracingShaderGroupHandlesNV_DEF

PFN_vkCmdTraceRaysNV                                   vkCmdTraceRaysNV_DEF;
#define vkCmdTraceRaysNV                               vkCmdTraceRaysNV_DEF

/* Debug extensions */
PFN_vkSetDebugUtilsObjectNameEXT                       vkSetDebugUtilsObjectNameEXT_DEF;
#define vkSetDebugUtilsObjectNameEXT                   vkSetDebugUtilsObjectNameEXT_DEF

VkalInfo * init_vulkan(GLFWwindow * window, char ** extensions, uint32_t extension_count, char ** instance_extensions, uint32_t instance_extension_count)
{
	vkal_info.window = window;
	vkal_info.mapped_uniform_memory = 0;
    
	init_platform(&p);
	//vkal_memory = p.initialize_memory(1000 * 1024 * 1024);

	create_instance(instance_extensions, instance_extension_count);
#ifdef _DEBUG
	vkSetDebugUtilsObjectNameEXT_DEF = (PFN_vkSetDebugUtilsObjectNameEXT)glfwGetInstanceProcAddress(vkal_info.instance, "vkSetDebugUtilsObjectNameEXT");
#endif 

	create_surface();
	pick_physical_device(extensions, extension_count);
	create_logical_device(extensions, extension_count);
	create_swapchain();
	create_image_views();
	create_render_pass();
	create_render_to_image_render_pass();
	{
		create_offscreen_render_pass();
		create_offscreen_framebuffer();
	}
	create_depth_buffer();
	create_framebuffer();
	create_descriptor_pool();
	create_command_pool();
	create_command_buffers();
	create_uniform_buffer(UNIFORM_BUFFER_SIZE);
	allocate_device_memory_uniform();
	create_vertex_buffer(VERTEX_BUFFER_SIZE);
	allocate_device_memory_vertex();
	create_index_buffer(INDEX_BUFFER_SIZE);
	allocate_device_memory_index();
	create_staging_buffer(STAGING_BUFFER_SIZE);
	//create_graphics_pipeline(uniform_size, uniform_offset);
	create_semaphores();
	vkal_info.frames_rendered = 0;


	return &vkal_info;
}

void create_instance(char ** instance_extensions, uint32_t instance_extension_count)
{
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vkal Application";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "VULKAN.HM.EDU";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_2;
    
	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
    
	// Query available extensions. Just print them here. Actual extension loading
	// is being done by glfw below.
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(0, &extension_count, 0);
	VkExtensionProperties * extensions = 0;
	make_array(extensions, VkExtensionProperties, extension_count)
		vkEnumerateInstanceExtensionProperties(0, &extension_count, extensions);
	printf("available instance extensions (%d):\n", extension_count);
	for (int i = 0; i < extension_count; ++i) {
		printf("%s\n", (extensions + i)->extensionName);
	}
	kill_array(extensions);
    
	// If debug build check if validation layers defined in struct are available and load them
	uint32_t layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);
	//MemoryArena * arena = create_arena();
	/*MemoryArena stuff_arena;
	initialize_arena(&stuff_arena, vkal_memory, 200 * 1024 * 1024);*/
	VkLayerProperties * layers = (VkLayerProperties*)malloc(layer_count * sizeof(VkLayerProperties));
	char ** available_layer_names = (char**)malloc(layer_count * sizeof(char*));
    
	vkEnumerateInstanceLayerProperties(&layer_count, layers);
	printf("\navailable instance layers (%d):\n", layer_count);
	for (int i = 0; i < layer_count; ++i) {
		available_layer_names[i] = (layers + i)->layerName;
		printf("%s\n", (layers + i)->layerName);
	}
    
#ifdef _DEBUG
	vkal_info.enable_validation_layers = 1;
#else
	vkal_info.enable_validation_layers = 0;
#endif
	int layer_ok = 0;
	if (vkal_info.enable_validation_layers) {
		for (int i = 0; i < array_length(validation_layers); ++i) {
			layer_ok = check_validation_layer_support(validation_layers[i], available_layer_names, layer_count);
			if (!layer_ok) {
				printf("validation layer not available: %s\n", validation_layers[i]);
				DBG_VULKAN_ASSERT(VK_ERROR_LAYER_NOT_PRESENT, "requested validation layer not present");
			}
		}
	}
	if (layer_ok) {
		create_info.enabledLayerCount = array_length(validation_layers);
		create_info.ppEnabledLayerNames = validation_layers;
	}
    
	uint32_t glfw_extension_count = 0;
	char const ** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	
	uint32_t inst_ext_count = glfw_extension_count + instance_extension_count;
	char ** all_instance_extensions;
	all_instance_extensions = (char**)malloc(inst_ext_count * sizeof(char*));
	for (int i = 0; i < inst_ext_count; ++i) {
		all_instance_extensions[i] = (char*)malloc(256 * sizeof(char));
	}
	int i = 0;
	for (; i < glfw_extension_count; ++i) {
		strcpy(all_instance_extensions[i], glfw_extensions[i]);
	}
	for (int j = 0; i < inst_ext_count; ++i) {
		strcpy(all_instance_extensions[i], instance_extensions[j++]);
	}
	create_info.enabledExtensionCount = inst_ext_count;
	create_info.ppEnabledExtensionNames = all_instance_extensions;
    
	DBG_VULKAN_ASSERT(vkCreateInstance(&create_info, 0, &vkal_info.instance), "failed to create VkInstance");
	for (int i = 0; i < inst_ext_count; ++i) {
		free(all_instance_extensions[i]);
	}

	//p.mdalloc(&stuff_arena);
}



	
void init_raytracing()
{
	vkal_info.nv_rt_ctx.properties =
	    (VkPhysicalDeviceRayTracingPropertiesNV){ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV };
	VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	properties.pNext = &(vkal_info.nv_rt_ctx.properties);
	vkGetPhysicalDeviceProperties2(vkal_info.physical_device, &properties);

	/* Init function pointers */
	vkCreateAccelerationStructureNV_DEF                = (PFN_vkCreateAccelerationStructureNV)glfwGetInstanceProcAddress(vkal_info.instance, "vkCreateAccelerationStructureNV");
	vkGetAccelerationStructureMemoryRequirementsNV_DEF = (PFN_vkGetAccelerationStructureMemoryRequirementsNV)glfwGetInstanceProcAddress(vkal_info.instance, "vkGetAccelerationStructureMemoryRequirementsNV");
	vkBindAccelerationStructureMemoryNV_DEF            = (PFN_vkBindAccelerationStructureMemoryNV)glfwGetInstanceProcAddress(vkal_info.instance, "vkBindAccelerationStructureMemoryNV");
	vkGetAccelerationStructureHandleNV_DEF             = (PFN_vkGetAccelerationStructureHandleNV)glfwGetInstanceProcAddress(vkal_info.instance, "vkGetAccelerationStructureHandleNV");
	vkCmdBuildAccelerationStructureNV_DEF              = (PFN_vkCmdBuildAccelerationStructureNV)glfwGetInstanceProcAddress(vkal_info.instance, "vkCmdBuildAccelerationStructureNV");
	vkCreateRayTracingPipelinesNV_DEF                  = (PFN_vkCreateRayTracingPipelinesNV)glfwGetInstanceProcAddress(vkal_info.instance, "vkCreateRayTracingPipelinesNV");
	vkCmdTraceRaysNV_DEF                               = (PFN_vkCmdTraceRaysNV)glfwGetInstanceProcAddress(vkal_info.instance, "vkCmdTraceRaysNV");
	vkGetRayTracingShaderGroupHandlesNV_DEF            = (PFN_vkGetRayTracingShaderGroupHandlesNV)glfwGetInstanceProcAddress(vkal_info.instance, "vkGetRayTracingShaderGroupHandlesNV");
}

// Create an image memory barrier for changing the layout of
// an image and put it into an active command buffer
// See chapter 11.4 "Image Layout" for details
// Copied from https://github.com/KhronosGroup/Vulkan-Samples/blob/master/framework/common/vk_common.cpp
void set_image_layout(
	VkCommandBuffer         command_buffer,
	VkImage                 image,
	VkImageLayout           old_layout,
	VkImageLayout           new_layout,
	VkImageSubresourceRange subresource_range,
	VkPipelineStageFlags    src_mask,
	VkPipelineStageFlags    dst_mask)
{
	// Create an image barrier object
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.image = image;
	barrier.subresourceRange = subresource_range;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (old_layout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		barrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source
		// Make sure any reads from the image have been finished
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (new_layout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (barrier.srcAccessMask == 0)
		{
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		command_buffer,
		src_mask,
		dst_mask,
		0,
		0, NULL,
		0, NULL,
		1, &barrier);
}

void flush_command_buffer(VkCommandBuffer command_buffer, VkQueue queue, int free)
{
	if (command_buffer == VK_NULL_HANDLE)
	{
		return;
	}

	DBG_VULKAN_ASSERT(vkEndCommandBuffer(command_buffer), "failed to end command buffer");

	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fence_info = {0};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = 0;

	VkFence fence;
	DBG_VULKAN_ASSERT(vkCreateFence(vkal_info.device, &fence_info, NULL, &fence), "failed to create fence");

	// Submit to the queue
	VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
	// Wait for the fence to signal that command buffer has finished executing
	DBG_VULKAN_ASSERT(vkWaitForFences(vkal_info.device, 1, &fence, VK_TRUE, UINT64_MAX), "failed waiting on fence");

	vkDestroyFence(vkal_info.device, fence, NULL);

	if (vkal_info.command_pools[0] && free)
	{
		vkFreeCommandBuffers(vkal_info.device, vkal_info.command_pools[0], 1, &command_buffer);
	}
}

/* Setup storage image the ray generation shader will be writing to */
void create_rt_storage_image()
{
	// Image
	{
		create_image(
			VKAL_RT_SIZE_X, VKAL_RT_SIZE_Y,
			1, 1,
			0,  // flags
			vkal_info.swapchain_image_format,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			&vkal_info.nv_rt_ctx.storage_image.image);
		vkal_dbg_image_name(get_image(vkal_info.nv_rt_ctx.storage_image.image), "RT storage image");

		// Back the image with actual memory:
		VkMemoryRequirements image_memory_requirements = {};
		vkGetImageMemoryRequirements(
			vkal_info.device,
			get_image(vkal_info.nv_rt_ctx.storage_image.image),
			&image_memory_requirements);
		uint32_t mem_type_bits = check_memory_type_index(
			image_memory_requirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		create_device_memory(
			image_memory_requirements.size,
			mem_type_bits,
			&vkal_info.nv_rt_ctx.storage_image.device_memory_id);
		VkResult result = vkBindImageMemory(
			vkal_info.device, get_image(vkal_info.nv_rt_ctx.storage_image.image),
			get_device_memory(vkal_info.nv_rt_ctx.storage_image.device_memory_id), 0);
		DBG_VULKAN_ASSERT(result, "failed to bind texture image memory!");
	}

	// Image View
	{
		create_image_view(
			get_image(vkal_info.nv_rt_ctx.storage_image.image),
			VK_IMAGE_VIEW_TYPE_2D,
			vkal_info.swapchain_image_format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, 1,
			0, 1,
			&vkal_info.nv_rt_ctx.storage_image.image_view);
	}

	// upload using staging buffer
	{
		VkCommandBuffer cmd_buf;
		VkCommandBufferAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandBufferCount = 1;
		allocate_info.commandPool = vkal_info.command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(vkal_info.device, &allocate_info, &cmd_buf);
		
		// start recording
		VkCommandBufferBeginInfo cmd_begin_info = {};
		cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		DBG_VULKAN_ASSERT(
			vkBeginCommandBuffer(cmd_buf, &cmd_begin_info), 
			"failed to put command buffer into recording state");

		set_image_layout(
			cmd_buf, 
			get_image(vkal_info.nv_rt_ctx.storage_image.image),
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			(VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		flush_command_buffer(cmd_buf, vkal_info.graphics_queue, 1);
	}
}

/* The image we will copy the raytracing storage image to. */
void create_rt_target_image()
{
	// Image
	{
		create_image(
			VKAL_RT_SIZE_X, VKAL_RT_SIZE_Y,
			1, 1,
			0,  // flags
			vkal_info.swapchain_image_format,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			&vkal_info.nv_rt_ctx.target_image.image);
		vkal_dbg_image_name(get_image(vkal_info.nv_rt_ctx.target_image.image), "RT target image");

		// Back the image with actual memory:
		VkMemoryRequirements image_memory_requirements = {};
		vkGetImageMemoryRequirements(
			vkal_info.device,
			get_image(vkal_info.nv_rt_ctx.target_image.image),
			&image_memory_requirements);
		uint32_t mem_type_bits = check_memory_type_index(
			image_memory_requirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		create_device_memory(
			image_memory_requirements.size,
			mem_type_bits,
			&vkal_info.nv_rt_ctx.target_image.device_memory_id);
		VkResult result = vkBindImageMemory(
			vkal_info.device, get_image(vkal_info.nv_rt_ctx.target_image.image),
			get_device_memory(vkal_info.nv_rt_ctx.target_image.device_memory_id), 0);
		DBG_VULKAN_ASSERT(result, "failed to bind texture image memory!");
	}

	// Image View
	{
		create_image_view(
			get_image(vkal_info.nv_rt_ctx.target_image.image),
			VK_IMAGE_VIEW_TYPE_2D,
			vkal_info.swapchain_image_format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, 1,
			0, 1,
			&vkal_info.nv_rt_ctx.target_image.image_view);
	}

	// upload using staging buffer
	{
		VkCommandBuffer cmd_buf;
		VkCommandBufferAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandBufferCount = 1;
		allocate_info.commandPool = vkal_info.command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(vkal_info.device, &allocate_info, &cmd_buf);

		// start recording
		VkCommandBufferBeginInfo cmd_begin_info = {};
		cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		DBG_VULKAN_ASSERT(
			vkBeginCommandBuffer(cmd_buf, &cmd_begin_info),
			"failed to put command buffer into recording state");

		set_image_layout(
			cmd_buf,
			get_image(vkal_info.nv_rt_ctx.target_image.image),
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			(VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		flush_command_buffer(cmd_buf, vkal_info.graphics_queue, 1);
	}
}

/* Create image, imageview and bind to device memory. Also perform image barrier to switch switch to wanted layout. */
VkalImage create_vkal_image(
	uint32_t width, uint32_t height,
	VkFormat format, VkImageUsageFlagBits usage_flags, VkImageAspectFlags aspect_bits,
	VkImageLayout layout, char const * name)
{
	VkalImage vkal_image;
	// Image
	{
		create_image(
			width, height,
			1, 1,
			0,  // flags
			format,
			usage_flags,
			&vkal_image.image);
		vkal_dbg_image_name(get_image(vkal_image.image), name);

		// Back the image with actual memory:
		VkMemoryRequirements image_memory_requirements = {};
		vkGetImageMemoryRequirements(
			vkal_info.device,
			get_image(vkal_image.image),
			&image_memory_requirements);
		uint32_t mem_type_bits = check_memory_type_index(
			image_memory_requirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		create_device_memory(image_memory_requirements.size, mem_type_bits, &vkal_image.device_memory);
		VkResult result = vkBindImageMemory(
			vkal_info.device, get_image(vkal_image.image),
			get_device_memory(vkal_image.device_memory), 0);
		DBG_VULKAN_ASSERT(result, "failed to bind texture image memory!");
	}

	// Image View
	{
		create_image_view(
			get_image(vkal_image.image),
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			aspect_bits,
			0, 1,
			0, 1,
			&vkal_image.image_view);
	}

	// upload
	{
		VkCommandBuffer cmd_buf;
		VkCommandBufferAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandBufferCount = 1;
		allocate_info.commandPool = vkal_info.command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(vkal_info.device, &allocate_info, &cmd_buf);

		// start recording
		VkCommandBufferBeginInfo cmd_begin_info = {};
		cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		DBG_VULKAN_ASSERT(
			vkBeginCommandBuffer(cmd_buf, &cmd_begin_info),
			"failed to put command buffer into recording state");

		set_image_layout(
			cmd_buf,
			get_image(vkal_image.image),
			VK_IMAGE_LAYOUT_UNDEFINED, layout,
			(VkImageSubresourceRange){ aspect_bits, 0, 1, 0, 1 },
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		flush_command_buffer(cmd_buf, vkal_info.graphics_queue, 1);
	}

	vkal_image.width = width;
	vkal_image.height = height;
	return vkal_image;
}

/* Can be used as a render target. Use this image when creating a framebuffer instead of eg. the swapchain image. */
RenderImage create_render_image(uint32_t width, uint32_t height)
{
	RenderImage render_image = {};
	render_image.depth_image = create_vkal_image(
		width, height,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		"render to image depth");
	// Image
	{
		create_image(
			width, height,
			1, 1,
			0,  // flags
			vkal_info.swapchain_image_format,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			&render_image.image);
		vkal_dbg_image_name(get_image(render_image.image), "RT target image");

		// Back the image with actual memory:
		VkMemoryRequirements image_memory_requirements = {};
		vkGetImageMemoryRequirements(
			vkal_info.device,
			get_image(render_image.image),
			&image_memory_requirements);
		uint32_t mem_type_bits = check_memory_type_index(
			image_memory_requirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		create_device_memory(image_memory_requirements.size, mem_type_bits, &render_image.device_memory);
		VkResult result = vkBindImageMemory(
			vkal_info.device, get_image(render_image.image),
			get_device_memory(render_image.device_memory), 0);
		DBG_VULKAN_ASSERT(result, "failed to bind texture image memory!");
	}

	// Image View
	{
		create_image_view(
			get_image(render_image.image),
			VK_IMAGE_VIEW_TYPE_2D,
			vkal_info.swapchain_image_format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, 1,
			0, 1,
			&render_image.image_view);
	}

	// upload
	{
		VkCommandBuffer cmd_buf;
		VkCommandBufferAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandBufferCount = 1;
		allocate_info.commandPool = vkal_info.command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(vkal_info.device, &allocate_info, &cmd_buf);

		// start recording
		VkCommandBufferBeginInfo cmd_begin_info = {};
		cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		DBG_VULKAN_ASSERT(
			vkBeginCommandBuffer(cmd_buf, &cmd_begin_info),
			"failed to put command buffer into recording state");

		set_image_layout(
			cmd_buf,
			get_image(render_image.image),
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			(VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		flush_command_buffer(cmd_buf, vkal_info.graphics_queue, 1);
	}

	render_image.framebuffer = create_render_image_framebuffer(render_image, width, height);
	render_image.width = width;
	render_image.height = height;
	return render_image;
}

/* BLAS: Contains scene's geometry (vertices, tris, ...) */
void create_rt_blas(VkGeometryNV * geometries, uint32_t geometryNV_count)
{
	vkal_info.nv_rt_ctx.blas_count = geometryNV_count;
	vkal_info.nv_rt_ctx.blas = (Blas*)malloc(geometryNV_count * sizeof(Blas));
	for (int i = 0; i < geometryNV_count; ++i) {
		{
			VkAccelerationStructureInfoNV acceleration_info = {};
			acceleration_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
			acceleration_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
			acceleration_info.instanceCount = 0;
			acceleration_info.geometryCount = 1;
			acceleration_info.pGeometries = &geometries[i];

			VkAccelerationStructureCreateInfoNV acceleration_create_info = {};
			acceleration_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
			acceleration_create_info.info = acceleration_info;
			DBG_VULKAN_ASSERT(vkCreateAccelerationStructureNV(
				vkal_info.device,
				&acceleration_create_info, 0, &vkal_info.nv_rt_ctx.blas[i].accel_structure), "Failed to create blas acceleration structure");
		}

		{
			VkAccelerationStructureMemoryRequirementsInfoNV memory_requirements = {};
			memory_requirements.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
			memory_requirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
			memory_requirements.accelerationStructure = vkal_info.nv_rt_ctx.blas[i].accel_structure;

			VkMemoryRequirements2 memory_requirements_2 = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
			vkGetAccelerationStructureMemoryRequirementsNV(vkal_info.device, &memory_requirements, &memory_requirements_2);

			uint32_t mem_type_index = check_memory_type_index(
				memory_requirements_2.memoryRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			create_device_memory(
				memory_requirements_2.memoryRequirements.size,
				mem_type_index, &vkal_info.nv_rt_ctx.blas[i].device_memory_handle);

			VkBindAccelerationStructureMemoryInfoNV accel_memory_info = {};
			accel_memory_info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
			accel_memory_info.accelerationStructure = vkal_info.nv_rt_ctx.blas[i].accel_structure;
			accel_memory_info.memory = get_device_memory(vkal_info.nv_rt_ctx.blas[i].device_memory_handle);
			DBG_VULKAN_ASSERT(vkBindAccelerationStructureMemoryNV(vkal_info.device, 1, &accel_memory_info),
				"Failed to bind blas acceleration structure to device memory");
		}

		DBG_VULKAN_ASSERT(vkGetAccelerationStructureHandleNV(vkal_info.device, vkal_info.nv_rt_ctx.blas[i].accel_structure,
			sizeof(uint64_t), &vkal_info.nv_rt_ctx.blas[i].handle),
			"Failed to get handle to blas acceleration structure");
	}
}

/* Create TLAS: Contains scene's object instances. */
void create_rt_tlas(uint32_t instance_count)
{
	VkAccelerationStructureInfoNV accel_info = {};
	accel_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accel_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accel_info.instanceCount = instance_count;
	accel_info.geometryCount = 0;

	VkAccelerationStructureCreateInfoNV create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	create_info.info = accel_info;
	DBG_VULKAN_ASSERT(vkCreateAccelerationStructureNV(vkal_info.device, &create_info, 0, 
		&vkal_info.nv_rt_ctx.tlas.accel_structure),
		"Failed to create TLAS structure");

	VkAccelerationStructureMemoryRequirementsInfoNV mem_requirements = {};
	mem_requirements.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	mem_requirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	mem_requirements.accelerationStructure = vkal_info.nv_rt_ctx.tlas.accel_structure;

	VkMemoryRequirements2 mem_requirements_2 = {};
	vkGetAccelerationStructureMemoryRequirementsNV(vkal_info.device, &mem_requirements, &mem_requirements_2);
	
	uint32_t mem_type_index = check_memory_type_index(
		mem_requirements_2.memoryRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	create_device_memory(mem_requirements_2.memoryRequirements.size, mem_type_index, &vkal_info.nv_rt_ctx.tlas.device_memory_handle);

	VkBindAccelerationStructureMemoryInfoNV accel_structure_mem_info = {};
	accel_structure_mem_info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accel_structure_mem_info.accelerationStructure = vkal_info.nv_rt_ctx.tlas.accel_structure;
	accel_structure_mem_info.memory = get_device_memory(vkal_info.nv_rt_ctx.tlas.device_memory_handle);
	DBG_VULKAN_ASSERT(vkBindAccelerationStructureMemoryNV(vkal_info.device, 1, &accel_structure_mem_info), 
		"Failed to bind acceleration structure to device memory");

	DBG_VULKAN_ASSERT(vkGetAccelerationStructureHandleNV(
		vkal_info.device,
		vkal_info.nv_rt_ctx.tlas.accel_structure,
		sizeof(uint64_t), &vkal_info.nv_rt_ctx.tlas.handle),
		"failed to get TLAS structure handle");
}

VkDeviceSize copy_shader_identifier(uint8_t * data, uint8_t * shader_handle_storage, uint32_t group_index)
{
	uint32_t shader_group_handle_size = vkal_info.nv_rt_ctx.properties.shaderGroupHandleSize;
	memcpy(data, shader_handle_storage + group_index * shader_group_handle_size, shader_group_handle_size);
	data += shader_group_handle_size;
	return shader_group_handle_size;
}

/* Create shader binding table: That will bind the shader-programs with the TLAS */
void create_rt_shader_binding_table()
{
	uint32_t shader_binding_table_size = vkal_info.nv_rt_ctx.properties.shaderGroupHandleSize * 6; /* TODO: Use #define MAX_SHADER_TABLE_INDEX instead of '6'! */
	vkal_info.nv_rt_ctx.shader_binding_table_device_memory = vkal_allocate_devicememory(
		16 * MB, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);
	vkal_info.nv_rt_ctx.shader_binding_table = vkal_create_buffer(
		shader_binding_table_size,
		&vkal_info.nv_rt_ctx.shader_binding_table_device_memory,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);
	map_memory(
		&vkal_info.nv_rt_ctx.shader_binding_table, VK_WHOLE_SIZE, vkal_info.nv_rt_ctx.shader_binding_table.offset);

	uint8_t * shader_handle_storage = (uint8_t*)malloc(sizeof(uint8_t)*shader_binding_table_size);
	vkGetRayTracingShaderGroupHandlesNV(
		vkal_info.device,
		vkal_info.nv_rt_ctx.pipeline,
		0, // first group
		6, // group count /* TODO use #define! */
		shader_binding_table_size,
		shader_handle_storage
	);
	uint8_t * data = (uint8_t*)vkal_info.nv_rt_ctx.shader_binding_table.mapped;
	VkDeviceSize offset = 0;
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_RAYGEN);
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_MISS);
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_SHADOWMISS);
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_CLOSEST_HIT);
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_SHADOWHIT);
	data += copy_shader_identifier(data, shader_handle_storage, INDEX_CLOSEST_HIT_DEBUG);
	unmap_memory(&vkal_info.nv_rt_ctx.shader_binding_table);
}

/* Descriptor Set for Raytracing dispatch. Not sure yet if we need all of this.*/
void create_rt_descriptor_sets(VkDescriptorSetLayout * layouts, uint32_t layout_count)
{
	VkDescriptorSetLayoutBinding set_layout[] = {
		{   // TLAS
			0, // binding id ( matches with shader )
			VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,
			1, // number of resources with this layout binding
			VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,
			0
		},
		{   // store result of raytacer in this attachment
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1,
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			0
		},
		{   // inverse view-/projectionmatrix
			2,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,
			0
		},
		{   // Vertex buffer
			3,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			2,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,
			0
		},
		{   // Index buffer
			4,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			2,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,
			0
		}
	};
	uint32_t set_layout_binding_count = sizeof(set_layout) / sizeof(*set_layout);
	vkal_info.nv_rt_ctx.descset_layout = vkal_create_descriptor_set_layout(set_layout, set_layout_binding_count);

	/* Actually allocate out Raytracing descriptor set. Exciting! */
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1},
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};
	VkDescriptorPoolCreateInfo descpool_create_info = {};
	descpool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descpool_create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(*pool_sizes);
	descpool_create_info.pPoolSizes = pool_sizes;
	descpool_create_info.maxSets = 1 + layout_count;
	DBG_VULKAN_ASSERT(vkCreateDescriptorPool(vkal_info.device, &descpool_create_info, 0, &vkal_info.nv_rt_ctx.descriptor_pool),
		"Failed to create Raytracing Descriptor Pool!");

	/*MemoryArena arena = {};
	initialize_arena(&arena, vkal_memory, 12 * sizeof(VkDescriptorSetLayout));*/
	VkDescriptorSetLayout * _layouts = (VkDescriptorSetLayout*)malloc((1 + layout_count)*sizeof(VkDescriptorSetLayout));
	memcpy(_layouts, &vkal_info.nv_rt_ctx.descset_layout, sizeof(VkDescriptorSetLayout));
	memcpy(_layouts + 1, layouts, layout_count * sizeof(VkDescriptorSetLayout));
	vkal_info.nv_rt_ctx.descriptor_sets = (VkDescriptorSet*)malloc((layout_count + 1) * sizeof(VkDescriptorSet));
	vkal_allocate_descriptor_sets(
		vkal_info.nv_rt_ctx.descriptor_pool,
		_layouts,
		1 + layout_count,
		&vkal_info.nv_rt_ctx.descriptor_sets
	);
	//p.mdalloc(&arena);

	/* TLAS Write Info */
	VkWriteDescriptorSetAccelerationStructureNV desc_acceleration_info = {};
	desc_acceleration_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	desc_acceleration_info.accelerationStructureCount = 1;
	desc_acceleration_info.pAccelerationStructures = &vkal_info.nv_rt_ctx.tlas.accel_structure;
	VkWriteDescriptorSet acceleration_write = {};
	acceleration_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	acceleration_write.pNext = &desc_acceleration_info;
	acceleration_write.dstSet = vkal_info.nv_rt_ctx.descriptor_sets[0];
	acceleration_write.dstBinding = 0;
	acceleration_write.descriptorCount = 1;
	acceleration_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

	/* Storage Image Info */
	VkDescriptorImageInfo storage_image_descriptor = {};
	storage_image_descriptor.imageView = get_image_view(vkal_info.nv_rt_ctx.storage_image.image_view);
	storage_image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	VkWriteDescriptorSet image_write = create_write_descriptor_set_image(
		vkal_info.nv_rt_ctx.descriptor_sets[0], 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storage_image_descriptor
	);

	/* Uniform Buffer Info for global data like inverse view/projection matrices */
	VkDescriptorBufferInfo buffer_descriptor_info = {};
	// NOTE: 'offset' and 'range' refer to the data within the buffer and NOT within the VkDeviceMemory!
	// so with offset = 0 does not mean the beginning of the VkDeviceMemory but rather the beginning
	// of the buffer.
	buffer_descriptor_info.buffer = vkal_info.nv_rt_ctx.ubo.buffer;
	buffer_descriptor_info.offset = 0;
	buffer_descriptor_info.range = VK_WHOLE_SIZE;
	VkWriteDescriptorSet uniform_buffer_write = create_write_descriptor_set_buffer(
		vkal_info.nv_rt_ctx.descriptor_sets[0], 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &buffer_descriptor_info);

	VkWriteDescriptorSet descriptor_sets[] = {
		acceleration_write, image_write, uniform_buffer_write
	};
	uint32_t write_count = sizeof(descriptor_sets) / sizeof(*descriptor_sets);
	vkUpdateDescriptorSets(vkal_info.device, write_count, descriptor_sets, 0, VK_NULL_HANDLE);
}

/* Raytracing pipeline */
void create_rt_pipeline(VkDescriptorSetLayout * layouts, uint32_t layout_count)
{
	VkPipelineLayoutCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayout * _layouts = (VkDescriptorSetLayout*)malloc((1 + layout_count) * sizeof(VkDescriptorSetLayout));
	memcpy(_layouts, &vkal_info.nv_rt_ctx.descset_layout, sizeof(VkDescriptorSetLayout));
	memcpy(_layouts + 1, layouts, layout_count * sizeof(VkDescriptorSetLayout));
	pipeline_info.setLayoutCount = 1 + layout_count;
	pipeline_info.pSetLayouts = _layouts;
	DBG_VULKAN_ASSERT(vkCreatePipelineLayout(vkal_info.device, &pipeline_info, 0, &vkal_info.nv_rt_ctx.pipeline_layout),
		"Failed to create Raytracing pipeline layout.");
	free(_layouts);

	uint32_t shader_index_raygen = 0;
	uint32_t shader_index_miss = 2;
	uint32_t shader_index_shadowmiss = 1;
	uint32_t shader_index_chit = 3;
	uint32_t shader_index_chit_debug = 4;

	uint8_t * rgen_code = 0;
	uint8_t * rmiss_code = 0;
	uint8_t * chit_code = 0;
	uint8_t * shadowmiss_code = 0;
	uint8_t * chit_debug_code = 0;
	int rgen_code_size = 0;
	int rmiss_code_size = 0;
	int chit_code_size = 0;
	int shadowmiss_code_size = 0;
	int chit_debug_code_size = 0;
	p.rfb("shaders/raygen.rgen.spv", &rgen_code, &rgen_code_size);
	p.rfb("shaders/miss.rmiss.spv", &rmiss_code, &rmiss_code_size);
	p.rfb("shaders/closesthit.rchit.spv", &chit_code, &chit_code_size);
	p.rfb("shaders/shadowmiss.rmiss.spv", &shadowmiss_code, &shadowmiss_code_size);
	p.rfb("shaders/closesthit_debug.rchit.spv", &chit_debug_code, &chit_debug_code_size);
	uint32_t rgen_module;
	uint32_t rmiss_module;
	uint32_t chit_module;
	uint32_t shadowmiss_module;
	uint32_t chit_debug_module;
	create_shader_module(rgen_code, rgen_code_size, &rgen_module);
	create_shader_module(rmiss_code, rmiss_code_size, &rmiss_module);
	create_shader_module(chit_code, chit_code_size, &chit_module);
	create_shader_module(shadowmiss_code, shadowmiss_code_size, &shadowmiss_module);
	create_shader_module(chit_debug_code, chit_debug_code_size, &chit_debug_module);

	/* Indices in the shader_stages array will be used as unique identifiers for the shaders in the
	   shader binding table
	*/
	VkPipelineShaderStageCreateInfo shader_stages[5];
	shader_stages[shader_index_raygen] = create_shader_stage_info(get_shader_module(rgen_module), VK_SHADER_STAGE_RAYGEN_BIT_NV);
	shader_stages[shader_index_miss] = create_shader_stage_info(get_shader_module(rmiss_module), VK_SHADER_STAGE_MISS_BIT_NV);
	shader_stages[shader_index_shadowmiss] = create_shader_stage_info(get_shader_module(shadowmiss_module), VK_SHADER_STAGE_MISS_BIT_NV);
	shader_stages[shader_index_chit] = create_shader_stage_info(get_shader_module(chit_module), VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	shader_stages[shader_index_chit_debug] = create_shader_stage_info(get_shader_module(chit_debug_module), VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

	/* Setup raytracing shader groups */
	VkRayTracingShaderGroupCreateInfoNV groups[6];
	for (int i = 0; i < 6; ++i) {
	    groups[i] = (VkRayTracingShaderGroupCreateInfoNV){0};
	    groups[i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	    groups[i].generalShader = VK_SHADER_UNUSED_NV;
	    groups[i].closestHitShader = VK_SHADER_UNUSED_NV;
	    groups[i].anyHitShader = VK_SHADER_UNUSED_NV;
	    groups[i].intersectionShader = VK_SHADER_UNUSED_NV;
	}

	// link shaders and types to raytracing shader groups
	groups[INDEX_RAYGEN].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[INDEX_RAYGEN].generalShader = shader_index_raygen;
	
	groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[INDEX_MISS].generalShader = shader_index_miss;
	groups[INDEX_SHADOWMISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[INDEX_SHADOWMISS].generalShader = shader_index_shadowmiss;
	
	groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groups[INDEX_CLOSEST_HIT].generalShader = VK_SHADER_UNUSED_NV;
	groups[INDEX_CLOSEST_HIT].closestHitShader = shader_index_chit;
	groups[INDEX_SHADOWHIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groups[INDEX_SHADOWHIT].generalShader = shader_index_chit_debug; //VK_SHADER_UNUSED_NV;
	//groups[INDEX_SHADOWHIT].closestHitShader = shader_index_shadowmiss;

	groups[INDEX_CLOSEST_HIT_DEBUG].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groups[INDEX_CLOSEST_HIT_DEBUG].generalShader = VK_SHADER_UNUSED_NV;
	groups[INDEX_CLOSEST_HIT_DEBUG].closestHitShader = shader_index_chit_debug;

	VkRayTracingPipelineCreateInfoNV rt_pipeline_info = {};
	rt_pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	rt_pipeline_info.stageCount = sizeof(shader_stages) / sizeof(shader_stages[0]);
	rt_pipeline_info.pStages = shader_stages;
	rt_pipeline_info.groupCount = sizeof(groups) / sizeof(groups[0]);
	rt_pipeline_info.pGroups = groups;
	rt_pipeline_info.maxRecursionDepth = 2;
	rt_pipeline_info.layout = vkal_info.nv_rt_ctx.pipeline_layout;
	DBG_VULKAN_ASSERT(vkCreateRayTracingPipelinesNV(
		vkal_info.device,
		VK_NULL_HANDLE,
		1,
		&rt_pipeline_info,
		NULL,
		&vkal_info.nv_rt_ctx.pipeline),
		"Failed to create RayTracing pipeline."
	);
}

void create_rt_command_buffers()
{
	vkal_info.nv_rt_ctx.command_buffers = (VkCommandBuffer*)malloc(vkal_info.swapchain_image_count * sizeof(VkCommandBuffer));
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandBufferCount = vkal_info.swapchain_image_count;
	allocate_info.commandPool = vkal_info.command_pools[0];
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	DBG_VULKAN_ASSERT( vkAllocateCommandBuffers(
		vkal_info.device,
		&allocate_info,
		vkal_info.nv_rt_ctx.command_buffers),
		"Failed to allocate Raytracing command buffers!" );
}

/* RayTracing Command Buffer generation */
void build_rt_commandbuffers()
{
	VkCommandBufferBeginInfo cmd_buf_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkImageSubresourceRange subresource_rage = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	
	for (int i = 0; i < vkal_info.swapchain_image_count; ++i) {
		vkBeginCommandBuffer(vkal_info.nv_rt_ctx.command_buffers[i], &cmd_buf_info);

		/* Dispatch RayTracing commands */
		vkCmdBindPipeline(vkal_info.nv_rt_ctx.command_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
			vkal_info.nv_rt_ctx.pipeline);
		vkCmdBindDescriptorSets(vkal_info.nv_rt_ctx.command_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
			vkal_info.nv_rt_ctx.pipeline_layout, 0, 2, vkal_info.nv_rt_ctx.descriptor_sets, 0, 0);

		// Calculate shader binding offsets
		VkDeviceSize bind_offset_raygen_shader = vkal_info.nv_rt_ctx.properties.shaderGroupHandleSize*INDEX_RAYGEN;
		VkDeviceSize bind_offset_miss_shader = vkal_info.nv_rt_ctx.properties.shaderGroupHandleSize*INDEX_MISS;
		/* NOTE: INDEX_SHADOWMISS is implied here. INDEX_CLOSEST_HIT value is 3, not 2! */
		VkDeviceSize bind_offset_hit_shader = vkal_info.nv_rt_ctx.properties.shaderGroupHandleSize*INDEX_CLOSEST_HIT;
		VkDeviceSize bind_offset_hit_debug_shader = vkal_info.nv_rt_ctx.properties.shaderGroupHandleSize*INDEX_CLOSEST_HIT_DEBUG;
		VkDeviceSize bind_stride = vkal_info.nv_rt_ctx.properties.shaderGroupHandleSize;
		vkCmdTraceRaysNV(
			vkal_info.nv_rt_ctx.command_buffers[i],
			vkal_info.nv_rt_ctx.shader_binding_table.buffer, bind_offset_raygen_shader,
			vkal_info.nv_rt_ctx.shader_binding_table.buffer, bind_offset_miss_shader, bind_stride,
			vkal_info.nv_rt_ctx.shader_binding_table.buffer, bind_offset_hit_shader, bind_stride,
			VK_NULL_HANDLE, 0, 0,
			VKAL_RT_SIZE_X, VKAL_RT_SIZE_Y, 1);

		/* Copy RayTracing output to the target image */

		// prepare current target image as transfer destination
		set_image_layout(
			vkal_info.nv_rt_ctx.command_buffers[i],
			get_image(vkal_info.nv_rt_ctx.target_image.image),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresource_rage,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		// prepare raytracing output image as transfer source
		set_image_layout(
			vkal_info.nv_rt_ctx.command_buffers[i],
			get_image(vkal_info.nv_rt_ctx.storage_image.image),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			subresource_rage,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		VkImageCopy copy_region = {};
		copy_region.srcSubresource = (VkImageSubresourceLayers){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copy_region.srcOffset = (VkOffset3D){ 0, 0, 0 };
		copy_region.dstSubresource = (VkImageSubresourceLayers){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copy_region.dstOffset = (VkOffset3D){ 0, 0, 0 };
		copy_region.extent = (VkExtent3D){ VKAL_RT_SIZE_X, VKAL_RT_SIZE_Y, 1 };
		vkCmdCopyImage(
			vkal_info.nv_rt_ctx.command_buffers[i],
			get_image(vkal_info.nv_rt_ctx.storage_image.image), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			get_image(vkal_info.nv_rt_ctx.target_image.image), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

		// Transition target image back for sampling
		set_image_layout(
			vkal_info.nv_rt_ctx.command_buffers[i],
			get_image(vkal_info.nv_rt_ctx.target_image.image),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			subresource_rage,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		// Transition RayTracing output image back to to general layout
		set_image_layout(
			vkal_info.nv_rt_ctx.command_buffers[i],
			get_image(vkal_info.nv_rt_ctx.storage_image.image),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
			subresource_rage,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		vkEndCommandBuffer(vkal_info.nv_rt_ctx.command_buffers[i]);
	}
}

VkResult map_memory(Buffer * buffer, VkDeviceSize size, VkDeviceSize offset)
{
	return vkMapMemory(vkal_info.device, buffer->device_memory, offset, size, 0, &((*buffer).mapped));
}

void unmap_memory(Buffer * buffer)
{
	if (buffer->mapped) {
		vkUnmapMemory(vkal_info.device, buffer->device_memory);
		buffer->mapped = NULL;
	}
}

void build_rt_acceleration_structure(VkGeometryNV * geometry, uint32_t geometryNV_count, VkBuffer instance_buffer)
{
	/* Scratch memory needed to store temporary information. */
	VkAccelerationStructureMemoryRequirementsInfoNV mem_requirements_info = {};
	mem_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	mem_requirements_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	VkMemoryRequirements2 mem_req_blas;
	VkDeviceSize biggest_blas_accel = 0;
	for (int i = 0; i < vkal_info.nv_rt_ctx.blas_count; ++i) {
		mem_requirements_info.accelerationStructure = vkal_info.nv_rt_ctx.blas[i].accel_structure;
		vkGetAccelerationStructureMemoryRequirementsNV(vkal_info.device, &mem_requirements_info, &mem_req_blas);
		if (mem_req_blas.memoryRequirements.size > biggest_blas_accel) {
			biggest_blas_accel = mem_req_blas.memoryRequirements.size;
		}
	}

	VkMemoryRequirements2 mem_req_tlas;
	mem_requirements_info.accelerationStructure = vkal_info.nv_rt_ctx.tlas.accel_structure;
	vkGetAccelerationStructureMemoryRequirementsNV(vkal_info.device, &mem_requirements_info, &mem_req_tlas);

	VkDeviceSize scratch_buffer_size = max(biggest_blas_accel, mem_req_tlas.memoryRequirements.size);
	DeviceMemory scratch_memory = vkal_allocate_devicememory(10 * 1024*1024,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	Buffer scratch_buffer = vkal_create_buffer(scratch_buffer_size, &scratch_memory, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV);


	VkMemoryBarrier memory_barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	memory_barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memory_barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	/* Now, build the actual BLAS */
	VkAccelerationStructureInfoNV build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	build_info.geometryCount = 1;
	VkCommandBuffer command_buffer = create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	for (int i = 0; i < geometryNV_count; ++i) {
		build_info.pGeometries = &geometry[i];
		vkCmdBuildAccelerationStructureNV(
			command_buffer,
			&build_info,
			VK_NULL_HANDLE,
			0,
			VK_FALSE,
			vkal_info.nv_rt_ctx.blas[i].accel_structure,
			VK_NULL_HANDLE,
			scratch_buffer.buffer,
			0);

		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			0,
			1,
			&memory_barrier,
			0,
			0,
			0,
			0
		);
	}

	/* Build the actual TLAS */
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	build_info.pGeometries = 0;
	build_info.geometryCount = 0;
	build_info.instanceCount = geometryNV_count;
	vkCmdBuildAccelerationStructureNV(
		command_buffer,
		&build_info,
		instance_buffer,
		0, // instance offset
		VK_FALSE,
		vkal_info.nv_rt_ctx.tlas.accel_structure,
		VK_NULL_HANDLE,
		scratch_buffer.buffer,
		0
	);

	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
		0,
		1,
		&memory_barrier,
		0,
		0,
		0,
		0
	);
	

	flush_command_buffer(command_buffer, vkal_info.graphics_queue, 1);
}

/* Convert vulkan vertex buffer object to Nvidias GeometryNV.
   The BLAS (create_rt_blas) expects the data to be in this format.
*/
/*
VkGeometryNV model_to_geometryNV(Model model)
{
	VkGeometryNV geo = {};
	geo.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	geo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	geo.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	geo.geometry.triangles.vertexData = vkal_info.vertex_buffer.buffer;
	geo.geometry.triangles.vertexOffset = model.offset;
	geo.geometry.triangles.vertexCount = model.vertex_count;
	geo.geometry.triangles.vertexStride = sizeof(Vertex);
	geo.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geo.geometry.triangles.indexData = vkal_info.index_buffer.buffer;
	geo.geometry.triangles.indexOffset = model.index_buffer_offset;
	geo.geometry.triangles.indexCount = model.index_count;
	geo.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geo.geometry.triangles.transformData = VK_NULL_HANDLE;
	geo.geometry.triangles.transformOffset = 0;
	geo.geometry.aabbs = (VkGeometryAABBNV){0};
	geo.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
	geo.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

	return geo;
}
*/

void create_surface()
{
	VkResult result = glfwCreateWindowSurface(vkal_info.instance, vkal_info.window, 0, &vkal_info.surface);
	DBG_VULKAN_ASSERT(result, "failed to create window surface");
}

int check_validation_layer_support(char const * requested_layer, char ** available_layers, int available_layer_count)
{
	char ** current_layer = available_layers;
	int found = 0;
	for (int i = 0; i < available_layer_count; ++i) {
		if (!strcmp(*current_layer, requested_layer)) {
			found = 1;
			break;
		}
		current_layer++;
	}
	return found;
}

SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device)
{
	SwapChainSupportDetails details = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkal_info.surface, &details.capabilities);
    
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkal_info.surface, &format_count, 0);
	details.format_count = format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkal_info.surface, &format_count, details.formats);
    
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkal_info.surface, &present_mode_count, 0);
	details.present_mode_count = present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkal_info.surface, &present_mode_count, details.present_modes);
    
	return details;
}

VkSurfaceFormatKHR choose_swapchain_surface_format(VkSurfaceFormatKHR * available_formats, uint32_t format_count)
{
	VkSurfaceFormatKHR * available_format = available_formats;
	for (int i = 0; i < format_count; ++i) {
		if (available_format->format == VK_FORMAT_R8G8B8A8_UNORM &&
			available_format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return *available_format;
		}
		available_format++;
	}
	return available_formats[0];
}

VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR * available_present_modes, uint32_t present_mode_count)
{
#if !VKAL_VSYNC_ON
	VkPresentModeKHR * available_present_mode = available_present_modes;
	for (int i = 0; i < present_mode_count; ++i) {
		if (*available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return *available_present_mode;
		}
		available_present_mode++;
	}
#endif
	return VK_PRESENT_MODE_FIFO_KHR; // only this mode is guaranteed to exist on _all_ Vk implementations.
}

VkExtent2D choose_swap_extent(VkSurfaceCapabilitiesKHR * capabilities)
{
	if (capabilities->currentExtent.width != UINT32_MAX) {
		return capabilities->currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(vkal_info.window, &width, &height);
		VkExtent2D actual_extent = { width, height };
		//actual_extent.width  = max(capabilities->minImageExtent.width, min(capabilities->maxImageExtent.width, actual_extent.width));
		//actual_extent.height = max(capabilities->minImageExtent.height, min(capabilities->maxImageExtent.height, actual_extent.height));
		return actual_extent;
	}
}

void cleanup_swapchain()
{
	for (int i = 0; i < vkal_info.framebuffer_count; ++i) {
		vkDestroyFramebuffer(vkal_info.device, vkal_info.framebuffers[i], 0);
	}
	kill_array(vkal_info.framebuffers);
    
	vkFreeCommandBuffers(vkal_info.device, vkal_info.command_pools[0], vkal_info.command_buffer_count, vkal_info.command_buffers);
	kill_array(vkal_info.command_buffers);
    
	for (int i = 0; i < vkal_info.swapchain_image_count; ++i) {
		vkDestroyImageView(vkal_info.device, vkal_info.swapchain_image_views[i], 0);
	}
	kill_array(vkal_info.swapchain_image_views);
    
	destroy_image(vkal_info.depth_stencil_image);
	destroy_image_view(vkal_info.depth_stencil_image_view);
    
	vkDestroySwapchainKHR(vkal_info.device, vkal_info.swapchain, 0);
	kill_array(vkal_info.swapchain_images);
}

void recreate_swapchain()
{
	vkDeviceWaitIdle(vkal_info.device);
    
	cleanup_swapchain();
    
	create_swapchain();
	create_image_views();
	create_depth_buffer();
	//create_render_pass();
	//VkPipeline pipeline = vkal_create_graphics_pipeline(vkal_info.uniform_size, vkal_info.uniform_offset, shader_setup);
	create_framebuffer();
	create_command_buffers();
}

void create_swapchain()
{
	SwapChainSupportDetails swap_chain_support = query_swapchain_support(vkal_info.physical_device);
    
	VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swap_chain_support.formats, swap_chain_support.format_count);
	VkPresentModeKHR present_mode = choose_swapchain_present_mode(swap_chain_support.present_modes, swap_chain_support.present_mode_count);
	VkExtent2D extent = choose_swap_extent(&swap_chain_support.capabilities);
    
	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 &&
		image_count > swap_chain_support.capabilities.maxImageArrayLayers) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}
    
	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = vkal_info.surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
	QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
	uint32_t queue_family_indicies[] = { indicies.graphics_family, indicies.present_family };
	if (indicies.graphics_family != indicies.present_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indicies;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = 0;
	}
    
	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;
    
	VkResult result = vkCreateSwapchainKHR(vkal_info.device, &create_info, 0, &vkal_info.swapchain);
	DBG_VULKAN_ASSERT(result, "failed to create swapchain!");
    
	vkGetSwapchainImagesKHR(vkal_info.device, vkal_info.swapchain, &image_count, 0);
	vkal_info.swapchain_image_count = image_count;
	make_array(vkal_info.swapchain_images, VkImage, image_count);
	vkGetSwapchainImagesKHR(vkal_info.device, vkal_info.swapchain, &image_count, vkal_info.swapchain_images);
    
	vkal_info.swapchain_image_format = surface_format.format;
	vkal_info.swapchain_extent = extent;
}

void create_image_views()
{
	make_array(vkal_info.swapchain_image_views, VkImageView, vkal_info.swapchain_image_count);
    
	for (int i = 0; i < vkal_info.swapchain_image_count; ++i) {
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = vkal_info.swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = vkal_info.swapchain_image_format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		VkResult result = vkCreateImageView(vkal_info.device, &create_info, 0, &vkal_info.swapchain_image_views[i]);
		DBG_VULKAN_ASSERT(result, "failed to create image view!");
	}
}

static int images_created = 0;
// TODO: VkImageCreateFlags param seems to be important for cube maps. For normal textures set to 0.
void create_image(uint32_t width, uint32_t height, uint32_t mip_levels, uint32_t array_layers, 
	VkImageCreateFlags flags, VkFormat format, VkImageUsageFlags usage_flags, uint32_t * out_image_id)
{
	QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.extent = (VkExtent3D){ width, height, 1 };
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = format;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.queueFamilyIndexCount = 1;
	image_info.pQueueFamilyIndices = &indicies.graphics_family;
	image_info.usage = usage_flags;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.mipLevels = mip_levels;
	image_info.arrayLayers = array_layers;
	image_info.flags = flags;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	uint32_t free_image_index;
	for (free_image_index = 0; free_image_index < VKAL_MAX_VKIMAGE; ++free_image_index) {
		if (vkal_info.user_images[free_image_index].used) { 
			continue;
		} 
		break;
	}
	VkResult result = vkCreateImage(vkal_info.device, &image_info, 0, &vkal_info.user_images[ free_image_index ].image);
	DBG_VULKAN_ASSERT(result, "failed to create VkImage!");
	vkal_info.user_images[free_image_index].used = 1;
	*out_image_id = free_image_index;
	images_created++;
}

static int images_destroyed = 0;
void destroy_image(uint32_t id)
{
	if (vkal_info.user_images[id].used) {
		vkDestroyImage(vkal_info.device, get_image(id), 0);
		vkal_info.user_images[id].used = 0;
		images_destroyed++;
	}
}

VkImage get_image(uint32_t id)
{
	assert(vkal_info.user_images[id].used > 0);
	return vkal_info.user_images[id].image;
}

void create_image_view(VkImage image,
	VkImageViewType view_type, VkFormat format, VkImageAspectFlags aspect_flags,
	uint32_t base_mip_level, uint32_t mip_level_count, 
	uint32_t base_array_layer, uint32_t array_layer_count,
	uint32_t * out_image_view)
{
	VkImageView image_view;
	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.components = (VkComponentMapping){ VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	view_info.format = format;
	view_info.viewType = view_type;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = base_mip_level;
	view_info.subresourceRange.levelCount = mip_level_count;
	view_info.subresourceRange.baseArrayLayer = base_array_layer;
	view_info.subresourceRange.layerCount = array_layer_count;
	uint32_t free_index;
	for (free_index = 0; free_index < VKAL_MAX_VKDEVICEMEMORY; ++free_index) {
		if (vkal_info.user_image_views[free_index].used) {
			continue;
		}
		break;
	}
	VkResult result = vkCreateImageView(vkal_info.device, &view_info, 0, &vkal_info.user_image_views[free_index].image_view);
	DBG_VULKAN_ASSERT(result, "failed to create VkImageView!");

	*out_image_view = free_index;
	vkal_info.user_image_views[free_index].used = 1;
}

void destroy_image_view(uint32_t id)
{
	if (vkal_info.user_image_views[id].used) {
		vkDestroyImageView(vkal_info.device, vkal_info.user_image_views[id].image_view, 0);
		vkal_info.user_image_views[id].used = 0;
	}
}

VkImageView get_image_view(uint32_t id)
{
	assert(id < VKAL_MAX_VKIMAGEVIEW);
	assert(vkal_info.user_image_views[id].used > 0);
	return vkal_info.user_image_views[id].image_view;
}

VkSampler create_sampler(VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode u,
	VkSamplerAddressMode v, VkSamplerAddressMode w)
{
	VkSampler sampler;
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	/* TODO: make address mode parameterized as we need repeat for raytracing bluenoise and calmp to border for shadow maps! */
	sampler_info.addressModeU = u; // VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_info.addressModeV = v; // VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_info.addressModeW = w; // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.minFilter = min_filter;
	sampler_info.magFilter = mag_filter;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.mipLodBias = 0.f;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = 1.f;
	sampler_info.minLod = 0.f;
	sampler_info.maxLod = 0.f;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	uint32_t id;
	internal_create_sampler(sampler_info, &id);
	return get_sampler(id);
}

void internal_create_sampler(VkSamplerCreateInfo create_info, uint32_t * out_sampler)
{
	uint32_t free_index;
	for (free_index = 0; free_index < VKAL_MAX_VKSAMPLER; ++free_index) {
		if (vkal_info.user_samplers[free_index].used) {
			continue;
		}
		break;
	}
	VkResult result = vkCreateSampler(vkal_info.device, &create_info, 0, &vkal_info.user_samplers[free_index].sampler);
	DBG_VULKAN_ASSERT(result, "failed to create VkSampler!");
	vkal_info.user_samplers[free_index].used = 1;
	*out_sampler = free_index;
}

VkSampler get_sampler(uint32_t id)
{
	assert(id < VKAL_MAX_VKSAMPLER);
	assert(vkal_info.user_samplers[id].used);
	return vkal_info.user_samplers[id].sampler;
}

void destroy_sampler(uint32_t id)
{
	if (vkal_info.user_samplers[id].used) {
		vkDestroySampler(vkal_info.device, get_sampler(id), 0);
		vkal_info.user_samplers[id].used = 0;
	}
}

void vkal_descriptor_set_add_image_sampler(VkDescriptorSet descriptor_set, uint32_t binding, VkImageView image_view, VkSampler sampler)
{
	VkDescriptorImageInfo image_infos[1];
	image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_infos[0].imageView = image_view;
	image_infos[0].sampler = sampler;

	VkWriteDescriptorSet write_set_image = create_write_descriptor_set_image(descriptor_set, binding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image_infos);
	vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_image, 0, 0);
}

void vkal_update_descriptor_set_texture(VkDescriptorSet descriptor_set, Texture texture)
{
	VkDescriptorImageInfo image_infos[1];
	image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_infos[0].imageView = get_image_view(texture.image_view);
	image_infos[0].sampler = texture.sampler;

	VkWriteDescriptorSet write_set_image = create_write_descriptor_set_image(descriptor_set, texture.binding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image_infos);
	vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_image, 0, 0);
}

void vkal_update_descriptor_set_texturearray(VkDescriptorSet descriptor_set, VkDescriptorType descriptor_type, uint32_t binding, uint32_t array_element, Texture texture)
{
	// update floats in fragment shader
	VkDescriptorImageInfo image_infos[1];
	image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_infos[0].imageView = get_image_view(texture.image_view);
	image_infos[0].sampler = texture.sampler; /* This is the offset _WITHIN_ the buffer, not its VkDeviceMemory! */
	VkWriteDescriptorSet write_set_uniform = create_write_descriptor_set_image2(
		descriptor_set,
		binding, array_element, 1,
		descriptor_type, image_infos);

	vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_uniform, 0, 0);
}

Texture vkal_create_texture(uint32_t binding,
                            unsigned char * texture_data, uint32_t width, uint32_t height, uint32_t channels, 
							VkImageCreateFlags flags, VkImageViewType view_type,
                            uint32_t base_mip_level, uint32_t mip_level_count, 
							uint32_t base_array_layer, uint32_t array_layer_count,
                            VkFilter min_filter, VkFilter mag_filter)
{
	Texture texture = {};
	texture.width = width;
	texture.height = height;
	texture.channels = channels;
	create_image(width, height, mip_level_count, array_layer_count, flags, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		&texture.image);
    
	// Back the image with actual memory:
	VkMemoryRequirements image_memory_requirements = {};
	vkGetImageMemoryRequirements(vkal_info.device, get_image(texture.image), &image_memory_requirements);
	uint32_t mem_type_bits = check_memory_type_index(image_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	create_device_memory(image_memory_requirements.size, mem_type_bits, &texture.device_memory_id);
	VkResult result = vkBindImageMemory(vkal_info.device, get_image(texture.image), get_device_memory(texture.device_memory_id), 0);
	DBG_VULKAN_ASSERT(result, "failed to bind texture image memory!");
    
	create_image_view(get_image(texture.image), view_type,
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,
		base_mip_level, mip_level_count, 
		base_array_layer, array_layer_count,
		&texture.image_view);
	texture.sampler = create_sampler(min_filter, mag_filter, 
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	texture.binding = binding;
	
	upload_texture(get_image(texture.image), width, height, channels, array_layer_count, texture_data);
    
	return texture;
}

void create_offscreen_descriptor_set(VkDescriptorSetLayout descriptor_set_layout, uint32_t binding)
{
	vkal_info.offscreen_pass.descriptor_sets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet)); /* TODO: make sure this is configurable, or use render image feature! */
	vkal_allocate_descriptor_sets(vkal_info.descriptor_pool, &descriptor_set_layout, 1, &vkal_info.offscreen_pass.descriptor_sets);
	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	image_info.imageView = get_image_view(vkal_info.offscreen_pass.image_view);
	image_info.sampler = vkal_info.offscreen_pass.depth_sampler;
	vkal_info.offscreen_pass.image_descriptor = image_info;
	VkWriteDescriptorSet write_set = create_write_descriptor_set_image(vkal_info.offscreen_pass.descriptor_sets[0], binding, 1, 
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &vkal_info.offscreen_pass.image_descriptor);
	vkUpdateDescriptorSets(vkal_info.device, 1, &write_set, 0, 0);
}

// TODO: can we use this buffer for any kind of data to stage??
void create_staging_buffer(uint32_t size)
{
	vkal_info.staging_buffer = create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
#ifdef _DEBUG
	vkal_dbg_buffer_name(vkal_info.staging_buffer, "Global Staging Buffer");
#endif
	// Allocate staging buffer memory
	VkMemoryRequirements buffer_memory_requirements = {};
	vkGetBufferMemoryRequirements(vkal_info.device, vkal_info.staging_buffer.buffer, &buffer_memory_requirements);
	uint32_t mem_type_bits = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	vkal_info.device_memory_staging = allocate_memory(buffer_memory_requirements.size, mem_type_bits);
	VkResult result = vkBindBufferMemory(vkal_info.device, vkal_info.staging_buffer.buffer, vkal_info.device_memory_staging, 0);
	DBG_VULKAN_ASSERT(result, "failed to bind memory");
}

DeviceMemory vkal_allocate_devicememory(uint32_t size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags)
{
	/* Create a dummy buffer so we can select the best possible memory for this type of buffer via vkGetBufferMemoryRequirements */
	VkBuffer buffer = create_buffer(size, buffer_usage_flags).buffer;
	VkMemoryRequirements buffer_memory_requirements = {};
	vkGetBufferMemoryRequirements(vkal_info.device, buffer, &buffer_memory_requirements);
	uint32_t mem_type_bits = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, memory_property_flags);

	VkDeviceMemory memory;
	VkMemoryAllocateInfo memory_info = {};
	memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_info.allocationSize = buffer_memory_requirements.size;
	memory_info.memoryTypeIndex = mem_type_bits;
	DBG_VULKAN_ASSERT(vkAllocateMemory(vkal_info.device, &memory_info, 0, &memory), "failed to allocate device memory.");

	DeviceMemory device_memory = {};
	device_memory.vk_device_memory = memory;
	device_memory.size = buffer_memory_requirements.size;
	device_memory.alignment = buffer_memory_requirements.alignment;
	device_memory.free = 0;
	return device_memory;
}

Buffer vkal_create_buffer(uint32_t size, DeviceMemory * device_memory, VkBufferUsageFlags buffer_usage_flags)
{
	VkBuffer vk_buffer;
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
	buffer_info.pQueueFamilyIndices = &indicies.graphics_family;
	buffer_info.queueFamilyIndexCount = 1;
	buffer_info.usage = buffer_usage_flags;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	DBG_VULKAN_ASSERT( vkCreateBuffer(vkal_info.device, &buffer_info, 0, &vk_buffer), "Failed to create VkBuffer" );
	DBG_VULKAN_ASSERT( vkBindBufferMemory(vkal_info.device, vk_buffer, device_memory->vk_device_memory, device_memory->free ),
		"Failed to bind VkBuffer to VkDeviceMemory" );
	/* NOTE: the offset in vkBindBufferMemory must be a multiple of alignment returend by vkGetBufferMemoryRequirements and denotes the
	   offset into VkDeviceMemory.
	*/
	
	Buffer result = {};
	result.size = size;
	result.offset = device_memory->free;
	result.device_memory = device_memory->vk_device_memory;
	result.usage = buffer_usage_flags;
	result.buffer = vk_buffer;
	result.mapped = NULL;

	VkDeviceSize alignment = device_memory->alignment;
	VkDeviceSize next_offset = device_memory->free + (size / alignment) * alignment;
	next_offset += size % alignment ? alignment : 0;
	device_memory->free = next_offset;

	return result;
}

void vkal_dbg_buffer_name(Buffer buffer, char const * name)
{
#ifdef _DEBUG
	VkDebugUtilsObjectNameInfoEXT obj_info = {};
	obj_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	obj_info.objectType = VK_OBJECT_TYPE_BUFFER;
	obj_info.objectHandle = (uint64_t)buffer.buffer;
	obj_info.pObjectName = name;
	DBG_VULKAN_ASSERT(vkSetDebugUtilsObjectNameEXT(vkal_info.device, &obj_info),
		"Failed to create debug name for Buffer");
#endif
}

void vkal_dbg_image_name(VkImage image, char const * name)
{
#ifdef _DEBUG
	VkDebugUtilsObjectNameInfoEXT obj_info = {};
	obj_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	obj_info.objectType = VK_OBJECT_TYPE_IMAGE;
	obj_info.objectHandle = (uint64_t)image;
	obj_info.pObjectName = name;
	DBG_VULKAN_ASSERT(vkSetDebugUtilsObjectNameEXT(vkal_info.device, &obj_info),
		"Failed to create debug name for Buffer");
#endif
}

void vkal_update_buffer(Buffer buffer, uint8_t* data)
{
	void * mapped_memory = 0;
	DBG_VULKAN_ASSERT( vkMapMemory(
		vkal_info.device, buffer.device_memory, 
		buffer.offset, buffer.size, 
		0,
		&mapped_memory),
		"Failed to map memory!"
	);

	memcpy(mapped_memory, data, sizeof(Buffer));
	VkMappedMemoryRange memory_range = {};
	memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memory_range.memory = buffer.device_memory;
	memory_range.offset = buffer.offset;
	memory_range.size = VK_WHOLE_SIZE; // TODO: figure out how much we need to flush, really.
	DBG_VULKAN_ASSERT( vkFlushMappedMemoryRanges(vkal_info.device, 1, &memory_range),
		"Failed to flush mapped memory!" );
}

void upload_texture(VkImage const image, uint32_t w, uint32_t h, uint32_t n, uint32_t array_layer_count, unsigned char * texture_data)
{
	// Copy image data to staging buffer
	void * staging_buffer;
	vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, array_layer_count*w*h*4, 0, &staging_buffer);
	memcpy(staging_buffer, texture_data, array_layer_count*w*h*4);
	VkMappedMemoryRange flush_range = {};
	flush_range.memory = vkal_info.device_memory_staging;
	flush_range.offset = 0;
	flush_range.size = array_layer_count*w*h*4;
	flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	vkFlushMappedMemoryRanges(vkal_info.device, 1, &flush_range);
	vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);
    
	//////////////////////////////////
	//////////////////////////////////
    
	// Actual upload to GPU
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	for (int i = 0; i < vkal_info.command_buffer_count; ++i) {
		vkBeginCommandBuffer(vkal_info.command_buffers[i], &begin_info);
        
		VkImageSubresourceRange image_subresource_range = {};
		image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_subresource_range.layerCount = array_layer_count;
		image_subresource_range.baseArrayLayer = 0;
		image_subresource_range.levelCount = 1;
		image_subresource_range.baseMipLevel = 0;
        
		VkImageMemoryBarrier image_memory_barrier_undef_to_transfer = {};
		image_memory_barrier_undef_to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier_undef_to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier_undef_to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrier_undef_to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier_undef_to_transfer.image = image;
		image_memory_barrier_undef_to_transfer.subresourceRange = image_subresource_range;
		vkCmdPipelineBarrier(vkal_info.command_buffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &image_memory_barrier_undef_to_transfer);
        
		VkBufferImageCopy copy_info = {};
		copy_info.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_info.imageSubresource.baseArrayLayer = 0;
		copy_info.imageSubresource.layerCount = array_layer_count;
		copy_info.imageSubresource.mipLevel = 0;
		copy_info.bufferOffset = 0;
		copy_info.bufferImageHeight = 0;
		copy_info.bufferRowLength = 0;
		copy_info.imageOffset = (VkOffset3D){ 0, 0, 0 };
		copy_info.imageExtent = (VkExtent3D){ w, h, 1 };
		vkCmdCopyBufferToImage(vkal_info.command_buffers[i], vkal_info.staging_buffer.buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);
        
		VkImageMemoryBarrier image_memory_barrier_transfer_to_shader_read = {};
		image_memory_barrier_transfer_to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier_transfer_to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier_transfer_to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier_transfer_to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_memory_barrier_transfer_to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_memory_barrier_transfer_to_shader_read.image = image;
		image_memory_barrier_transfer_to_shader_read.subresourceRange = image_subresource_range;
		vkCmdPipelineBarrier(vkal_info.command_buffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &image_memory_barrier_transfer_to_shader_read);
        
		vkEndCommandBuffer(vkal_info.command_buffers[i]);
        
	}
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = vkal_info.command_buffer_count;
	submit_info.pCommandBuffers = vkal_info.command_buffers;
	vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkDeviceWaitIdle(vkal_info.device);	
}

void create_depth_buffer()
{
	{
		create_image(
			vkal_info.swapchain_extent.width, vkal_info.swapchain_extent.height,
			1, 1, 0,
			VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			&vkal_info.depth_stencil_image);
	}
    
	{
		// Check how much size is required for this image. Calculating by hand _might_ work, but who knows what
		// the GPU is doing behind the scenes with an OPTIMAL TILING set! Alignment stuff might blow up the whole thing when done manually!
		VkMemoryRequirements image_memory_requirements;
		vkGetImageMemoryRequirements(vkal_info.device, get_image(vkal_info.depth_stencil_image), &image_memory_requirements);
        
		// TODO: check if I am doing this property query correctly!!!!!!!
		// Check what LunarG is doing in their samples!
		uint32_t mem_type_index = check_memory_type_index(image_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		create_device_memory(image_memory_requirements.size, mem_type_index, &vkal_info.device_memory_depth_stencil);
		vkBindImageMemory(vkal_info.device, get_image(vkal_info.depth_stencil_image), get_device_memory(vkal_info.device_memory_depth_stencil), 0);
	}
    
	{
		// depth stencil image view
		create_image_view(get_image(vkal_info.depth_stencil_image), 
			VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT,
			0, 1,
			0, 1,
			&vkal_info.depth_stencil_image_view);
	}
}

uint32_t check_memory_type_index(uint32_t const memory_requirement_bits, VkMemoryPropertyFlags const wanted_property)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(vkal_info.physical_device, &memory_properties);
	uint32_t mem_type_index = 0;
	uint32_t found = 0;
	uint32_t type_bits = memory_requirement_bits;
	for (; mem_type_index < memory_properties.memoryTypeCount; ++mem_type_index) {
		if (type_bits & 1) {
			if ((memory_properties.memoryTypes[mem_type_index].propertyFlags & wanted_property) == wanted_property) {
				found = 1;
				break;
			}
		}
		type_bits >>= 1;
	}
	assert(found == 1);
    
	return mem_type_index;
}

int rate_device(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);
    
	switch (device_properties.deviceType)
	{
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        {
            return 1000;
        } break;
        
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        {
            return 500;
        } break;
        
        default: return 0;
	}
}

int check_device_extension_support(VkPhysicalDevice device, char ** extensions, uint32_t extension_count)
{
	int * extensions_left;
	make_array(extensions_left, int, extension_count);
	memset(extensions_left, 1, extension_count * sizeof(int));
	printf("\nquery device extensions:\n");
	uint32_t supported_extension_count = 0;
	vkEnumerateDeviceExtensionProperties(device, 0, &supported_extension_count, 0);
	VkExtensionProperties * available_extensions = 0;
	make_array(available_extensions, VkExtensionProperties, supported_extension_count);
	vkEnumerateDeviceExtensionProperties(device, 0, &supported_extension_count, available_extensions);
	int extensions_found = 0;
	for (int i = 0; i < supported_extension_count; ++i) {
		for (int k = 0; k < extension_count; ++k) {
			if (!strcmp(available_extensions[i].extensionName, extensions[k])) {
				printf("requested device extension: %s found!\n", extensions[k]);
				extensions_found++;
				extensions_left[k] = 0;
				goto gt_next_extension;
			}
		}
        gt_next_extension:;
	}
	for (int i = 0; i < extension_count; ++i) {
		if (extensions_left[i]) {
			printf("requested extension %s not available!\n", extensions[i]);
		}
	}
	kill_array(extensions_left);
	kill_array(available_extensions);
	return extension_count == extensions_found;
}

int is_device_suitable(VkPhysicalDevice device, char ** extensions, uint32_t extension_count)
{
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(device, &device_features);
	SwapChainSupportDetails swapchain_support = query_swapchain_support(device);
	int swapchain_adequate = 1;
	if (!swapchain_support.formats || !swapchain_support.present_modes) {
		swapchain_adequate = 0;
	}
	// NOTE: only test for swapchain support after the extension support has been checked!
	return check_device_extension_support(device, extensions, extension_count) && swapchain_adequate;
}

QueueFamilyIndicies find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	// Just look for a queue family that can do graphics for now
	QueueFamilyIndicies indicies;
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
	VkQueueFamilyProperties * queue_families;
	make_array(queue_families, VkQueueFamilyProperties, queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
	indicies.has_graphics_family = 0;
	for (int i = 0; i < queue_family_count; ++i) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indicies.graphics_family = i;
			indicies.has_graphics_family = 1;
			break;
		}
	}
	for (int i = 0; i < queue_family_count; ++i) {
		VkBool32 present_support = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
		if (present_support) {
			indicies.has_present_family = 1;
			indicies.present_family = i;
			break;
		}
	}
	return indicies;
}

void pick_physical_device(char ** extensions, uint32_t extension_count)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(vkal_info.instance, &device_count, 0);
	if (!device_count) {
		printf("No GPU with Vulkan support found\n");
		exit(-1);
	}
	VkPhysicalDevice * physical_devices = 0;
	make_array(physical_devices, VkPhysicalDevice, device_count);
	vkEnumeratePhysicalDevices(vkal_info.instance, &device_count, physical_devices);
	int current_best_device = 0;
	for (int i = 0; i < device_count; ++i) {
		VkPhysicalDeviceProperties physical_device_property = {};
		vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_property);
		printf("physical device found: %s\n", physical_device_property.deviceName);
		if (is_device_suitable(physical_devices[i], extensions, extension_count), extensions, extension_count) {
			int current_device_rating = rate_device(physical_devices[i]);
			if (current_device_rating > current_best_device) {
				QueueFamilyIndicies indicies = find_queue_families(physical_devices[i], vkal_info.surface);
				if (indicies.has_graphics_family && indicies.has_present_family) {
					vkal_info.physical_device = physical_devices[i];
					vkal_info.physical_device_properties = physical_device_property;
					current_best_device = current_device_rating;
				}
			}
		}
	}
	if (vkal_info.physical_device == VK_NULL_HANDLE) {
		printf("failed to find suitable GPU\n");
		exit(-1);
	}
	printf("device picked: %s\n", vkal_info.physical_device_properties.deviceName);
}

void create_logical_device(char ** extensions, uint32_t extension_count)
{
	QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
	uint32_t unique_queue_families[2] = { indicies.graphics_family, indicies.present_family };
	VkDeviceQueueCreateInfo queue_create_infos[2];
	uint32_t info_count = 0;
	if (indicies.graphics_family != indicies.present_family) {
		info_count = 2;
	}
	else {
		info_count = 1;
	}
	float queue_prio = 1.f;
	for (int i = 0; i < info_count; ++i) {
	    queue_create_infos[i] = (VkDeviceQueueCreateInfo){0};
		queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[i].queueFamilyIndex = unique_queue_families[i];
		queue_create_infos[i].queueCount = 1;
		queue_create_infos[i].pQueuePriorities = &queue_prio;
	}
    
	VkPhysicalDeviceFeatures device_features = {};
	VkPhysicalDeviceDescriptorIndexingFeatures indexing_features = {};
	indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	indexing_features.runtimeDescriptorArray = VK_TRUE;
	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pNext = &indexing_features;
	create_info.pQueueCreateInfos = queue_create_infos;
	create_info.queueCreateInfoCount = info_count;
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extensions;
	// device specific validation layers are deprecated.
	// just specify for compatib. reasons:
	if (vkal_info.enable_validation_layers) {
		create_info.enabledLayerCount = array_length(validation_layers);
		create_info.ppEnabledLayerNames = validation_layers;
	}
	else {
		create_info.enabledLayerCount = 0;
	}
    
	VkResult result = vkCreateDevice(vkal_info.physical_device, &create_info, 0, &vkal_info.device);
	DBG_VULKAN_ASSERT(result, "failed to create logical device");
    
	vkGetDeviceQueue(vkal_info.device, indicies.graphics_family, 0, &vkal_info.graphics_queue);
	vkGetDeviceQueue(vkal_info.device, indicies.present_family, 0, &vkal_info.present_queue);
	//kill_array(queue_create_infos);
}

void create_shader_module(uint8_t const * shader_byte_code, int size, uint32_t * out_shader_module)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = size;
	create_info.pCode = (uint32_t*)shader_byte_code;
	uint32_t free_index;
	for (free_index = 0; free_index < VKAL_MAX_VKSHADERMODULE; ++free_index) {
		if (vkal_info.user_shader_modules[free_index].used) {
		} 
		else break;
	}
	VkResult result = vkCreateShaderModule(vkal_info.device, &create_info, 0, &vkal_info.user_shader_modules[free_index].shader_module);
	DBG_VULKAN_ASSERT(result, "failed to create shader module!");
	vkal_info.user_shader_modules[free_index].used = 1;
	*out_shader_module = free_index;
}

VkShaderModule get_shader_module(uint32_t id)
{
	assert(id < VKAL_MAX_VKSHADERMODULE);
	assert(vkal_info.user_shader_modules[id].used);
	return vkal_info.user_shader_modules[id].shader_module;
}

void destroy_shader_module(uint32_t id)
{
	if (vkal_info.user_shader_modules[id].used) {
		vkDestroyShaderModule(vkal_info.device, get_shader_module(id), 0);
		vkal_info.user_shader_modules[id].used = 0;
	}
}

void create_descriptor_pool()
{
	VkDescriptorPoolSize pool_sizes[] =
	{
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,     // type of the resource
			10							           // number of descriptors of that type to be stored in the pool. This is per set maybe?
		},
		{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			10 // TODO(Michael): figure out why we must have one additional available even if we're only using two textures!
		},
		{ // for sampling the shadow map
			VK_DESCRIPTOR_TYPE_SAMPLER,
			10
		}
	};
    
	VkDescriptorPoolCreateInfo descriptor_pool_info = {};
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptor_pool_info.maxSets = 20; // NOTE: This is an arbitrary number at the moment. We don't _have_ to use all of them.
	descriptor_pool_info.poolSizeCount = sizeof(pool_sizes)/sizeof(*pool_sizes);
	descriptor_pool_info.pPoolSizes = pool_sizes;
    
	VkResult  result = vkCreateDescriptorPool(vkal_info.device, &descriptor_pool_info, 0, &vkal_info.descriptor_pool);
	DBG_VULKAN_ASSERT(result, "failed to create descriptor pool!");
}

void create_offscreen_render_pass()
{
	VkAttachmentDescription attachments[] = 
	{
		// Depth Attachment for Shadow map
		{
			0,
			VK_FORMAT_D32_SFLOAT,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,						// layout before rendering
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL // final layout: going to read from it in the second subpass
		}
	};

	VkAttachmentReference depth_reference =
	{
		0, // This is the index into the above array of attachments
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // translate layout into a layout so that the GPU can use efficient memory access.	
	};

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pDepthStencilAttachment = &depth_reference;

	// dependency actually not used here
	VkSubpassDependency dependencies[2] = {};
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = attachments;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 0;// sizeof(dependencies) / sizeof(*dependencies);
	info.pDependencies = 0; //dependencies;

	VkResult result = vkCreateRenderPass(vkal_info.device, &info, 0, &vkal_info.offscreen_pass.render_pass);
}

void create_offscreen_framebuffer()
{
	vkal_info.offscreen_pass.width  = VKAL_SHADOW_MAP_DIMENSION;
	vkal_info.offscreen_pass.height = VKAL_SHADOW_MAP_DIMENSION;
	
	VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;

	create_image(VKAL_SHADOW_MAP_DIMENSION, VKAL_SHADOW_MAP_DIMENSION, 1, 1, 0,
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		&vkal_info.offscreen_pass.image);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(vkal_info.device, get_image(vkal_info.offscreen_pass.image), &memory_requirements);
	uint32_t mem_type_index = check_memory_type_index(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	create_device_memory(memory_requirements.size, mem_type_index, &vkal_info.offscreen_pass.device_memory);
	VkResult result = vkBindImageMemory(vkal_info.device, get_image(vkal_info.offscreen_pass.image), get_device_memory(vkal_info.offscreen_pass.device_memory), 0);
	DBG_VULKAN_ASSERT(result, "failed to bind offscreen image to offscreen memory!");

	create_image_view(get_image(vkal_info.offscreen_pass.image),
		VK_IMAGE_VIEW_TYPE_2D, 
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT,
		0, 1, 
		0, 1,
		&vkal_info.offscreen_pass.image_view);

	vkal_info.offscreen_pass.depth_sampler = create_sampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, 
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = vkal_info.offscreen_pass.render_pass;
	info.attachmentCount = 1;
	VkImageView image_view = get_image_view(vkal_info.offscreen_pass.image_view);
	info.pAttachments = &image_view;
	info.width = vkal_info.offscreen_pass.width;
	info.height = vkal_info.offscreen_pass.height;
	info.layers = 1;
	internal_create_framebuffer(info, &vkal_info.offscreen_pass.framebuffer);
}
/*
void build_shadow_command_buffers(Model * models, uint32_t model_draw_count, VkPipeline pipeline, VkPipelineLayout pipeline_layout,
	VkDescriptorSet * descriptor_sets, uint32_t first_set, uint32_t descriptor_set_count)
{
	VkCommandBufferBeginInfo cmd_buffer_info = {};
	cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//cmd_buffer_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkClearValue clear_values[2];
	for (int i = 0; i < vkal_info.offscreen_pass.command_buffer_count; ++i) {
	    clear_values[0].depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 };
		vkBeginCommandBuffer(vkal_info.offscreen_pass.command_buffers[i], &cmd_buffer_info);
		VkRenderPassBeginInfo render_pass_info = (VkRenderPassBeginInfo){};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = vkal_info.offscreen_pass.render_pass;
		render_pass_info.framebuffer = get_framebuffer(vkal_info.offscreen_pass.framebuffer);
		render_pass_info.renderArea.extent.width = vkal_info.offscreen_pass.width;
		render_pass_info.renderArea.extent.height = vkal_info.offscreen_pass.height;
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = clear_values;

		vkCmdBeginRenderPass(vkal_info.offscreen_pass.command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		
		VkViewport viewport = {};
		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.width = vkal_info.swapchain_extent.width; // (float)vp_width;
		viewport.height = vkal_info.swapchain_extent.height; // (float)vp_height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;
		vkCmdSetViewport(vkal_info.offscreen_pass.command_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0,0 };
		scissor.extent = vkal_info.swapchain_extent;
		vkCmdSetScissor(vkal_info.offscreen_pass.command_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(vkal_info.offscreen_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		vkCmdBindDescriptorSets(vkal_info.offscreen_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, first_set, descriptor_set_count, descriptor_sets, 0, 0);
		
		for (int j = 0; j < model_draw_count; ++j) {
			Model model = models[j]; 
			vkCmdBindVertexBuffers(vkal_info.offscreen_pass.command_buffers[i], 0, 1, &vkal_info.vertex_buffer.buffer, &model.offset);
			vkCmdDraw(vkal_info.offscreen_pass.command_buffers[i], model.vertex_count, 1, 0, 0); // TODO: Model input
		}
		vkCmdEndRenderPass(vkal_info.offscreen_pass.command_buffers[i]);
		vkEndCommandBuffer(vkal_info.offscreen_pass.command_buffers[i]);
	}
}

void update_shadow_command_buffer(uint32_t image_id, Model * models, uint32_t model_draw_count, VkPipeline pipeline, VkPipelineLayout pipeline_layout,
	VkDescriptorSet * descriptor_sets, uint32_t first_set, uint32_t descriptor_set_count)
{
	VkCommandBufferBeginInfo cmd_buffer_info = {};
	cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//cmd_buffer_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkClearValue clear_values[2];
	
	clear_values[0].depthStencil = { 1.0f, 0 };
	vkBeginCommandBuffer(vkal_info.offscreen_pass.command_buffers[image_id], &cmd_buffer_info);
	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = vkal_info.offscreen_pass.render_pass;
	render_pass_info.framebuffer = get_framebuffer(vkal_info.offscreen_pass.framebuffer);
	render_pass_info.renderArea.extent.width = vkal_info.offscreen_pass.width;
	render_pass_info.renderArea.extent.height = vkal_info.offscreen_pass.height;
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(vkal_info.offscreen_pass.command_buffers[image_id], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = vkal_info.offscreen_pass.width; // (float)vp_width;
	viewport.height = vkal_info.offscreen_pass.height; // (float)vp_height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(vkal_info.offscreen_pass.command_buffers[image_id], 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent.width = vkal_info.offscreen_pass.width;
	scissor.extent.height = vkal_info.offscreen_pass.height;
	vkCmdSetScissor(vkal_info.offscreen_pass.command_buffers[image_id], 0, 1, &scissor);

	vkCmdBindPipeline(vkal_info.offscreen_pass.command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	vkCmdBindDescriptorSets(vkal_info.offscreen_pass.command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, first_set, descriptor_set_count, descriptor_sets, 0, 0);

	for (int j = 0; j < model_draw_count; ++j) {
		Model model = models[j];
		vkCmdBindVertexBuffers(vkal_info.offscreen_pass.command_buffers[image_id], 0, 1, &vkal_info.vertex_buffer.buffer, &model.offset);
		vkCmdPushConstants(vkal_info.offscreen_pass.command_buffers[image_id], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)&model.model_matrix);
		vkCmdDraw(vkal_info.offscreen_pass.command_buffers[image_id], model.vertex_count, 1, 0, 0); // TODO: Model input
	}
	vkCmdEndRenderPass(vkal_info.offscreen_pass.command_buffers[image_id]);
	vkEndCommandBuffer(vkal_info.offscreen_pass.command_buffers[image_id]);
	
}
*/

void create_render_to_image_render_pass()
{
	VkAttachmentDescription attachments[] =
	{
		// Color Attachment
		{
			0,
			vkal_info.swapchain_image_format,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED, // layout before rendering
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // final layout
		},
		// Depth Stencil Attachment
		{
			0,
			VK_FORMAT_D32_SFLOAT,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	};

	VkAttachmentReference color_attachment_refs[] =
	{
		{
			0, // This is the index into the above array of color_attachments
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // translate layout into a layout so that the GPU can use efficient memory access.
		}
	};

	VkAttachmentReference depth_attachment_ref =
	{
		1,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpasses[] =
	{
		{
			0, // flags
			VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
			0, // inputAttachmentCount
			0, // pInputAttachments
			1, // colorAttachmentCount
			color_attachment_refs, // pColorAttachments
			0, // pResolveAttachments
			&depth_attachment_ref, // pDepthStencilAttachment
			0, // preserveAttachmentCount
			0 // pPreserveAttachments
		}
	};

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // refers to implicit subpass before/after renderpass
	dependency.dstSubpass = 0; // index into the (only) subpass created above
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = subpasses;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;
	VkResult result = vkCreateRenderPass(vkal_info.device, &render_pass_info, 0, &vkal_info.render_to_image_render_pass);
	DBG_VULKAN_ASSERT(result, "failed to create render pass!");
}

void create_render_pass()
{
	VkAttachmentDescription attachments[] =
	{
		// Color Attachment
		{
			0,
			vkal_info.swapchain_image_format,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED, // layout before rendering
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // final layout
		},
		// Depth Stencil Attachment
		{
			0,
			VK_FORMAT_D32_SFLOAT,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		} 
	};
    
	VkAttachmentReference color_attachment_refs[] =
	{
		{
			0, // This is the index into the above array of color_attachments
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // translate layout into a layout so that the GPU can use efficient memory access.
		}
	};
    
	VkAttachmentReference depth_attachment_ref =
	{
		1,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
    
	VkSubpassDescription subpasses[] =
	{
		{
			0, // flags
			VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
			0, // inputAttachmentCount
			0, // pInputAttachments
			1, // colorAttachmentCount
			color_attachment_refs, // pColorAttachments
			0, // pResolveAttachments
			&depth_attachment_ref, // pDepthStencilAttachment
			0, // preserveAttachmentCount
			0 // pPreserveAttachments
		}
	};
    
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // refers to implicit subpass before/after renderpass
	dependency.dstSubpass = 0; // index into the (only) subpass created above
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = subpasses;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;
	VkResult result = vkCreateRenderPass(vkal_info.device, &render_pass_info, 0, &vkal_info.render_pass);
	DBG_VULKAN_ASSERT(result, "failed to create render pass!");
    
    
	// ImGui Renderpass
	{
		VkResult err;
		// Create the Render Pass
		{
			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = VK_FORMAT_D32_SFLOAT;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            
			VkAttachmentDescription attachment = {};
			attachment.format = vkal_info.swapchain_image_format;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			
			VkAttachmentReference color_attachment = {};
			color_attachment.attachment = 0;
			color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachment;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
            
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            
			VkAttachmentDescription attachments[] = { attachment, depthAttachment };
			VkRenderPassCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount = 2;
			info.pAttachments = attachments;
			info.subpassCount = 1;
			info.pSubpasses = &subpass;
			info.dependencyCount = 1;
			info.pDependencies = &dependency;
			err = vkCreateRenderPass(vkal_info.device, &info, NULL, &vkal_info.imgui_render_pass);
			DBG_VULKAN_ASSERT(err, "ImGui Renderpass could not be created!");
		}
	}
}

/* Renderpass uses raytracing output as input attachment */
void create_rt_renderpass()
{
	VkAttachmentDescription attachments[] = {
		// Color Attachment
		{
			0,
			vkal_info.swapchain_image_format,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED, // layout before rendering
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // final layout
		},
		// Depth Stencil Attachment
		{
			0,
			VK_FORMAT_D32_SFLOAT,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		},
		{ /* Input Attachments: output from Raytracer */
			0,
			vkal_info.swapchain_image_format,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,               // layout before rendering
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // final layout
		},
	};

	VkAttachmentReference swapchain_image_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depth_ref = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkAttachmentReference input_image_ref = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
}

void create_framebuffer()
{
	VkImageView attachments[2];
    // The order matches the order of the VkAttachmentDescriptions of the Renderpass.
    
	uint32_t framebuffer_count = 0;
	make_array(vkal_info.framebuffers, VkFramebuffer, vkal_info.swapchain_image_count);
	for (int i = 0; i < vkal_info.swapchain_image_count; ++i) {
		attachments[0] = vkal_info.swapchain_image_views[i]; 
		attachments[1] = get_image_view(vkal_info.depth_stencil_image_view);
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = vkal_info.render_pass;
		framebuffer_info.width = vkal_info.swapchain_extent.width;
		framebuffer_info.height = vkal_info.swapchain_extent.height;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.attachmentCount = 2; // matches the VkAttachmentDescription array-size in renderpass
		framebuffer_info.layers = 1;
		VkResult result = vkCreateFramebuffer(vkal_info.device, &framebuffer_info, 0, &vkal_info.framebuffers[i]);
		DBG_VULKAN_ASSERT(result, "failed to create framebuffer!");
		framebuffer_count++;
	}
	vkal_info.framebuffer_count = framebuffer_count;
}



uint32_t create_render_image_framebuffer(RenderImage render_image, uint32_t width, uint32_t height)
{
	VkFramebuffer framebuffer;
	VkImageView attachments[2];
	attachments[0] = get_image_view(render_image.image_view);
	attachments[1] = get_image_view(render_image.depth_image.image_view);
	VkFramebufferCreateInfo framebuffer_info = {};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.renderPass = vkal_info.render_to_image_render_pass;
	framebuffer_info.width = width;
	framebuffer_info.height = height;
	framebuffer_info.pAttachments = attachments;
	framebuffer_info.attachmentCount = 2; // matches the VkAttachmentDescription array-size in renderpass
	framebuffer_info.layers = 1;
	uint32_t id;
	internal_create_framebuffer(framebuffer_info, &id);
	return id;
}

void internal_create_framebuffer(VkFramebufferCreateInfo create_info, uint32_t * out_framebuffer)
{
	uint32_t free_index;
	for (free_index = 0; free_index < VKAL_MAX_VKFRAMEBUFFER; ++free_index) {
		if (vkal_info.user_framebuffers[free_index].used) {
			continue;
		}
		break;
	}
	VkResult result = vkCreateFramebuffer(vkal_info.device, &create_info, 0, &vkal_info.user_framebuffers[free_index].framebuffer);
	DBG_VULKAN_ASSERT(result, "failed to create VkFramebuffer!");
	vkal_info.user_framebuffers[free_index].used = 1;
	*out_framebuffer = free_index;
}

VkFramebuffer get_framebuffer(uint32_t id)
{
	assert(id < VKAL_MAX_VKFRAMEBUFFER);
	assert(vkal_info.user_framebuffers[id].used);
	return vkal_info.user_framebuffers[id].framebuffer;
}

void destroy_framebuffer(uint32_t id)
{
	if (vkal_info.user_framebuffers[id].used) {
		vkDestroyFramebuffer(vkal_info.device, get_framebuffer(id), 0);
		vkal_info.user_framebuffers[id].used = 0;
	}
}

RenderImage recreate_render_image(RenderImage render_image, uint32_t width, uint32_t height)
{
	vkDeviceWaitIdle(vkal_info.device);

	destroy_framebuffer(render_image.framebuffer);

	destroy_image(render_image.image);
	destroy_image(render_image.depth_image.image);

	destroy_image_view(render_image.image_view);
	destroy_image_view(render_image.depth_image.image_view);
	destroy_device_memory(render_image.device_memory);
	destroy_device_memory(render_image.depth_image.device_memory);

	RenderImage new_render_image = create_render_image(width, height);
	return new_render_image;
}

VkPipelineLayout vkal_create_pipeline_layout(VkDescriptorSetLayout * descriptor_set_layouts, uint32_t descriptor_set_layout_count, VkPushConstantRange * push_constant_ranges, uint32_t push_constant_range_count)
{
	uint32_t id;
	create_pipeline_layout(descriptor_set_layouts, descriptor_set_layout_count,
		push_constant_ranges, push_constant_range_count, &id);
	return get_pipeline_layout(id);
}

void create_pipeline_layout(
	VkDescriptorSetLayout * descriptor_set_layouts, uint32_t descriptor_set_layout_count,
	VkPushConstantRange * push_constant_ranges, uint32_t push_constant_range_count,
	uint32_t * out_pipeline_layout)
{
	VkPipelineLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.pSetLayouts = descriptor_set_layouts;
	layout_info.setLayoutCount = descriptor_set_layout_count;
	layout_info.pushConstantRangeCount = push_constant_range_count;
	layout_info.pPushConstantRanges = push_constant_ranges;
	uint32_t free_index = 0;
	for (free_index = 0; free_index < VKAL_MAX_VKPIPELINELAYOUT; ++free_index) {
		if (vkal_info.user_pipeline_layouts[free_index].used) {

		}
		else break;
	}
	VkResult result = vkCreatePipelineLayout(vkal_info.device, &layout_info, 0, &vkal_info.user_pipeline_layouts[free_index].pipeline_layout);
	DBG_VULKAN_ASSERT(result, "failed to create pipeline layout!");
	vkal_info.user_pipeline_layouts[free_index].used = 1;
	*out_pipeline_layout = free_index;
}

void destroy_pipeline_layout(uint32_t id)
{
	if (vkal_info.user_pipeline_layouts[id].used) {
		vkDestroyPipelineLayout(vkal_info.device, get_pipeline_layout(id), 0);
		vkal_info.user_pipeline_layouts[id].used = 0;
	}
}

VkPipelineLayout get_pipeline_layout(uint32_t id)
{
	assert(id < VKAL_MAX_VKPIPELINELAYOUT);
	assert(vkal_info.user_pipeline_layouts[id].used);
	return vkal_info.user_pipeline_layouts[id].pipeline_layout;
}

void vkal_allocate_descriptor_sets(VkDescriptorPool pool, VkDescriptorSetLayout * layout, uint32_t layout_count, VkDescriptorSet ** out_descriptor_set)
{
	VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = pool;
	allocate_info.pSetLayouts = layout;
	allocate_info.descriptorSetCount = layout_count;
	VkResult result = vkAllocateDescriptorSets(vkal_info.device, &allocate_info, *out_descriptor_set);
	DBG_VULKAN_ASSERT(result, "failed to allocate descriptor set(s)!");
}

ShaderStageSetup vkal_create_shaders(const uint8_t * vertex_shader_code, uint32_t vertex_shader_code_size, const uint8_t * fragment_shader_code, uint32_t fragment_shader_code_size)
{
	ShaderStageSetup shader_setup = {};
	create_shader_module(vertex_shader_code, vertex_shader_code_size, &shader_setup.vert_shader_module);
	create_shader_module(fragment_shader_code, fragment_shader_code_size, &shader_setup.frag_shader_module);
    
	shader_setup.vertex_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_setup.vertex_shader_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_setup.vertex_shader_create_info.module = get_shader_module(shader_setup.vert_shader_module);
	shader_setup.vertex_shader_create_info.pName = "main";
    
	shader_setup.fragment_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_setup.fragment_shader_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_setup.fragment_shader_create_info.module = get_shader_module(shader_setup.frag_shader_module);
	shader_setup.fragment_shader_create_info.pName = "main";
    
	return shader_setup;
}

VkPipelineShaderStageCreateInfo create_shader_stage_info(VkShaderModule module, VkShaderStageFlagBits shader_stage_flag_bits)
{
	VkPipelineShaderStageCreateInfo shader_stage_info = {};
	shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_info.stage = shader_stage_flag_bits;
	shader_stage_info.module = module;
	shader_stage_info.pName = "main";
	return shader_stage_info;
}

VkDescriptorSetLayout vkal_create_descriptor_set_layout(VkDescriptorSetLayoutBinding * layout, uint32_t binding_count)
{
	uint32_t id;
	create_descriptor_set_layout(layout, binding_count, &id);
	return get_descriptor_set_layout(id);
}

void create_descriptor_set_layout(VkDescriptorSetLayoutBinding * layout, uint32_t binding_count, uint32_t * out_descriptor_set_layout)
{
	DescriptorSetLayout set_layout = {};
	VkDescriptorSetLayoutCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = binding_count;
	info.pBindings = layout;
	uint32_t free_index;
	for (free_index = 0; free_index < VKAL_MAX_VKDESCRIPTORSETLAYOUT; ++free_index) {
		if (vkal_info.user_descriptor_set_layouts[free_index].used) {

		}
		else break;
	}
	VkResult result = vkCreateDescriptorSetLayout(vkal_info.device, &info, 0, &vkal_info.user_descriptor_set_layouts[free_index].descriptor_set_layout);
	DBG_VULKAN_ASSERT(result, "failed to create descriptor set layout(s)!");
	vkal_info.user_descriptor_set_layouts[free_index].used = 1;
	*out_descriptor_set_layout = free_index;
}

VkDescriptorSetLayout get_descriptor_set_layout(uint32_t id)
{
	assert(id < VKAL_MAX_VKDESCRIPTORSETLAYOUT);
	assert(vkal_info.user_descriptor_set_layouts[id].used);
	return vkal_info.user_descriptor_set_layouts[id].descriptor_set_layout;
}

void destroy_descriptor_set_layout(uint32_t id)
{
	if (vkal_info.user_descriptor_set_layouts[id].used) {
		vkDestroyDescriptorSetLayout(vkal_info.device, get_descriptor_set_layout(id), 0);
		vkal_info.user_descriptor_set_layouts[id].used = 0;
	}
}

void vkal_destroy_graphics_pipeline(VkPipeline pipeline)
{
	vkDestroyPipeline(vkal_info.device, pipeline, 0);
}

VkPipeline vkal_create_graphics_pipeline(ShaderStageSetup shader_setup, 
                                         VkBool32 depth_test_enable, VkCompareOp depth_compare_op, 
                                         VkCullModeFlags cull_mode,
                                         VkPolygonMode polygon_mode,
                                         VkPrimitiveTopology primitive_topology,
                                         VkFrontFace face_winding, VkRenderPass render_pass,
					 VkPipelineLayout pipeline_layout)
{    
    VkPipelineShaderStageCreateInfo shader_stages_infos[] = { shader_setup.vertex_shader_create_info, shader_setup.fragment_shader_create_info };
	
    //
    VkVertexInputBindingDescription vertex_input_bindings[] =
	{
	    { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
	};
    
    VkVertexInputAttributeDescription vertex_attributes[] =
	{
	    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },								                            // position
	    { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) },					                            // UV
	    { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec2) + sizeof(vec3) },                        // normal
	    { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(vec3) + sizeof(vec2) + sizeof(vec3) },                   // color
	    { 4, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3) + sizeof(vec2) + sizeof(vec3) + sizeof(vec4) } //  tangent
	};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = vertex_input_bindings;
    vertex_input_info.vertexAttributeDescriptionCount = 5;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;
    //
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {0};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = primitive_topology;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport = {0};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = vkal_info.swapchain_extent.width;
    viewport.height = vkal_info.swapchain_extent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){ 0,0 };
    scissor.extent = vkal_info.swapchain_extent;
    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pViewports = &viewport;
    viewport_state.viewportCount = 1;
    viewport_state.pScissors = &scissor;
    viewport_state.scissorCount = 1;
    
    VkPipelineRasterizationStateCreateInfo rasterizer_info = {0};
    rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.frontFace = face_winding;
    rasterizer_info.cullMode = cull_mode;
    rasterizer_info.polygonMode = polygon_mode;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.depthBiasEnable = VK_FALSE;
    rasterizer_info.lineWidth = 1.f;
    
    // COME BACK LATER: Set up depth/stencil testing (we need to create a buffer for that as it is
    // not created with the Swapchain
    
    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkPipelineColorBlendStateCreateInfo color_blending_info = {};
    color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_info.logicOpEnable = VK_FALSE; // enabling this will set color_blend_attachment.blendEnable to VK_FALSE!
    color_blending_info.pAttachments = &color_blend_attachment;
    color_blending_info.attachmentCount = 1; // must match the attachment count of render subpass!
    // it affects ALL framebuffers
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
    depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_info.depthTestEnable = depth_test_enable;
    depth_stencil_info.depthCompareOp = depth_compare_op;
    depth_stencil_info.depthWriteEnable = VK_TRUE;
    depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_info.stencilTestEnable = VK_FALSE;
    depth_stencil_info.back.failOp = VK_STENCIL_OP_KEEP;
    depth_stencil_info.back.passOp = VK_STENCIL_OP_KEEP;
    depth_stencil_info.back.compareOp = depth_compare_op;
    depth_stencil_info.back.compareMask = 0;
    depth_stencil_info.back.reference = 0;
    depth_stencil_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
    depth_stencil_info.back.writeMask = 0;
    depth_stencil_info.front = depth_stencil_info.back;
    
    VkPipelineMultisampleStateCreateInfo ms_info = {};
    ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_info.sampleShadingEnable = VK_FALSE;
    ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // must match renderpass's color attachment
    
    // dynamic state will force us to provide viewport dimensions and linewidth at drawing-time
    VkDynamicState dynamic_states[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pDynamicStates = dynamic_states;
    dynamic_state_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(*dynamic_states);
    
    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages_infos;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &ms_info;
    pipeline_info.pColorBlendState = &color_blending_info;
    pipeline_info.pDepthStencilState = &depth_stencil_info;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    
    uint32_t id;
    create_graphics_pipeline(pipeline_info, &id);
    return get_graphics_pipeline(id);
}

void create_graphics_pipeline(VkGraphicsPipelineCreateInfo create_info, uint32_t * out_graphics_pipeline)
{
    uint32_t free_index;
    for (free_index = 0; free_index < VKAL_MAX_VKPIPELINE; ++free_index) {
	if (vkal_info.user_pipelines[free_index].used) {

	}
	else break;
    }
    VkResult result = vkCreateGraphicsPipelines(vkal_info.device, VK_NULL_HANDLE, 1, &create_info, 0, &vkal_info.user_pipelines[free_index].pipeline);
    DBG_VULKAN_ASSERT(result, "failed to create graphics pipeline!");
    vkal_info.user_pipelines[free_index].used = 1;
    *out_graphics_pipeline = free_index;
}

VkPipeline get_graphics_pipeline(uint32_t id)
{
    assert(id < VKAL_MAX_VKPIPELINE);
    assert(vkal_info.user_pipelines[id].used);
    return vkal_info.user_pipelines[id].pipeline;
}

void destroy_graphics_pipeline(uint32_t id)
{
    if (vkal_info.user_pipelines[id].used) {
	vkDestroyPipeline(vkal_info.device, get_graphics_pipeline(id), 0);
	vkal_info.user_pipelines[id].used = 0;
    }
}

VkWriteDescriptorSet create_write_descriptor_set_image(VkDescriptorSet dst_descriptor_set, uint32_t dst_binding,
                                                       uint32_t count, VkDescriptorType type, VkDescriptorImageInfo * image_info)
{
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = dst_descriptor_set;
    write_set.dstBinding = dst_binding;
    write_set.descriptorCount = count;
    write_set.descriptorType = type;
    write_set.pImageInfo = image_info;
	
    return write_set;
}

VkWriteDescriptorSet create_write_descriptor_set_image2(VkDescriptorSet dst_descriptor_set, uint32_t dst_binding, uint32_t array_element,
							uint32_t count, VkDescriptorType type, VkDescriptorImageInfo * image_info)
{
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = dst_descriptor_set;
    write_set.dstBinding = dst_binding;
    write_set.descriptorCount = count;
    write_set.descriptorType = type;
    write_set.pImageInfo = image_info;
    write_set.dstArrayElement = array_element;

    return write_set;
}

VkWriteDescriptorSet create_write_descriptor_set_buffer(VkDescriptorSet dst_descriptor_set, uint32_t dst_binding,
                                                        uint32_t count, VkDescriptorType type, VkDescriptorBufferInfo * buffer_info)
{
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = dst_descriptor_set;
    write_set.dstBinding = dst_binding;
    write_set.descriptorCount = count;
    write_set.descriptorType = type;
    write_set.pBufferInfo = buffer_info;
    
    return write_set;
}

VkWriteDescriptorSet create_write_descriptor_set_buffer2(VkDescriptorSet dst_descriptor_set, uint32_t dst_binding, uint32_t dst_array_element,
							 uint32_t count, VkDescriptorType type, VkDescriptorBufferInfo * buffer_info)
{
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = dst_descriptor_set;
    write_set.dstBinding = dst_binding;
    write_set.dstArrayElement = dst_array_element;
    write_set.descriptorCount = count;
    write_set.descriptorType = type;
    write_set.pBufferInfo = buffer_info;

    return write_set;
}

void create_command_pool()
{
    VkCommandPoolCreateInfo cmdpool_info = {};
    cmdpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
    if (indicies.has_graphics_family && indicies.has_present_family) {
	if (indicies.graphics_family != indicies.present_family) {
	    cmdpool_info.queueFamilyIndex = indicies.graphics_family;
	    VkResult result = vkCreateCommandPool(vkal_info.device, &cmdpool_info, 0, &vkal_info.command_pools[0]);
	    DBG_VULKAN_ASSERT(result, "failed to create command pool for graphics family");
            
	    cmdpool_info.queueFamilyIndex = indicies.present_family;
	    result = vkCreateCommandPool(vkal_info.device, &cmdpool_info, 0, &vkal_info.command_pools[1]);
	    DBG_VULKAN_ASSERT(result, "failed to create command pool for present family");
	    vkal_info.commandpool_count = 2;
	}
	else {
	    cmdpool_info.queueFamilyIndex = indicies.graphics_family;
	    VkResult result = vkCreateCommandPool(vkal_info.device, &cmdpool_info, 0, &vkal_info.command_pools[0]);
	    DBG_VULKAN_ASSERT(result, "failed to create command pool for both present and graphics families");
	    vkal_info.commandpool_count = 1;
	}
    }
    else {
	printf("no graphics and present queues available!\n");
	getchar();
	exit(-1);
    }
}

VkCommandBuffer create_command_buffer(VkCommandBufferLevel cmd_buffer_level, uint32_t begin)
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = vkal_info.command_pools[0];
    alloc_info.level = cmd_buffer_level;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    DBG_VULKAN_ASSERT(vkAllocateCommandBuffers(vkal_info.device, &alloc_info,
					       &command_buffer), "Failed to allocate command buffer from pool");

    if (begin) {
	VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	DBG_VULKAN_ASSERT(vkBeginCommandBuffer(command_buffer, &begin_info),
			  "Failed to begin command buffer recording");
    }

    return command_buffer;
}

void create_command_buffers()
{
    // Regular
    {
	make_array(vkal_info.command_buffers, VkCommandBuffer, vkal_info.framebuffer_count);
	vkal_info.command_buffer_count = vkal_info.framebuffer_count;
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandBufferCount = vkal_info.command_buffer_count;
	allocate_info.commandPool = vkal_info.command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers(vkal_info.device, &allocate_info, vkal_info.command_buffers);
    }
    // ImGui
    {
	make_array(vkal_info.command_buffers_imgui, VkCommandBuffer, vkal_info.framebuffer_count);
	vkal_info.command_buffer_imgui_count = vkal_info.framebuffer_count;
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandBufferCount = vkal_info.command_buffer_imgui_count;
	allocate_info.commandPool = vkal_info.command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers(vkal_info.device, &allocate_info, vkal_info.command_buffers_imgui);
    }
    // Offscreen Rendering (Shadow Map)
    {
	make_array(vkal_info.offscreen_pass.command_buffers, VkCommandBuffer, vkal_info.framebuffer_count);
	vkal_info.offscreen_pass.command_buffer_count = vkal_info.framebuffer_count;
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandBufferCount = vkal_info.offscreen_pass.command_buffer_count;
	allocate_info.commandPool = vkal_info.command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers(vkal_info.device, &allocate_info, vkal_info.offscreen_pass.command_buffers);
    }
}

void vkal_begin(uint32_t image_id, VkCommandBuffer command_buffer, VkRenderPass render_pass)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffer, &begin_info);
    
    VkRenderPassBeginInfo pass_begin_info = {};
    pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_begin_info.renderPass = render_pass;
    pass_begin_info.framebuffer = vkal_info.framebuffers[image_id];
    pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
    pass_begin_info.renderArea.extent = vkal_info.swapchain_extent;
    //VkClearValue clear_values[] = { { .2f, .2f, .2f, 1.f },{ 1.f, 0.f } };
    VkClearValue clear_values[] = { { 1.f, 1.f, 1.f, 1.f },{ 1.f, 0.f } };

    pass_begin_info.clearValueCount = 2;
    pass_begin_info.pClearValues = clear_values;
    vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vkal_begin_render_pass(uint32_t image_id, VkCommandBuffer command_buffer, VkRenderPass render_pass)
{
    VkRenderPassBeginInfo pass_begin_info = {0};
    pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_begin_info.renderPass = render_pass;
    pass_begin_info.framebuffer = vkal_info.framebuffers[image_id];
    pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
    pass_begin_info.renderArea.extent = vkal_info.swapchain_extent;
    //VkClearValue clear_values[] = { { .2f, .2f, .2f, 1.f },{ 1.f, 0.f } };
    VkClearValue clear_values[] = { { 1.f, 1.f, 1.f, 1.f },{ 1.f, 0.f } };

    pass_begin_info.clearValueCount = 2;
    pass_begin_info.pClearValues = clear_values;
    vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vkal_begin_command_buffer(VkCommandBuffer command_buffer)
{
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffer, &begin_info);
}

void vkal_render_to_image(uint32_t image_id, VkCommandBuffer command_buffer, VkRenderPass render_pass, RenderImage render_image)
{
    VkRenderPassBeginInfo pass_begin_info = {0};
    pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_begin_info.renderPass = render_pass;
    pass_begin_info.framebuffer = get_framebuffer(render_image.framebuffer);
    pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
    VkExtent2D extent = { render_image.width, render_image.height };
    pass_begin_info.renderArea.extent = extent;
    //VkClearValue clear_values[] = { { .2f, .2f, .2f, 1.f },{ 1.f, 0.f } };
    VkClearValue clear_values[] = { { 1.f, 1.f, 1.f, 1.f },{ 1.f, 0.f } };

    pass_begin_info.clearValueCount = 2;
    pass_begin_info.pClearValues = clear_values;
    vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vkal_end_renderpass(VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
}

void vkal_end_command_buffer(VkCommandBuffer command_buffer)
{
    VkResult result = vkEndCommandBuffer(command_buffer);
    DBG_VULKAN_ASSERT(result, "failed to end command buffer");
}

void vkal_end(VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
    VkResult result = vkEndCommandBuffer(command_buffer);
    DBG_VULKAN_ASSERT(result, "failed to end command buffer");
}

void vkal_bind_descriptor_set(uint32_t image_id, uint32_t first_set, VkDescriptorSet * descriptor_sets, uint32_t descriptor_set_count, VkPipelineLayout pipeline_layout)
{
    vkCmdBindDescriptorSets(vkal_info.command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            pipeline_layout, first_set, descriptor_set_count, descriptor_sets, 0, 0);
}


/* Draw models. If you use this it is expected to use the predefined push constants for materials and model-matrices! */
/*
  void vkal_record_models_pc(
  uint32_t image_id, VkPipeline pipeline, VkPipelineLayout pipeline_layout,
  Model * models, uint32_t model_draw_count)
  {
  vkCmdBindPipeline(vkal_info.command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
  VkViewport viewport = {};
  viewport.x = 0.f;
  viewport.y = 0.f;
  viewport.width = vkal_info.swapchain_extent.width; // (float)vp_width;
  viewport.height = vkal_info.swapchain_extent.height; // (float)vp_height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;
  vkCmdSetViewport(vkal_info.command_buffers[image_id], 0, 1, &viewport);
    
  VkRect2D scissor = {};
  scissor.offset = { 0,0 };
  scissor.extent = vkal_info.swapchain_extent;
  vkCmdSetScissor(vkal_info.command_buffers[image_id], 0, 1, &scissor);
    
  //vkCmdCopyBuffer(vkal_info.command_buffers[image_id], vkal_info.src_buffer, vkal_info.dst_buffer, regioncount, pRegions);
  for (int i = 0; i < model_draw_count; ++i) {
  Model model = models[i];
  vkCmdPushConstants(vkal_info.command_buffers[image_id], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)&(model.model_matrix));
  vkCmdPushConstants(vkal_info.command_buffers[image_id], pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(Material), (void*)&(model.material));
  uint64_t vertex_buffer_offsets[] = { model.offset };
  VkBuffer vertex_buffers[] = { vkal_info.vertex_buffer.buffer };
  vkCmdBindVertexBuffers(vkal_info.command_buffers[image_id], 0, 1, vertex_buffers, vertex_buffer_offsets);
  vkCmdDraw(vkal_info.command_buffers[image_id], model.vertex_count, 1, 0, 0);
  }
  }
*/

/* Records a model without the predefined material and model matrix push constants */
/*
  void vkal_record_models(
  uint32_t image_id, VkPipeline pipeline, VkPipelineLayout pipeline_layout,
  UniformBuffer * uniforms, uint32_t uniform_count,
  Model * models, uint32_t model_draw_count)
  {
  vkCmdBindPipeline(vkal_info.command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkViewport viewport = {};
  viewport.x = 0.f;
  viewport.y = 0.f;
  viewport.width = vkal_info.swapchain_extent.width; // (float)vp_width;
  viewport.height = vkal_info.swapchain_extent.height; // (float)vp_height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;
  vkCmdSetViewport(vkal_info.command_buffers[image_id], 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = { 0,0 };
  scissor.extent = vkal_info.swapchain_extent;
  vkCmdSetScissor(vkal_info.command_buffers[image_id], 0, 1, &scissor);

  //vkCmdCopyBuffer(vkal_info.command_buffers[image_id], vkal_info.src_buffer, vkal_info.dst_buffer, regioncount, pRegions);
  for (int i = 0; i < model_draw_count; ++i) {
  Model model = models[i];
  uint64_t vertex_buffer_offsets[] = { model.offset };
  VkBuffer vertex_buffers[] = { vkal_info.vertex_buffer.buffer };
  vkCmdBindVertexBuffers(vkal_info.command_buffers[image_id], 0, 1, vertex_buffers, vertex_buffer_offsets);
  vkCmdDraw(vkal_info.command_buffers[image_id], model.vertex_count, 1, 0, 0);
  }
  }
*/

/* Same as vkal_record_models but lets user specify width, height of Viewport. This is needed for offscreen render targets
   that do not meet the swapchain-images extent.
*/
/*
  void vkal_record_models_dimensions(
  uint32_t image_id, VkPipeline pipeline, VkPipelineLayout pipeline_layout,
  UniformBuffer * uniforms, uint32_t uniform_count,
  Model * models, uint32_t model_draw_count, float width, float height)
  {
  vkCmdBindPipeline(vkal_info.command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkViewport viewport = {};
  viewport.x = 0.f;
  viewport.y = 0.f;
  viewport.width = width; // (float)vp_width;
  viewport.height = height; // (float)vp_height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;
  vkCmdSetViewport(vkal_info.command_buffers[image_id], 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = { 0,0 };
  scissor.extent = { uint32_t(width), uint32_t(height) };
  vkCmdSetScissor(vkal_info.command_buffers[image_id], 0, 1, &scissor);

  //vkCmdCopyBuffer(vkal_info.command_buffers[image_id], vkal_info.src_buffer, vkal_info.dst_buffer, regioncount, pRegions);
  for (int i = 0; i < model_draw_count; ++i) {
  Model model = models[i];
  vkCmdPushConstants(vkal_info.command_buffers[image_id], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)&(model.model_matrix));
  vkCmdPushConstants(vkal_info.command_buffers[image_id], pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(Material), (void*)&(model.material));
  uint64_t vertex_buffer_offsets[] = { model.offset };
  VkBuffer vertex_buffers[] = { vkal_info.vertex_buffer.buffer };
  vkCmdBindVertexBuffers(vkal_info.command_buffers[image_id], 0, 1, vertex_buffers, vertex_buffer_offsets);
  vkCmdDraw(vkal_info.command_buffers[image_id], model.vertex_count, 1, 0, 0);
  }
  }
*/

/*
  void vkal_record_models_indexed(
  uint32_t image_id, VkPipeline pipeline, VkPipelineLayout pipeline_layout,
  UniformBuffer * uniforms, uint32_t uniform_count,
  Model * models, uint32_t model_draw_count)
  {
  vkCmdBindPipeline(vkal_info.command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
  VkViewport viewport = {};
  viewport.x = 0.f;
  viewport.y = 0.f;
  viewport.width = vkal_info.swapchain_extent.width; // (float)vp_width;
  viewport.height = vkal_info.swapchain_extent.height; // (float)vp_height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;
  vkCmdSetViewport(vkal_info.command_buffers[image_id], 0, 1, &viewport);
    
  VkRect2D scissor = {};
  scissor.offset = { 0,0 };
  scissor.extent = vkal_info.swapchain_extent;
  vkCmdSetScissor(vkal_info.command_buffers[image_id], 0, 1, &scissor);
    
  //vkCmdCopyBuffer(vkal_info.command_buffers[image_id], vkal_info.src_buffer, vkal_info.dst_buffer, regioncount, pRegions);
  for (int i = 0; i < model_draw_count; ++i) {
  Model model = models[i];
  //vkCmdPushConstants(vkal_info.command_buffers[image_id], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)&(model.model_matrix));
  //vkCmdPushConstants(vkal_info.command_buffers[image_id], pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(Material), (void*)&(model.material));
  vkCmdBindIndexBuffer(vkal_info.command_buffers[image_id], vkal_info.index_buffer.buffer, model.index_buffer_offset, VK_INDEX_TYPE_UINT16);
  uint64_t vertex_buffer_offsets[] = { model.offset };
  VkBuffer vertex_buffers[] = { vkal_info.vertex_buffer.buffer };
  vkCmdBindVertexBuffers(vkal_info.command_buffers[image_id], 0, 1, vertex_buffers, vertex_buffer_offsets);
  vkCmdDrawIndexed(vkal_info.command_buffers[image_id], model.index_count, 1, 0, 0, 0);
  }
  }
*/


uint32_t vkal_get_image()
{
    uint32_t image_index;
    // don't actually wait for the semaphore here. just associate it with this operation. check when needed during vkQueueSubmit
    VkResult result = vkAcquireNextImageKHR(vkal_info.device, vkal_info.swapchain, UINT64_MAX, vkal_info.present_complete_semaphore, VK_NULL_HANDLE, &image_index);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
	vkal_info.should_recreate_swapchain = 0;
	recreate_swapchain();
	return -1; // TODO: return -1 here. This image is useless when too old. User has to check for this!
    }
    else {
	// TODO
    }
    
    return image_index;
}

void vkal_queue_submit(VkCommandBuffer * command_buffers, uint32_t command_buffer_count)
{
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = { vkal_info.present_complete_semaphore };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores; // wait until image is available from swapchain ringbuffer
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = command_buffer_count;
    submit_info.pCommandBuffers = command_buffers;
    VkSemaphore signal_semaphores[] = { vkal_info.render_complete_semaphore };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    VkResult result = vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    DBG_VULKAN_ASSERT(result, "Failed to submit command buffer to queue!");
}

void offscreen_buffers_submit(uint32_t image_id)
{
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vkal_info.offscreen_pass.command_buffers[image_id];
    VkResult result = vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    DBG_VULKAN_ASSERT(result, "failed to submit offscreen command buffers!");
    //vkDeviceWaitIdle(vkal_info.device); // wait until stuff is on GPU
}

void vkal_present(uint32_t image_id)
{
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    VkSemaphore signal_semaphores[] = { vkal_info.render_complete_semaphore };
    present_info.pWaitSemaphores = signal_semaphores;
    VkSwapchainKHR swap_chains[] = { vkal_info.swapchain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_id;
    VkResult result = vkQueuePresentKHR(vkal_info.present_queue, &present_info);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vkal_info.should_recreate_swapchain) {
	vkal_info.should_recreate_swapchain = 0;
	recreate_swapchain();
	return;
    }
    else {
	// TODO
    }
    
    vkal_info.frames_rendered++;
    vkQueueWaitIdle(vkal_info.graphics_queue);
}

void create_semaphores()
{
    /*make_array(vkal_info.image_in_flight_fences, VkFence, vkal_info.swapchain_image_count);
      for (int i = 0; i < VKAL_MAX_IMAGES_IN_FLIGHT; ++i) {
      {
      VkSemaphoreCreateInfo semaphore_info = {};
      semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      VkResult result = vkCreateSemaphore(vkal_info.device, &semaphore_info, 0, &vkal_info.image_available_semaphores[i]);
      DBG_VULKAN_ASSERT(result, "failed to create image-available semaphore");
      }
      {
      VkSemaphoreCreateInfo semaphore_info = {};
      semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      VkResult result = vkCreateSemaphore(vkal_info.device, &semaphore_info, 0, &vkal_info.render_finished_semaphores[i]);
      DBG_VULKAN_ASSERT(result, "failed to create render-finished semaphore");
      }
      {
      VkFenceCreateInfo fenceInfo;
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.pNext = NULL;
      fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      VkResult result = vkCreateFence(vkal_info.device, &fenceInfo, NULL, &vkal_info.draw_fences[i]);
      DBG_VULKAN_ASSERT(result, "failed to create draw-fence");
      }
      }*/
    //memset(vkal_info.image_in_flight_fences, VK_NULL_HANDLE, vkal_info.swapchain_image_count * sizeof(VkFence));
    
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(vkal_info.device, &sem_info, 0, &vkal_info.present_complete_semaphore);
    vkCreateSemaphore(vkal_info.device, &sem_info, 0, &vkal_info.render_complete_semaphore);
    
    
}

void allocate_device_memory_uniform()
{
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(vkal_info.device, vkal_info.uniform_buffer.buffer, &buffer_memory_requirements);
    uint32_t mem_type_index = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, 
						      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkal_info.device_memory_uniform = allocate_memory(buffer_memory_requirements.size, mem_type_index);
    
    VkResult result = vkBindBufferMemory(vkal_info.device, vkal_info.uniform_buffer.buffer, vkal_info.device_memory_uniform, 0); // the last param is the memory offset!
    DBG_VULKAN_ASSERT(result, "failed to bind uniform buffer to device memory!");
}

void allocate_device_memory_vertex()
{
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(vkal_info.device, vkal_info.vertex_buffer.buffer, &buffer_memory_requirements);
    uint32_t mem_type_index = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkal_info.device_memory_vertex = allocate_memory(buffer_memory_requirements.size, mem_type_index);
    
    VkResult result = vkBindBufferMemory(vkal_info.device, vkal_info.vertex_buffer.buffer, vkal_info.device_memory_vertex, 0);
    DBG_VULKAN_ASSERT(result, "failed to bind vertex buffer memory!");
    
}

void allocate_device_memory_index()
{
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(vkal_info.device, vkal_info.index_buffer.buffer, &buffer_memory_requirements);
    uint32_t mem_type_index = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkal_info.device_memory_index = allocate_memory(buffer_memory_requirements.size, mem_type_index);
    
    VkResult result = vkBindBufferMemory(vkal_info.device, vkal_info.index_buffer.buffer, vkal_info.device_memory_index, 0);
    DBG_VULKAN_ASSERT(result, "failed to bind vertex buffer memory!");
}

void create_uniform_buffer(uint32_t size)
{
    vkal_info.uniform_buffer = create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
#ifdef _DEBUG
    vkal_dbg_buffer_name(vkal_info.uniform_buffer, "Global Uniform Buffer");
#endif
}

void vkal_update_uniform(UniformBuffer * uniform_buffer, void * data)
{
    void * mapped_uniform_memory = 0;
    VkResult result = vkMapMemory(
	vkal_info.device, 
	vkal_info.device_memory_uniform, 
	uniform_buffer->offset, uniform_buffer->size, 
	0, 
	&mapped_uniform_memory);
    DBG_VULKAN_ASSERT(result, "failed to map device memory");
	
    //memset(mapped_device_memory, 0, size);
    memcpy(mapped_uniform_memory, data, uniform_buffer->size);
    VkMappedMemoryRange flush_range = {
	VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
	0,
	vkal_info.device_memory_uniform,
	uniform_buffer->offset,
	VK_WHOLE_SIZE // 2*sizeof(float) does not work due to allignment issues. TODO: figure this out. (must be multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize)
    };
    result = vkFlushMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    DBG_VULKAN_ASSERT(result, "failed to flush mapped memory range(s)!");
    result = vkInvalidateMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    DBG_VULKAN_ASSERT(result, "failed to invalidate mapped memory range(s)!");
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_uniform); // invalidated _all_ previously acquired pointers via vkMapMemory
}

void create_device_memory(uint32_t size, uint32_t mem_type_bits, uint32_t * out_memory_id)
{
    uint32_t free_index;
    for (free_index = 0; free_index < VKAL_MAX_VKDEVICEMEMORY; ++free_index) {
	if (vkal_info.user_device_memory[free_index].used) {
	    continue;
	}
	break;
    }
    vkal_info.user_device_memory[free_index].device_memory = allocate_memory(size, mem_type_bits);
    vkal_info.user_device_memory[free_index].used = 1;
    *out_memory_id = free_index;
}

VkDeviceMemory get_device_memory(uint32_t id)
{
    assert(id < VKAL_MAX_VKDEVICEMEMORY);
    assert(vkal_info.user_device_memory[id].used);
    return vkal_info.user_device_memory[id].device_memory;
}

void destroy_device_memory(uint32_t id)
{
    if (vkal_info.user_device_memory[id].used) {
	vkFreeMemory(vkal_info.device, get_device_memory(id), 0);
	vkal_info.user_device_memory[id].used = 0;
    }
}


static int memory_allocs = 0;
VkDeviceMemory allocate_memory(uint32_t size, uint32_t mem_type_bits)
{
    memory_allocs++;
    VkDeviceMemory memory;
    VkMemoryAllocateInfo memory_info_image = {};
    memory_info_image.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info_image.allocationSize = size;
    memory_info_image.memoryTypeIndex = mem_type_bits;
    VkResult result = vkAllocateMemory(vkal_info.device, &memory_info_image, 0, &memory);
    DBG_VULKAN_ASSERT(result, "failed to allocate memory!");
    return memory;
}

Buffer create_buffer(uint32_t size, VkBufferUsageFlags usage)
{
    VkBuffer vk_buffer;
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
    buffer_info.pQueueFamilyIndices = &indicies.graphics_family;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateBuffer(vkal_info.device, &buffer_info, 0, &vk_buffer);
    
    Buffer buffer = {};
    buffer.size = size;
    buffer.usage = usage;
    buffer.buffer = vk_buffer;
    return buffer;
}

void create_vertex_buffer(uint32_t size)
{
    vkal_info.vertex_buffer = create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkal_info.vertex_buffer_offset = 0;
#ifdef _DEBUG
    vkal_dbg_buffer_name(vkal_info.vertex_buffer, "Global Vertex Buffer");
#endif
}

void create_index_buffer(uint32_t size)
{
    vkal_info.index_buffer = create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkal_info.index_buffer_offset = 0;
#ifdef _DEBUG
    vkal_dbg_buffer_name(vkal_info.index_buffer, "Global Index Buffer");
#endif
}

void flush_to_memory(VkDeviceMemory device_memory, void * dst_memory, void * src_memory, uint32_t size, uint32_t offset)
{
    memcpy(dst_memory, src_memory, size);
    VkMappedMemoryRange flush_range =
	{
	    VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
	    0,
	    device_memory,
	    offset,
	    VK_WHOLE_SIZE
	};
    VkResult result = vkFlushMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    DBG_VULKAN_ASSERT(result, "failed to flush mapped memory range(s) for vertex buffer!");
    result = vkInvalidateMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    DBG_VULKAN_ASSERT(result, "failed to invalidate mapped memory range(s) for vertex buffer!");
}

void vkal_update_descriptor_set_uniform(VkDescriptorSet descriptor_set, UniformBuffer uniform_buffer)
{
    VkDescriptorBufferInfo buffer_infos[1];
    buffer_infos[0].buffer = vkal_info.uniform_buffer.buffer;
    buffer_infos[0].range = uniform_buffer.size;
    buffer_infos[0].offset = uniform_buffer.offset;
    VkWriteDescriptorSet write_set_uniform = create_write_descriptor_set_buffer(
	descriptor_set, 
	uniform_buffer.binding, 1,
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer_infos);

    vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_uniform, 0, 0);
}

void vkal_update_descriptor_set_bufferarray(VkDescriptorSet descriptor_set, VkDescriptorType descriptor_type, uint32_t binding, uint32_t array_element, Buffer buffer)
{
    // update floats in fragment shader
    VkDescriptorBufferInfo buffer_infos[1];
    buffer_infos[0].buffer = buffer.buffer;
    buffer_infos[0].range = buffer.size;
    buffer_infos[0].offset = 0; /* This is the offset _WITHIN_ the buffer, not its VkDeviceMemory! */
    VkWriteDescriptorSet write_set_uniform = create_write_descriptor_set_buffer2(
	descriptor_set,
	binding, array_element, 1,
	descriptor_type, buffer_infos);

    vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_uniform, 0, 0);
}

UniformBuffer vkal_create_uniform_buffer(uint32_t size, uint32_t binding)
{
    UniformBuffer uniform_buffer = {};
    uniform_buffer.offset = vkal_info.uniform_buffer_offset;
    uniform_buffer.size = size;
    uniform_buffer.binding = binding;

    uint64_t alignment = vkal_info.physical_device_properties.limits.minUniformBufferOffsetAlignment;
    uint64_t next_offset = alignment * (size / alignment + 1); // TODO: This is dumb!
    vkal_info.uniform_buffer_offset += next_offset;
    return uniform_buffer;
}

// NOTE: If vertex_count is higher than the current buffer, vertex data after offset+vertex_count (in bytes) will be overwritten!!!
uint32_t vkal_vertex_buffer_update(Vertex * vertices, uint32_t vertex_count, VkDeviceSize offset)
{
    uint32_t vertices_in_bytes = vertex_count * sizeof(Vertex);
    
    // map staging memory and upload vertex data
    void * staging_memory;
    VkResult result = vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, vertices_in_bytes, 0, &staging_memory);
    DBG_VULKAN_ASSERT(result, "failed to map device staging memory!");
    flush_to_memory(vkal_info.device_memory_staging, staging_memory, vertices, vertices_in_bytes, 0);
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);
    
    // copy vertex buffer data from staging memory (host visible) to device local memory for every command buffer
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    for (int i = 0; i < vkal_info.command_buffer_count; ++i) {
	vkBeginCommandBuffer(vkal_info.command_buffers[i], &begin_info);
	VkBufferCopy buffer_copy = {};
	buffer_copy.dstOffset = offset;
	buffer_copy.srcOffset = 0;
	buffer_copy.size = vertices_in_bytes;
	vkCmdCopyBuffer(vkal_info.command_buffers[i], vkal_info.staging_buffer.buffer, vkal_info.vertex_buffer.buffer, 1, &buffer_copy);
	vkEndCommandBuffer(vkal_info.command_buffers[i]);
    }
    
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = vkal_info.command_buffer_count;
    submit_info.pCommandBuffers = vkal_info.command_buffers;
    vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(vkal_info.device);
    
    return offset;
}

/* TODO: Maybe create a temporary command buffer to copy from staging buffer to device only memory... */
uint32_t vkal_vertex_buffer_add(Vertex * vertices, uint32_t vertex_count)
{
    uint32_t vertices_in_bytes = vertex_count * sizeof(Vertex);
    
    // map staging memory and upload vertex data
    void * staging_memory;
    VkResult result = vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, vertices_in_bytes, 0, &staging_memory);
    DBG_VULKAN_ASSERT(result, "failed to map device staging memory!");
    flush_to_memory(vkal_info.device_memory_staging, staging_memory, vertices, vertices_in_bytes, 0);
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);
    
    // copy vertex buffer data from staging memory (host visible) to device local memory for every command buffer
    uint32_t offset = vkal_info.vertex_buffer_offset;
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    for (int i = 0; i < 1; ++i) {
	vkBeginCommandBuffer(vkal_info.command_buffers[i], &begin_info);
	VkBufferCopy buffer_copy = {};
	buffer_copy.dstOffset = offset;
	buffer_copy.srcOffset = 0;
	buffer_copy.size = vertices_in_bytes;
	vkCmdCopyBuffer(vkal_info.command_buffers[i], vkal_info.staging_buffer.buffer, vkal_info.vertex_buffer.buffer, 1, &buffer_copy);
	vkEndCommandBuffer(vkal_info.command_buffers[i]);
    }
    
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;// vkal_info.command_buffer_count;
    submit_info.pCommandBuffers = &vkal_info.command_buffers[0];
    vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(vkal_info.device);
    
    // TODO: this is not COOL!
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint64_t next_offset = vkal_info.vertex_buffer_offset + (vertices_in_bytes / alignment) * alignment;
    next_offset += vertices_in_bytes % alignment ? alignment : 0;
    vkal_info.vertex_buffer_offset = next_offset;
    
    return offset;
}

uint32_t vkal_index_buffer_add(uint32_t * indices, uint32_t index_count)
{
    uint32_t indices_in_bytes = index_count * sizeof(uint32_t);
    
    // map staging memory and upload index data
    void * staging_memory;
    VkResult result = vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, indices_in_bytes, 0, &staging_memory);
    DBG_VULKAN_ASSERT(result, "failed to map device staging memory!");
    flush_to_memory(vkal_info.device_memory_staging, staging_memory, indices, indices_in_bytes, 0);
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);
    
    // copy vertex index data from staging memory (host visible) to device local memory for every command buffer
    uint32_t offset = vkal_info.index_buffer_offset;
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    for (int i = 0; i < vkal_info.command_buffer_count; ++i) {
	vkBeginCommandBuffer(vkal_info.command_buffers[i], &begin_info);
	VkBufferCopy buffer_copy = {};
	buffer_copy.dstOffset = offset;
	buffer_copy.srcOffset = 0;
	buffer_copy.size = indices_in_bytes;
	vkCmdCopyBuffer(vkal_info.command_buffers[i], vkal_info.staging_buffer.buffer, vkal_info.index_buffer.buffer, 1, &buffer_copy);
	vkEndCommandBuffer(vkal_info.command_buffers[i]);
    }
    
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = vkal_info.command_buffer_count;
    submit_info.pCommandBuffers = vkal_info.command_buffers;
    vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(vkal_info.device);
    
    // TODO: this is not COOL!
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint64_t next_offset = vkal_info.index_buffer_offset + (indices_in_bytes / alignment) * alignment;
    next_offset += indices_in_bytes % alignment ? alignment : 0;
    vkal_info.index_buffer_offset = next_offset;
    
    return offset;
}

void vkal_cleanup() {
    static int memory_destroyed = 0;
    vkQueueWaitIdle(vkal_info.graphics_queue);

    for (int i = 0; i < vkal_info.swapchain_image_count; ++i) {
	vkDestroyImageView(vkal_info.device, vkal_info.swapchain_image_views[i], 0);
    }
    for (int i = 0; i < vkal_info.framebuffer_count; ++i) {
	vkDestroyFramebuffer(vkal_info.device, vkal_info.framebuffers[i], 0);
    }
    vkDestroyRenderPass(vkal_info.device, vkal_info.render_pass, 0);
    vkDestroyRenderPass(vkal_info.device, vkal_info.imgui_render_pass, 0);
    vkDestroyRenderPass(vkal_info.device, vkal_info.render_to_image_render_pass, 0);
    vkDestroyRenderPass(vkal_info.device, vkal_info.offscreen_pass.render_pass, 0);

    vkDestroySwapchainKHR(vkal_info.device, vkal_info.swapchain, 0);
    glfwDestroyWindow(vkal_info.window);
    vkDestroySurfaceKHR(vkal_info.instance, vkal_info.surface, 0);
	
    for (int i = 0; i < vkal_info.commandpool_count; ++i) {
	vkFreeCommandBuffers(vkal_info.device, vkal_info.command_pools[0], 1, &vkal_info.command_buffers[i]);
    }
    for (int i = 0; i < vkal_info.commandpool_count; ++i) {
	vkDestroyCommandPool(vkal_info.device, vkal_info.command_pools[i], 0);
    }

    vkDestroySemaphore(vkal_info.device, vkal_info.present_complete_semaphore, 0);
    vkDestroySemaphore(vkal_info.device, vkal_info.render_complete_semaphore, 0);
	
    for (int i = 0; i < VKAL_MAX_VKDEVICEMEMORY; ++i) {
	destroy_device_memory(i);
	memory_destroyed++;
    }
    vkFreeMemory(vkal_info.device, vkal_info.device_memory_staging, 0); memory_destroyed++;
    vkFreeMemory(vkal_info.device, vkal_info.device_memory_index, 0); memory_destroyed++;
    vkFreeMemory(vkal_info.device, vkal_info.device_memory_uniform, 0); memory_destroyed++;
    vkFreeMemory(vkal_info.device, vkal_info.device_memory_vertex, 0); memory_destroyed++;
    
    kill_array(vkal_info.image_in_flight_fences);
    
    vkDestroyBuffer(vkal_info.device, vkal_info.uniform_buffer.buffer, 0);
    vkDestroyBuffer(vkal_info.device, vkal_info.vertex_buffer.buffer, 0);
    vkDestroyBuffer(vkal_info.device, vkal_info.index_buffer.buffer, 0);
    vkDestroyBuffer(vkal_info.device, vkal_info.staging_buffer.buffer, 0);
    
    for (int i = 0; i < VKAL_MAX_VKIMAGEVIEW; ++i) {
	destroy_image_view(i);
    }
    for (int i = 0; i < VKAL_MAX_VKIMAGE; ++i) {
	destroy_image(i);
    }

    for (int i = 0; i < VKAL_MAX_VKSHADERMODULE; ++i) {
	destroy_shader_module(i);
    }

    for (int i = 0; i < VKAL_MAX_VKPIPELINELAYOUT; ++i) {
	destroy_pipeline_layout(i);
    }

    for (int i = 0; i < VKAL_MAX_VKDESCRIPTORSETLAYOUT; ++i) {
	destroy_descriptor_set_layout(i);
    }

    for (int i = 0; i < VKAL_MAX_VKPIPELINE; ++i) {
	destroy_graphics_pipeline(i);
    }

    for (int i = 0; i < VKAL_MAX_VKSAMPLER; ++i) {
	destroy_sampler(i);
    }

    for (int i = 0; i < VKAL_MAX_VKFRAMEBUFFER; ++i) {
	destroy_framebuffer(i);
    }

    vkDestroyDescriptorPool(vkal_info.device, vkal_info.descriptor_pool, 0);
    
    vkDestroyDevice(vkal_info.device, 0);
    vkDestroyInstance(vkal_info.instance, 0);

    printf("Memory Allocs: %d\n", memory_allocs);
    printf("Memory Destroyed: %d\n", memory_destroyed);
    printf("Images Created: %d\n", images_created);
    printf("Images Destroyed: %d\n", images_destroyed);
    glfwTerminate();
}
