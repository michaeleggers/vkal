#ifndef Q2_R_LOCAL_H
#define Q2_R_LOCAL_H

#include <stdint.h>

#include "utils/tr_math.h"

// Contains resources that are used for rendering. Multiple
// compilation units might need these. Definition is in quake2_level.c
extern VkDescriptorSet * descriptor_set;
extern int               r_visframecount;
extern int               r_framecount;
extern uint32_t          image_id;
extern VkPipeline        graphics_pipeline;
extern VkPipelineLayout  pipeline_layout;
void                     draw_world(vec3 pos);
void                     q2bsp_init(uint8_t * data);


#endif