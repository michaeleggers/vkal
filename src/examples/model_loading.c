/* Michael Eggers, 9/20/2020

   The model matrices of the entities are provided to the shader through a dynamic uniform buffer.
   Note that only two models are loaded but for each a dedicated draw-call is issued. This
   is not very efficient. Instanced drawing should be used instead.
   The models come from an obj not using indexed drawing and a hard coded rect which, on the other
   hand uses indexed vertex data.
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <GLFW/glfw3.h>

#include "../vkal/vkal.h"
#include "utils/platform.h"
#define TRM_NDC_ZERO_TO_ONE
#include "utils/tr_math.h"
#include "utils/model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 800

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct ViewProjection
{
    mat4 view;
    mat4 proj;
} ViewProjection;

typedef struct ModelData
{
    mat4 model_mat;
} ModelData;

typedef struct ViewportData
{
    vec2 dimensions;
} ViewportData;

typedef struct Entity
{
    Model model;
    vec3 position;
    vec3 orientation;
    vec3 scale;
} Entity;

static GLFWwindow * window;
Platform p;

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        printf("escape key pressed\n");
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static void glfw_window_refresh_callback(GLFWwindow * window)
{
    glfwSwapBuffers(window);
}

void glfw_window_size_callback(GLFWwindow * window, int width, int height)
{
    printf("Window Size CB called!\n");
}

void glfw_window_pos_callback(GLFWwindow* window, int xpos, int ypos)
{
}

void init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vulkan", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetWindowRefreshCallback(window, glfw_window_refresh_callback);
    glfwSetWindowSizeCallback(window, glfw_window_size_callback);
    glfwSetWindowPosCallback(window, glfw_window_pos_callback);
    //glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    //glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
}

Image load_image_file(char const * file)
{
    Image image = (Image){0};
    int tw, th, tn;
    image.data = stbi_load(file, &tw, &th, &tn, 4);
    assert(image.data != NULL);
    image.width = tw;
    image.height = th;
    image.channels = tn;

    return image;
}

int main(int argc, char ** argv)
{
    init_window();
    init_platform(&p);

    char path[128];
    int path_size = 128*sizeof(char);
    p.get_exe_path(path, path_size);
    
    char * device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME
        // VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME /* is core already in Vulkan 1.2, not necessary */
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
        "VK_LAYER_KHRONOS_validation", //"VK_LAYER_LUNARG_standard_validation", <- deprecated!
        //"VK_LAYER_LUNARG_monitor"
    };
    uint32_t instance_layer_count = 0;
#ifdef _DEBUG
    instance_layer_count = sizeof(instance_layers) / sizeof(*instance_layers);    
#endif
   
    vkal_create_instance_glfw(window,
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
    
    /* Shader Setup */
    uint8_t * vertex_byte_code = 0;
    int vertex_code_size;
    p.read_file("../src/examples/assets/shaders/model_loading_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    p.read_file("../src/examples/assets/shaders/model_loading_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);

    /* Vertex Input Assembly */
    VkVertexInputBindingDescription vertex_input_bindings[] =
	{
	    { 0, 3*sizeof(vec3), VK_VERTEX_INPUT_RATE_VERTEX }
	};
    
    VkVertexInputAttributeDescription vertex_attributes[] =
	{
	    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },               // pos
	    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3) },    // normal
	    { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 2*sizeof(vec3) },  // color
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
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    1,
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 2);

    VkDescriptorSetLayoutBinding set_layout_dynamic[] = {
	{
	    0,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
	    1,
	    VK_SHADER_STAGE_VERTEX_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout_dynamic = vkal_create_descriptor_set_layout(set_layout_dynamic, 1);
    
    VkDescriptorSetLayout layouts[2];
    layouts[0] = descriptor_set_layout;
    layouts[1] = descriptor_set_layout_dynamic;
    
    uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);
    VkDescriptorSet * descriptor_sets = (VkDescriptorSet*)malloc(descriptor_set_layout_count*sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_sets);

    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	layouts, descriptor_set_layout_count, 
	NULL, 0);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE,
	vkal_info->render_pass, pipeline_layout);

    /* Model Data */
    float rect_vertices[] = {
		// Pos            // Normal       // Color       
		-1.0,  1.0, 1.0,  0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  
		 1.0,  1.0, 1.0,  0.0, 0.0, 1.0,  0.0, 1.0, 0.0,  
		-1.0, -1.0, 1.0,  0.0, 0.0, 1.0,  0.0, 0.0, 1.0,  
    	 1.0, -1.0, 1.0,  0.0, 0.0, 1.0,  1.0, 1.0, 0.0, 
    };
    uint32_t vertex_count = sizeof(rect_vertices)/sizeof(*rect_vertices);
    
    uint16_t rect_indices[] = {
 		0, 1, 2,
		2, 1, 3
    };
    uint32_t index_count = sizeof(rect_indices)/sizeof(*rect_indices);
  
    uint64_t offset_vertices = vkal_vertex_buffer_add(rect_vertices, 2*sizeof(vec3) + sizeof(vec2), 4);
    uint64_t offset_indices  = vkal_index_buffer_add(rect_indices, index_count);
    Model rect_model = { 0 };
    rect_model.is_indexed = 1;
    rect_model.vertex_buffer_offset = offset_vertices;
    rect_model.vertex_count = vertex_count;
    rect_model.index_buffer_offset = offset_indices;
    rect_model.index_count = index_count;
    
    Model model = {0};
    model.is_indexed = 0;
    float bmin[3], bmax[3];
    load_obj(bmin, bmax, "../src/examples/assets/models/lego.obj", &model);
    model.vertex_buffer_offset = vkal_vertex_buffer_add(model.vertices, 9*sizeof(float), model.vertex_count);
    clear_model(&model);

#define NUM_ENTITIES 1000
    /* Entities */
    Entity entities[NUM_ENTITIES];
    for (int i = 0; i < NUM_ENTITIES; ++i) {
    	vec3 pos   = { 0 };
        pos.x = rand_between(-25.f, 25.f);
        pos.y = rand_between(-15.f, 15.f);
        pos.z = rand_between(-15.f, 15.f);
        vec3 rot   = { 0 };
        rot.x = rand_between(-15.f, 15.f); rot.y = rand_between(-15.f, 15.f); rot.z = rand_between(-15.f, 15.f);
        float scale_xyz = rand_between(.01f, 3.f);
        vec3 scale = { 1, 1, 1 };
        //scale.x = scale_xyz; scale.y = scale_xyz; scale.z = scale_xyz;
        uint8_t model_type = (uint8_t)rand_between(0.0f, 1.99f);
        if (model_type == 0) {
            entities[i].model       = rect_model;
            entities[i].position    = pos;
            entities[i].orientation = rot;
            entities[i].scale       = scale;
        }
        else {
            entities[i].model       = model;
            entities[i].position    = pos;
            entities[i].orientation = rot;
            entities[i].scale       = scale;
        }
    }
    
    /* View Projection */
    mat4 view = mat4_identity();
    view = translate(view, (vec3){ 0.f, 0.f, -50.f });
    ViewProjection view_proj_data;
    view_proj_data.view = view;
    view_proj_data.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f );

    /* Uniform Buffers */
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);
    ViewportData viewport_data;
    viewport_data.dimensions = (vec2){ SCREEN_WIDTH, SCREEN_HEIGHT };
    UniformBuffer viewport_ubo = vkal_create_uniform_buffer(sizeof(ViewportData), 1, 1);
    vkal_update_descriptor_set_uniform(descriptor_sets[0], viewport_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_uniform(&viewport_ubo, &viewport_data);

    /* Dynamic Uniform Buffers */
    UniformBuffer model_ubo = vkal_create_uniform_buffer(sizeof(ModelData), NUM_ENTITIES, 0);
    ModelData * model_data = (ModelData*)malloc(NUM_ENTITIES*model_ubo.alignment);
    for (int i = 0; i < NUM_ENTITIES; ++i) {
        mat4 model_mat = mat4_identity();
        model_mat = translate(model_mat, entities[i].position);
        ((ModelData*)((uint8_t*)model_data + i*model_ubo.alignment))->model_mat = model_mat;
    }
    /* If NUM_ENTITIES is large the maxUniformBufferRange limit of the physical device properties may be
       violated when updating the descriptor set! To be sure that the requirements are always met 
       do not updated ranges larger than 16384 bytes.
       See: https://vulkan.lunarg.com/doc/view/1.2.148.1/windows/chunked_spec/chap40.html#limits-minmax
    */
    vkal_update_descriptor_set_uniform(descriptor_sets[1], model_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vkal_update_uniform(&model_ubo, model_data);
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
        // HACK on MacOS: Prevents laggy window move/resize.
        // see: https://stackoverflow.com/questions/19102189/noticable-lag-in-a-simple-opengl-program-with-mouse-input-through-glfw
        //glfwWaitEventsTimeout(0.0167);
        glfwPollEvents(); // OK on Windows
		
        /* Update View Projection Matrices */
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        view_proj_data.proj = perspective( tr_radians(45.f), (float)width/(float)height, 0.1f, 100.f );
        vkal_update_uniform(&view_proj_ubo, &view_proj_data);

        /* Update Info about screen */
        viewport_data.dimensions.x = (float)width;
        viewport_data.dimensions.y = (float)height;
        vkal_update_uniform(&viewport_ubo, &viewport_data);

        /* Update Model Matrices */
        for (int i = 0; i < NUM_ENTITIES; ++i) {
            mat4 model_mat = mat4_identity();
            static float d = 1.f;
            static float r = .01f;
            d += 0.00001f;
            // entities[i].position.x += sinf(d);
            entities[i].orientation.x += r;
            entities[i].orientation.y += r;
            entities[i].orientation.z += r;
            model_mat = translate(model_mat, entities[i].position);
            model_mat = tr_scale(model_mat, entities[i].scale);
            mat4 rot_x = rotate_x( entities[i].orientation.x );
            mat4 rot_y = rotate_y( entities[i].orientation.y );
            mat4 rot_z = rotate_z( entities[i].orientation.z );
            model_mat = mat4_x_mat4(model_mat, rot_x);
            model_mat = mat4_x_mat4(model_mat, rot_y);
            model_mat = mat4_x_mat4(model_mat, rot_z);
            ((ModelData*)((uint8_t*)model_data + i*model_ubo.alignment))->model_mat = model_mat;
        }
        vkal_update_uniform(&model_ubo, model_data);
        
        {
            uint32_t image_id = vkal_get_image();

            vkal_begin_command_buffer(image_id);

            vkal_begin_render_pass(image_id, vkal_info->render_pass);
            vkal_viewport(vkal_info->default_command_buffers[image_id],
                  0, 0,
                  (float)width, (float)height);
            vkal_scissor(vkal_info->default_command_buffers[image_id],
                 0, 0,
                 (float)width, (float)height);
            for (int i = 0; i < NUM_ENTITIES; ++i) {
                uint32_t dynamic_offset = (uint32_t)(i*model_ubo.alignment);
                vkal_bind_descriptor_sets(image_id, descriptor_sets, descriptor_set_layout_count,
                              &dynamic_offset, 1,
                              pipeline_layout);
                Model model_to_draw = entities[i].model;
                if (model_to_draw.is_indexed) {
                    vkal_draw_indexed(image_id, graphics_pipeline,
                              model_to_draw.index_buffer_offset, model_to_draw.index_count,
                              model_to_draw.vertex_buffer_offset);
                }
                else {
                    vkal_draw(image_id, graphics_pipeline,
                          model_to_draw.vertex_buffer_offset, model_to_draw.vertex_count);
                }
            }

            vkal_end_renderpass(image_id);

            vkal_end_command_buffer(image_id);
            VkCommandBuffer command_buffers[1];
            command_buffers[0] = vkal_info->default_command_buffers[image_id];
            vkal_queue_submit(command_buffers, 1);

            vkal_present(image_id);
        }
    }
    
    vkal_cleanup();

    glfwDestroyWindow(window);
 
    glfwTerminate();
    
    return 0;
}
