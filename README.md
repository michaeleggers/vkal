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
boost your learning experience, then VKAL  might be worth a look at. That being said, VKAL
is being developed simply because I want to get more familiar with the Vulkan-API myself.

## Dependencies

In order to initialize VKAL, the OS-specific Window-Handle is needed. At the moment
VKAL only supports GLFW3 to do this. GLFW3 can be found here: [GLFW3](https://www.glfw.org/).

Otherwise, VKAL does depend on the C Standard Library.

## Example

### Initialization

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
   
/* Create the Vulkan instance */
vkal_create_instance(window,
   instance_extensions, instance_extension_count,
   instance_layers, instance_layer_count);
    
/* Check if a suitable physical device (GPU) exists for the demanded extensions */
VkalPhysicalDevice * devices = 0;
uint32_t device_count;
vkal_find_suitable_devices(device_extensions, device_extension_count,
                           &devices, &device_count);
assert(device_count > 0);
printf("Suitable Devices:\n");
for (uint32_t i = 0; i < device_count; ++i) {
  printf("    Phyiscal Device %d: %s\n", i, devices[i].property.deviceName);
}

/* The user may select a physical device at this point. We just take
   the first one available in the list.
*/
vkal_select_physical_device(&devices[0]);

/* Finally, initialize VKAL itself. All state will be accessible through vkal_info */
VkalInfo * vkal_info =  vkal_init(device_extensions, device_extension_count);
```

### Creating a Graphics Pipeline

Since the graphics pipeline is central to any Vulkan graphical application.
The creation of a VkPipeline object is divided into the following steps in VKAL:

1. Create Vertex- and Fragment Shader Stage Setup.
2. Define the layout for Vertex Input Assembly.
3. Create Descriptor Sets and Push Constants.
4. Create the (Graphics-)Pipeline using the resources created in previous steps.

For each of these steps VKAL provides a few helper-functions:

#### 1. Shader Stage Setup
```c
ShaderStageSetup shader_setup = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);
```
where `vertex_byte_code` holds the binary vertex-shader SPIR-V data of `vertex_code_size` bytes.
The same applies to the fragment shader using the 2nd and 3rd arguments (start counting at 0 ;) ).

#### 2. Vertex Input Assembly Layout
```c
VkVertexInputBindingDescription vertex_input_bindings[] =
{
    { 0, 2*sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
};

VkVertexInputAttributeDescription vertex_attributes[] =
{
    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },               // pos
    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3) },    // color
    { 2, 0, VK_FORMAT_R32G32_SFLOAT,    2*sizeof(vec3) },  // UV 
};
uint32_t vertex_attribute_count = sizeof(vertex_attributes)/sizeof(*vertex_attributes);
```
As you can see, this is nothing special. This is bare Vulkan code. both arrays will be
needed for creating the pipeline later...

#### 3. Create Descriptor Sets and Push Constants
While both of them actually are optional, every meaningful application will
probably at least use a Descriptor Set to pass some data to shaders:
```c
VkDescriptorSetLayoutBinding set_layout[] = {
	{
	    0,
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	    VKAL_MAX_TEXTURES, /* Texture Array-Count: How many Textures do we need? */
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
};

VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 1);

VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout
};
uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);

VkDescriptorSet * descriptor_sets = (VkDescriptorSet*)malloc(descriptor_set_layout_count*sizeof(VkDescriptorSet));
vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, 1, &descriptor_sets);
```
`vkal_create_descriptor_set_layout` creates - as its name implies - a `VkDescriptorSetLayout`. Those are
placed in the `layouts` array. Remember that the order of layouts in the array defines the Set's index.
In this example there is only one layout, so its index is 0.
Then `descriptor_set_layout_count` `VkDescriptorSet`s are created. They need to be allocated
from a descriptor pool. A default descriptor pool is created when calling `vkal_init(...)`. We refer
to this pool through `vkal_info`. If the pool does not serve your needs then you can change
the amount of resources to be available for allocation in `vkal.c` in the function `create_descriptor_pool(void)`.

#### 4. Creating the Graphics Pipeline
```c
VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	layouts, descriptor_set_layout_count, 
	NULL, 0);
VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout);
```

