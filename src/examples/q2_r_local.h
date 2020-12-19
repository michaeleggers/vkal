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

typedef struct StorageBuffer
{
	DeviceMemory memory;
	Buffer       buffer;
} StorageBuffer;

extern VkDescriptorSet * r_descriptor_set;
extern int               r_visframecount;
extern int               r_framecount;
extern uint32_t          r_image_id;
extern VkPipeline        r_graphics_pipeline;
extern VkPipelineLayout  r_pipeline_layout;

/* Buffers */
extern VertexBuffer      r_transient_vertex_buffer; // scratch memory gets updated every frame with faces depending on viewpos
extern StorageBuffer     r_transient_material_buffer;

void                     draw_world(vec3 pos);
void                     draw_static_geometry(void);
void					 draw_transluscent_chain(void);
void                     q2bsp_init(uint8_t * data);
void                     update_transient_vertex_buffer(VertexBuffer * vertex_buf, uint32_t offset, Vertex * vertices, uint32_t vertex_count);



#endif