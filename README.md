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

## Using VKAL

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
vkal_create_instance(window, /* Created through GLFW3 */
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
Finally, all the created resources in previous steps are now needed for creating the graphics pipeline:
```c
VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	layouts, descriptor_set_layout_count, 
	NULL, 0); /* No push constants for now. */
VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout);
```

### Vertex- and Indexbuffers
Creating a rectangle out of two triangles using indexed mesh-data:
```c
float rect_vertices[] = {
	// Pos            // Color        // UV
	-1.0,  1.0, 1.0,  1.0, 0.0, 0.0,  0.0, 0.0,
	 1.0,  1.0, 1.0,  0.0, 1.0, 0.0,  1.0, 0.0,
	-1.0, -1.0, 1.0,  0.0, 0.0, 1.0,  0.0, 1.0,
	 1.0, -1.0, 1.0,  1.0, 1.0, 0.0,  1.0, 1.0
};
uint32_t vertex_count = sizeof(rect_vertices)/sizeof(*rect_vertices);

uint16_t rect_indices[] = {
	0, 1, 2,
	2, 1, 3
};
uint32_t index_count = sizeof(rect_indices)/sizeof(*rect_indices);

uint32_t offset_vertices = vkal_vertex_buffer_add(rect_vertices, 2*sizeof(vec3) + sizeof(vec2), 4);
uint32_t offset_indices  = vkal_index_buffer_add(rect_indices, index_count);
```
Firstly, data for vertex attributes and their indices are defined. Now, both `vkal_vertex_buffer_add` and
`vkal_index_buffer_add` put the data into buffers that are device local. The functions take care of transferring
the data via a staging buffer to GPU local memory. Both functions return an `uint32_t` describing the offset
into the buffers where the data is to be found. Both Vertex- and Indexbuffers used by
those functions are created during vkal initialization. The `VkDeviceMemory` objects which are
used back vertex- and indexbuffers with actual memory are also created during startup. That means rather
than creating (and allocating) new `VkDeviceMemory` objects for each new mesh to be rendered the same buffer
will be reused.

This way of using device memory is not only recommended by the official Vulkan Guide but also
by Nvidia, as described in further detail here: [Nvidia Vulkan Memory Management](https://developer.nvidia.com/vulkan-memory-management)

### Uniform Buffers
Like Vertex- and Indexbuffers, uniform buffers are put next to each other in memory sharing the same `VkDeviceMemory`
with respecting alignment required by the Vulkan implementation:

![alt text](https://github.com/michaeleggers/vkal/blob/master/readme_figures/uniform_buffer_alignment.png "Uniform Buffers")

Use
```c
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
```
to create a uniform buffer and update the correct descriptor set with data in `view_proj_data`, which is simply
a struct containing data about view and projection matrices.

To update the content of a uniform buffer use
```c
vkal_update_uniform(&view_proj_ubo, &view_proj_data);	
```

### Textures
To create a texture which can be sampled from:
```c
Texture texture = vkal_create_texture(
			0,                                                      /* binding */
			image.data, image.width, image.height, image.channels,  
			0,                                                      /* VkImageCreateFlags */  
			VK_IMAGE_VIEW_TYPE_2D, 
			0, 1,                                                   /* base mip-level, mip-level count */
			0, 1,                                                   /* base array-layer, array-layer count */
			VK_FILTER_LINEAR, VK_FILTER_LINEAR);                    

```
Then, the corresponding Descriptor has to be updated:
```c
vkal_update_descriptor_set_texture(descriptor_set[0], texture);	
```
### Renderloop

### Cleanup

## Building the examples




