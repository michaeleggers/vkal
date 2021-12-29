/* Michael Eggers, 10/22/2020
   
   Simple example showing how to draw a rect (two triangles) and mapping
   a texture on it.
*/
#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "../vkal.h"

#include "utils/platform.h"
#include "utils/tr_math.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"
#define TRM_NDC_ZERO_TO_ONE
#include "utils/tr_math.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768

static HWND      window;

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct ViewProjection
{
    mat4  view;
    mat4  proj;
    float image_aspect;
} ViewProjection;

typedef struct Camera
{
	vec3 pos;
	vec3 center;
	vec3 up;
	vec3 right;
} Camera;


Image load_image_file(char const * file)
{
	char exe_path[256];
	get_exe_path(exe_path, 256);
	char* lastBackslash = exe_path;
	while (*lastBackslash != '\0') {
		lastBackslash++;
	}
	lastBackslash -= 1;
	*lastBackslash = '\0';
	char abs_path[256];
	memcpy(abs_path, exe_path, 256);
	strcat(abs_path, file);

    Image image = (Image){0};
    int tw, th, tn;
    image.data = stbi_load(abs_path, &tw, &th, &tn, 4);
    assert(image.data != NULL);
    image.width = tw;
    image.height = th;
    image.channels = tn;

    return image;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = { 0 };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// Create the window.
	window = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		L"WIN32 and Vulkan",		    // Window text
		WS_OVERLAPPEDWINDOW,            // Window style

										// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (window == NULL)
	{
		return 0;
	}

	ShowWindow(window, nCmdShow);

	// init console
	AllocConsole();
	FILE* pCin;
	FILE* pCout;
	FILE* pCerr;
	freopen_s(&pCin, "conin$", "r", stdin);
	freopen_s(&pCout, "conout$", "w", stdout);
	freopen_s(&pCerr, "conout$", "w", stderr);
    
	// Init VKAL
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
   
	vkal_create_instance_win32(
		window, hInstance,
		instance_extensions, instance_extension_count,
 		instance_layers, instance_layer_count);
    
    VkalPhysicalDevice * devices = 0;
    uint32_t device_count;
    vkal_find_suitable_devices(
		device_extensions, device_extension_count,
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
    read_file("/../../src/examples/assets/shaders/texture_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    read_file("/../../src/examples/assets/shaders/texture_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup = vkal_create_shaders(
		vertex_byte_code, vertex_code_size, 
		fragment_byte_code, fragment_code_size);

    /* Vertex Input Assembly */
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
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_set);
    
    /* Pipeline */
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

    /* Model Data */
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

    /* Uniform Buffer for view projection matrices */
    Camera camera;
    camera.pos = (vec3){ 0, 0.f, 5.f };
    camera.center = (vec3){ 0 };
    camera.up = (vec3){ 0, 1, 0 };
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    view_proj_data.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f );
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_set[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);
    
    /* Texture Data */
    Image image = load_image_file("/../../src/examples/assets/textures/vklogo.jpg");
    VkalTexture texture = vkal_create_texture(
		1, image.data, image.width, image.height, 4, 0,
		VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    free(image.data);
    vkal_update_descriptor_set_texture(descriptor_set[0], texture);
    view_proj_data.image_aspect = (float)texture.width/(float)texture.height;

    // Main Loop
	int running = 1;
    while (running)
    {
		MSG msg = { 0 };
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				running = 0;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		int width, height;
		RECT rect;
		GetClientRect(vkal_info->window, &rect);
		width = rect.right;
		height = rect.bottom;

		//glfwGetFramebufferSize(window, &width, &height);
		view_proj_data.proj = perspective(tr_radians(45.f), (float)width / (float)height, 0.1f, 100.f);
		vkal_update_uniform(&view_proj_ubo, &view_proj_data);

		{
			vkal_set_clear_color((VkClearColorValue){ 0.2f, 0.2f, 0.2f, 1.0f });
			vkDeviceWaitIdle(vkal_info->device);
			vkal_update_descriptor_set_texture(descriptor_set[0], texture);

			uint32_t image_id = vkal_get_image();

			vkal_begin_command_buffer(image_id);
			vkal_begin_render_pass(image_id, vkal_info->render_pass);
			vkal_viewport(vkal_info->default_command_buffers[image_id],
				0, 0,
				width, height);
			vkal_scissor(vkal_info->default_command_buffers[image_id],
				0, 0,
				width, height);
			vkal_bind_descriptor_set(image_id, &descriptor_set[0], pipeline_layout);
			vkal_draw_indexed(image_id, graphics_pipeline,
				offset_indices, index_count,
				offset_vertices);
			vkal_end_renderpass(image_id);
			vkal_end_command_buffer(image_id);
			VkCommandBuffer command_buffers1[] = { vkal_info->default_command_buffers[image_id] };
			vkal_queue_submit(command_buffers1, 1);

			vkal_present(image_id);
		}
		
		
    }

	// Close Console
	fclose(pCin);
	fclose(pCout);
	fclose(pCerr);
	FreeConsole();
    
    vkal_cleanup();
 
	DestroyWindow(window);

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
	}
	break;
	//case WM_PAINT:
	//{
	//	PAINTSTRUCT ps;
	//	HDC hdc = BeginPaint(hwnd, &ps);
	//		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
	//	EndPaint(hwnd, &ps);
	//}
	//return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
