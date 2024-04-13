/* Michael Eggers, 10/22/2020
   
   Drawing lines, filled rectangles, circles, filled circles and textured rectangles into Host-side buffers and copy
   them over to GPU (Host-visible) memory every frame. Render commands are used to group
   similar draw-calls together. If primitive drawings are performed one right after another
   we can use the same pipeline and therefore group all those primitives together. Only if
   a textured rectangle is used we have to use a new drawcall since the shaders differ
   for textured rects from regular lines, rects, and so on...
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <GLFW/glfw3.h>

#include <vkal.h>
#include "platform.h"
#include "tr_math.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define TRM_NDC_ZERO_TO_ONE
#include "tr_math.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 768

#define MAX_PRIMITIVES                 2*8192
#define PRIMITIVES_VERTEX_BUFFER_SIZE  (MAX_PRIMITIVES * 4 * sizeof(Vertex))
#define PRIMITIVES_INDEX_BUFFER_SIZE   (MAX_PRIMITIVES * 6 * sizeof(uint16_t))

typedef struct ViewProjection
{
    mat4  view;
    mat4  proj;
} ViewProjection;

typedef struct Camera
{
	vec3 pos;
	vec3 center;
	vec3 up;
	vec3 right;
} Camera;

typedef struct Vertex
{
    vec3 pos;
    vec3 color;
    vec2 uv;
} Vertex;

typedef struct Image
{
    uint32_t width, height, channels;
    unsigned char * data;
} Image;

typedef struct MyTexture
{
    VkalTexture texture;
    uint32_t id;
} MyTexture;

typedef struct Batch
{
    Vertex * vertices;
    uint32_t vertex_count;
    uint16_t * indices;
    uint32_t index_count;
    uint32_t rect_count;
    uint32_t poly3_count;

    DeviceMemory index_memory;
    DeviceMemory vertex_memory;
    VkalBuffer       index_buffer;
    VkalBuffer       vertex_buffer;
} Batch;

typedef enum RenderCmdType
{
    RENDER_CMD_STD,
    RENDER_CMD_TEXTURED_RECT
} RenderCmdType;

typedef struct RenderCmd
{
    RenderCmdType type;
    uint32_t      index_buffer_offset;
    uint32_t      index_count;
    uint32_t      index_buffer_size;
    uint32_t      vertex_buffer_offset;
    uint32_t      vertex_buffer_size;

    uint32_t      texture_id;

    Batch *       batch;
} RenderCmd;

void line(float x0, float y0, float x1, float y1, float thickness, vec3 color);
void fill_circle(float x, float y, float r, uint32_t spans, vec3 color);

/* Globals */
static GLFWwindow * window;
static int          width, height; /* current framebuffer width/height */
static Batch        g_default_batch = { 0 };
static Batch        g_persistent_batch = { 0 };
static RenderCmd    render_commands[1024];
static uint32_t     render_cmd_count;

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
	printf("escape key pressed\n");
	glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	double gridx_step = 50;
	double gridy_step = 50;
	double xposi = xpos/gridx_step;
	double yposi = ypos/gridy_step;
	if ( (xposi - (int)xposi) > 0.5 ) {
	    xpos = (int)xposi * 50 + 50;
	}
	else {
	    xpos = (int)xposi * 50;
	}
	if ( (yposi - (int)yposi) > 0.5 ) {
	    ypos = (int)yposi * 50 + 50;
	}
	else {
	    ypos = (int)yposi * 50;
	}
	
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
	    printf("right mouse button clicked at: %f, %f\n", xpos, ypos);
	}
	static int click_state = 0;
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
	    printf("left mouse button clicked at: %f, %f\n", xpos, ypos);
	    click_state++;
	    static double x_old;
	    static double y_old;
	    if (click_state == 1) {
		fill_circle( xpos, ypos, 5, 16, {1, 1, 0} );
	    }
	    else if (click_state == 2) {
		fill_circle( xpos, ypos, 5, 16, {1, 1, 0} );
		line( x_old, y_old, xpos, ypos, 2, {1, 1, 1});
		click_state = 0;
	    }
	    x_old = xpos;
	    y_old = ypos;
	}
    }
}

void init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Dynamic Primitives", 0, 0);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;
}

Image load_image_file(char const * file)
{
    char exe_path[256];
    get_exe_path(exe_path, 256 * sizeof(char));
    char abs_path[256];
    memcpy(abs_path, exe_path, 256);
    strcat(abs_path, file);

    Image image = { };
    int tw, th, tn;
    image.data = stbi_load(abs_path, &tw, &th, &tn, 4);
    assert(image.data != NULL);
    image.width = tw;
    image.height = th;
    image.channels = tn;

    return image;
}

void create_batch(VkalInfo * vkal_info, Batch * batch)
{
    batch->index_memory = vkal_allocate_devicememory(PRIMITIVES_INDEX_BUFFER_SIZE,
						    VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
						    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
						    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |			 
						    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);
    batch->index_buffer = vkal_create_buffer(PRIMITIVES_INDEX_BUFFER_SIZE,
					    &batch->index_memory,
					    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VKAL_DBG_BUFFER_NAME(vkal_info->device, batch->index_buffer, "Index Buffer");
    vkal_map_buffer(&batch->index_buffer);

    batch->vertex_memory = vkal_allocate_devicememory(PRIMITIVES_VERTEX_BUFFER_SIZE,
						     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
						     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
						     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |			 
						     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);
    batch->vertex_buffer = vkal_create_buffer(PRIMITIVES_VERTEX_BUFFER_SIZE,
					     &batch->vertex_memory,
					     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	VKAL_DBG_BUFFER_NAME(vkal_info->device, batch->vertex_buffer, "Vertex Buffer");
    vkal_map_buffer(&batch->vertex_buffer);
    
    batch->indices = (uint16_t*)malloc(PRIMITIVES_INDEX_BUFFER_SIZE);
    batch->vertices = (Vertex*)malloc(PRIMITIVES_VERTEX_BUFFER_SIZE);
}

void update_batch(Batch * batch)
{
    memcpy(batch->index_buffer.mapped, batch->indices, PRIMITIVES_INDEX_BUFFER_SIZE);
    memcpy(batch->vertex_buffer.mapped, batch->vertices, PRIMITIVES_VERTEX_BUFFER_SIZE);      
}

void destroy_batch(VkalInfo * vkal_info, Batch * batch)
{
    vkDestroyBuffer(vkal_info->device, batch->index_buffer.buffer, NULL);
    vkDestroyBuffer(vkal_info->device, batch->vertex_buffer.buffer, NULL);
    vkFreeMemory(vkal_info->device, batch->index_memory.vk_device_memory, NULL);
    vkFreeMemory(vkal_info->device, batch->vertex_memory.vk_device_memory, NULL);
}

MyTexture create_texture(char const * file, uint32_t id)
{
    MyTexture my_texture;
    Image img = load_image_file(file);
    VkalTexture tex = vkal_create_texture(1, img.data, img.width, img.height, 4,
				      0, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
				      0, 1,
				      0, 1,
				      VK_FILTER_LINEAR, VK_FILTER_LINEAR,
				      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    
    my_texture.texture = tex;
    my_texture.id = id;
    return my_texture;
}

void reset_batch(Batch * batch)
{
    batch->index_count = 0;
    batch->vertex_count = 0;
    batch->rect_count = 0;
    batch->poly3_count = 0;
}

void fill_rect(float x, float y, float width, float height, vec3 color)
{
    RenderCmd * render_cmd_ptr = NULL;
    if (render_cmd_count >= 1) {
	RenderCmd * last_render_cmd = &render_commands[render_cmd_count - 1];
	if ( last_render_cmd->type == RENDER_CMD_STD ) {
	    last_render_cmd->index_count += 6;	    
	    render_cmd_ptr = last_render_cmd;
	}
    }
    if (render_cmd_ptr == NULL) {
	RenderCmd render_cmd;       
	render_cmd.type                     = RENDER_CMD_STD;
	render_cmd.index_buffer_offset      = g_default_batch.index_count*sizeof(uint16_t);
	render_cmd.index_count              = 6;
	render_cmd.batch                    = &g_default_batch;
	render_commands[render_cmd_count++] = render_cmd;
	render_cmd_ptr = &render_commands[render_cmd_count - 1];
    }
    
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;

    tl.pos = {x,y,-1};
    bl.pos = {x, y+height,-1};
    tr.pos = {x+width, y, -1};
    br.pos = {x+width, y+height, -1};
    tl.color = color;
    bl.color = color;
    tr.color = color;
    br.color = color;
    
    Batch * batch = render_cmd_ptr->batch;
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 3;
    batch->rect_count++;
}

void fill_rect_cmd(RenderCmd * render_cmd, float x, float y, float width, float height, vec3 color)
{
    render_cmd->index_count += 6;
    
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;

    tl.pos = {x,y,-1};
    bl.pos = {x, y+height,-1};
    tr.pos = {x+width, y, -1};
    br.pos = {x+width, y+height, -1};
    tl.color = color;
    bl.color = color;
    tr.color = color;
    br.color = color;

    Batch * batch = render_cmd->batch;
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 3;
    batch->rect_count++;
}

void add_render_cmd(RenderCmd cmd)
{
    render_commands[render_cmd_count++] = cmd;
}

void textured_rect(float x, float y, float width, float height, MyTexture texture)
{
    RenderCmd * render_cmd_ptr = NULL;
    if (render_cmd_count >= 1) {
	RenderCmd * last_render_cmd = &render_commands[render_cmd_count - 1];
	if ( (last_render_cmd->type == RENDER_CMD_TEXTURED_RECT) && (last_render_cmd->texture_id == texture.id) ) {
	    last_render_cmd->index_count += 6;
	    render_cmd_ptr = last_render_cmd;
	}
    }
    if (render_cmd_ptr == NULL) {
	RenderCmd render_cmd;
	render_cmd.type                     = RENDER_CMD_TEXTURED_RECT;
	render_cmd.index_buffer_offset      = g_default_batch.index_count*sizeof(uint16_t);
	render_cmd.index_count              = 6;
	render_cmd.texture_id               = texture.id;
	render_cmd.batch                    = &g_default_batch;
	render_commands[render_cmd_count++] = render_cmd;
	render_cmd_ptr = &render_commands[render_cmd_count - 1];
    }
	
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;

    tl.pos = {x,y,-1};
    bl.pos = {x, y+height,-1};
    tr.pos = {x+width, y, -1};
    br.pos = {x+width, y+height, -1};
    tl.uv = {0, 0};
    bl.uv = {0, 1};
    tr.uv = {1, 0};
    br.uv = {1, 1};

    Batch * batch = render_cmd_ptr->batch;
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 3;
    batch->rect_count++;
}

void line(float x0, float y0, float x1, float y1, float thickness, vec3 color)
{
    RenderCmd * render_cmd_ptr = NULL;
    if (render_cmd_count >= 1) {
	RenderCmd * last_render_cmd = &render_commands[render_cmd_count - 1];
	if ( last_render_cmd->type == RENDER_CMD_STD ) {
	    last_render_cmd->index_count += 6;
	    render_cmd_ptr = last_render_cmd;
	}
    }
    if (render_cmd_ptr == NULL) {
	RenderCmd render_cmd;
	render_cmd.type                     = RENDER_CMD_STD;
	render_cmd.index_buffer_offset      = g_default_batch.index_count*sizeof(uint16_t);
	render_cmd.index_count              = 6;
	render_cmd.batch                    = &g_default_batch;
	render_commands[render_cmd_count++] = render_cmd;
	render_cmd_ptr = &render_commands[render_cmd_count - 1];
    }
    
    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;
    
    float dx = fabs(x1 - x0);
    float dy = fabs(y1 - y0);
    /* The lines' normals are (-dy, dx), (dy, =dx)
       but we only are interested in (dy, dx) because
       it is easier to reason about when applying the
       normal components as offsets.
    */
    vec2 normal = {dy, dx};
    normal = vec2_normalize(normal);
    float offset_x = thickness*normal.x/2.0;
    float offset_y = thickness*normal.y/2.0;
    if ( (x1 > x0) && (y1 > y0) ) { /* bottom right */
	tl.pos = {x0 + offset_x, y0 - offset_y, -1};
	bl.pos = {x0 - offset_x, y0 + offset_y, -1};
	tr.pos = {x1 + offset_x, y1 - offset_y, -1};
	br.pos = {x1 - offset_x, y1 + offset_y, -1};
    }
    else if ( (x1 > x0) && (y1 < y0) ) { /* top right */
	tl.pos = {x0 - offset_x, y0 - offset_y, -1};
	bl.pos = {x0 + offset_x, y0 + offset_y, -1};
	tr.pos = {x1 - offset_x, y1 - offset_y, -1};
	br.pos = {x1 + offset_x, y1 + offset_y, -1};
    }
    else if ( (x1 < x0) && (y1 < y0) ) { /* top left */
	tl.pos = {x0 - offset_x, y0 + offset_y, -1};
	bl.pos = {x0 + offset_x, y0 - offset_y, -1};
	tr.pos = {x1 - offset_x, y1 + offset_y, -1};
	br.pos = {x1 + offset_x, y1 - offset_y, -1};
    }
    else { /* bottom left*/
	tl.pos = {x0 + offset_x, y0 + offset_y, -1};
	bl.pos = {x0 - offset_x, y0 - offset_y, -1};
	tr.pos = {x1 + offset_x, y1 + offset_y, -1};
	br.pos = {x1 - offset_x, y1 - offset_y, -1};
    }

    tl.color = color;
    bl.color = color;
    tr.color = color;
    br.color = color;
    
    Batch * batch = render_cmd_ptr->batch;
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 3;
    batch->rect_count++;
}

void line_cmd(RenderCmd * render_cmd, float x0, float y0, float x1, float y1, float thickness, vec3 color)
{
    render_cmd->index_count += 6;

    Vertex tl;
    Vertex bl;
    Vertex tr;
    Vertex br;
    
    float dx = fabs(x1 - x0);
    float dy = fabs(y1 - y0);
    /* The lines' normals are (-dy, dx), (dy, =dx)
       but we only are interested in (dy, dx) because
       it is easier to reason about when applying the
       normal components as offsets.
    */
    vec2 normal = {dy, dx};
    normal = vec2_normalize(normal);
    float offset_x = thickness*normal.x/2.0;
    float offset_y = thickness*normal.y/2.0;
    if ( (x1 > x0) && (y1 > y0) ) { /* bottom right */
	tl.pos = {x0 + offset_x, y0 - offset_y, -1};
	bl.pos = {x0 - offset_x, y0 + offset_y, -1};
	tr.pos = {x1 + offset_x, y1 - offset_y, -1};
	br.pos = {x1 - offset_x, y1 + offset_y, -1};
    }
    else if ( (x1 > x0) && (y1 < y0) ) { /* top right */
	tl.pos = {x0 - offset_x, y0 - offset_y, -1};
	bl.pos = {x0 + offset_x, y0 + offset_y, -1};
	tr.pos = {x1 - offset_x, y1 - offset_y, -1};
	br.pos = {x1 + offset_x, y1 + offset_y, -1};
    }
    else if ( (x1 < x0) && (y1 < y0) ) { /* top left */
	tl.pos = {x0 - offset_x, y0 + offset_y, -1};
	bl.pos = {x0 + offset_x, y0 - offset_y, -1};
	tr.pos = {x1 - offset_x, y1 + offset_y, -1};
	br.pos = {x1 + offset_x, y1 - offset_y, -1};
    }
    else { /* bottom left*/
	tl.pos = {x0 + offset_x, y0 + offset_y, -1};
	bl.pos = {x0 - offset_x, y0 - offset_y, -1};
	tr.pos = {x1 + offset_x, y1 + offset_y, -1};
	br.pos = {x1 - offset_x, y1 - offset_y, -1};
    }

    tl.color = color;
    bl.color = color;
    tr.color = color;
    br.color = color;

    Batch * batch = render_cmd->batch;
    batch->vertices[batch->vertex_count++] = tl;
    batch->vertices[batch->vertex_count++] = bl;
    batch->vertices[batch->vertex_count++] = tr;
    batch->vertices[batch->vertex_count++] = br;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 1;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 3;
    batch->rect_count++;
}

void poly3(vec2 xy1, vec2 xy2, vec2 xy3, vec3 color)
{
    RenderCmd * render_cmd_ptr = NULL;
    if (render_cmd_count >= 1) {
	RenderCmd * last_render_cmd = &render_commands[render_cmd_count - 1];
	if ( last_render_cmd->type == RENDER_CMD_STD ) {
	    last_render_cmd->index_count += 3;
	    render_cmd_ptr = last_render_cmd;
	}
    }
    if (render_cmd_ptr == NULL) {
	RenderCmd render_cmd;
	render_cmd.type                     = RENDER_CMD_STD;
	render_cmd.index_buffer_offset      = g_default_batch.index_count*sizeof(uint16_t);
	render_cmd.index_count              = 3;
	render_cmd.batch                    = &g_default_batch;
	render_commands[render_cmd_count++] = render_cmd;
	render_cmd_ptr = &render_commands[render_cmd_count - 1];
    }
    
    vec3 xyz1;
    vec3 xyz2;
    vec3 xyz3;
    Vertex v1;
    Vertex v2;
    Vertex v3;
    xyz1.x = xy1.x; xyz1.y = xy1.y; xyz1.z = -1;
    xyz2.x = xy2.x; xyz2.y = xy2.y; xyz2.z = -1;
    xyz3.x = xy3.x; xyz3.y = xy3.y; xyz3.z = -1;
    v1.pos = xyz1; v2.pos = xyz2; v3.pos = xyz3;
    v1.color = color; v2.color = color; v3.color = color;
    
    Batch * batch = render_cmd_ptr->batch;
    batch->vertices[batch->vertex_count++] = v1;
    batch->vertices[batch->vertex_count++] = v2;    
    batch->vertices[batch->vertex_count++] = v3;

    uint32_t offset = 4*batch->rect_count + 3*batch->poly3_count;
    batch->indices[batch->index_count++] = offset + 0;
    batch->indices[batch->index_count++] = offset + 2;
    batch->indices[batch->index_count++] = offset + 1;

    batch->poly3_count++;
}

void circle(float x, float y, float r, uint32_t spans, float thickness, vec3 color)
{
    float theta = 2*TR_PI/(float)spans;
    for (uint32_t i = 0; i < spans; ++i) {
	vec2 xy;
	vec2 xy2;
	xy.x = x + r * cosf( (i+1)*theta );
	xy.y = y + r * sinf( (i+1)*theta );
	xy2.x = x + r * cosf( (i+2)*theta );
	xy2.y = y + r * sinf( (i+2)*theta );
	line( xy.x, xy.y, xy2.x, xy2.y, thickness, color );
    }
}

void fill_circle(float x, float y, float r, uint32_t spans, vec3 color)
{
    float theta = 2*TR_PI/(float)spans;
    for (uint32_t i = 0; i < spans; ++i) {
	vec2 xy1;
	vec2 xy2;
	vec2 xy3;
	xy1.x = x;
	xy1.y = y;
	xy2.x = x + r * cosf( (i+1)*theta );
	xy2.y = y + r * sinf( (i+1)*theta );
	xy3.x = x + r * cosf( (i+2)*theta );
	xy3.y = y + r * sinf( (i+2)*theta );
	poly3(xy1, xy2, xy3, color);
    }
}

int main(int argc, char ** argv)
{
    init_window();
    
    char * device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME
    };
    uint32_t device_extension_count = sizeof(device_extensions) / sizeof(*device_extensions);

    char* instance_extensions[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        #ifdef __APPLE__
            ,VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        #endif
        #ifdef _DEBUG
            ,VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        #endif
    };
    uint32_t instance_extension_count = sizeof(instance_extensions) / sizeof(*instance_extensions);

    char* instance_layers[] = {
        "VK_LAYER_KHRONOS_validation"
#if defined(WIN32) || defined(WIN32)
        ,"VK_LAYER_LUNARG_monitor" // Not available on MacOS!
#endif
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
   
    VkalWantedFeatures vulkan_features{};
    vulkan_features.features12.runtimeDescriptorArray = VK_TRUE;
    VkalInfo* vkal_info = vkal_init(device_extensions, device_extension_count, vulkan_features);

    /* Shader Setup */
    uint8_t * vertex_byte_code = 0;
    int vertex_code_size;
    read_file("../../src/examples/assets/shaders/primitives_vert.spv", &vertex_byte_code, &vertex_code_size);
    uint8_t * fragment_byte_code = 0;
    int fragment_code_size;
    read_file("../../src/examples/assets/shaders/primitives_frag.spv", &fragment_byte_code, &fragment_code_size);
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
	}
    };
    VkDescriptorSetLayout descriptor_set_layout = vkal_create_descriptor_set_layout(set_layout, 1);

    VkDescriptorSetLayoutBinding set_layout_textured_rect[] = {
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
	    2,
	    VK_SHADER_STAGE_FRAGMENT_BIT,
	    0
	}
    };
    VkDescriptorSetLayout descriptor_set_layout_textured_rect = vkal_create_descriptor_set_layout(set_layout_textured_rect, 2);
    
    VkDescriptorSetLayout layouts[] = {
	descriptor_set_layout,
	descriptor_set_layout_textured_rect
    };
    uint32_t descriptor_set_layout_count = sizeof(layouts)/sizeof(*layouts);
    VkDescriptorSet * descriptor_sets = (VkDescriptorSet*)malloc(descriptor_set_layout_count*sizeof(VkDescriptorSet));
    vkal_allocate_descriptor_sets(vkal_info->default_descriptor_pool, layouts, descriptor_set_layout_count, &descriptor_sets);
    
    /* Pipeline */
    VkPipelineLayout pipeline_layout = vkal_create_pipeline_layout(
	&layouts[0], 1, 
	NULL, 0);
    /* Vertices are defined CCW, but we flip y-axis in vertex-shader, so the
       winding of the polys appear CW after vertex-shader has run. That is
       why this graphics pipeline defines front-faces as CW, not CCW.

       Maybe, for primitive rendering we do not want to cull polys at all,
       but not sure yet...

       -> Update: Changed to CULL_MODE_NONE because that way line-drawing
       becomes easier. If drawn from eg bottom right to top left the
       winding is changed -> line would be culled!
    */
    VkPipeline graphics_pipeline = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE, 
	vkal_info->render_pass, pipeline_layout);

    /* Shader Setup Textured Rect */
    vertex_byte_code = 0;
    read_file("../../src/examples/assets/shaders/primitives_textured_rect_vert.spv", &vertex_byte_code, &vertex_code_size);
    fragment_byte_code = 0;
    read_file("../../src/examples/assets/shaders/primitives_textured_rect_frag.spv", &fragment_byte_code, &fragment_code_size);
    ShaderStageSetup shader_setup_textured_rect = vkal_create_shaders(
	vertex_byte_code, vertex_code_size, 
	fragment_byte_code, fragment_code_size);

    /* Push Constants */	
    VkPushConstantRange push_constant_ranges[] =
	{
	    { // Model Matrix
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0, 
		sizeof(uint32_t)
	    }
	};
    uint32_t push_constant_range_count = sizeof(push_constant_ranges) / sizeof(*push_constant_ranges);
    VkPipelineLayout pipeline_layout_textured_rect = vkal_create_pipeline_layout(
	&layouts[1], 1, 
	push_constant_ranges, push_constant_range_count);
    VkPipeline graphics_pipeline_textured_rect = vkal_create_graphics_pipeline(
	vertex_input_bindings, 1,
	vertex_attributes, vertex_attribute_count,
	shader_setup_textured_rect, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_FRONT_FACE_CLOCKWISE, 
	vkal_info->render_pass, pipeline_layout_textured_rect);

    /* Create batches that hold Buffers for indices and vertices that can get updated every frame */
    /* Global Batch */
    create_batch(vkal_info, &g_default_batch);
    create_batch(vkal_info, &g_persistent_batch);
    
    /* Uniform Buffer for view projection matrices */
    Camera camera;
    camera.pos = { 0, 0.f, 0.f };
    camera.center = { 0 };
    camera.up = { 0, 1, 0 };
    ViewProjection view_proj_data;
    view_proj_data.view = look_at(camera.pos, camera.center, camera.up);
    view_proj_data.proj = ortho(0.f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.f, -1.f, 2.f);
    UniformBuffer view_proj_ubo = vkal_create_uniform_buffer(sizeof(view_proj_data), 1, 0);
    
    vkal_update_descriptor_set_uniform(descriptor_sets[0], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkal_update_descriptor_set_uniform(descriptor_sets[1], view_proj_ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    
    vkal_update_uniform(&view_proj_ubo, &view_proj_data);    

    /* Draw grid */
    RenderCmd persistent_command = { };
    persistent_command.type = RENDER_CMD_STD;
    persistent_command.batch = &g_persistent_batch;
    
    fill_rect_cmd(&persistent_command,  0, 0, width, height, {0, 0, 0});
    for (int i = 0; i < 200; ++i) {
	line_cmd( &persistent_command, i*50, 0, i*50, height, 1, {.4, .4, .4});
	line_cmd( &persistent_command, 0, i*50, width, i*50, 1,  {.4, .4, .4});
    }
    update_batch(&g_persistent_batch);
    
    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
	glfwPollEvents();
	
	glfwGetFramebufferSize(window, &width, &height);
	view_proj_data.proj = ortho(0, width, height, 0, 0.1f, 2.f);
	vkal_update_uniform(&view_proj_ubo, &view_proj_data);
	

	/* Draw Some Primitives and update buffers */
	render_cmd_count = 0;
	reset_batch(&g_default_batch);

	float radius = 100.f;
	static float pulse = 0.f;
	pulse += 0.1f;
	radius += 20*sinf( pulse );
	float theta = 2*TR_PI/64;

	for (int i = 0; i < 64; ++i) {	   
	    circle(  500 + radius*cosf(i*theta), 500 + radius*sinf(i*theta), radius, 32, 1, {0, 0.5, 1.0});
	}
    circle(500, 500, 200.0f, 64, 30, { 1.0, 0.5, 1.0 });

	update_batch(&g_default_batch);
	
	{
	    uint32_t image_id = vkal_get_image();

	    vkal_begin_command_buffer(image_id);
	    vkal_begin_render_pass(image_id, vkal_info->render_pass);
	    vkal_viewport(vkal_info->default_command_buffers[image_id],
			  0, 0,
			  width, height);
	    vkal_scissor(vkal_info->default_command_buffers[image_id],
			 0, 0,
			 width, height);	       
	    for (uint32_t i = 0; i < render_cmd_count; ++i) {
		RenderCmd render_cmd = render_commands[i];
		uint32_t index_offset = render_cmd.index_buffer_offset;
		uint32_t index_count = render_cmd.index_count;
		if (render_cmd.type == RENDER_CMD_TEXTURED_RECT) {
		    uint32_t texture_id = render_cmd.texture_id;
		    vkCmdPushConstants(vkal_info->default_command_buffers[image_id], pipeline_layout_textured_rect,
				       VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), (void*)&render_cmd.texture_id);
		    vkal_bind_descriptor_set(image_id, &descriptor_sets[1], pipeline_layout_textured_rect);
		    vkal_draw_indexed_from_buffers(render_cmd.batch->index_buffer, index_offset, index_count,					       
						   render_cmd.batch->vertex_buffer, 0,
						   image_id, graphics_pipeline_textured_rect);
		}
		else if (render_cmd.type == RENDER_CMD_STD) {
		    vkal_bind_descriptor_set(image_id, &descriptor_sets[0], pipeline_layout);
		    vkal_draw_indexed_from_buffers(render_cmd.batch->index_buffer, index_offset, index_count,					       
						   render_cmd.batch->vertex_buffer, 0,
						   image_id, graphics_pipeline);		
		}
	    }
	
	    vkal_end_renderpass(image_id);
	    vkal_end_command_buffer(image_id);
	    VkCommandBuffer command_buffers1[] = { vkal_info->default_command_buffers[image_id] };
	    vkal_queue_submit(command_buffers1, 1);

	    vkal_present(image_id);
	}
    }

    vkDeviceWaitIdle(vkal_info->device);

    destroy_batch(vkal_info, &g_persistent_batch);
    destroy_batch(vkal_info, &g_default_batch);
    
    vkal_cleanup();

    return 0;
}
