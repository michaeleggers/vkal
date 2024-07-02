#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <assert.h>

#if defined (VKAL_WIN32)
	#define VK_USE_PLATFORM_WIN32_KHR
#endif


#if defined (VKAL_GLFW)
    #include <vulkan/vulkan.h>
    #include <GLFW/glfw3.h>
#elif defined (VKAL_WIN32)
    #include <vulkan/vulkan.h>
	#include <Windows.h>
#elif defined (VKAL_SDL)
    #include <vulkan/vulkan.h>
    #include <SDL.h>
    #include <SDL_vulkan.h>
#endif


#include "vkal.h"

#ifdef _DEBUG
    PFN_vkSetDebugUtilsObjectNameEXT                       vkSetDebugUtilsObjectName;
#endif 

PFN_vkGetAccelerationStructureBuildSizesKHR           vkGetAccelerationStructureBuildSizes;
PFN_vkCreateAccelerationStructureKHR                  vkCreateAccelerationStructure;
PFN_vkCmdBuildAccelerationStructuresKHR               vkCmdBuildAccelerationStructures;
PFN_vkGetAccelerationStructureDeviceAddressKHR        vkGetAccelerationStructureDeviceAddress;
PFN_vkCreateRayTracingPipelinesKHR                    vkCreateRayTracingPipelines;
PFN_vkGetRayTracingShaderGroupHandlesKHR              vkGetRayTracingShaderGroupHandles;
PFN_vkCmdTraceRaysKHR                                 vkCmdTraceRays;
//PFN_vkGetBufferDeviceAddressKHR                       vkGetBufferDeviceAddress;

static VkalInfo vkal_info;

static size_t vkal_index_size;
static VkIndexType vkal_index_type;

VkalInfo * vkal_init(char ** extensions, uint32_t extension_count, VkalWantedFeatures vulkan_features, VkIndexType index_type)
{

#ifdef _DEBUG

	#ifdef __cplusplus
		extern "C" {
	#endif

	#if defined (VKAL_GLFW)
			vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)glfwGetInstanceProcAddress(vkal_info.instance, "vkSetDebugUtilsObjectNameEXT");
	#elif defined (VKAL_WIN32)
			vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vkal_info.instance, "vkSetDebugUtilsObjectNameEXT");
	#elif defined (VKAL_SDL)
			vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vkal_info.instance, "vkSetDebugUtilsObjectNameEXT");
	#endif

	#ifdef __cplusplus
		}
	#endif

#endif 

    vkal_index_size = sizeof(uint16_t);
    vkal_index_type = VK_INDEX_TYPE_UINT16;

    if (index_type == VK_INDEX_TYPE_UINT32) {
        vkal_index_size = sizeof(uint32_t);
        vkal_index_type = VK_INDEX_TYPE_UINT32;
    }

//    pick_physical_device(extensions, extension_count);
    create_logical_device(extensions, extension_count, vulkan_features);
    create_swapchain();
    create_image_views();
    create_default_render_pass();
    create_render_to_image_render_pass();
    create_default_depth_buffer();
    create_default_framebuffers();
    create_default_descriptor_pool();
    create_default_command_pool();
    create_default_command_buffers();
    create_default_uniform_buffer(UNIFORM_BUFFER_SIZE);
    allocate_default_device_memory_uniform();
    create_default_vertex_buffer(VERTEX_BUFFER_SIZE);
    allocate_default_device_memory_vertex();
    create_default_index_buffer(INDEX_BUFFER_SIZE);
    allocate_default_device_memory_index();
    create_staging_buffer(STAGING_BUFFER_SIZE);
    create_default_semaphores();
    vkal_info.frames_rendered = 0;

    // Setup some flags required for feature enable/disable
    vkal_info.raytracing_enabled = 0;


    return &vkal_info;
}

void vkal_init_raytracing() {
    #ifdef __cplusplus
        extern "C" {
    #endif
            vkal_info.raytracing_enabled = 1;

            // TODO: Check if function pointers are loaded correctly!
    #if defined (VKAL_GLFW)
            vkGetAccelerationStructureBuildSizes = (PFN_vkGetAccelerationStructureBuildSizesKHR)glfwGetInstanceProcAddress(vkal_info.instance, "vkGetAccelerationStructureBuildSizesKHR");
            vkCreateAccelerationStructure = (PFN_vkCreateAccelerationStructureKHR)glfwGetInstanceProcAddress(vkal_info.instance, "vkCreateAccelerationStructureKHR");
            vkCmdBuildAccelerationStructures = (PFN_vkCmdBuildAccelerationStructuresKHR)glfwGetInstanceProcAddress(vkal_info.instance, "vkCmdBuildAccelerationStructuresKHR");
            vkGetAccelerationStructureDeviceAddress = (PFN_vkGetAccelerationStructureDeviceAddressKHR)glfwGetInstanceProcAddress(vkal_info.instance, "vkGetAccelerationStructureDeviceAddressKHR");
            vkCreateRayTracingPipelines = (PFN_vkCreateRayTracingPipelinesKHR)glfwGetInstanceProcAddress(vkal_info.instance, "vkCreateRayTracingPipelinesKHR");
            vkGetRayTracingShaderGroupHandles = (PFN_vkGetRayTracingShaderGroupHandlesKHR)glfwGetInstanceProcAddress(vkal_info.instance, "vkGetRayTracingShaderGroupHandlesKHR");
            //vkCmdTraceRays                          = (PFN_vkCmdTraceRaysKHR)glfwGetInstanceProcAddress(vkal_info.instance, "PFN_vkCmdTraceRaysKHR");
            vkCmdTraceRays = (PFN_vkCmdTraceRaysKHR)vkGetInstanceProcAddr(vkal_info.instance, "vkCmdTraceRaysKHR"); // TODO: Update GLFW (cannot load fn-ptr for vkCmdTraceRaysKHR!)

    #elif defined (VKAL_WIN32)
            //vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vkal_info.instance, "vkSetDebugUtilsObjectNameEXT");
    #elif defined (VKAL_SDL)
            //vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vkal_info.instance, "vkSetDebugUtilsObjectNameEXT");
    #endif

    #ifdef __cplusplus
        }
    #endif
}

#if defined(VKAL_GLFW)
void vkal_create_instance_glfw(
    GLFWwindow * window,
    char ** instance_extensions, uint32_t instance_extension_count,
    char ** instance_layers, uint32_t instance_layer_count)
{
    vkal_info.window = window;
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "VKAL Application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "VKAL Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    #if defined (WIN32) || defined(__linux__)
        app_info.apiVersion = VK_API_VERSION_1_3;
    #elif __APPLE__
        app_info.apiVersion = VK_API_VERSION_1_2; // MoltenVK only goes up to Vulkan version 1.2
    #endif
    
    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    #ifdef __APPLE__
        create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
    create_info.pApplicationInfo = &app_info;

    // Query available extensions.
    {
		vkEnumerateInstanceExtensionProperties(0, &vkal_info.available_instance_extension_count, 0);
        VKAL_MALLOC(vkal_info.available_instance_extensions, vkal_info.available_instance_extension_count);
		vkEnumerateInstanceExtensionProperties(0, &vkal_info.available_instance_extension_count,
							   vkal_info.available_instance_extensions);
    }
    
    // If debug build check if validation layers defined in struct are available and load them
    {
        vkEnumerateInstanceLayerProperties(&vkal_info.available_instance_layer_count, 0);
        VKAL_MALLOC(vkal_info.available_instance_layers, vkal_info.available_instance_layer_count);
        vkEnumerateInstanceLayerProperties(&vkal_info.available_instance_layer_count,
                           vkal_info.available_instance_layers);
    #ifdef _DEBUG
        vkal_info.enable_instance_layers = 1;
    #else
        vkal_info.enable_instance_layers = 0;
    #endif
        int layer_ok = 0;
        if (vkal_info.enable_instance_layers) {
            for (uint32_t i = 0; i < instance_layer_count; ++i) {
            layer_ok = check_instance_layer_support(instance_layers[i],
                                vkal_info.available_instance_layers,
                                vkal_info.available_instance_layer_count);
                if (!layer_ok) {
                    printf("[VKAL] validation layer not available: %s\n", instance_layers[i]);
                    VKAL_ASSERT(VK_ERROR_LAYER_NOT_PRESENT);
                }
            }
        }
        if (layer_ok) {
            create_info.enabledLayerCount = instance_layer_count;
            create_info.ppEnabledLayerNames = (const char * const *)instance_layers;
        }
    }

    // Check if requested instance extensions are available and if so, load them.
    {
		uint32_t required_extension_count = 0;
		char const ** required_extensions;
	
		required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
    
		uint32_t total_instance_ext_count = required_extension_count + instance_extension_count;
		char ** all_instance_extensions = (char**)malloc(total_instance_ext_count * sizeof(char*));
		for (uint32_t i = 0; i < total_instance_ext_count; ++i) {
			all_instance_extensions[i] = (char*)malloc(256 * sizeof(char));
		}
		uint32_t i = 0;
		for (; i < required_extension_count; ++i) {
			strcpy(all_instance_extensions[i], required_extensions[i]);
		}
		for (uint32_t j = 0; i < total_instance_ext_count; ++i) {
			strcpy(all_instance_extensions[i], instance_extensions[j++]);
		}
		int extension_ok = 0;
		for (uint32_t i = 0; i < total_instance_ext_count; ++i) {
			extension_ok = check_instance_extension_support(all_instance_extensions[i],
									vkal_info.available_instance_extensions,
									vkal_info.available_instance_extension_count);
			if (!extension_ok) {
				printf("[VKAL] instance extension not available: %s\n", all_instance_extensions[i]);
				VKAL_ASSERT(VK_ERROR_EXTENSION_NOT_PRESENT);
			}
		}
		create_info.enabledExtensionCount = total_instance_ext_count;
		create_info.ppEnabledExtensionNames = (const char * const *)all_instance_extensions;
    
        VkResult result = vkCreateInstance(&create_info, 0, &vkal_info.instance);
		VKAL_ASSERT(result && "failed to create VkInstance");

		for (uint32_t i = 0; i < total_instance_ext_count; ++i) {
			free(all_instance_extensions[i]);
		}
    }

    create_glfw_surface();
}

#elif defined (VKAL_WIN32)
void vkal_create_instance_win32(
	HWND window, HINSTANCE hInstance,
	char ** instance_extensions, uint32_t instance_extension_count,
	char ** instance_layers, uint32_t instance_layer_count)
{
	vkal_info.window = window;
	VkApplicationInfo app_info = { 0 };
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "VKAL Application";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "VKAL Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    #ifdef __APPLE__
        create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
	create_info.pApplicationInfo = &app_info;

	// Query available extensions.
	{
		vkEnumerateInstanceExtensionProperties(0, &vkal_info.available_instance_extension_count, 0);
        VKAL_MALLOC(vkal_info.available_instance_extensions, vkal_info.available_instance_extension_count);
		vkEnumerateInstanceExtensionProperties(0, &vkal_info.available_instance_extension_count,
			vkal_info.available_instance_extensions);
	}

	// If debug build check if validation layers defined in struct are available and load them
	{
		vkEnumerateInstanceLayerProperties(&vkal_info.available_instance_layer_count, 0);
        VKAL_MALLOC(vkal_info.available_instance_layers, vkal_info.available_instance_layer_count);
		vkEnumerateInstanceLayerProperties(&vkal_info.available_instance_layer_count,
			vkal_info.available_instance_layers);
#ifdef _DEBUG
		vkal_info.enable_instance_layers = 1;
#else
		vkal_info.enable_instance_layers = 0;
#endif
		int layer_ok = 0;
		if (vkal_info.enable_instance_layers) {
			for (uint32_t i = 0; i < instance_layer_count; ++i) {
				layer_ok = check_instance_layer_support(instance_layers[i],
					vkal_info.available_instance_layers,
					vkal_info.available_instance_layer_count);
				if (!layer_ok) {
					printf("[VKAL] validation layer not available: %s\n", instance_layers[i]);
					VKAL_ASSERT(VK_ERROR_LAYER_NOT_PRESENT && "requested isntance layer not present!");
				}
			}
		}
		if (layer_ok) {
			create_info.enabledLayerCount = instance_layer_count;
			create_info.ppEnabledLayerNames = (const char * const *)instance_layers;
		}
	}

	// Check if requested instance extensions are available and if so, load them.
	{
		char const * required_extensions[] = 
		{
			"VK_KHR_surface",
			"VK_KHR_win32_surface"
		};
		uint32_t required_extension_count = sizeof(required_extensions)/sizeof(*required_extensions);

		uint32_t total_instance_ext_count = required_extension_count + instance_extension_count;
		char ** all_instance_extensions;
		all_instance_extensions = (char**)malloc(total_instance_ext_count * sizeof(char*));
		for (uint32_t i = 0; i < total_instance_ext_count; ++i) {
			all_instance_extensions[i] = (char*)malloc(256 * sizeof(char));
		}
		uint32_t i = 0;
		for (; i < required_extension_count; ++i) {
			strcpy(all_instance_extensions[i], required_extensions[i]);
		}
		for (uint32_t j = 0; i < total_instance_ext_count; ++i) {
			strcpy(all_instance_extensions[i], instance_extensions[j++]);
		}
		int extension_ok = 0;
		for (uint32_t i = 0; i < total_instance_ext_count; ++i) {
			extension_ok = check_instance_extension_support(all_instance_extensions[i],
				vkal_info.available_instance_extensions,
				vkal_info.available_instance_extension_count);
			if (!extension_ok) {
				printf("[VKAL] instance extension not available: %s\n", all_instance_extensions[i]);
				VKAL_ASSERT(VK_ERROR_EXTENSION_NOT_PRESENT && "requested instance extension not present!");
			}
		}
		create_info.enabledExtensionCount = total_instance_ext_count;
		create_info.ppEnabledExtensionNames = (const char * const *)all_instance_extensions;

        VkResult result = vkCreateInstance(&create_info, 0, &vkal_info.instance);
		VKAL_ASSERT(result && "failed to create VkInstance");

		for (uint32_t i = 0; i < total_instance_ext_count; ++i) {
			free(all_instance_extensions[i]);
		}
	}

	create_win32_surface(hInstance);
}
#elif defined (VKAL_SDL)
void vkal_create_instance_sdl(
    SDL_Window* window,
    char** instance_extensions, uint32_t instance_extension_count,
    char** instance_layers, uint32_t instance_layer_count)
{
    vkal_info.window = window;
    VkApplicationInfo app_info = { 0 };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "VKAL Application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "VKAL Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    #if defined (WIN32) || defined(__linux__)
        app_info.apiVersion = VK_API_VERSION_1_3;
    #elif __APPLE__
        app_info.apiVersion = VK_API_VERSION_1_2; // MoltenVK only goes up to Vulkan version 1.2
    #endif

    VkInstanceCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    #ifdef __APPLE__
        create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
    create_info.pApplicationInfo = &app_info;

    // Query available extensions.
    {
        vkEnumerateInstanceExtensionProperties(0, &vkal_info.available_instance_extension_count, 0);
        VKAL_MALLOC(vkal_info.available_instance_extensions, vkal_info.available_instance_extension_count);
        vkEnumerateInstanceExtensionProperties(0, &vkal_info.available_instance_extension_count,
            vkal_info.available_instance_extensions);
    }

    // If debug build check if validation layers defined in struct are available and load them
    {
        vkEnumerateInstanceLayerProperties(&vkal_info.available_instance_layer_count, 0);      
        VKAL_MALLOC(vkal_info.available_instance_layers, vkal_info.available_instance_layer_count);
        vkEnumerateInstanceLayerProperties(&vkal_info.available_instance_layer_count,
            vkal_info.available_instance_layers);
#ifdef _DEBUG
        vkal_info.enable_instance_layers = 1;
#else
        vkal_info.enable_instance_layers = 0;
#endif
        int layer_ok = 0;
        if (vkal_info.enable_instance_layers) {
            for (uint32_t i = 0; i < instance_layer_count; ++i) {
                layer_ok = check_instance_layer_support(instance_layers[i],
                    vkal_info.available_instance_layers,
                    vkal_info.available_instance_layer_count);
                if (!layer_ok) {
                    printf("[VKAL] validation layer not available: %s\n", instance_layers[i]);
                    VKAL_ASSERT(VK_ERROR_LAYER_NOT_PRESENT && "requested isntance layer not present!");
                }
            }
        }
        if (layer_ok) {
            create_info.enabledLayerCount = instance_layer_count;
            create_info.ppEnabledLayerNames = (const char* const*)instance_layers;
        }
    }

    // Check if requested instance extensions are available and if so, load them.
    {
        uint32_t required_extension_count = 0;    
        if (!SDL_Vulkan_GetInstanceExtensions(window, &required_extension_count, NULL)) {
            printf("[VKAL] %s\n", SDL_GetError());
            getchar();
            exit(-1);
        }

        uint32_t total_instance_ext_count = required_extension_count + instance_extension_count;
        
        const char* all_instance_extensions[256] = { 0 };

        SDL_Vulkan_GetInstanceExtensions(window, &required_extension_count, all_instance_extensions);
 
        uint32_t j = 0;
        for (uint32_t i = required_extension_count; i < total_instance_ext_count; ++i) {          
            all_instance_extensions[i] = instance_extensions[j++];
        }

        int extension_ok = 0;
        for (uint32_t i = 0; i < total_instance_ext_count; ++i) {
            extension_ok = check_instance_extension_support(all_instance_extensions[i],
                vkal_info.available_instance_extensions,
                vkal_info.available_instance_extension_count);
            if (!extension_ok) {
                printf("[VKAL] instance extension not available: %s\n", all_instance_extensions[i]);
                VKAL_ASSERT(VK_ERROR_EXTENSION_NOT_PRESENT && "requested instance extension not present!");
            }
        }
        create_info.enabledExtensionCount = total_instance_ext_count;
        create_info.ppEnabledExtensionNames = (const char* const*)all_instance_extensions;

        VkResult result = vkCreateInstance(&create_info, 0, &vkal_info.instance);
        VKAL_ASSERT(result && "failed to create VkInstance");
    }

    create_sdl_surface();
}
#endif



	

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

void vkal_flush_command_buffer(VkCommandBuffer command_buffer, VkQueue queue, int free)
{
    if (command_buffer == VK_NULL_HANDLE)
    {
		return;
    }

    VkResult result = vkEndCommandBuffer(command_buffer);
    VKAL_ASSERT(result && "failed to end command buffer");

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;

    VkFence fence = VK_NULL_HANDLE;
    result = vkCreateFence(vkal_info.device, &fence_info, NULL, &fence);
    VKAL_ASSERT(result && "failed to create fence");

    // Submit to the queue
    result = vkQueueSubmit(queue, 1, &submit_info, fence);
    // Wait for the fence to signal that command buffer has finished executing
    result = vkWaitForFences(vkal_info.device, 1, &fence, VK_TRUE, UINT64_MAX);
    VKAL_ASSERT(result && "failed waiting on fence");

    vkDestroyFence(vkal_info.device, fence, NULL);

    if (vkal_info.default_command_pools[0] && free)
    {
		vkFreeCommandBuffers(vkal_info.device, vkal_info.default_command_pools[0], 1, &command_buffer);
    }
}

/* Create image, imageview and bind to device memory. Also perform image barrier to switch switch to wanted layout. */
VkalImage create_vkal_image(
    uint32_t width, uint32_t height,
    VkFormat format, 
    VkImageUsageFlags usage_flags,
	VkImageAspectFlags aspect_bits,
    VkImageLayout layout)
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

		// Back the image with actual memory:
		VkMemoryRequirements image_memory_requirements = { 0 };
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
		VKAL_ASSERT(result && "failed to bind texture image memory!");
    }

    // Image View
    {
        vkal_create_image_view(
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
		VkCommandBufferAllocateInfo allocate_info = {0};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandBufferCount = 1;
		allocate_info.commandPool = vkal_info.default_command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(vkal_info.device, &allocate_info, &cmd_buf);

		// start recording
		VkCommandBufferBeginInfo cmd_begin_info = {0};
		cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VkResult result = vkBeginCommandBuffer(cmd_buf, &cmd_begin_info);
		VKAL_ASSERT(
			result &&
			"failed to put command buffer into recording state");

		VkImageSubresourceRange subresource_range;
		subresource_range.aspectMask     = aspect_bits;	
		subresource_range.baseMipLevel   = 0;
		subresource_range.levelCount     = 1;
		subresource_range.baseArrayLayer = 0;
		subresource_range.layerCount     = 1;
		set_image_layout(
			cmd_buf,
			get_image(vkal_image.image),
			VK_IMAGE_LAYOUT_UNDEFINED, layout,
			subresource_range,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		vkal_flush_command_buffer(cmd_buf, vkal_info.graphics_queue, 1);
    }

    vkal_image.width = width;
    vkal_image.height = height;
    return vkal_image;
}

/* Can be used as a render target. Use this image when creating a framebuffer instead of eg. the swapchain image. */
RenderImage create_render_image(uint32_t width, uint32_t height)
{
    RenderImage render_image = {0};
    render_image.depth_image = create_vkal_image(
		width, height,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	render_image.color_image = create_vkal_image(
		width, height,
		vkal_info.swapchain_image_format,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    for (uint32_t i = 0; i < vkal_info.swapchain_image_count; ++i) {
		render_image.framebuffers[i] = create_render_image_framebuffer(render_image, width, height);
    }
    render_image.width = width;
    render_image.height = height;
    return render_image;
}

void vkal_unmap_buffer(VkalBuffer * buffer)
{
    if (buffer->mapped) {
		vkUnmapMemory(vkal_info.device, buffer->device_memory);
		buffer->mapped = NULL;
    }
}


#if defined (VKAL_GLFW)
void create_glfw_surface(void)
{
	VkResult result = glfwCreateWindowSurface(vkal_info.instance, vkal_info.window, VKAL_NULL, &vkal_info.surface);
    VKAL_ASSERT(result && "failed to create window surface through GLFW.");
}

#elif defined (VKAL_WIN32)
void create_win32_surface(HINSTANCE hInstance)
{
    VkWin32SurfaceCreateInfoKHR surface_create_info = { 0 };
	surface_create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_create_info.pNext     = VKAL_NULL;
	surface_create_info.flags     = 0;
	surface_create_info.hinstance = GetModuleHandle(NULL);
	surface_create_info.hwnd      = vkal_info.window;
	vkCreateWin32SurfaceKHR(vkal_info.instance, &surface_create_info, VKAL_NULL, &vkal_info.surface);
}

#elif defined (VKAL_SDL)
void create_sdl_surface(void)
{
    SDL_Vulkan_CreateSurface(vkal_info.window, vkal_info.instance, &vkal_info.surface); // TODO: Check for success.
}
#endif    


int check_instance_layer_support(char const * requested_layer,
				   VkLayerProperties * available_layers, uint32_t available_layer_count)
{
    int found = 0;
    for (uint32_t i = 0; i < available_layer_count; ++i) {
	    if (!strcmp(available_layers[i].layerName, requested_layer)) {
	        found = 1;
	        break;
	    }
    }
    return found;
}

int check_instance_extension_support(char const * requested_extension,
				     VkExtensionProperties * available_extensions, uint32_t available_extension_count)
{
    int found = 0;
    for (uint32_t i = 0; i < available_extension_count; ++i) {
	    if (!strcmp(available_extensions[i].extensionName, requested_extension)) {
	        found = 1;
	        break;
	    }
    }
    return found;
}

SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device)
{
    SwapChainSupportDetails details = { 0 };
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkal_info.surface, &details.capabilities);
    
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
    for (uint32_t i = 0; i < format_count; ++i) {
	    if (available_format->format == VK_FORMAT_R8G8B8A8_UNORM &&
	        available_format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	        return *available_format;
	    }
	    available_format++;
    }
    return available_formats[0];
}

/* If the user wants sync to the vertical blank of the display then
*  VK_PRESENT_MODE_FIFO_KHR is selected.This mode is guaranteed to 
*  exist on any Vulkan implementation.
*  Otherwise we try to use MAILBOX.
*/
VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR* available_present_modes, uint32_t present_mode_count) // TODO: BUG IN UBUNTU: WON'T SELECT MAILBOX EVER EVEN IF AVAILABLE!
{
    VkPresentModeKHR* available_present_mode = available_present_modes;
    printf("[VKAL] available swapchain present modes:\n");
    for (uint32_t i = 0; i < present_mode_count; ++i) {
        if (*available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            printf("[VKAL]     PRESENT_MODE_IMMEDIATE\n");
        }
        else if (*available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            printf("[VKAL]     PRESENT_MODE_MAILBOX\n");
        }
        else if (*available_present_mode == VK_PRESENT_MODE_FIFO_KHR) {
            printf("[VKAL]     PRESENT_MODE_FIFO\n");
        }
        else if (*available_present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
            printf("[VKAL]     PRESENT_MODE_FIFO_RELAXED\n");
        }
        else if (*available_present_mode == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR) {
            printf("[VKAL]     PRESENT_MODE_SHARED_DEMAND_REFRESH\n");
        }
        else if (*available_present_mode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR) {
            printf("[VKAL]     PRESENT_MODE_SHARED_CONTINUOUS_REFRESH\n");
        }
        available_present_mode++;
    }

    printf("[VKAL] swapchain present mode selected: ");
#if !VKAL_VSYNC_ON 
    // Try to select Mailbox or Immediate
    available_present_mode = available_present_modes;
    for (uint32_t i = 0; i < present_mode_count; ++i) {
        if (*available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            printf("PRESENT_MODE_MAILBOX\n");
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (*available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            printf("PRESENT_MODE_IMMEDIATE\n");
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        available_present_mode++;
    }
    printf("PRESENT_MODE_FIFO\n");
    return VK_PRESENT_MODE_FIFO_KHR;
#elif VKAL_VSYNC_ON
    printf("PRESENT_MODE_FIFO\n");
    return VK_PRESENT_MODE_FIFO_KHR; 
#endif
}

VkExtent2D choose_swap_extent(VkSurfaceCapabilitiesKHR * capabilities)
{
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }
    else {
        int width, height;

    #if defined (VKAL_GLFW)
        glfwGetFramebufferSize(vkal_info.window, &width, &height);
    #elif defined (VKAL_WIN32)
        RECT rect;
        GetClientRect(vkal_info.window, &rect);
        width  = rect.right;
        height = rect.bottom;
    #elif defined (VKAL_SDL)
        SDL_Vulkan_GetDrawableSize(vkal_info.window, &width, &height);
    #endif

        VkExtent2D actual_extent;
        actual_extent.width  = width;
        actual_extent.height = height;
        //actual_extent.width  = max(capabilities->minImageExtent.width, VKAL_MIN(capabilities->maxImageExtent.width, actual_extent.width));
        //actual_extent.height = max(capabilities->minImageExtent.height, VKAL_MIN(capabilities->maxImageExtent.height, actual_extent.height));
        return actual_extent;
    }
}

void cleanup_swapchain(void)
{
    for (uint32_t i = 0; i < vkal_info.framebuffer_count; ++i) {
        vkDestroyFramebuffer(vkal_info.device, vkal_info.framebuffers[i], 0);
    }
    VKAL_FREE(vkal_info.framebuffers);

    // TODO: Do I really have to FREE them???
    //vkFreeCommandBuffers(vkal_info.device, vkal_info.default_command_pools[0], vkal_info.default_command_buffer_count, vkal_info.default_command_buffers);
    //VKAL_FREE(vkal_info.default_command_buffers);
    
    for (uint32_t i = 0; i < vkal_info.swapchain_image_count; ++i) {
        vkDestroyImageView(vkal_info.device, vkal_info.swapchain_image_views[i], 0);
    }
    
    vkDestroySwapchainKHR(vkal_info.device, vkal_info.swapchain, 0);
}

void recreate_swapchain(void)
{
    vkDeviceWaitIdle(vkal_info.device);
    
    cleanup_swapchain();
    
    create_swapchain();
    create_image_views();

    vkal_destroy_image_view( vkal_info.depth_stencil_image_view );
    vkal_destroy_image( vkal_info.depth_stencil_image );
    vkal_destroy_device_memory( vkal_info.device_memory_depth_stencil ); // TODO: Is this smart to destroy the whole memory object?
    create_default_depth_buffer();
    create_default_framebuffers();
    // TODO: Maybe we need to recreate the default command buffers (if client uses them)
    // If so, check cleanup_spwapchain, that frees the default command buffers.
    // create_default_command_buffers();
}

void create_swapchain(void)
{
    SwapChainSupportDetails swap_chain_support = query_swapchain_support(vkal_info.physical_device);
    
    VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swap_chain_support.formats, swap_chain_support.format_count);
    VkPresentModeKHR present_mode = choose_swapchain_present_mode(swap_chain_support.present_modes, swap_chain_support.present_mode_count);
    VkExtent2D extent = choose_swap_extent(&swap_chain_support.capabilities);
    
    uint32_t image_count = VKAL_MAX_SWAPCHAIN_IMAGES;
    if (swap_chain_support.capabilities.maxImageCount > 0) {
		image_count = VKAL_MIN(image_count, swap_chain_support.capabilities.maxImageCount);
    }
    
    VkSwapchainCreateInfoKHR create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = vkal_info.surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    QueueFamilyIndicies indices = find_queue_families(vkal_info.physical_device, vkal_info.surface);
    uint32_t queue_family_indices[2];
    queue_family_indices[0] = indices.graphics_family;
    queue_family_indices[1] = indices.present_family;
    if (indices.graphics_family != indices.present_family) {
	    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	    create_info.queueFamilyIndexCount = 2;
	    create_info.pQueueFamilyIndices = queue_family_indices;
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
    VKAL_ASSERT(result && "failed to create swapchain!");
    
    vkGetSwapchainImagesKHR(vkal_info.device, vkal_info.swapchain, &image_count, 0);
    vkal_info.swapchain_image_count = image_count;
    vkGetSwapchainImagesKHR(vkal_info.device, vkal_info.swapchain, &image_count, vkal_info.swapchain_images);
    
    vkal_info.swapchain_image_format = surface_format.format;
    vkal_info.swapchain_extent = extent;
}

void create_image_views(void)
{
    for (uint32_t i = 0; i < vkal_info.swapchain_image_count; ++i) {
		VkImageViewCreateInfo create_info = { 0 };
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
		VKAL_ASSERT(result && "failed to create image view!");
    }
}

// TODO: VkImageCreateFlags param seems to be important for cube maps. For normal textures set to 0.
void create_image(uint32_t width, uint32_t height, uint32_t mip_levels, uint32_t array_layers, 
		  VkImageCreateFlags flags, VkFormat format, VkImageUsageFlags usage_flags, uint32_t * out_image_id)
{
    QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
    VkImageCreateInfo image_info = { 0 };
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.extent.width  = width;
    image_info.extent.height = height;
    image_info.extent.depth  = 1;
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
    VKAL_ASSERT(result && "failed to create VkImage!");
    vkal_info.user_images[free_image_index].used = 1;
    *out_image_id = free_image_index;
}

void vkal_destroy_image(uint32_t id)
{
    if (vkal_info.user_images[id].used) {
		vkDestroyImage(vkal_info.device, get_image(id), 0);
		vkal_info.user_images[id].used = 0;
    }
}

VkImage get_image(uint32_t id)
{
    assert(vkal_info.user_images[id].used > 0);
    return vkal_info.user_images[id].image;
}

void vkal_create_image_view(VkImage image,
			      VkImageViewType view_type, VkFormat format, VkImageAspectFlags aspect_flags,
			      uint32_t base_mip_level, uint32_t mip_level_count, 
			      uint32_t base_array_layer, uint32_t array_layer_count,
			      uint32_t * out_image_view)
{
    VkImageViewCreateInfo view_info = { 0 };
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.components = (VkComponentMapping){
	VK_COMPONENT_SWIZZLE_IDENTITY,
	VK_COMPONENT_SWIZZLE_IDENTITY,
	VK_COMPONENT_SWIZZLE_IDENTITY,
	VK_COMPONENT_SWIZZLE_IDENTITY };
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
    VkResult result = vkCreateImageView(vkal_info.device, &view_info,
					0,
					&vkal_info.user_image_views[free_index].image_view);
    VKAL_ASSERT(result && "failed to create VkImageView!");

    *out_image_view = free_index;
    vkal_info.user_image_views[free_index].used = 1;
}

void vkal_destroy_image_view(uint32_t id)
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

// TODO: Too view options!
VkSampler create_sampler(VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode u,
			 VkSamplerAddressMode v, VkSamplerAddressMode w)
{
    VkSamplerCreateInfo sampler_info = { 0 };
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    /* TODO: make address mode parameterized as we need repeat for raytracing bluenoise and calmp to border for shadow maps! */
    sampler_info.addressModeU = u; // VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV = v; // VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW = w; // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
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

static void internal_create_sampler(VkSamplerCreateInfo create_info, uint32_t * out_sampler)
{
    uint32_t free_index;
    for (free_index = 0; free_index < VKAL_MAX_VKSAMPLER; ++free_index) {
		if (vkal_info.user_samplers[free_index].used) {
			continue;
		}
		break;
    }
    VkResult result = vkCreateSampler(vkal_info.device, &create_info, 0, &vkal_info.user_samplers[free_index].sampler);
    VKAL_ASSERT(result && "failed to create VkSampler!");
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

void vkal_update_descriptor_set_render_image(VkDescriptorSet descriptor_set, uint32_t binding,
					     VkImageView image_view, VkSampler sampler)
{
    VkDescriptorImageInfo image_infos[1];
    image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_infos[0].imageView = image_view;
    image_infos[0].sampler = sampler;

    VkWriteDescriptorSet write_set_image = create_write_descriptor_set_image(descriptor_set, binding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image_infos);
    vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_image, 0, NULL);
}

void vkal_update_descriptor_set_texture(VkDescriptorSet descriptor_set, VkalTexture texture)
{
    VkDescriptorImageInfo image_infos[1];
    image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_infos[0].imageView = get_image_view(texture.image_view);
    image_infos[0].sampler = texture.sampler;

    VkWriteDescriptorSet write_set_image = create_write_descriptor_set_image(
		descriptor_set,
		texture.binding, 1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		image_infos);
    vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_image, 0, NULL);
}

void vkal_update_descriptor_set_texturearray(
	VkDescriptorSet descriptor_set, 
	VkDescriptorType descriptor_type,
	uint32_t array_element, 
	VkalTexture texture)
{
    // update floats in fragment shader
    VkDescriptorImageInfo image_infos[1];
    image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_infos[0].imageView = get_image_view(texture.image_view);
    image_infos[0].sampler = texture.sampler; /* This is the offset _WITHIN_ the buffer, not its VkDeviceMemory! */
    VkWriteDescriptorSet write_set_uniform = create_write_descriptor_set_image2(
	descriptor_set,
	texture.binding, array_element, 1,
	descriptor_type, image_infos);

    vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_uniform, 0, NULL);
}

VkalTexture vkal_create_texture(
	uint32_t binding,
    unsigned char * texture_data, 
	uint32_t width, 
	uint32_t height, 
	uint32_t channels, 
	VkImageCreateFlags flags, 
	VkImageViewType view_type, 
	VkFormat format,
    uint32_t base_mip_level, 
	uint32_t mip_level_count, 
	uint32_t base_array_layer, 
	uint32_t array_layer_count,
    VkFilter min_filter, 
	VkFilter mag_filter,
	VkSamplerAddressMode sampler_u, 
	VkSamplerAddressMode sampler_v, 
	VkSamplerAddressMode sampler_w)
{
    VkalTexture texture = { 0 };
    texture.width = width;
    texture.height = height;
    texture.channels = channels;
    create_image(width, height, mip_level_count, array_layer_count, flags, format,
		 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		 &texture.image);
    
    // Back the image with actual memory:	
    VkMemoryRequirements image_memory_requirements = { 0 };
    vkGetImageMemoryRequirements(vkal_info.device, get_image(texture.image), &image_memory_requirements);
    uint32_t mem_type_bits = check_memory_type_index(image_memory_requirements.memoryTypeBits,
						     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    create_device_memory(image_memory_requirements.size, mem_type_bits, &texture.device_memory_id);
    VkResult result = vkBindImageMemory(vkal_info.device,
					get_image(texture.image), get_device_memory(texture.device_memory_id), 0);
    VKAL_ASSERT(result && "failed to bind texture image memory!");
    
    vkal_create_image_view(get_image(texture.image), view_type,
		      format, VK_IMAGE_ASPECT_COLOR_BIT,
		      base_mip_level, mip_level_count, 
		      base_array_layer, array_layer_count,
		      &texture.image_view);
    texture.sampler = create_sampler(min_filter, mag_filter, 
				     sampler_u, sampler_v,
				     sampler_w);
    texture.binding = binding;
	
    upload_texture(get_image(texture.image), width, height, channels, array_layer_count, texture_data);
    
    return texture;
}

// TODO: can we use this buffer for any kind of data to stage??
// TODO: Should this also be called the default_staging_buffer?
void create_staging_buffer(uint32_t size) 
{
    vkal_info.staging_buffer = create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	VKAL_DBG_BUFFER_NAME(vkal_info.device, vkal_info.staging_buffer, "Default Staging Buffer");
    // Allocate staging buffer memory
    VkMemoryRequirements buffer_memory_requirements = { 0 };
    vkGetBufferMemoryRequirements(vkal_info.device, vkal_info.staging_buffer.buffer, &buffer_memory_requirements);
    uint32_t mem_type_bits = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkal_info.device_memory_staging = allocate_memory(buffer_memory_requirements.size, mem_type_bits);
    VkResult result = vkBindBufferMemory(vkal_info.device, vkal_info.staging_buffer.buffer, vkal_info.device_memory_staging, 0);
    VKAL_ASSERT(result &&  "failed to bind memory");
}

DeviceMemory vkal_allocate_devicememory(uint32_t size,
					VkBufferUsageFlags buffer_usage_flags,
					VkMemoryPropertyFlags memory_property_flags,
                    VkFlags mem_alloc_flags)
{
    /* Create a dummy buffer so we can select the best possible memory for this type
       of buffer via vkGetBufferMemoryRequirements 
    */
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint64_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

    VkBuffer buffer = create_buffer(aligned_size, buffer_usage_flags).buffer;
    VkMemoryRequirements buffer_memory_requirements = { 0 };
    vkGetBufferMemoryRequirements(vkal_info.device, buffer, &buffer_memory_requirements);
    uint32_t mem_type_bits = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, memory_property_flags);
    vkDestroyBuffer(vkal_info.device, buffer, NULL);

    VkMemoryAllocateFlagsInfo mem_alloc_flags_info = { 0 };
    mem_alloc_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    mem_alloc_flags_info.flags = mem_alloc_flags;    

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryAllocateInfo memory_info = { 0 };
    memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info.allocationSize = buffer_memory_requirements.size;
    memory_info.memoryTypeIndex = mem_type_bits;
    memory_info.pNext = (mem_alloc_flags == 0 ? 0 : &mem_alloc_flags_info);
    VkResult result = vkAllocateMemory(vkal_info.device, &memory_info, 0, &memory);
    VKAL_ASSERT(result && "failed to allocate device memory.");

    DeviceMemory device_memory = { 0 };
    device_memory.vk_device_memory = memory;
    device_memory.size = buffer_memory_requirements.size;
    device_memory.alignment = buffer_memory_requirements.alignment;
    device_memory.free = 0;
    device_memory.mem_type_index = mem_type_bits;
    return device_memory;
}

VkalBuffer vkal_create_buffer(VkDeviceSize size, DeviceMemory * device_memory, VkBufferUsageFlags buffer_usage_flags)
{
    VkalBuffer buffer0 = create_buffer(size, buffer_usage_flags);
    VkMemoryRequirements buffer_memory_requirements = { 0 };
    vkGetBufferMemoryRequirements(vkal_info.device, buffer0.buffer, &buffer_memory_requirements);

    uint64_t alignment = buffer_memory_requirements.alignment;
    uint64_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

    assert(size <= device_memory->size && "vkal_create_buffer: Requested Buffer size exceeds Device Memory size!");

    VkBuffer vk_buffer = VK_NULL_HANDLE;
    VkBufferCreateInfo buffer_info = { 0 };
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
    buffer_info.pQueueFamilyIndices = &indicies.graphics_family;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.usage = buffer_usage_flags;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateBuffer(vkal_info.device, &buffer_info, 0, &vk_buffer);
    VKAL_ASSERT( result && "Failed to create VkBuffer" );
    result = vkBindBufferMemory(vkal_info.device, vk_buffer, device_memory->vk_device_memory, device_memory->free);
    VKAL_ASSERT( result && "Failed to bind VkBuffer to VkDeviceMemory" );
    /* NOTE: the offset in vkBindBufferMemory must be a multiple of alignment returend by vkGetBufferMemoryRequirements and denotes the
       offset into VkDeviceMemory.
    */	
    VkalBuffer buffer = { 0 };
    buffer.size = size;
    buffer.offset = device_memory->free;
    buffer.device_memory = device_memory->vk_device_memory;
    buffer.vkal_device_memory = device_memory;
    buffer.usage = buffer_usage_flags;
    buffer.buffer = vk_buffer;
    buffer.mapped = NULL;

    VkDeviceSize device_memory_alignment = device_memory->alignment;
    VkDeviceSize next_offset = device_memory->free + (aligned_size / device_memory_alignment) * device_memory_alignment;
    next_offset += aligned_size % device_memory_alignment ? device_memory_alignment : 0;
    device_memory->free = next_offset;

    return buffer;
}

void vkal_map_buffer(VkalBuffer* buffer) 
{
    uint64_t alignment = vkal_info.physical_device_properties.limits.minMemoryMapAlignment;

    VkResult result = vkMapMemory(
        vkal_info.device, buffer->device_memory,
        buffer->offset, buffer->size,
        0,
        &buffer->mapped);
    VKAL_ASSERT(result && "Failed to map memory!");
}

// TODO: Fix offset into device memory (must be multiple of nonCoherentAtomSize)
//       Buffer updating must be fixed!!!!
void vkal_update_buffer_offset(VkalBuffer* buffer, uint8_t* data, uint32_t byte_count, uint32_t offset)
{
    assert(byte_count <= buffer->size && "Byte-count is larger than available buffer-size");
    assert(offset <= buffer->size && "Offset is beyond buffer-size!");

    uint64_t alignment = vkal_info.physical_device_properties.limits.minMemoryMapAlignment;
    uint64_t aligned_size = (byte_count + alignment - 1) & ~(alignment - 1);
    uint64_t aligned_offset = (offset + alignment - 1) & ~(alignment - 1);
    uint64_t test = 0;
    if (offset > 0) {
        test = (offset / alignment) * alignment;
    }

    VkResult result = vkMapMemory(
        vkal_info.device, buffer->device_memory,
        buffer->offset, aligned_size,
        0,
        &buffer->mapped);
    VKAL_ASSERT(result && "Failed to map memory!");

    memcpy((uint8_t*)buffer->mapped + offset, data, byte_count);

    // FLUSH IF VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is not set!
    //
    alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    aligned_size = (byte_count + alignment - 1) & ~(alignment - 1);
    aligned_offset = (offset + alignment - 1) & ~(alignment - 1);
    VkMappedMemoryRange memory_range = { 0 };
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = buffer->device_memory;
    memory_range.offset = buffer->offset + aligned_offset;
    memory_range.size = aligned_size; // TODO: figure out how much we need to flush, really.
    result = vkFlushMappedMemoryRanges(vkal_info.device, 1, &memory_range);
    VKAL_ASSERT(result && "Failed to flush mapped memory!");
}

// TODO: Call vkal_update_buffer_offset with buffer.offset
// TODO: Make sure buffer mapping, flushing does not violate vulkan spec! (see function above).
void vkal_update_buffer(VkalBuffer* buffer, uint8_t* data, uint32_t byte_count)
{
    vkal_update_buffer_offset(buffer, data, byte_count, 0);
}

void upload_texture(VkImage const image,
		    uint32_t w, uint32_t h, uint32_t n,
		    uint32_t array_layer_count,
		    unsigned char * texture_data)
{
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint64_t size = array_layer_count * w * h * n;
    uint64_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

    // Copy image data to staging buffer
    void * staging_buffer;
    vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, aligned_size, 0, &staging_buffer);
    memcpy(staging_buffer, texture_data, size);
    VkMappedMemoryRange flush_range = { 0 };
    flush_range.memory = vkal_info.device_memory_staging;
    flush_range.offset = 0;
    flush_range.size = aligned_size;
    flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    vkFlushMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);
    
    //////////////////////////////////
    //////////////////////////////////
    
    // Actual upload to GPU
    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    for (uint32_t i = 0; i < vkal_info.default_command_buffer_count; ++i) {
	    vkBeginCommandBuffer(vkal_info.default_command_buffers[i], &begin_info);
        
	    VkImageSubresourceRange image_subresource_range = { 0 };
	    image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	    image_subresource_range.layerCount = array_layer_count;
	    image_subresource_range.baseArrayLayer = 0;
	    image_subresource_range.levelCount = 1;
	    image_subresource_range.baseMipLevel = 0;
        
	    VkImageMemoryBarrier image_memory_barrier_undef_to_transfer = { 0 };
	    image_memory_barrier_undef_to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	    image_memory_barrier_undef_to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	    image_memory_barrier_undef_to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    image_memory_barrier_undef_to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	    image_memory_barrier_undef_to_transfer.image = image;
	    image_memory_barrier_undef_to_transfer.subresourceRange = image_subresource_range;
	    vkCmdPipelineBarrier(vkal_info.default_command_buffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &image_memory_barrier_undef_to_transfer);
        
	    VkBufferImageCopy copy_info = { 0 };
	    copy_info.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	    copy_info.imageSubresource.baseArrayLayer = 0;
	    copy_info.imageSubresource.layerCount = array_layer_count;
	    copy_info.imageSubresource.mipLevel = 0;
	    copy_info.bufferOffset = 0;
	    copy_info.bufferImageHeight = 0;
	    copy_info.bufferRowLength = 0;
	    copy_info.imageOffset = (VkOffset3D){ 0, 0, 0 };
	    copy_info.imageExtent.width  = w;
	    copy_info.imageExtent.height = h;
	    copy_info.imageExtent.depth  = 1;
	    vkCmdCopyBufferToImage(vkal_info.default_command_buffers[i], vkal_info.staging_buffer.buffer, image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);
        
	    VkImageMemoryBarrier image_memory_barrier_transfer_to_shader_read = { 0 };
	    image_memory_barrier_transfer_to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	    image_memory_barrier_transfer_to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	    image_memory_barrier_transfer_to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	    image_memory_barrier_transfer_to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	    image_memory_barrier_transfer_to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	    image_memory_barrier_transfer_to_shader_read.image = image;
	    image_memory_barrier_transfer_to_shader_read.subresourceRange = image_subresource_range;
	    vkCmdPipelineBarrier(vkal_info.default_command_buffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &image_memory_barrier_transfer_to_shader_read);
        
	    vkEndCommandBuffer(vkal_info.default_command_buffers[i]);        
    }

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = vkal_info.default_command_buffer_count;
    submit_info.pCommandBuffers = vkal_info.default_command_buffers;
    vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(vkal_info.device);	
}

void create_default_depth_buffer(void)
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
        vkal_create_image_view(
			get_image(vkal_info.depth_stencil_image), 
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
    uint32_t mem_type_index      = 0;
    uint32_t best_mem_type_index = 0;
    uint32_t found               = 0;
    uint32_t type_bits           = memory_requirement_bits;
    for (; mem_type_index < memory_properties.memoryTypeCount; ++mem_type_index) {
	    if (type_bits & 1) {
	        if ((memory_properties.memoryTypes[mem_type_index].propertyFlags & wanted_property) == wanted_property) {
		        found = 1;
		        best_mem_type_index = mem_type_index;
		        break;		
	        }
	        found = 1;
	        best_mem_type_index = mem_type_index;
	    }
	    type_bits >>= 1;
    }
    assert(found == 1);
    
    return best_mem_type_index;
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
    int* extensions_left = NULL;
    VKAL_MALLOC(extensions_left, extension_count);
    memset(extensions_left, 1, extension_count * sizeof(int));

    printf("\n[VKAL] query device extensions:\n");
    uint32_t supported_extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, 0, &supported_extension_count, 0);
    VkExtensionProperties* available_extensions = NULL;
    VKAL_MALLOC(available_extensions, supported_extension_count);
    vkEnumerateDeviceExtensionProperties(device, 0, &supported_extension_count, available_extensions);
    
    uint32_t extensions_found = 0;
    for (uint32_t i = 0; i < supported_extension_count; ++i) {
	    for (uint32_t k = 0; k < extension_count; ++k) {
	        if (!strcmp(available_extensions[i].extensionName, extensions[k])) {
		        printf("[VKAL] requested device extension: %s found!\n", extensions[k]);
		        extensions_found++;
		        extensions_left[k] = 0;
		        goto gt_next_extension;
	        }
	    }
        gt_next_extension:;
    }
    
    for (uint32_t i = 0; i < extension_count; ++i) {
	    if (extensions_left[i]) {
	        printf("[VKAL] requested extension %s not available!\n", extensions[i]);
	    }
    }
    VKAL_FREE(extensions_left);
    VKAL_FREE(available_extensions);
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
   
    VkQueueFamilyProperties* queue_families = NULL;
    VKAL_MALLOC(queue_families, queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
    indicies.has_graphics_family = 0;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
	    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
	        indicies.graphics_family = i;
	        indicies.has_graphics_family = 1;
	        break;
	    }
    }
    for (uint32_t i = 0; i < queue_family_count; ++i) {
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

void vkal_find_suitable_devices(char ** extensions, uint32_t extension_count,
				VkalPhysicalDevice ** out_devices, uint32_t * out_device_count)
{
	if (vkal_info.instance == NULL) {
		printf("[VKAL] Error: VkInstance is null!\n");
        system("pause");
		exit(-1);
	}
    vkal_info.physical_device_count = 0;
    vkEnumeratePhysicalDevices(vkal_info.instance, &vkal_info.physical_device_count, 0);
    if (!vkal_info.physical_device_count) {
	    printf("[VKAL] No GPU with Vulkan support found\n");
        system("pause");
	    exit(-1);
    }
    
    VKAL_MALLOC(vkal_info.physical_devices, vkal_info.physical_device_count);
    VKAL_MALLOC(vkal_info.suitable_devices, vkal_info.physical_device_count);
    vkal_info.suitable_device_count = 0;
    vkEnumeratePhysicalDevices(vkal_info.instance, &vkal_info.physical_device_count, vkal_info.physical_devices);
    
    for (uint32_t i = 0; i < vkal_info.physical_device_count; ++i) {
	    VkPhysicalDeviceProperties physical_device_property = { 0 };
	    vkGetPhysicalDeviceProperties(vkal_info.physical_devices[i], &physical_device_property);
	    printf("[VKAL] physical device found: %s\n", physical_device_property.deviceName);
	    if ( is_device_suitable(vkal_info.physical_devices[i], extensions, extension_count) ) {
	        QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_devices[i], vkal_info.surface);
	        if (indicies.has_graphics_family && indicies.has_present_family) {
		        vkal_info.suitable_devices[i].device = vkal_info.physical_devices[i];
		        vkal_info.suitable_devices[i].property = physical_device_property;
		        vkal_info.suitable_device_count++;
	        }
	    }
    }

    *out_devices = vkal_info.suitable_devices;
    *out_device_count = vkal_info.suitable_device_count;
}

void vkal_select_physical_device(VkalPhysicalDevice * physical_device)
{
    vkal_info.physical_device = physical_device->device;
    vkal_info.physical_device_properties = physical_device->property;
    printf("[VKAL] physcial device limits: nonCoherentAtomSize: %llu\n", vkal_info.physical_device_properties.limits.nonCoherentAtomSize);
}

void create_logical_device(char** extensions, uint32_t extension_count, VkalWantedFeatures vulkan_features)
{
    QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
    uint32_t unique_queue_families[2];
    unique_queue_families[0] = indicies.graphics_family;
    unique_queue_families[1] = indicies.present_family;
    VkDeviceQueueCreateInfo queue_create_infos[2] = { 0 };
    uint32_t info_count = 0;
    if (indicies.graphics_family != indicies.present_family) {
        info_count = 2;
    }
    else {
        info_count = 1;
    }
    float queue_prio = 1.f;
    for (uint32_t i = 0; i < info_count; ++i) {
        queue_create_infos[i] = (VkDeviceQueueCreateInfo){ 0 };
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = unique_queue_families[i];
        queue_create_infos[i].queueCount = 1;
        queue_create_infos[i].pQueuePriorities = &queue_prio;
    }

    /* Complete the wanted features struct and chain them */
    vulkan_features.features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan_features.features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan_features.rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    vulkan_features.accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    vulkan_features.features11.pNext = &vulkan_features.features12;
    if (vkal_info.raytracing_enabled) {
        vulkan_features.features12.pNext = &vulkan_features.rayTracingPipelineFeatures;
        vulkan_features.rayTracingPipelineFeatures.pNext = &vulkan_features.accelerationStructureFeatures;
    }

    /* Query what features are supported for the selected device */
    VkPhysicalDeviceVulkan11Features device_features11 = { 0 };
    device_features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    VkPhysicalDeviceVulkan12Features device_features12 = { 0 };
    device_features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    device_features12.pNext = &device_features11;

    /* Raytracing */
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_features = { 0 };
    ray_tracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    ray_tracing_features.pNext = &device_features12;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = { 0 };
    acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration_structure_features.pNext = &ray_tracing_features;
    
    VkPhysicalDeviceFeatures2 available_features2 = { 0 };
    available_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    if (vkal_info.raytracing_enabled) {
        available_features2.pNext = &acceleration_structure_features;
    }
    else {
        available_features2.pNext = &device_features12;
    }
    vkGetPhysicalDeviceFeatures2(vkal_info.physical_device, &available_features2);

    /* TODO: Complete feature-checking */

    /* Check Features2 (which now contains VkalPhysicalDeviceFeatures). For now, just check, what we need */
    VKAL_CHECK_FEATURE(vulkan_features.features2.features.fillModeNonSolid, available_features2.features.fillModeNonSolid);

    /* Check Features 1_1 */
    VKAL_CHECK_FEATURE(vulkan_features.features11.multiview, device_features11.multiview);
    VKAL_CHECK_FEATURE(vulkan_features.features11.multiviewGeometryShader, device_features11.multiviewGeometryShader);
    VKAL_CHECK_FEATURE(vulkan_features.features11.multiviewTessellationShader, device_features11.multiviewTessellationShader);
    VKAL_CHECK_FEATURE(vulkan_features.features11.protectedMemory, device_features11.protectedMemory);
    VKAL_CHECK_FEATURE(vulkan_features.features11.samplerYcbcrConversion, device_features11.samplerYcbcrConversion);
    VKAL_CHECK_FEATURE(vulkan_features.features11.shaderDrawParameters, device_features11.shaderDrawParameters);
    VKAL_CHECK_FEATURE(vulkan_features.features11.storageBuffer16BitAccess, device_features11.storageBuffer16BitAccess);
    VKAL_CHECK_FEATURE(vulkan_features.features11.storageInputOutput16, device_features11.storageInputOutput16);
    VKAL_CHECK_FEATURE(vulkan_features.features11.storagePushConstant16, device_features11.storagePushConstant16);
    VKAL_CHECK_FEATURE(vulkan_features.features11.uniformAndStorageBuffer16BitAccess, device_features11.uniformAndStorageBuffer16BitAccess);
    VKAL_CHECK_FEATURE(vulkan_features.features11.variablePointers, device_features11.variablePointers);
    VKAL_CHECK_FEATURE(vulkan_features.features11.variablePointersStorageBuffer, device_features11.variablePointersStorageBuffer);

    /* Check Features 1_2. For now, just check for the ones that are actually being used often by examples. (There are many!) */
    VKAL_CHECK_FEATURE(vulkan_features.features12.bufferDeviceAddress, device_features12.bufferDeviceAddress);
    VKAL_CHECK_FEATURE(vulkan_features.features12.uniformAndStorageBuffer8BitAccess, device_features12.uniformAndStorageBuffer8BitAccess);
    VKAL_CHECK_FEATURE(vulkan_features.features12.runtimeDescriptorArray, device_features12.runtimeDescriptorArray);
    VKAL_CHECK_FEATURE(vulkan_features.features12.shaderStorageImageArrayNonUniformIndexing, device_features12.shaderStorageImageArrayNonUniformIndexing);
    VKAL_CHECK_FEATURE(vulkan_features.features12.shaderSampledImageArrayNonUniformIndexing, device_features12.shaderSampledImageArrayNonUniformIndexing);
    VKAL_CHECK_FEATURE(vulkan_features.features12.descriptorIndexing, device_features12.descriptorIndexing);

    /* Check Raytracing features */
    VKAL_CHECK_FEATURE(vulkan_features.rayTracingPipelineFeatures.rayTracingPipeline, ray_tracing_features.rayTracingPipeline);
    VKAL_CHECK_FEATURE(vulkan_features.rayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay, ray_tracing_features.rayTracingPipelineShaderGroupHandleCaptureReplay);
    VKAL_CHECK_FEATURE(vulkan_features.rayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplayMixed, ray_tracing_features.rayTracingPipelineShaderGroupHandleCaptureReplayMixed);
    VKAL_CHECK_FEATURE(vulkan_features.rayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect, ray_tracing_features.rayTracingPipelineTraceRaysIndirect);
    VKAL_CHECK_FEATURE(vulkan_features.rayTracingPipelineFeatures.rayTraversalPrimitiveCulling, ray_tracing_features.rayTraversalPrimitiveCulling);

    /* Check Acceleration Structure Features */
    VKAL_CHECK_FEATURE(vulkan_features.accelerationStructureFeatures.accelerationStructure, acceleration_structure_features.accelerationStructure);
    VKAL_CHECK_FEATURE(vulkan_features.accelerationStructureFeatures.accelerationStructureCaptureReplay, acceleration_structure_features.accelerationStructureCaptureReplay);
    VKAL_CHECK_FEATURE(vulkan_features.accelerationStructureFeatures.accelerationStructureHostCommands, acceleration_structure_features.accelerationStructureHostCommands);
    VKAL_CHECK_FEATURE(vulkan_features.accelerationStructureFeatures.accelerationStructureIndirectBuild, acceleration_structure_features.accelerationStructureIndirectBuild);
    VKAL_CHECK_FEATURE(vulkan_features.accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind, acceleration_structure_features.descriptorBindingAccelerationStructureUpdateAfterBind);

    vulkan_features.features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vulkan_features.features2.pNext = &vulkan_features.features11;

    VkDeviceCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &vulkan_features.features2;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.queueCreateInfoCount = info_count;
    create_info.pEnabledFeatures = VKAL_NULL;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = (const char* const*)extensions;

    VKAL_ASSERT(vkCreateDevice(vkal_info.physical_device, &create_info, 0, &vkal_info.device));

    vkGetDeviceQueue(vkal_info.device, indicies.graphics_family, 0, &vkal_info.graphics_queue);
    vkGetDeviceQueue(vkal_info.device, indicies.present_family, 0, &vkal_info.present_queue);
}

void create_shader_module(uint8_t const * shader_byte_code, int size, uint32_t * out_shader_module)
{
    VkShaderModuleCreateInfo create_info = { 0 };
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
    VKAL_ASSERT(result && "failed to create shader module!");
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

void create_default_descriptor_pool(void)
{
    VkDescriptorPoolSize pool_sizes[] =
	{
	    {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,     // type of the resource
		1024							           // number of descriptors of that type to be stored in the pool. This is per set maybe?
	    },
	    {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		1024
	    },
	    {
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1024 // TODO(Michael): figure out why we must have one additional available even if we're only using two textures!
	    },
	    { // for sampling the shadow map
		VK_DESCRIPTOR_TYPE_SAMPLER,
		1024
	    }
	};
    
    VkDescriptorPoolCreateInfo descriptor_pool_info = { 0 };
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_info.maxSets = 20; // NOTE: This is an arbitrary number at the moment. We don't _have_ to use all of them.
    descriptor_pool_info.poolSizeCount = sizeof(pool_sizes)/sizeof(*pool_sizes);
    descriptor_pool_info.pPoolSizes = pool_sizes;
    
    VkResult  result = vkCreateDescriptorPool(vkal_info.device, &descriptor_pool_info, 0, &vkal_info.default_descriptor_pool);
    VKAL_ASSERT(result && "failed to create descriptor pool!");
}


void create_render_to_image_render_pass(void)
{
    VkAttachmentDescription attachments[2];
    // Color Attachment
    attachments[0].flags          = 0;
    attachments[0].format         = vkal_info.swapchain_image_format;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // Depth Stencil Attachment
    attachments[1].flags          = 0;
    attachments[1].format         = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

    VkSubpassDescription subpasses[1];
    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount    = 0;
    subpasses[0].pInputAttachments       = 0;
    subpasses[0].colorAttachmentCount    = 1;
    subpasses[0].pColorAttachments       = color_attachment_refs;
    subpasses[0].pResolveAttachments     = 0;
    subpasses[0].pDepthStencilAttachment = &depth_attachment_ref;
    subpasses[0].preserveAttachmentCount = 0;
    subpasses[0].pPreserveAttachments    = 0;


    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // refers to implicit subpass before/after renderpass
    dependency.dstSubpass = 0; // index into the (only) subpass created above
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = { 0 };
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = subpasses;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    VkResult result = vkCreateRenderPass(vkal_info.device, &render_pass_info, 0, &vkal_info.render_to_image_render_pass);
    VKAL_ASSERT(result && "failed to create render pass!");
}

void create_default_render_pass(void)
{
    VkAttachmentDescription attachments[2];
    // Color Attachment
    attachments[0].flags          = 0;
    attachments[0].format         = vkal_info.swapchain_image_format;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth Stencil Attachment
    attachments[1].flags          = 0;
    attachments[1].format         = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
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
    
    VkSubpassDescription subpasses[1];
    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount    = 0;
    subpasses[0].pInputAttachments       = 0;
    subpasses[0].colorAttachmentCount    = 1;
    subpasses[0].pColorAttachments       = color_attachment_refs;
    subpasses[0].pResolveAttachments     = 0;
    subpasses[0].pDepthStencilAttachment = &depth_attachment_ref;
    subpasses[0].preserveAttachmentCount = 0;
    subpasses[0].pPreserveAttachments    = 0;
    
    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // refers to implicit subpass before/after renderpass
    dependency.dstSubpass = 0; // index into the (only) subpass created above
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
    VkRenderPassCreateInfo render_pass_info = { 0 };
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = subpasses;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    VkResult result = vkCreateRenderPass(vkal_info.device, &render_pass_info, 0, &vkal_info.render_pass);
    VKAL_ASSERT(result && "failed to create render pass!");

}

void create_default_framebuffers(void)
{
    VkImageView attachments[2];
    // The order matches the order of the VkAttachmentDescriptions of the Renderpass.
    
    uint32_t framebuffer_count = 0;
    VKAL_MALLOC(vkal_info.framebuffers, vkal_info.swapchain_image_count);

    for (uint32_t i = 0; i < vkal_info.swapchain_image_count; ++i) {
	    attachments[0] = vkal_info.swapchain_image_views[i]; 
	    attachments[1] = get_image_view(vkal_info.depth_stencil_image_view);
	    VkFramebufferCreateInfo framebuffer_info = { 0 };
	    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	    framebuffer_info.renderPass = vkal_info.render_pass;
	    framebuffer_info.width = vkal_info.swapchain_extent.width;
	    framebuffer_info.height = vkal_info.swapchain_extent.height;
	    framebuffer_info.pAttachments = attachments;
	    framebuffer_info.attachmentCount = 2; // matches the VkAttachmentDescription array-size in renderpass
	    framebuffer_info.layers = 1;
	    VkResult result = vkCreateFramebuffer(vkal_info.device, &framebuffer_info, 0, &vkal_info.framebuffers[i]);
	    VKAL_ASSERT(result && "failed to create framebuffer!");
	    framebuffer_count++;
    }
    vkal_info.framebuffer_count = framebuffer_count;
}



uint32_t create_render_image_framebuffer(RenderImage render_image, uint32_t width, uint32_t height)
{
    VkImageView attachments[2];
    attachments[0] = get_image_view(render_image.color_image.image_view);
    attachments[1] = get_image_view(render_image.depth_image.image_view);
    VkFramebufferCreateInfo framebuffer_info = { 0 };
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
    VKAL_ASSERT(result && "failed to create VkFramebuffer!");
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

    for (uint32_t i = 0; i < vkal_info.swapchain_image_count; ++i) {
		destroy_framebuffer(render_image.framebuffers[i]);
    }

    vkal_destroy_image(render_image.color_image.image);
    vkal_destroy_image(render_image.depth_image.image);

    vkal_destroy_image_view(render_image.color_image.image_view);
    vkal_destroy_image_view(render_image.depth_image.image_view);
    vkal_destroy_device_memory(render_image.color_image.device_memory);
    vkal_destroy_device_memory(render_image.depth_image.device_memory);

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
    VkPipelineLayoutCreateInfo layout_info = { 0 };
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pSetLayouts = descriptor_set_layouts;
    layout_info.setLayoutCount = descriptor_set_layout_count;
    layout_info.pushConstantRangeCount = push_constant_range_count;
    layout_info.pPushConstantRanges = push_constant_ranges;
    uint32_t free_index = 0;
    for (free_index = 0; free_index < VKAL_MAX_VKPIPELINELAYOUT; ++free_index) {
		if (vkal_info.user_pipeline_layouts[free_index].used) {

		}
		else 
			break;
    }
    VkResult result = vkCreatePipelineLayout(vkal_info.device, &layout_info, 0, &vkal_info.user_pipeline_layouts[free_index].pipeline_layout);
    VKAL_ASSERT(result && "failed to create pipeline layout!");
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

void vkal_allocate_descriptor_sets(VkDescriptorPool pool,
				   VkDescriptorSetLayout * layout, uint32_t layout_count,
				   VkDescriptorSet ** out_descriptor_set)
{
    VkDescriptorSetAllocateInfo allocate_info = { 0 };
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = pool;
    allocate_info.pSetLayouts = layout;
    allocate_info.descriptorSetCount = layout_count;
    VkResult result = vkAllocateDescriptorSets(vkal_info.device, &allocate_info, *out_descriptor_set);
    VKAL_ASSERT(result && "failed to allocate descriptor set(s)!");    
}

SingleShaderStageSetup vkal_create_shader(const uint8_t* shader_byte_code, uint32_t shader_byte_code_size, VkShaderStageFlagBits shader_stage_flag_bits)
{
    SingleShaderStageSetup shader_setup = { 0 };
    create_shader_module(shader_byte_code, shader_byte_code_size, &shader_setup.module);

    shader_setup.create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_setup.create_info.stage = shader_stage_flag_bits;
    shader_setup.create_info.module = get_shader_module(shader_setup.module);
    shader_setup.create_info.pName = "main";

    return shader_setup;
}

ShaderStageSetup vkal_create_shaders(
    const uint8_t * vertex_shader_code, uint32_t vertex_shader_code_size, 
    const uint8_t * fragment_shader_code, uint32_t fragment_shader_code_size,
    const uint8_t * tctrl_shader_code, uint32_t tctrl_shader_code_size,
    const uint8_t * teval_shader_code, uint32_t teval_shader_code_size,
    const uint8_t * geometry_shader_code, uint32_t geometry_shader_code_size)
{
    ShaderStageSetup shader_setup = { 0 };

    SingleShaderStageSetup vertex_shader_setup   = vkal_create_shader(vertex_shader_code, vertex_shader_code_size, VK_SHADER_STAGE_VERTEX_BIT);
    shader_setup.vertex_shader_create_info = vertex_shader_setup.create_info;
    shader_setup.vertex_shader_module = vertex_shader_setup.module;

    SingleShaderStageSetup fragment_shader_setup = vkal_create_shader(fragment_shader_code, fragment_shader_code_size, VK_SHADER_STAGE_FRAGMENT_BIT);
    shader_setup.fragment_shader_create_info = fragment_shader_setup.create_info;
    shader_setup.fragment_shader_module = fragment_shader_setup.module;
    
    if (tctrl_shader_code) {
        SingleShaderStageSetup tctrl_shader_setup = vkal_create_shader(tctrl_shader_code, tctrl_shader_code_size, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
        shader_setup.tctrl_shader_create_info = tctrl_shader_setup.create_info;
        shader_setup.tctrl_shader_module = tctrl_shader_setup.module;
    }

    if (teval_shader_code) {
        SingleShaderStageSetup teval_shader_setup = vkal_create_shader(teval_shader_code, teval_shader_code_size, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
        shader_setup.teval_shader_create_info = teval_shader_setup.create_info;
        shader_setup.teval_shader_module = teval_shader_setup.module;
    }

    if (geometry_shader_code) {
        SingleShaderStageSetup geometry_shader_setup = vkal_create_shader(geometry_shader_code, geometry_shader_code_size, VK_SHADER_STAGE_GEOMETRY_BIT);
        shader_setup.geometry_shader_create_info = geometry_shader_setup.create_info;
        shader_setup.geometry_shader_module = geometry_shader_setup.module;
    }

    return shader_setup;
}

VkPipelineShaderStageCreateInfo create_shader_stage_info(VkShaderModule module, VkShaderStageFlagBits shader_stage_flag_bits)
{
    VkPipelineShaderStageCreateInfo shader_stage_info = { 0 };
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
    DescriptorSetLayout set_layout = { 0 };
    VkDescriptorSetLayoutCreateInfo info = { 0 };
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
    VKAL_ASSERT(result && "failed to create descriptor set layout(s)!");
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

void vkal_set_clear_color(VkClearColorValue value)
{
    vkal_info.clear_color_value = value;
}

VkPipeline vkal_create_graphics_pipeline(
	VkVertexInputBindingDescription * vertex_input_bindings, 
	uint32_t vertex_input_binding_count,
	VkVertexInputAttributeDescription * vertex_attributes, 
	uint32_t vertex_attribute_count,
    uint32_t patch_control_points,
	ShaderStageSetup shader_setup,
    VkBool32 depth_test_enable, 
	VkCompareOp depth_compare_op, 
    VkCullModeFlags cull_mode,
    VkPolygonMode polygon_mode,
    VkPrimitiveTopology primitive_topology,
    VkFrontFace face_winding, 
	VkRenderPass render_pass,
	VkPipelineLayout pipeline_layout)
{        
    VkPipelineShaderStageCreateInfo shader_stages_infos[5] = { 0 };
    shader_stages_infos[0] = shader_setup.vertex_shader_create_info;
    shader_stages_infos[1] = shader_setup.fragment_shader_create_info;
    uint32_t num_shader_stages = 2;
    uint32_t tessellationShaderActive = 0;
    if (shader_setup.tctrl_shader_create_info.stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
        shader_stages_infos[num_shader_stages] = shader_setup.tctrl_shader_create_info;
        num_shader_stages += 1;
    }
    if (shader_setup.teval_shader_create_info.stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
        shader_stages_infos[num_shader_stages] = shader_setup.teval_shader_create_info;
        num_shader_stages += 1;
        tessellationShaderActive = 1;
    }
    if (shader_setup.geometry_shader_create_info.stage == VK_SHADER_STAGE_GEOMETRY_BIT) {
        shader_stages_infos[num_shader_stages] = shader_setup.geometry_shader_create_info;
        num_shader_stages += 1;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = vertex_input_binding_count;
    vertex_input_info.pVertexBindingDescriptions = vertex_input_bindings;
    vertex_input_info.vertexAttributeDescriptionCount = vertex_attribute_count;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;
    //
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {0};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = primitive_topology;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;
    
    VkPipelineTessellationStateCreateInfo tessellation_state_info = { 0 };
    tessellation_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellation_state_info.patchControlPoints = patch_control_points;

    VkViewport viewport = {0};
    viewport.x        = 0.f;
    viewport.y        = 0.f;
    viewport.width    = (float)vkal_info.swapchain_extent.width;
    viewport.height   = (float)vkal_info.swapchain_extent.height;
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
    rasterizer_info.lineWidth = 1.0f;
    
    // COME BACK LATER: Set up depth/stencil testing (we need to create a buffer for that as it is
    // not created with the Swapchain
    
    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    VkPipelineColorBlendStateCreateInfo color_blending_info = { 0 };
    color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_info.logicOpEnable = VK_FALSE; // enabling this will set color_blend_attachment.blendEnable to VK_FALSE!    
    color_blending_info.pAttachments = &color_blend_attachment;
    color_blending_info.attachmentCount = 1; // must match the attachment count of render subpass!
    // it affects ALL framebuffers
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = { 0 };
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
    
    VkPipelineMultisampleStateCreateInfo ms_info = { 0 };
    ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_info.sampleShadingEnable = VK_FALSE;
    ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // must match renderpass's color attachment
    
    // dynamic state will force us to provide viewport dimensions and linewidth at drawing-time
    VkDynamicState dynamic_states[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_info = { 0 };
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pDynamicStates = dynamic_states;
    dynamic_state_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(*dynamic_states);
    
    VkGraphicsPipelineCreateInfo pipeline_info = { 0 };
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = num_shader_stages;
    pipeline_info.pStages = shader_stages_infos;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    if (tessellationShaderActive) {
        pipeline_info.pTessellationState = &tessellation_state_info;
    }
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
    VKAL_ASSERT(result && "failed to create graphics pipeline!");
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
    VkWriteDescriptorSet write_set = { 0 };
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = dst_descriptor_set;
    write_set.dstBinding = dst_binding;
    write_set.descriptorCount = count;
    write_set.descriptorType = type;
    write_set.pImageInfo = image_info;
	
    return write_set;
}

VkWriteDescriptorSet create_write_descriptor_set_image2(VkDescriptorSet dst_descriptor_set,
							uint32_t dst_binding, uint32_t array_element,
							uint32_t count,
							VkDescriptorType type, VkDescriptorImageInfo * image_info)
{
    VkWriteDescriptorSet write_set = { 0 };
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = dst_descriptor_set;
    write_set.dstBinding = dst_binding;
    write_set.descriptorCount = count;
    write_set.descriptorType = type;
    write_set.pImageInfo = image_info;
    write_set.dstArrayElement = array_element;

    return write_set;
}

VkWriteDescriptorSet create_write_descriptor_set_buffer(
	VkDescriptorSet dst_descriptor_set, uint32_t dst_binding,
    uint32_t count, VkDescriptorType type, VkDescriptorBufferInfo * buffer_info)
{
    VkWriteDescriptorSet write_set = { 0 };
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
    VkWriteDescriptorSet write_set = { 0 };
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = dst_descriptor_set;
    write_set.dstBinding = dst_binding;
    write_set.dstArrayElement = dst_array_element;
    write_set.descriptorCount = count;
    write_set.descriptorType = type;
    write_set.pBufferInfo = buffer_info;

    return write_set;
}

void create_default_command_pool(void)
{
    VkCommandPoolCreateInfo cmdpool_info = { 0 };
    cmdpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
    if (indicies.has_graphics_family && indicies.has_present_family) {
		if (indicies.graphics_family != indicies.present_family) {
			cmdpool_info.queueFamilyIndex = indicies.graphics_family;
			VkResult result = vkCreateCommandPool(vkal_info.device, &cmdpool_info, 0, &vkal_info.default_command_pools[0]);
			VKAL_ASSERT(result && "failed to create command pool for graphics family");
            
			cmdpool_info.queueFamilyIndex = indicies.present_family;
			result = vkCreateCommandPool(vkal_info.device, &cmdpool_info, 0, &vkal_info.default_command_pools[1]);
			VKAL_ASSERT(result && "failed to create command pool for present family");
			vkal_info.default_commandpool_count = 2;
		}
		else {
			cmdpool_info.queueFamilyIndex = indicies.graphics_family;
			VkResult result = vkCreateCommandPool(vkal_info.device, &cmdpool_info, 0, &vkal_info.default_command_pools[0]);
			VKAL_ASSERT(result && "failed to create command pool for both present and graphics families");
			vkal_info.default_commandpool_count = 1;
		}
    }
	else {
		printf("[VKAL] no graphics and present queues available!\n");
		getchar();
		exit(-1);
	}
}

VkCommandBuffer vkal_create_command_buffer(VkCommandBufferLevel cmd_buffer_level, uint32_t begin)
{
    VkCommandBufferAllocateInfo alloc_info = { 0 };
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = vkal_info.default_command_pools[0];
    alloc_info.level = cmd_buffer_level;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkResult result = vkAllocateCommandBuffers(vkal_info.device, &alloc_info, &command_buffer);
    VKAL_ASSERT(result && "Failed to allocate command buffer from pool");

    if (begin) {
		VkCommandBufferBeginInfo begin_info = {0};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        result = vkBeginCommandBuffer(command_buffer, &begin_info);
		VKAL_ASSERT(result && "Failed to begin command buffer recording");
    }

    return command_buffer;
}

void create_default_command_buffers(void)
{
    VKAL_MALLOC(vkal_info.default_command_buffers, vkal_info.framebuffer_count);
	vkal_info.default_command_buffer_count = vkal_info.framebuffer_count;
	VkCommandBufferAllocateInfo allocate_info = { 0 };
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandBufferCount = vkal_info.default_command_buffer_count;
	allocate_info.commandPool = vkal_info.default_command_pools[0]; // NOTE: What if present and graphics queue are not from the same family?
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers(vkal_info.device, &allocate_info, vkal_info.default_command_buffers); 
}

void vkal_begin(uint32_t image_id, VkCommandBuffer command_buffer, VkRenderPass render_pass)
{
    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffer, &begin_info);
    
    VkRenderPassBeginInfo pass_begin_info = { 0 };
    pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_begin_info.renderPass = render_pass;
    pass_begin_info.framebuffer = vkal_info.framebuffers[image_id];
    pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
    pass_begin_info.renderArea.extent = vkal_info.swapchain_extent;
    VkClearValue clear_values[2];
    clear_values[0].color = vkal_info.clear_color_value;
    clear_values[1].depthStencil  = (VkClearDepthStencilValue){ 1.0f, 0 };

    pass_begin_info.clearValueCount = 2;
    pass_begin_info.pClearValues = clear_values;
    vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vkal_begin_render_pass(uint32_t image_id, VkRenderPass render_pass)
{
    VkRenderPassBeginInfo pass_begin_info = {0};
    pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_begin_info.renderPass = render_pass;
    pass_begin_info.framebuffer = vkal_info.framebuffers[image_id];
    pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
    pass_begin_info.renderArea.extent = vkal_info.swapchain_extent;
    VkClearValue clear_values[2];
    clear_values[0].color = vkal_info.clear_color_value;
    clear_values[1].depthStencil  = (VkClearDepthStencilValue){ 1.0f, 0 };

    pass_begin_info.clearValueCount = 2;
    pass_begin_info.pClearValues = clear_values;
    vkCmdBeginRenderPass(vkal_info.default_command_buffers[image_id], &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vkal_begin_command_buffer(uint32_t image_id)
{
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(vkal_info.default_command_buffers[image_id], &begin_info);
}

void vkal_begin_render_to_image_render_pass(
	uint32_t image_id, 
	VkCommandBuffer command_buffer,
	VkRenderPass render_pass, 
	RenderImage render_image)
{
    VkRenderPassBeginInfo pass_begin_info = {0};
    pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_begin_info.renderPass = render_pass;
    pass_begin_info.framebuffer = get_framebuffer(render_image.framebuffers[image_id]);
    pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };

    VkExtent2D extent;
    extent.width  = render_image.width;
    extent.height = render_image.height;
    
    pass_begin_info.renderArea.extent = extent;
    VkClearValue clear_values[2];
    clear_values[0].color = vkal_info.clear_color_value;
    clear_values[1].depthStencil  = (VkClearDepthStencilValue){ 1.0f, 0 };

    pass_begin_info.clearValueCount = 2;
    pass_begin_info.pClearValues = clear_values;
    vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vkal_end_renderpass(uint32_t image_id)
{
    vkCmdEndRenderPass(vkal_info.default_command_buffers[image_id]);
}

void vkal_end_command_buffer(uint32_t image_id)
{
    VkResult result = vkEndCommandBuffer(vkal_info.default_command_buffers[image_id]);
    VKAL_ASSERT(result && "failed to end command buffer");
}

void vkal_end(VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
    VkResult result = vkEndCommandBuffer(command_buffer);
    VKAL_ASSERT(result && "failed to end command buffer");
}

void vkal_bind_descriptor_set(
	uint32_t image_id,
	VkDescriptorSet * descriptor_set,
	VkPipelineLayout pipeline_layout)
{
    vkal_bind_descriptor_sets_from_to(image_id, descriptor_set, 0, 1, pipeline_layout);
}

void vkal_bind_descriptor_sets_from_to(
    uint32_t image_id,
    VkDescriptorSet* descriptor_sets,
    uint32_t first_set, uint32_t set_count,
    VkPipelineLayout pipeline_layout)
{
    vkCmdBindDescriptorSets(
        vkal_info.default_command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout, first_set, set_count, descriptor_sets, 0, 0);
}

void vkal_bind_descriptor_sets(
	uint32_t image_id,
	VkDescriptorSet * descriptor_sets, uint32_t descriptor_set_count,
	uint32_t * dynamic_offsets, uint32_t dynamic_offset_count,
	VkPipelineLayout pipeline_layout)
{
    vkCmdBindDescriptorSets(
		vkal_info.default_command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout, 0, descriptor_set_count, descriptor_sets,
		dynamic_offset_count, dynamic_offsets);
}

void vkal_bind_descriptor_set_dynamic(
	uint32_t image_id,
	VkDescriptorSet * descriptor_sets,
	VkPipelineLayout pipeline_layout, uint32_t dynamic_offset)
{
    vkCmdBindDescriptorSets(
		vkal_info.default_command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout, 0, 1, descriptor_sets, 1, &dynamic_offset);
}

void vkal_bind_descriptor_set2(
	VkCommandBuffer command_buffer,
	uint32_t first_set, VkDescriptorSet * descriptor_sets, uint32_t descriptor_set_count,
	VkPipelineLayout pipeline_layout)
{
    vkCmdBindDescriptorSets(
		command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipeline_layout, first_set, descriptor_set_count, descriptor_sets, 0, 0);
}

void vkal_viewport(VkCommandBuffer command_buffer, float x, float y, float width, float height)
{
    VkViewport viewport = { 0 };
    viewport.x = x;
    viewport.y = y;
    viewport.width = width; // (float)vp_width;
    viewport.height = height; // (float)vp_height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
}

void vkal_scissor(VkCommandBuffer command_buffer, float offset_x, float offset_y, float extent_x, float extent_y)
{
    VkRect2D scissor      = { 0 };
    scissor.offset.x      = (int32_t)offset_x;
    scissor.offset.y      = (int32_t)offset_y;
    scissor.extent.width  = (uint32_t)extent_x;
    scissor.extent.height = (uint32_t)extent_y;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor); 
}


void vkal_draw_indexed(
    uint32_t image_id, VkPipeline pipeline,
    VkDeviceSize index_buffer_offset, uint32_t index_count,
    VkDeviceSize vertex_buffer_offset, uint32_t instance_count)
{
    vkCmdBindPipeline(vkal_info.default_command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindIndexBuffer(
		vkal_info.default_command_buffers[image_id],
		vkal_info.default_index_buffer.buffer, index_buffer_offset, vkal_index_type);
    uint64_t vertex_buffer_offsets[1];
    vertex_buffer_offsets[0] = vertex_buffer_offset;
    VkBuffer vertex_buffers[1];
	vertex_buffers[0] = vkal_info.default_vertex_buffer.buffer;
    vkCmdBindVertexBuffers(vkal_info.default_command_buffers[image_id], 0, 1, vertex_buffers, vertex_buffer_offsets);
    vkCmdDrawIndexed(vkal_info.default_command_buffers[image_id], index_count, instance_count, 0, 0, 0);
}

void vkal_draw_indexed_from_buffers(
    VkalBuffer index_buffer, uint64_t index_buffer_offset, 
	uint32_t index_count, 
	VkalBuffer vertex_buffer, uint64_t vertex_buffer_offset,
    uint32_t image_id, 
	VkPipeline pipeline)
{
    vkCmdBindPipeline(vkal_info.default_command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindIndexBuffer(vkal_info.default_command_buffers[image_id],
			 index_buffer.buffer, index_buffer_offset, vkal_index_type);
    uint64_t vertex_buffer_offsets[1];
    vertex_buffer_offsets[0] = vertex_buffer_offset;
    VkBuffer vertex_buffers[1];
    vertex_buffers[0] = vertex_buffer.buffer;
    vkCmdBindVertexBuffers(vkal_info.default_command_buffers[image_id], 0, 1, vertex_buffers, vertex_buffer_offsets);
    vkCmdDrawIndexed(vkal_info.default_command_buffers[image_id], index_count, 1, 0, 0, 0);
}

// TODO: Bind pipeline not here. Let it user do manually?
void vkal_draw(
    uint32_t image_id, VkPipeline pipeline,
    VkDeviceSize vertex_buffer_offset, uint32_t vertex_count)
{
    vkCmdBindPipeline(vkal_info.default_command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    uint64_t vertex_buffer_offsets[1];
    vertex_buffer_offsets[0] = vertex_buffer_offset;
    VkBuffer vertex_buffers[1];
    vertex_buffers[0] = vkal_info.default_vertex_buffer.buffer;
    vkCmdBindVertexBuffers(vkal_info.default_command_buffers[image_id], 0, 1, vertex_buffers, vertex_buffer_offsets);
    vkCmdDraw(vkal_info.default_command_buffers[image_id], vertex_count, 1, 0, 0);
}

void vkal_draw_from_buffers(
    VkalBuffer vertex_buffer,
    uint32_t image_id, 
	VkPipeline pipeline,
    VkDeviceSize vertex_buffer_offset, uint32_t vertex_count)
{
    vkCmdBindPipeline(vkal_info.default_command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    uint64_t vertex_buffer_offsets[1];
    vertex_buffer_offsets[0] = vertex_buffer_offset;
    VkBuffer vertex_buffers[1];
    vertex_buffers[0] = vertex_buffer.buffer;
    vkCmdBindVertexBuffers(vkal_info.default_command_buffers[image_id], 0, 1, vertex_buffers, vertex_buffer_offsets);
    vkCmdDraw(vkal_info.default_command_buffers[image_id], vertex_count, 1, 0, 0);
}

void vkal_draw_indexed2(
    VkCommandBuffer command_buffer, 
	VkPipeline pipeline,
    VkDeviceSize index_buffer_offset, uint32_t index_count,
    VkDeviceSize vertex_buffer_offset)
{
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    VkViewport viewport = { 0 };
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = (float)vkal_info.swapchain_extent.width; // (float)vp_width;
    viewport.height = (float)vkal_info.swapchain_extent.height; // (float)vp_height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    
    VkRect2D scissor = { 0 };
    scissor.offset = (VkOffset2D){ 0,0 };
    scissor.extent = vkal_info.swapchain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    
    vkCmdBindIndexBuffer(command_buffer,
			 vkal_info.default_index_buffer.buffer, index_buffer_offset, vkal_index_size);
    uint64_t vertex_buffer_offsets[1];
    vertex_buffer_offsets[0] = vertex_buffer_offset;
    VkBuffer vertex_buffers[1];
    vertex_buffers[0] = vkal_info.default_vertex_buffer.buffer;
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, vertex_buffer_offsets);
    vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}

uint32_t vkal_get_image(void)
{
    vkWaitForFences(vkal_info.device, 1, &vkal_info.in_flight_fences[vkal_info.frames_rendered], VK_TRUE, UINT64_MAX);
    vkResetFences(vkal_info.device, 1, &vkal_info.in_flight_fences[vkal_info.frames_rendered]);
    
    uint32_t image_index;
    // don't actually wait for the semaphore here. just associate it with this operation.
    // check when needed during vkQueueSubmit
    VkResult result = vkAcquireNextImageKHR(vkal_info.device,
					    vkal_info.swapchain,
					    UINT64_MAX, vkal_info.image_available_semaphores[vkal_info.frames_rendered],
					    VK_NULL_HANDLE, &image_index);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vkal_info.should_recreate_swapchain = 0;
        recreate_swapchain();
        return 666; // TODO: return -1 here. This image is useless when too old. User has to check for this!
    }
    else {
        // TODO
    }
    
    return image_index;
}

void vkal_queue_submit(VkCommandBuffer * command_buffers, uint32_t command_buffer_count)
{
    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[1];
    wait_semaphores[0] = vkal_info.image_available_semaphores[vkal_info.frames_rendered];
    VkPipelineStageFlags wait_stages[1];
    wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores; // wait until image is available from swapchain ringbuffer
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = command_buffer_count;
    submit_info.pCommandBuffers = command_buffers;
    VkSemaphore signal_semaphores[1];
    signal_semaphores[0] = vkal_info.render_finished_semaphores[vkal_info.frames_rendered];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    VkResult result = vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info,
				    vkal_info.in_flight_fences[vkal_info.frames_rendered]);
    VKAL_ASSERT(result && "Failed to submit command buffer to queue!");
}

void vkal_present(uint32_t image_id)
{
    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    VkSemaphore wait_semaphores[1];
    wait_semaphores[0] = vkal_info.render_finished_semaphores[vkal_info.frames_rendered];
    present_info.pWaitSemaphores = wait_semaphores;
    VkSwapchainKHR swap_chains[1];
    swap_chains[0] = vkal_info.swapchain;
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
    
    vkal_info.frames_rendered = (vkal_info.frames_rendered+1) % VKAL_MAX_IMAGES_IN_FLIGHT;
}

void create_default_semaphores(void)
{
    for (int i = 0; i < VKAL_MAX_IMAGES_IN_FLIGHT; ++i) {
		VkFenceCreateInfo fenceInfo = { 0 };
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.pNext = NULL;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VkResult result = vkCreateFence(vkal_info.device, &fenceInfo, NULL, &vkal_info.in_flight_fences[i]);
		VKAL_ASSERT(result && "failed to create draw-fence");

		VkSemaphoreCreateInfo sem_info = (VkSemaphoreCreateInfo){ 0 };
		sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(vkal_info.device, &sem_info, 0, &vkal_info.image_available_semaphores[i]);
		vkCreateSemaphore(vkal_info.device, &sem_info, 0, &vkal_info.render_finished_semaphores[i]);

    }
    //memset(vkal_info.image_in_flight_fences, VK_NULL_HANDLE, vkal_info.swapchain_image_count * sizeof(VkFence));
}

void allocate_default_device_memory_uniform(void)
{
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(vkal_info.device, vkal_info.default_uniform_buffer.buffer, &buffer_memory_requirements);
    uint32_t mem_type_index = check_memory_type_index(
		buffer_memory_requirements.memoryTypeBits, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	vkal_info.default_device_memory_uniform = allocate_memory(buffer_memory_requirements.size, mem_type_index);
    
    VkResult result = vkBindBufferMemory(
		vkal_info.device, vkal_info.default_uniform_buffer.buffer,
		vkal_info.default_device_memory_uniform, 0); // the last param is the memory offset!
    VKAL_ASSERT(result && "failed to bind uniform buffer to device memory!");
}

void allocate_default_device_memory_vertex(void)
{
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(vkal_info.device, vkal_info.default_vertex_buffer.buffer, &buffer_memory_requirements);
    uint32_t mem_type_index = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkal_info.default_device_memory_vertex = allocate_memory(buffer_memory_requirements.size, mem_type_index);
    
    VkResult result = vkBindBufferMemory(vkal_info.device, vkal_info.default_vertex_buffer.buffer, vkal_info.default_device_memory_vertex, 0);
    VKAL_ASSERT(result && "failed to bind vertex buffer memory!");
}

void allocate_default_device_memory_index(void)
{
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(vkal_info.device, vkal_info.default_index_buffer.buffer, &buffer_memory_requirements);
    uint32_t mem_type_index = check_memory_type_index(buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkal_info.default_device_memory_index = allocate_memory(buffer_memory_requirements.size, mem_type_index);
    
    VkResult result = vkBindBufferMemory(vkal_info.device, vkal_info.default_index_buffer.buffer, vkal_info.default_device_memory_index, 0);
    VKAL_ASSERT(result && "failed to bind vertex buffer memory!");
}

void create_default_uniform_buffer(uint32_t size)
{
    vkal_info.default_uniform_buffer = create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	VKAL_DBG_BUFFER_NAME(vkal_info.device, vkal_info.default_uniform_buffer, "Default Uniform Buffer");
}

uint32_t vkal_aligned_size(uint32_t size, uint32_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

/* NOTE: It probably is not a good idea to map and unmap the buffer every frame. This function is supposed to be 
         'safe' in that you don't have to worry about touching the memory after update. But when building for 
         production the uniform buffer should probably stay mapped. */
void vkal_update_uniform(UniformBuffer * uniform_buffer, void * data) // TODO: Does uniform_buffer really have to be a pointer?
{
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint64_t data_in_bytes = uniform_buffer->size;
    uint64_t aligned_size = (data_in_bytes + alignment - 1) & ~(alignment - 1);

    void * mapped_uniform_memory = 0;
    VkResult result = vkMapMemory(
		vkal_info.device, 
		vkal_info.default_device_memory_uniform, 
		uniform_buffer->offset, aligned_size, 
		0, 
		&mapped_uniform_memory);
    VKAL_ASSERT(result && "failed to map device memory");

    uint64_t offset = uniform_buffer->offset;
    uint64_t offset_alignment = (offset + alignment - 1) & ~(alignment - 1);

    memcpy(mapped_uniform_memory, data, uniform_buffer->size);
    VkMappedMemoryRange flush_range;
    flush_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    flush_range.pNext  = 0;
    flush_range.memory = vkal_info.default_device_memory_uniform;
    flush_range.offset = offset_alignment;
    flush_range.size   = VK_WHOLE_SIZE;
    
    result = vkFlushMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    VKAL_ASSERT(result && "failed to flush mapped memory range(s)!");
    result = vkInvalidateMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    VKAL_ASSERT(result && "failed to invalidate mapped memory range(s)!");
    vkUnmapMemory(vkal_info.device, vkal_info.default_device_memory_uniform); // invalidated _all_ previously acquired pointers via vkMapMemory
}

void create_device_memory(VkDeviceSize size, uint32_t mem_type_bits, uint32_t * out_memory_id)
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

uint32_t vkal_destroy_device_memory(uint32_t id)
{
    uint32_t is_destroyed = 0;
    if (vkal_info.user_device_memory[id].used) {
	    vkFreeMemory(vkal_info.device, get_device_memory(id), 0);
	    vkal_info.user_device_memory[id].used = 0;
	    is_destroyed = 1;
    }
    return is_destroyed;
}

VkDeviceMemory allocate_memory(VkDeviceSize size, uint32_t mem_type_bits)
{
    VkDeviceMemory memory;
    VkMemoryAllocateInfo memory_info_image = { 0 };
    memory_info_image.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_info_image.allocationSize = size;
    memory_info_image.memoryTypeIndex = mem_type_bits;
    VkResult result = vkAllocateMemory(vkal_info.device, &memory_info_image, 0, &memory);
    VKAL_ASSERT(result && "failed to allocate memory!");
    return memory;
}

VkalBuffer create_buffer(uint32_t size, VkBufferUsageFlags usage)
{
    VkBuffer vk_buffer;
    VkBufferCreateInfo buffer_info = { 0 };
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    QueueFamilyIndicies indicies = find_queue_families(vkal_info.physical_device, vkal_info.surface);
    buffer_info.pQueueFamilyIndices = &indicies.graphics_family;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateBuffer(vkal_info.device, &buffer_info, 0, &vk_buffer);
    VKAL_ASSERT(result && "Failed to create buffer.");
    
    VkalBuffer buffer = { 0 };
    buffer.size = size;
    buffer.usage = usage;
    buffer.buffer = vk_buffer;
    return buffer;
}

// TODO: Destroy Buffer

void create_default_vertex_buffer(uint32_t size)
{
    vkal_info.default_vertex_buffer = create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkal_info.default_vertex_buffer_offset = 0;
	VKAL_DBG_BUFFER_NAME(vkal_info.device, vkal_info.default_vertex_buffer, "Default Vertex Buffer");
}

void create_default_index_buffer(uint32_t size)
{
    vkal_info.default_index_buffer = create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkal_info.default_index_buffer_offset = 0;
	VKAL_DBG_BUFFER_NAME(vkal_info.device, vkal_info.default_index_buffer, "Default Index Buffer");
}

// TODO: Error Assert messages out of date!
void flush_to_memory(VkDeviceMemory device_memory, void * dst_memory, void * src_memory, uint32_t size, uint32_t offset)
{
    memcpy(dst_memory, src_memory, size);
    VkMappedMemoryRange flush_range;
    flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    flush_range.pNext = 0;
    flush_range.memory = device_memory;
    flush_range.offset = offset;
    flush_range.size = VK_WHOLE_SIZE;

    VkResult result = vkFlushMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    VKAL_ASSERT(result && "failed to flush mapped memory range(s) for vertex buffer!");
    result = vkInvalidateMappedMemoryRanges(vkal_info.device, 1, &flush_range);
    VKAL_ASSERT(result && "failed to invalidate mapped memory range(s) for vertex buffer!");
}

void vkal_update_descriptor_set_uniform(
	VkDescriptorSet descriptor_set, 
	UniformBuffer uniform_buffer,
	VkDescriptorType descriptor_type)
{
    assert (uniform_buffer.size < vkal_info.physical_device_properties.limits.maxUniformBufferRange);
    VkDescriptorBufferInfo buffer_infos[1] = { 0 };
    buffer_infos[0].buffer = vkal_info.default_uniform_buffer.buffer;
    buffer_infos[0].range = uniform_buffer.size;
    buffer_infos[0].offset = uniform_buffer.offset;
    VkWriteDescriptorSet write_set_uniform = create_write_descriptor_set_buffer(
		descriptor_set, 
		uniform_buffer.binding, 1,
		descriptor_type, buffer_infos);

    vkUpdateDescriptorSets(vkal_info.device, 1, &write_set_uniform, 0, VK_NULL_HANDLE);
}

void vkal_update_descriptor_set_bufferarray(VkDescriptorSet descriptor_set, VkDescriptorType descriptor_type, 
    uint32_t binding, uint32_t array_element, VkalBuffer buffer)
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

UniformBuffer vkal_create_uniform_buffer(uint32_t size, uint32_t elements, uint32_t binding)
{
    UniformBuffer uniform_buffer = { 0 };
    uniform_buffer.offset = vkal_info.default_uniform_buffer_offset;
//    uniform_buffer.size = size;
    uniform_buffer.binding = binding;
    //uint64_t min_ubo_alignment = vkal_info.physical_device_properties.limits.minUniformBufferOffsetAlignment;
    uint64_t min_ubo_alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uniform_buffer.alignment = (size + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
    uniform_buffer.size = elements * uniform_buffer.alignment;
    uint64_t next_offset = uniform_buffer.size;
    vkal_info.default_uniform_buffer_offset += next_offset;
    return uniform_buffer;
}

uint64_t vkal_vertex_buffer_add(void * vertices, uint32_t vertex_size, uint32_t vertex_count)
{
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint32_t vertices_in_bytes = vertex_count * vertex_size;
    uint64_t size = (vertices_in_bytes + alignment - 1) & ~(alignment - 1);
    
    // map staging memory and upload vertex data
    void * staging_memory;
    VkResult result = vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, size, 0, &staging_memory);
    VKAL_ASSERT(result && "failed to map device staging memory!");
    flush_to_memory(vkal_info.device_memory_staging, staging_memory, vertices, vertices_in_bytes, 0);
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);
    
    // copy vertex buffer data from staging memory (host visible) to device local memory for every command buffer
    uint64_t offset = vkal_info.default_vertex_buffer_offset;

	VkCommandBuffer cmd_buffer = vkal_create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VkBufferCopy buffer_copy = { 0 };
	buffer_copy.dstOffset = offset;
	buffer_copy.srcOffset = 0;
	buffer_copy.size = vertices_in_bytes;
	vkCmdCopyBuffer(cmd_buffer,
			vkal_info.staging_buffer.buffer, vkal_info.default_vertex_buffer.buffer, 1, &buffer_copy);
	vkEndCommandBuffer(cmd_buffer);
    
    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;// vkal_info.command_buffer_count;
	submit_info.pCommandBuffers = &cmd_buffer;
    vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(vkal_info.device);
    
    // When mapping memory later again to copy into it (see:fluch_to_memory) we must respect
    // the devices alignment.
    // See: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMappedMemoryRange.html
    vkal_info.default_vertex_buffer_offset += size;
    
    return offset;
}

void vkal_vertex_buffer_reset(void)
{
    vkal_info.default_vertex_buffer_offset = 0;
}

// NOTE: If vertex_count is higher than the current buffer, vertex data after offset+vertex_count (in bytes) will be overwritten!!!
void vkal_vertex_buffer_update(void* vertices, uint32_t vertex_count, uint32_t vertex_size, VkDeviceSize offset)
{
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint32_t vertices_in_bytes = vertex_count * vertex_size;
    uint64_t size = (vertices_in_bytes + alignment - 1) & ~(alignment - 1);

    // map staging memory and upload vertex data
    void* staging_memory;
    VkResult result = vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, size, 0, &staging_memory);
    VKAL_ASSERT(result && "failed to map device staging memory!");
    flush_to_memory(vkal_info.device_memory_staging, staging_memory, vertices, vertices_in_bytes, 0);
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);

    // copy vertex buffer data from staging memory (host visible) to device local memory at offset position
    VkCommandBuffer cmd_buffer = vkal_create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkBufferCopy buffer_copy = { 0 };
    buffer_copy.dstOffset = offset;
    buffer_copy.srcOffset = 0;
    buffer_copy.size = vertices_in_bytes;
    vkCmdCopyBuffer(cmd_buffer,
        vkal_info.staging_buffer.buffer, vkal_info.default_vertex_buffer.buffer, 1, &buffer_copy);
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;// vkal_info.command_buffer_count;
    submit_info.pCommandBuffers = &cmd_buffer;
    vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(vkal_info.device);
}

uint64_t vkal_index_buffer_add(void * indices, uint32_t index_count)
{
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint32_t indices_in_bytes = index_count * vkal_index_size;
    uint64_t size = (indices_in_bytes + alignment - 1) & ~(alignment - 1);
    
    // map staging memory and upload index data
    void * staging_memory;
    VkResult result = vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, size, 0, &staging_memory);
    VKAL_ASSERT(result && "failed to map device staging memory!");
    flush_to_memory(vkal_info.device_memory_staging, staging_memory, indices, indices_in_bytes, 0);
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);
    
    // copy vertex index data from staging memory (host visible) to device local memory through a command buffer
    uint64_t offset = vkal_info.default_index_buffer_offset;
	VkCommandBuffer cmd_buffer = vkal_create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VkBufferCopy buffer_copy = { 0 };
	buffer_copy.dstOffset = offset;
	buffer_copy.srcOffset = 0;
	buffer_copy.size = indices_in_bytes;
	vkCmdCopyBuffer(cmd_buffer, vkal_info.staging_buffer.buffer, vkal_info.default_index_buffer.buffer, 1, &buffer_copy);
	vkEndCommandBuffer(cmd_buffer);
    
    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1; // vkal_info.default_command_buffer_count;
	submit_info.pCommandBuffers = &cmd_buffer;  //vkal_info.default_command_buffers;
    vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(vkal_info.device);
    
    // When mapping memory later again to copy into it (see:fluch_to_memory) we must respect
    // the devices alignment.
    // See: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMappedMemoryRange.html
    vkal_info.default_index_buffer_offset += size;
    
    return offset;
}

uint64_t vkal_index_buffer_update(void *indices, uint32_t index_count, uint32_t offset) {
    uint64_t alignment = vkal_info.physical_device_properties.limits.nonCoherentAtomSize;
    uint32_t indices_in_bytes = index_count * vkal_index_size;
    uint64_t size = (indices_in_bytes + alignment - 1) & ~(alignment - 1);

    // map staging memory and upload index data
    void *staging_memory;
    VkResult result = vkMapMemory(vkal_info.device, vkal_info.device_memory_staging, 0, size, 0, &staging_memory);
    VKAL_ASSERT(result && "failed to map device staging memory!");
    flush_to_memory(vkal_info.device_memory_staging, staging_memory, indices, indices_in_bytes, 0);
    vkUnmapMemory(vkal_info.device, vkal_info.device_memory_staging);

    // copy vertex index data from staging memory (host visible) to device local memory through a command buffer
    VkCommandBuffer cmd_buffer = vkal_create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkBufferCopy buffer_copy = {0};
    buffer_copy.dstOffset = offset;
    buffer_copy.srcOffset = 0;
    buffer_copy.size = indices_in_bytes;
    vkCmdCopyBuffer(cmd_buffer, vkal_info.staging_buffer.buffer, vkal_info.default_index_buffer.buffer, 1,
                    &buffer_copy);
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1; // vkal_info.default_command_buffer_count;
    submit_info.pCommandBuffers = &cmd_buffer;  //vkal_info.default_command_buffers;
    vkQueueSubmit(vkal_info.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(vkal_info.device);

    // When mapping memory later again to copy into it (see:fluch_to_memory) we must respect
    // the devices alignment.
    // See: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMappedMemoryRange.html
    vkal_info.default_index_buffer_offset += size;

    return offset;
}

void vkal_index_buffer_reset(void)
{
    vkal_info.default_index_buffer_offset = 0;
}

VkDeviceAddress vkal_get_buffer_device_address(VkBuffer buffer)
{
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        NULL,
        buffer
    };

    return vkGetBufferDeviceAddress(vkal_info.device, &bufferDeviceAddressInfo);
}

void vkal_cleanup(void) {



    vkQueueWaitIdle(vkal_info.graphics_queue);
    
    VKAL_FREE(vkal_info.available_instance_extensions);
    VKAL_FREE(vkal_info.available_instance_layers);
    VKAL_FREE(vkal_info.physical_devices);
    VKAL_FREE(vkal_info.suitable_devices);
    
    cleanup_swapchain();

    vkDestroyRenderPass(vkal_info.device, vkal_info.render_pass, 0);
    vkDestroyRenderPass(vkal_info.device, vkal_info.render_to_image_render_pass, 0);

    for (uint32_t i = 0; i < vkal_info.default_commandpool_count; ++i) {
		vkFreeCommandBuffers(vkal_info.device, vkal_info.default_command_pools[0], 1, &vkal_info.default_command_buffers[i]);
    }
    for (uint32_t i = 0; i < vkal_info.default_commandpool_count; ++i) {
		vkDestroyCommandPool(vkal_info.device, vkal_info.default_command_pools[i], 0);
    }
	
    for (uint32_t i = 0; i < VKAL_MAX_VKDEVICEMEMORY; ++i) {
        vkal_destroy_device_memory(i);
    }
    vkFreeMemory(vkal_info.device, vkal_info.device_memory_staging, 0); 
    vkFreeMemory(vkal_info.device, vkal_info.default_device_memory_index, 0);
    vkFreeMemory(vkal_info.device, vkal_info.default_device_memory_uniform, 0);
    vkFreeMemory(vkal_info.device, vkal_info.default_device_memory_vertex, 0);
    
    for (uint32_t i = 0; i < VKAL_MAX_IMAGES_IN_FLIGHT; ++i) {
		vkDestroyFence(vkal_info.device, vkal_info.in_flight_fences[i], NULL);
		vkDestroySemaphore(vkal_info.device, vkal_info.render_finished_semaphores[i], NULL);
		vkDestroySemaphore(vkal_info.device, vkal_info.image_available_semaphores[i], NULL);
    }
    
    vkDestroyBuffer(vkal_info.device, vkal_info.default_uniform_buffer.buffer, 0);
    vkDestroyBuffer(vkal_info.device, vkal_info.default_vertex_buffer.buffer, 0);
    vkDestroyBuffer(vkal_info.device, vkal_info.default_index_buffer.buffer, 0);
    vkDestroyBuffer(vkal_info.device, vkal_info.staging_buffer.buffer, 0);
    
    for (uint32_t i = 0; i < VKAL_MAX_VKIMAGEVIEW; ++i) {
        vkal_destroy_image_view(i);
    }
    for (uint32_t i = 0; i < VKAL_MAX_VKIMAGE; ++i) {
        vkal_destroy_image(i);
    }

    for (uint32_t i = 0; i < VKAL_MAX_VKSHADERMODULE; ++i) {
		destroy_shader_module(i);
    }

    for (uint32_t i = 0; i < VKAL_MAX_VKPIPELINELAYOUT; ++i) {
		destroy_pipeline_layout(i);
    }

    for (uint32_t i = 0; i < VKAL_MAX_VKDESCRIPTORSETLAYOUT; ++i) {
		destroy_descriptor_set_layout(i);
    }

    for (uint32_t i = 0; i < VKAL_MAX_VKPIPELINE; ++i) {
		destroy_graphics_pipeline(i);
    }

    for (uint32_t i = 0; i < VKAL_MAX_VKSAMPLER; ++i) {
		destroy_sampler(i);
    }

    for (uint32_t i = 0; i < VKAL_MAX_VKFRAMEBUFFER; ++i) {
		destroy_framebuffer(i);
    }

    vkDestroyDescriptorPool(vkal_info.device, vkal_info.default_descriptor_pool, 0);
    
    vkDestroySurfaceKHR(vkal_info.instance, vkal_info.surface, 0);

#if defined (VKAL_GLFW)
    glfwDestroyWindow(vkal_info.window);
#elif defined (VKAL_WIN32)
    DestroyWindow(vkal_info.window);
#elif defined (VKAL_SDL)

#endif
    vkDestroyDevice(vkal_info.device, 0);
    vkDestroyInstance(vkal_info.instance, 0);

#ifdef VKAL_GLFW
    glfwTerminate();
#endif
}
