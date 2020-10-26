# VKAL

The Vulkan Abstraction Layer (short: VKAL) is being created in order to help setting up a Vulkan based
renderer. VKAL is still in development and the API is not final. It is written in C99 and compiles with
MSVC, CLANG and GCC.

## The reason for VKAL

Despite its name, VKAL does not completely abstract away the Vulkan API. It rather is a set of functions that wrap
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

### Building
Either MSVC, CLANG or GCC are fine. It is important to define `VKAL_GLFW` through a command line parameter
so VKAL will you GLFW3 for window-creation.

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

The graphics pipeline is central to any Vulkan application that draws something
onto the screen.
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
As you can see, this is nothing special. This is bare Vulkan code. Both arrays will be
needed for creating the pipeline later...

#### 3. Create Descriptor Sets and Push Constants
While both of them actually are optional, every meaningful application will
probably at least use a Descriptor Set to pass some data to shaders:
```c
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
	    1,
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
};

VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 2);

VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout
};
uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);

VkDescriptorSet * descriptor_set = (VkDescriptorSet*)malloc(descriptor_set_layout_count*sizeof(VkDescriptorSet));
vkal_allocate_descriptor_sets(vkal_info->descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_set);
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
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(
    					sizeof(view_proj_data), 
					1,  /* element count */ 
					0); /* binding */
    vkal_update_descriptor_set_uniform(descriptor_set[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
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
			1,                                                      /* binding */
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
```c
while (!glfwWindowShouldClose(window))
{
    glfwPollEvents();
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    view_proj_data.proj = perspective( tr_radians(45.f), (float)width/(float)height, 0.1f, 100.f );
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);

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
        vkal_bind_descriptor_set(image_id, &descriptor_set[0], pipeline_layout);
        vkal_draw_indexed(image_id, graphics_pipeline,
		          offset_indices, index_count,
		          offset_vertices);
        vkal_end_renderpass(image_id);
        vkal_end_command_buffer(image_id);
        VkCommandBuffer command_buffers1[] = { vkal_info->command_buffers[image_id] };
        vkal_queue_submit(command_buffers1, 1);

        vkal_present(image_id);
    }
}
```
After updating the projection matrix based on the framebuffer dimensions a swapchain's image-index
is returned via `vkal_get_image()`. This id is being used throughout the following calls. Since
both Scissor and Viewport states are set to dynamic implicitly during pipeline-creation they must be
updated. The descriptor set, which was initialized above is being bound and a draw-call is issued. 
VKAL drawing commands come in two flavors: `vkal_draw` and `vkal_draw_indexed` for un-indexed and indexed
vertex-data respectively. Remember the offsets we got returned from the calls to `vkal_vertex(index)_buffer_add(...)`?
We use those offsets to tell the draw command where the vertex- and indexdata is to be found in both the Vertex-
and Indexbuffers.
All calls are encapsulated by both a begin/end pair of both command-buffer and renderpass-calls.
The defautl renderpass and command-buffers are used which are created during VKAL initialization.

The result of this example:
![alt text](https://github.com/michaeleggers/vkal/blob/master/readme_figures/texture_example_result.png "textures.c result")

### Cleanup
When creating buffers, pipelines, descriptor sets, image views and so on using the functions above, VKAL will
keep track of the resources. This is being done through a simple arrays where each resource is associated with
an id that is used to look up the Vulkan handle for the resource in an array. Thus, in the end, VKAL just
iterates through these arrays and destroys all Vulkan resources. The function to call is:
```c
vkal_cleanup();
```
Since VKAL needs the window handle through GLFW3 or SDL2 (not yet implemented) you should also shut down GLFW3 (or SDL2)
properly:
```c
glfwDestroyWindow(window);
glfwTerminate();
```
The full code for this example presented can be found [here](https://github.com/michaeleggers/vkal/blob/master/src/examples/texture.c)

### Examples

## Building

The following libraries are required:

* [stb_image](https://github.com/nothings/stb) for loading image-data (PNG, JPG, ...)
* [tinyobjloader-c](https://github.com/syoyo/tinyobjloader-c) for loading OBJ model files
* [GLFW3](https://www.glfw.org/) for input handling and window-creation

Thanks to the authors of those libs, creating the examples was much easier. Thank you!

Since the examples use some win32 specific code to load files from disk the examples will
not build on any other platform at the moment. Sorry!

The root directory contains a `build.bat` that can build the examples.

| Example        | What it does |
| ------------- |:-------------:|
| [texture.c](https://github.com/michaeleggers/vkal/blob/readme-update/src/examples/texture.c) | Load an image from disk and map it onto a rectangle (two triangles). |
| [textures.c](https://github.com/michaeleggers/vkal/blob/readme-update/src/examples/textures.c)      | Two different images are loaded and displayed by using distinct descriptors. |
| [textures_dynamic_descriptor.c](https://github.com/michaeleggers/vkal/blob/readme-update/src/examples/textures_dynamic_descriptor.c) | Using a dynamic descriptor-set only one descriptor-set has to be created for multiple textures. |
| [textures_descriptorarray_push_constant.c](https://github.com/michaeleggers/vkal/blob/readme-update/src/examples/textures_descriptorarray_push_constant.c)] | Using a descriptor-array each texture gets its own index into the array. The indices get transmitted via a push constant during command-buffer recording. |
| [rendertexture.c](https://github.com/michaeleggers/vkal/blob/readme-update/src/examples/rendertexture.c) | Two textures get rendered into a texture. They get sampled from in a second pass to do some image-compositing. |
| [model_loading.c](https://github.com/michaeleggers/vkal/blob/readme-update/src/examples/model_loading.c) | Two different meshes get rendered multiple times using a dynamic uniform buffer for the model-matrices. |



