/* Michael Eggers, 27/12/2021

   The Hello World of computer graphics
*/


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>

#define  GLM_FORCE_RADIANS
#define  GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <vkal.h>

#include "platform.h"
//#include "tr_math.h"
#include <../utils/common.h>

#define  STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* current framebuffer width/height */
static int width  = SCREEN_WIDTH;
static int height = SCREEN_HEIGHT;

/* Max number of framedata on the GPU */
#define MAX_GPU_FRAMES 1024

static SDL_Window * window;

struct Camera
{
	glm::vec3 pos;
	glm::vec3 center;
	glm::vec3 up;
	glm::vec3 right;
};

struct PerFrameData
{
    glm::mat4   view;
    glm::mat4   proj;
    float       screenWidth, screenHeight;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

// Vertices arranged in CCW order
struct Quad {
    Vertex v0; // Top left
    Vertex v1; // Top right
    Vertex v2; // Bottom right
    Vertex v3; // Bottom left
};

//#pragma pack(push, 1)
struct GPUSprite {
    glm::mat4  transform;
    // col 0: 
    //       x = textureID, 
    //       y = current frame: Index into the GPUFrame buffer. Init this with 0 for now. If multiple spritesheets exist we need to offset to the correct location in the gpuFrameBuffer!
    glm::mat4  metaData;
};
//#pragma pack(pop)

struct GPUFrame {
    glm::vec2 uv;
    glm::vec2 widthHeight;
};

struct Frame {
    uint32_t x, y; // Top left in image
    uint32_t width, height;
};

struct Sequence {
    std::vector<Frame>  frames;
    uint32_t            first;
    uint32_t            last;
    uint32_t            current;
    double              timePerFrameMS;
    double              currentTimeMS;
};

struct Sprite {
    glm::vec3 pos;
    glm::vec3 velocity;
    uint32_t  textureID;
    Sequence  sequence;
};

struct Image
{
    uint32_t width, height, channels;
    unsigned char* data;
};

float rand_between(float min, float max)
{
    float range = max - min;
    float step = range / RAND_MAX;
    return (step * rand()) + min;
}

void init_window()
{
    SDL_Init(SDL_INIT_EVERYTHING);    
    
    window = SDL_CreateWindow(
        "Hello Vulkan SDL",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        printf("SDL_CreateWindow:\n%s\n", SDL_GetError());
        getchar();
        exit(-1);
    }

    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;
}

Image load_image_file(char const* file)
{
    char exe_path[256];
    get_exe_path(exe_path, 256 * sizeof(char));
    char abs_path[256];
    memcpy(abs_path, exe_path, 256);
    strcat(abs_path, file);

    Image image = { 0 };
    int tw, th, tn;
    image.data = stbi_load(abs_path, &tw, &th, &tn, 4);
    assert(image.data != NULL);
    image.width = tw;
    image.height = th;
    image.channels = tn;

    return image;
}

int main(int argc, char** argv)
{
    init_window();

    char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME
    };
    uint32_t device_extension_count = sizeof(device_extensions) / sizeof(*device_extensions);

    char* instance_extensions[] = {
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#ifdef _DEBUG
    ,VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };
    uint32_t instance_extension_count = sizeof(instance_extensions) / sizeof(*instance_extensions);

    char* instance_layers[] = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_monitor"
    };
    uint32_t instance_layer_count = 0;
#ifdef _DEBUG
    instance_layer_count = sizeof(instance_layers) / sizeof(*instance_layers);
#endif

    vkal_create_instance_sdl(window, instance_extensions, instance_extension_count, instance_layers, instance_layer_count);

    VkalPhysicalDevice* devices = 0;
    uint32_t device_count;
    vkal_find_suitable_devices(device_extensions, device_extension_count, &devices, &device_count);
    assert(device_count > 0);
    printf("Suitable Devices:\n");
    for (uint32_t i = 0; i < device_count; ++i) {
        printf("    Phyiscal Device %d: %s\n", i, devices[i].property.deviceName);
    }
    vkal_select_physical_device(&devices[0]);
    VkalInfo* vkal_info = vkal_init(device_extensions, device_extension_count);

    /* Shader Setup */
    uint8_t* vertex_byte_code = 0;
    int vertex_code_size;
    read_file("/../../src/examples/assets/shaders/instancing_vert.spv", &vertex_byte_code, &vertex_code_size);

    uint8_t* fragment_byte_code = 0;
    int fragment_code_size;
    read_file("/../../src/examples/assets/shaders/instancing_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup = vkal_create_shaders(vertex_byte_code, vertex_code_size, fragment_byte_code, fragment_code_size);
    
    /* Vertex Input Assembly */
    VkVertexInputBindingDescription vertex_input_bindings[] =
    {        // pos              // uv
        { 0, sizeof(glm::vec3) + sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    VkVertexInputAttributeDescription vertex_attributes[] =
    {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },					  // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec3) }          // UV 
    };
    uint32_t vertex_attribute_count = sizeof(vertex_attributes) / sizeof(*vertex_attributes);

    uint32_t numSprites = 100000;
    uint32_t maxTextures = 32;
    /* Descriptor Sets */
    VkDescriptorSetLayoutBinding set_layout[] = 
    {
        {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0
        },
        {
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            numSprites,
            VK_SHADER_STAGE_VERTEX_BIT,
            0
        },
        {
            3,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            MAX_GPU_FRAMES,
            VK_SHADER_STAGE_VERTEX_BIT,
            0
        },
        {
            2,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            maxTextures,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0
        }
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 4);

    VkDescriptorSetLayout layouts[] = {
        descriptor_set_layout
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts) / sizeof(*layouts);
    VkDescriptorSet* descriptor_sets = (VkDescriptorSet*)malloc(descriptor_set_layout_count * sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_sets);

    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(layouts, descriptor_set_layout_count, NULL, 0);
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
        vertex_input_bindings, 0,
        vertex_attributes, 0,
        shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        vkal_info->render_pass, pipeline_layout);

    /* Model Data */
    Quad quad{};
    quad.v0 = { glm::vec3(0, 0, 0), glm::vec2(0, 0) };
    quad.v1 = { glm::vec3(1, 0, 0), glm::vec2(1, 0) };
    quad.v2 = { glm::vec3(1, 1, 0), glm::vec2(1, 1) };
    quad.v3 = { glm::vec3(0, 1, 0), glm::vec2(0, 1) };

    uint16_t quadIndices[] = {
        2, 1, 0, // Upper right tri
        0, 3, 2  // Lower left tri
    };

    /* Textures */
    Image vulkanImage = load_image_file("../../src/examples/assets/textures/vklogo.jpg");
    VkalTexture vulkanTexture = vkal_create_texture(
        2, vulkanImage.data, vulkanImage.width, vulkanImage.height, 4, 0,
        VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    Image hkImage = load_image_file("../../src/examples/assets/textures/hk.jpg");
    VkalTexture hkTexture = vkal_create_texture(
        2, hkImage.data, hkImage.width, hkImage.height, 4, 0,
        VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

    Image asteroidsImage = load_image_file("../../src/examples/assets/textures/asteroidsheet.png");
    VkalTexture asteroidsTexture = vkal_create_texture(
        2, asteroidsImage.data, asteroidsImage.width, asteroidsImage.height, 4, 0,
        VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 1, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

    // Upload Model Data to GPU
	uint64_t offset_indices = vkal_index_buffer_add(quadIndices, 6);           // 3 indices
    Vertex vertices[] = { quad.v0, quad.v1, quad.v2, quad.v3 };
    uint64_t offset_vertices = vkal_vertex_buffer_add(vertices, sizeof(Vertex), 4); // 4 vertices

    /* Storage Buffer Spritedata (texture ID and frameIndex) per quad (= per InstanceID) */
    /* Firstly, get the memory on GPU */
    DeviceMemory gpuSpriteDeviceMem = vkal_allocate_devicememory(
        numSprites * sizeof(GPUSprite), 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        0);
    VkalBuffer gpuSpriteBuffer = vkal_create_buffer(numSprites * sizeof(GPUSprite), &gpuSpriteDeviceMem, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    /* Storage buffer for Frame Data: Stores a sequence of frames */
    DeviceMemory gpuFrameDeviceMem = vkal_allocate_devicememory(
        MAX_GPU_FRAMES * sizeof(GPUFrame),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        0);
    VkalBuffer gpuFrameBuffer = vkal_create_buffer(MAX_GPU_FRAMES * sizeof(GPUFrame), &gpuFrameDeviceMem, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    /* Create a sequence, that holds all the frames the sprites need to cycle through */
    Sequence asteroidSequence{};
    Frame asteroidFrame = {0, 0, 128, 128};
    uint32_t asteroidCols = asteroidsImage.width / asteroidFrame.width;
    uint32_t asteroidRows = asteroidsImage.height / asteroidFrame.height;
    size_t asteroidFrameCount = int(asteroidCols*asteroidRows) - 11;
    asteroidSequence.first = 0;
    asteroidSequence.last = asteroidFrameCount - 1;
    asteroidSequence.timePerFrameMS = 15.0;
    asteroidSequence.currentTimeMS = 0.0;
    for (size_t row = 0; row < asteroidRows; row++) {
        uint32_t y = row * asteroidFrame.height;
        asteroidFrame.y = y;
        for (size_t col = 0; col < asteroidCols; col++) {
            uint32_t x = (col * asteroidFrame.width) % asteroidsImage.width;
            asteroidFrame.x = x;
            asteroidSequence.frames.push_back(asteroidFrame);
        }        
    }

    /* Create some sprites that live on the CPU and are manipulated on CPU */
    Sprite* sprites = new Sprite[numSprites];
    for (size_t i = 0; i < numSprites; i++) {
        float xPos = rand_between(0.0f, (float)width);
        float yPos = rand_between(0.0f, (float)height);
        float zPos = -1.0f * i;
        glm::vec3 velocity = glm::vec3(rand_between(-1.0, 1.0), rand_between(-1.0, 1.0), 0.0f);
        uint32_t textureID = 2; // Asteroids
        asteroidSequence.current = (int)rand_between(0.0f, 100.0f); // Start each at a different frame in the spritesheet
        sprites[i] = {
            glm::vec3(xPos, yPos, zPos), 
            glm::normalize(velocity),
            textureID,
            asteroidSequence
        };
    }

    /* Secondly, upload GPU sprite-data to the GPU */
    GPUSprite gpuSpriteData[1]; 
    //gpuSpriteData[1] = { transform1 };
    float s = 10.0f;
    for (size_t i = 0; i < numSprites; i++) {    
        glm::vec3 pos = sprites[i].pos;
        uint32_t textureID = sprites[i].textureID;
        uint32_t currentFrame = sprites[i].sequence.current;
        glm::mat4 transform = glm::mat4(1.0);
        transform = glm::translate(transform, pos);
        glm::mat4 metaData = glm::mat4(0.0);
        metaData[0].x = (float)textureID;
        metaData[0].y = (float)currentFrame;
        gpuSpriteData[0] = { transform, metaData };
        vkal_update_buffer_offset(&gpuSpriteBuffer, (uint8_t*)gpuSpriteData, sizeof(GPUSprite), i*sizeof(GPUSprite));
        unmap_memory(&gpuSpriteBuffer);
    }
    map_memory(&gpuSpriteBuffer, numSprites * sizeof(GPUSprite), 0);

    /* Also upload per-frame data onto the GPU */
    /* We will use the asteroid spritesheet in this example */
    for (size_t i = 0; i < asteroidSequence.frames.size(); i++) {
        Frame* frame = &asteroidSequence.frames[i];
        float s = (float)frame->x / (float)asteroidsImage.width;
        float t = (float)frame->y / (float)asteroidsImage.height;
        float width = (float)frame->width / (float)asteroidsImage.width;
        float height = (float)frame->height / (float)asteroidsImage.height;
        GPUFrame gpuFrame = { glm::vec2(s, t), glm::vec2(width, height) };
        vkal_update_buffer_offset(&gpuFrameBuffer, (uint8_t*)&gpuFrame, sizeof(GPUFrame), i * sizeof(GPUFrame));
        unmap_memory(&gpuFrameBuffer);
    }
    //map_memory(&gpuFrameBuffer, asteroidSequence.frames.size() * sizeof(GPUFrame), 0);

    /* Update Descriptor Set */
    for (size_t i = 0; i < numSprites; i++) {
        vkal_update_descriptor_set_bufferarray(descriptor_sets[0], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, i, gpuSpriteBuffer);
    }
    for (size_t i = 0; i < MAX_GPU_FRAMES; i++) {
        vkal_update_descriptor_set_bufferarray(descriptor_sets[0], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, i, gpuFrameBuffer);        
    }
    for (size_t i = 0; i < maxTextures; i++) { // Update all so Vulkan does not complain
        vkal_update_descriptor_set_texturearray(descriptor_sets[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, i, vulkanTexture);
    }
    vkal_update_descriptor_set_texturearray(descriptor_sets[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, vulkanTexture);
    vkal_update_descriptor_set_texturearray(descriptor_sets[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, hkTexture);
    vkal_update_descriptor_set_texturearray(descriptor_sets[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, asteroidsTexture);

	// Setup the camera and setup storage for Uniform Buffer
    Camera camera{};
	camera.pos = glm::vec3(0.0f, 0.0f, 30.f);
	camera.center = glm::vec3(0.0f);
	camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    PerFrameData perFrameData{};
    perFrameData.view = glm::lookAt(camera.pos, camera.center, camera.up);

    // Uniform Buffer for View-Projection Matrix
    UniformBuffer perFrameUniformBuffer = vkal_create_uniform_buffer(sizeof(PerFrameData), 1, 0);
	vkal_update_descriptor_set_uniform(descriptor_sets[0], perFrameUniformBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	vkal_update_uniform(&perFrameUniformBuffer, &perFrameData);

    // Main Loop
    uint32_t verticesPerSprite = 6;
    uint32_t totalVertexCount = numSprites * verticesPerSprite;
    uint64_t end_time = 0;
    bool running = true;
    double dt = 0.0;
    double titleUpdateTimer = 0.0;
    while (running)
    {
        uint64_t start_time = SDL_GetTicks64();        

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                {
                    running = false;
                }
                break;
                }
            }
            break;
            }
        }
		
        SDL_Vulkan_GetDrawableSize(window, &width, &height);

		//view_proj_data.proj = adjust_y_for_vulkan_ndc * glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
        perFrameData.proj = adjust_y_for_vulkan_ndc  * glm::ortho(0.0f, (float)width, 0.0f, (float)height, 0.1f, 1000000.f);
        perFrameData.screenWidth = (float)width;
        perFrameData.screenHeight = (float)height;
		vkal_update_uniform(&perFrameUniformBuffer, &perFrameData);

        /* Update Sprites (game logic) and update on GPU */
        uint64_t timeUpdateFrame = 0;
        uint64_t startUpdateFrameTime = SDL_GetTicks64();
        for (size_t i = 0; i < numSprites; i++) {
            Sprite* sprite = &sprites[i];
            GPUSprite* gpuSprite = (GPUSprite*)gpuSpriteBuffer.mapped + i;
            
            if (sprite->pos.x >= width || sprite->pos.x < 0.0) sprite->velocity.x *= -1.0;
            if (sprite->pos.y >= height || sprite->pos.y < 0.0) sprite->velocity.y *= -1.0;
            sprite->pos.x += sprite->velocity.x;
            sprite->pos.y += sprite->velocity.y;
            gpuSprite->transform = glm::translate(glm::mat4(1), sprite->pos);

            sprite->sequence.currentTimeMS += dt;
            gpuSprite->metaData[0].y = sprite->sequence.current;
            if (sprite->sequence.currentTimeMS >= sprite->sequence.timePerFrameMS) {
                sprite->sequence.currentTimeMS = 0.0;
                sprite->sequence.current++;
                if (sprite->sequence.current > sprite->sequence.last) {
                    sprite->sequence.current = sprite->sequence.first;
                }
            }
        }        
        uint64_t endUpdateFrameTime = SDL_GetTicks64();
        timeUpdateFrame = endUpdateFrameTime - startUpdateFrameTime;

        {      
            uint32_t image_id = vkal_get_image();
               
            VkCommandBuffer currentCmdBuffer = vkal_info->default_command_buffers[image_id];

            vkal_set_clear_color({0.2f, 0.2f, 0.4f, 1.0f});

            vkal_begin_command_buffer(image_id);
            vkal_begin_render_pass(image_id, vkal_info->render_pass);
            vkal_viewport(currentCmdBuffer,
                0, 0,
                (float)width, (float)height);
            vkal_scissor(currentCmdBuffer,
                0, 0,
                (float)width, (float)height);
            vkal_bind_descriptor_set(image_id, &descriptor_sets[0], pipeline_layout);
            //vkal_draw(image_id, graphics_pipeline, 0, )
            //vkal_draw_indexed(image_id, graphics_pipeline,
            //    offset_indices, 6,
            //    offset_vertices, 2);
            //vkCmdDrawIndexedIndirect(vkal_info.default_command_buffers[image_id], )
            vkCmdBindPipeline(vkal_info->default_command_buffers[image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
            //vkCmdDraw(vkal_info->default_command_buffers[image_id], totalVertexCount, 1, 0, 0);
            vkCmdDraw(vkal_info->default_command_buffers[image_id], 6, numSprites, 0, 0);

            vkal_end_renderpass(image_id);
            vkal_end_command_buffer(image_id);

            vkal_queue_submit(&currentCmdBuffer, 1);

            vkal_present(image_id);
        }
        
        //SDL_Delay(6);

        end_time = SDL_GetTicks64();
        dt = (double)end_time - (double)start_time;
        titleUpdateTimer += dt;
        if (titleUpdateTimer > 1000.0) {
            char window_title[256];
            sprintf(window_title, "frametime: %fms (%f FPS) || UpdateAsteroidTime (ms): %d", dt, 1000.0 / (float)dt, timeUpdateFrame);
            SDL_SetWindowTitle(window, window_title);
            titleUpdateTimer = 0.0;
        }
        uint64_t timePassed = end_time - start_time;     
    }

	free(descriptor_sets);

    vkal_cleanup();

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
