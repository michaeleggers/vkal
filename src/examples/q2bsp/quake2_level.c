/* Michael Eggers, 10/22/2020
   
   Simple example showing how to draw a rect (two triangles) and mapping
   a texture on it.
*/


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <GLFW/glfw3.h>
#include <physfs.h>

#include "../../vkal.h"
#include "../../platform.h"

#define TRM_NDC_ZERO_TO_ONE
#define TRM_LH
#include "../utils/tr_math.h"

#include "q2_common.h"
#include "q2_io.h"
#include "q2bsp.h"
#include "q2_r_local.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../stb_image.h"


#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

typedef enum KeyCmd
{
    W,
    A,
    S,
    D,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    MAX_KEYS
} KeyCmd;

void camera_dolly(Camera * camera, vec3 translate);
void camera_yaw(Camera * camera, float angle);
void camera_pitch(Camera * camera, float angle);

static GLFWwindow * g_window;

static Camera g_camera;
static int g_keys[MAX_KEYS];

static Vertex * map_vertices;
static uint32_t offset_vertices;


/* Global Rendering Stuff (across compilation units) */
 
/* Platform */
Platform                p;

/* Static geometry, such as a cube */
static Vertex g_skybox_verts[8] =
{
    { .pos = { -1, 1, 1 } },
    { .pos =  { 1, 1, 1 } },
    { .pos = { 1, -1, 1 } },
    { .pos = { -1,-1, 1} },
    { .pos = { -1, 1, -1 } },
    { .pos = { 1, 1, -1 } },
    { .pos = { 1, -1, -1 } },
    { .pos = { -1, -1, -1 } }
};

static uint16_t g_skybox_indices[36] =
{
    // front
    0, 1, 2,
    2, 3, 0,
    // right
    1, 5, 6,
    6, 2, 1,
    // back
    5, 4, 7,
    7, 6, 5,
    // left
    4, 0, 3,
    3, 7, 4,
    // top
    4, 5, 1,
    1, 0, 4,
    // bottom
    3, 2, 6,
    6, 7, 3    
};

// GLFW callbacks
static void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
	if (key == GLFW_KEY_ESCAPE) {
	    printf("escape key pressed\n");
	    glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
    }
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
	if (key == GLFW_KEY_W) {
	    g_keys[W] = 1;
	}
	if (key == GLFW_KEY_S) {
	    g_keys[S] = 1;
	}
	if (key == GLFW_KEY_A) {
	    g_keys[A] = 1;
	}
	if (key == GLFW_KEY_D) {
	    g_keys[D] = 1;
	}
	
	if (key == GLFW_KEY_UP) {
	    g_keys[UP] = 1;
	}	
	if (key == GLFW_KEY_DOWN) {
	    g_keys[DOWN] = 1;
	}
	if (key == GLFW_KEY_LEFT) {
	    g_keys[LEFT] = 1;
	}	
	if (key == GLFW_KEY_RIGHT) {
	    g_keys[RIGHT] = 1;
	}
    }
    else if (action == GLFW_RELEASE) {
	if (key == GLFW_KEY_W) {
	    g_keys[W] = 0;
	}
	if (key == GLFW_KEY_S) {
	    g_keys[S] = 0;
	}
	if (key == GLFW_KEY_A) {
	    g_keys[A] = 0;
	}
	if (key == GLFW_KEY_D) {
	    g_keys[D] = 0;
	}

	if (key == GLFW_KEY_UP) {
	    g_keys[UP] = 0;
	}	
	if (key == GLFW_KEY_DOWN) {
	    g_keys[DOWN] = 0;
	}
	if (key == GLFW_KEY_LEFT) {
	    g_keys[LEFT] = 0;
	}	
	if (key == GLFW_KEY_RIGHT) {
	    g_keys[RIGHT] = 0;
	}
    }
}

void init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	
    g_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Michi's Quake 2 BSP Vulkan Renderer ;)", 0, 0);
	
	glfwSetWindowPos(g_window, 200, 200);
	//glfwSetWindowMonitor(g_window, glfwGetPrimaryMonitor(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GLFW_DONT_CARE);

    glfwSetKeyCallback(g_window, glfw_key_callback);
}

void camera_dolly(Camera * camera, vec3 translate)
{
    camera->pos = vec3_add(camera->pos, vec3_mul(camera->velocity, translate) );    
}

void camera_yaw(Camera * camera, float angle)
{
    mat4 rot = rotate(camera->right, angle);
    vec3 new_forward = vec4_as_vec3( mat4_x_vec4(rot, vec3_to_vec4(g_camera.forward, 1.0)) );
	new_forward = vec3_normalize( new_forward );
    float fwd_dot_up = vec3_dot( new_forward, (vec3){0, 0, 1} );
	printf("camera fwd_dot_up: %f\n", fabs(fwd_dot_up));
    if ( fabs(fwd_dot_up) > 0.999 ) return;    
	g_camera.forward = new_forward;
}

void camera_pitch(Camera * camera, float angle)
{
    mat4 rot_z       = rotate_z(angle);    
    g_camera.forward = vec4_as_vec3( mat4_x_vec4( rot_z, vec3_to_vec4(g_camera.forward, 1.0) ) );
	g_camera.forward = vec3_normalize( g_camera.forward );
    camera->right    = vec4_as_vec3( mat4_x_vec4( rot_z, vec3_to_vec4(camera->right, 1.0) ) );
}

void init_physfs(char const * argv0)
{
    if (!PHYSFS_init(argv0)) {
        printf("PHYSFS_init() failed!\n  reason: %s.\n", PHYSFS_getLastError());
        exit(-1);
    }

}

void deinit_physfs()
{
    if (!PHYSFS_deinit())
        printf("PHYSFS_deinit() failed!\n  reason: %s.\n", PHYSFS_getLastError());
}

int main(int argc, char ** argv)
{
	
    init_physfs(argv[0]);
    init_window();
    init_platform(&p);

	p.get_exe_path(g_exe_dir, 128);

	uint8_t textures_dir[128];
	concat_str(g_exe_dir, "q2_textures.zip", textures_dir);	
	if (!PHYSFS_mount(textures_dir, "/", 0)) {
		printf("PHYSFS_mount() failed!\n  reason: %s.\n", PHYSFS_getLastError());
	}

    init_render( g_window );
    
    /* Load Quake 2 BSP map */

    uint8_t * bsp_data = NULL;
    int bsp_data_size;
	uint8_t map_path[128];
	uint8_t map[64];
	if (argc > 1) {
		concat_str("assets/maps/", argv[1], map);
		concat_str(g_exe_dir, map, map_path);
	}
	else {
		concat_str(g_exe_dir, "/assets/maps/michi5.bsp", map_path);
	}
    p.read_file(map_path, &bsp_data, &bsp_data_size);
    assert(bsp_data != NULL);
    q2bsp_init(bsp_data);   
    
    /* Uniform Buffer for view projection matrices */
    g_camera.pos = (vec3){ 2, 46, 42 };	
	g_camera.pos = (vec3){ -208, 608, 100 };	

    g_camera.forward = (vec3){ 1, 0, 0 }; 
    g_camera.up = (vec3){ 0, 0, 1 };
    g_camera.right = vec3_normalize(vec3_cross(g_camera.forward, g_camera.up));
    g_camera.velocity = 10.f;   
    r_view_proj_data.view = look_at(g_camera.pos, vec3_add( g_camera.pos, g_camera.forward ), g_camera.up);
    r_view_proj_data.proj = perspective( tr_radians(45.f), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.f );
    
	double start_time;
	double second = 0.0;

    // Main Loop
    while (!glfwWindowShouldClose(g_window))
    {
		start_time = glfwGetTime();

		glfwPollEvents();

		if (g_keys[W]) {			
			camera_dolly(&g_camera, g_camera.forward );
		}
		if (g_keys[S]) {			
			camera_dolly(&g_camera, vec3_mul(-1, g_camera.forward) );
		}
		if (g_keys[A]) {
			camera_dolly(&g_camera, vec3_mul(-1, g_camera.right) );
		}
		if (g_keys[D]) {
			camera_dolly(&g_camera, g_camera.right);
		}
		if (g_keys[UP]) {
			camera_yaw(&g_camera, tr_radians(-2.0f) );	    
		}
		if (g_keys[DOWN]) {
			camera_yaw(&g_camera, tr_radians(2.0f) );
		}
		if (g_keys[LEFT]) {
			camera_pitch(&g_camera, tr_radians(2.0f) );
		}
		if (g_keys[RIGHT]) {
			camera_pitch(&g_camera, tr_radians(-2.0f) );
		}
	
		glfwGetFramebufferSize(g_window, &r_width, &r_height);
		r_view_proj_data.view    = look_at(g_camera.pos, vec3_add( g_camera.pos, g_camera.forward ), g_camera.up);
		r_view_proj_data.proj    = perspective( tr_radians(90.f), (float)r_width/(float)r_height, 0.1f, 10000.f );
		r_view_proj_data.cam_pos = g_camera.pos;
	
		{
			begin_frame();

	    
		
			// TODO: bind descriptor set
			// TODO: draw stuff
		
			draw_world( g_camera.pos );		
			draw_static_geometry( g_worldmodel.transient_vertex_buffer, g_worldmodel.transient_vertex_count );
			//draw_bb();
			draw_sky( g_worldmodel.transient_vertex_buffer_sky, g_worldmodel.transient_vertex_count_sky );
			draw_transluscent_chain( g_worldmodel.transluscent_face_chain );
	
			end_frame();
		}

		double end_time = glfwGetTime();
		double time_elapsed = end_time - start_time;
		second += time_elapsed;
		if ( second >= 1.0 ) {
			printf( "frametime: %fms, FPS: %f\n", time_elapsed*1000.0, (1.0/time_elapsed) );
			printf( "camera pos: %f, %f, %f\n", g_camera.pos.x, g_camera.pos.y, g_camera.pos.z );
			second = 0.0;
		}
	}

    vkal_cleanup();

    glfwDestroyWindow(g_window);
 
    glfwTerminate();

    deinit_physfs();

    return 0;
}
