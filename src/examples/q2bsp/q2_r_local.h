#ifndef Q2_R_LOCAL_H
#define Q2_R_LOCAL_H

#include <stdint.h>

#define TRM_NDC_ZERO_TO_ONE
#define TRM_QUAKE2
#include "../utils/tr_math.h"

#include "q2bsp.h"
#include "q2_common.h"

// Interface to renderer

typedef struct ViewProjection
{
	mat4  view;
	mat4  proj;
	vec3  cam_pos;
} ViewProjection;


void                     init_render(void * window);
uint32_t                 register_texture(char * texture_name, uint32_t * out_width, uint32_t * out_height);
void                     create_cubemap(unsigned char * data, uint32_t width, uint32_t height, uint32_t channels);
void                     begin_frame(void);
void                     end_frame(void);
void                     draw_world(vec3 pos);
void                     draw_static_geometry(Vertex * vertices, uint32_t vertex_count);
void					 draw_bb(void);
void					 draw_sky(Vertex * vertices, uint32_t vertex_count);
void                     draw_transluscent_chain(MapFace * transluscent_chain);
void                     add_light( Vertex * vertex );


extern int               r_width;
extern int               r_height;
extern int               r_visframecount;
extern int               r_framecount;
extern int               r_image_id;
extern void            * r_window;
extern ViewProjection    r_view_proj_data;

#endif