#include <stdio.h>
#include <stdint.h>

#include <GLFW/glfw3.h>

#include "vkal.h"
#include "platform.h"

static GLFWwindow * window;
static Platform p;

void init_window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(VKAL_SCREEN_WIDTH, VKAL_SCREEN_HEIGHT, "Vulkan", 0, 0);
    //glfwSetKeyCallback(window, glfw_key_callback);
    //glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    //glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
}

int main(int argc, char ** argv)
{
    init_window();
    init_platform(&p);
    
    char * device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
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
#ifdef _DEBUG
	"VK_LAYER_LUNARG_standard_validation",
	"VK_LAYER_LUNARG_monitor"
#else
	0
#endif
    };
    uint32_t instance_layer_count = sizeof(instance_layers) / sizeof(*instance_layers);

    vkal_create_instance(window,
			 instance_extensions, instance_extension_count,
			 instance_layers, instance_layer_count);
    vkal_find_suitable_devices(device_extensions, device_extension_count);
    VkalInfo * vkal_info =  vkal_init(device_extensions, device_extension_count);

    
    vkal_cleanup();
    
    return 0;
}
