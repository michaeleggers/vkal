
#include <stdint.h>

#include "q2_r_local.h"

#include "utils/q2bsp.h"

// Include concrete rendering backend here ( Macro select )
#include "q2_vk_render.h"

int                 r_width;
int                 r_height;
int					r_visframecount;
int					r_framecount;
int					r_image_id;
ViewProjection		r_view_proj_data;

void init_render(void * window)
{
	vk_init_render( window );
}

uint32_t register_texture(char * texture_name, uint32_t * out_width, uint32_t * out_height)
{
	return vk_register_texture( texture_name, out_width, out_height );
}

void create_cubemap(unsigned char * data, uint32_t width, uint32_t height, uint32_t channels)
{
	vk_create_cubemap( data, width, height, channels );
}

void begin_frame(void)
{
	vk_begin_frame();
}

void end_frame(void)
{
	vk_end_frame();
}

void draw_static_geometry(Vertex * vertices, uint32_t vertex_count)
{
	vk_draw_static_geometry( vertices, vertex_count );
}

void draw_bb(void)
{
	//vkal_bind_descriptor_set(r_image_id, &r_descriptor_set[2], r_pipeline_layout_bb);
	//vkal_draw_indexed_from_buffers( 
	//	r_transient_index_buffer_bb.buffer, 0, g_worldmodel.trans_index_count_bb, 
	//	r_transient_vertex_buffer_bb.buffer, 0, r_image_id, r_graphics_pipeline_bb );
}

void draw_sky(Vertex * vertices, uint32_t vertex_count)
{
	vk_draw_sky(vertices, vertex_count);
}

void draw_transluscent_chain(MapFace * transluscent_chain)
{
	MapFace * trans_surf = transluscent_chain;
	while ( trans_surf ) {		
		uint32_t vbuf_offset = trans_surf->vertex_buffer_offset;
		memcpy( g_worldmodel.transient_vertex_buffer_trans + g_worldmodel.transient_vertex_count_trans, 
			g_worldmodel.map_vertices + vbuf_offset, trans_surf->vertex_count*sizeof(Vertex) );
		g_worldmodel.transient_vertex_count_trans += trans_surf->vertex_count;
		trans_surf = trans_surf->transluscent_chain;
	}	
	vk_draw_transluscent( g_worldmodel.transient_vertex_buffer_trans, g_worldmodel.transient_vertex_count_trans );
}

void add_light( Vertex * vertex )
{
	vk_add_light( vertex );
}
