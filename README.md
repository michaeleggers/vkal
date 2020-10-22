# VKAL

The Vulkan Abstraction Layer (short: VKAL) is being created in order to help setting up a Vulkan based
renderer. VKAL is still in development and the API is not final. It is written in C99 and compiles with
MSVC, CLANG and GCC.

## The reason for VKAL

Despite its name, VKAL does not abstract away the Vulkan API. It rather is a set of functions that wrap
many Vulkan-calls in order to make the developer's life easier. It does not prevent the user
to mix in Vulkan calls directly. In fact, VKAL expects the user to do this. If you need a
renderering API that is completely API-agnostic, VKAL is not for you. Have a look at
sokol_gfx which  is part of the Sokol single-header files if you need a clean rendering API that
does not require you to learn a specific graphics API such as DX11/12, OpenGL or Vulkan: 
[Sokol-Headers](https://github.com/floooh/sokol) created by Andre Weissflog.

If you're starting to learn Vulkan and you do want to take a more top-down approach in order to
boost your learning experience, then Vulkan  might be worth a look at. That being said, VKAL
is being developed simply because I want to get more familiar with the Vulkan-API myself.

## Dependencies

In order to initialize VKAL, the OS-specific Window-Handle is needed. At the moment
VKAL only supports GLFW3 to do this. GLFW3 can be found here: [GLFW3](https://www.glfw.org/).

Otherwise, VKAL does depend on the C Standard Library.

## Example

```c
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
VkalInfo * vkal_info =  vkal_init(device_extensions, device_extension_count);
```

