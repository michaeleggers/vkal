#ifndef Q2_R_LOCAL_H
#define Q2_R_LOCAL_H

#include <stdint.h>

#include "utils/tr_math.h"

// Contains resources that are used for rendering. Multiple
// compilation units might need these. Definition is in quake2_level.c

#define MAX_MAP_TEXTURES       1024
#define MAX_MAP_VERTS          65536 
#define	MAX_MAP_FACES	       65536
#define MAX_MAP_LEAVES         65536

typedef struct VertexBuffer
{
	DeviceMemory memory;
	Buffer       buffer;
} VertexBuffer;

extern VkDescriptorSet * descriptor_set;
extern int               r_visframecount;
extern int               r_framecount;
extern uint32_t          image_id;
extern VkPipeline        graphics_pipeline;
extern VkPipelineLayout  pipeline_layout;

/* Buffers */
extern VertexBuffer      transient_vertex_buffer; // scratch memory gets updated every frame with faces depending on viewpos

void                     draw_world(vec3 pos);
void					 draw_transluscent_chain(void);
void                     q2bsp_init(uint8_t * data);
void                     update_transient_vertex_buffer(VertexBuffer * vertex_buf, uint32_t offset, Vertex * vertices, uint32_t vertex_count);



#endif