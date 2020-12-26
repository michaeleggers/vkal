#ifndef Q2_VK_RENDER_H
#define Q2_VK_RENDER_H

#include "../vkal.h"
#include "q2_common.h"
#include "utils/q2bsp.h"

typedef struct VertexBuffer
{
	DeviceMemory memory;
	Buffer       buffer;
} VertexBuffer;

typedef struct IndexBuffer
{
	DeviceMemory memory;
	Buffer       buffer;
} IndexBuffer;

typedef struct StorageBuffer
{
	DeviceMemory memory;
	Buffer       buffer;
} StorageBuffer;

typedef struct Q2Texture
{
	Texture vk_texture;
	uint32_t width;
	uint32_t height;
	char    name[64];
} Q2Texture;

void					 vk_init_render(void * window);
uint32_t                 vk_register_texture(char * texture_name, uint32_t * out_width, uint32_t * out_height);
void					 vk_create_cubemap(unsigned char * data, uint32_t width, uint32_t height, uint32_t channels);
void                     vk_begin_frame(void);
void				     vk_end_frame(void);
void                     vk_draw_static_geometry(Vertex * vertices, uint32_t vertex_count);
void					 vk_draw_transluscent_chain(MapFace * transluscent_chain);
void					 vk_draw_bb(Vertex * vertices, uint32_t vertex_count, uint16_t * indices, uint32_t index_count);
void                     vk_draw_transluscent(Vertex * vertices, uint32_t vertex_count);
void					 vk_draw_sky(Vertex * vertices, uint32_t vertex_count);
void                     vk_add_light(Vertex * vertex);

#endif