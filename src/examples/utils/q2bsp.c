#include <stdint.h>
#include <stdlib.h>

#include "q2bsp.h"
#include "../q2_io.h"
#include "../q2_common.h"

#include "../../platform.h"


extern Platform p;

BspWorldModel   g_worldmodel;

// Some buffers used in q2bsp_triangulateFace() function.
// TODO: Maybe replace thouse?
static vec3     g_verts[1024];
static uint16_t g_indices[1024];
static uint16_t g_tmp_indices[1024];


Q2Bsp q2bsp_init(uint8_t * data)
{
    Q2Bsp bsp = { 0 };
    bsp.data = data;

    bsp.header = (BspHeader*)bsp.data;
    bsp.nodes = (BspNode*)(bsp.data + bsp.header->lumps[LT_NODES].offset);
    bsp.node_count = bsp.header->lumps[LT_NODES].length / sizeof(BspNode);
    bsp.entities = (char*)(bsp.data + bsp.header->lumps[LT_ENTITIES].offset);
    bsp.planes = (BspPlane*)(bsp.data + bsp.header->lumps[LT_PLANES].offset);
    bsp.plane_count = bsp.header->lumps[LT_PLANES].length / sizeof(BspPlane);
    bsp.sub_models = (BspSubModel*)(bsp.data + bsp.header->lumps[LT_MODELS].offset);
    bsp.sub_model_count = bsp.header->lumps[LT_MODELS].length / sizeof(BspSubModel);
    bsp.faces = (BspFace*)(bsp.data + bsp.header->lumps[LT_FACES].offset);
    bsp.face_count = bsp.header->lumps[LT_FACES].length / sizeof(BspFace);
    bsp.leaves = (BspLeaf*)(bsp.data + bsp.header->lumps[LT_LEAVES].offset);
    bsp.leaf_count = bsp.header->lumps[LT_LEAVES].length / sizeof(BspLeaf);
    bsp.leaf_face_table = (uint16_t*)(bsp.data + bsp.header->lumps[LT_LEAF_FACE_TABLE].offset);
    bsp.face_edge_table = (int32_t*)(bsp.data + bsp.header->lumps[LT_FACE_EDGE_TABLE].offset);
    bsp.face_edge_count = bsp.header->lumps[LT_FACE_EDGE_TABLE].length / sizeof(int32_t);
    bsp.edges = (uint32_t*)(bsp.data + bsp.header->lumps[LT_EDGES].offset);
    bsp.edge_count = bsp.header->lumps[LT_EDGES].length / sizeof(uint32_t);
    bsp.vertices = (vec3*)(bsp.data + bsp.header->lumps[LT_VERTICES].offset);
    bsp.vertex_count = bsp.header->lumps[LT_VERTICES].length / sizeof(vec3);
    bsp.vis = (BspVis*)(bsp.data + bsp.header->lumps[LT_VISIBILITY].offset);
    bsp.vis_offsets = (BspVisOffset*)( ((uint8_t*)(bsp.vis)) + sizeof(uint32_t) );
    bsp.texinfos = (BspTexinfo*)(bsp.data + bsp.header->lumps[LT_TEX_INFO].offset);
    bsp.texinfo_count = bsp.header->lumps[LT_TEX_INFO].length / sizeof(BspTexinfo);
    return bsp;
}

void init_worldmodel(Q2Bsp bsp, VkDescriptorSet descriptor_set)
{
	g_worldmodel.descriptor_set = descriptor_set;
	load_faces( bsp );
}

Q2Tri q2bsp_triangulateFace(Q2Bsp * bsp, BspFace face)
{
    uint16_t num_edges  = face.num_edges;
    uint16_t plane_idx = face.plane;
    uint16_t normal_is_flipped = face.plane_side;
    vec3 face_normal = bsp->planes[plane_idx].normal;
    if (normal_is_flipped) {
	face_normal = vec3_mul(-1.0f, face_normal);
    }
    
    for (uint32_t i = 0; i < (uint32_t)num_edges; ++i) {
	uint32_t face_edge_idx  = face.first_edge + i;
	int32_t edge_idx        = bsp->face_edge_table[face_edge_idx];
	int is_negative         = edge_idx & 0x08000000 ? 1 : 0;
        uint32_t abs_edge_idx;
	if (is_negative) {
	    abs_edge_idx = (edge_idx - 1) ^ 0xFFFFFFFF; // assume 2's complement architecture
	}
	else {
	    abs_edge_idx = (uint32_t)edge_idx;
	}

	uint32_t edge = bsp->edges[abs_edge_idx];
	uint32_t vertex1_idx = edge >> 16;
	uint32_t vertex2_idx = edge & 0x0000FFFF;
	if (is_negative) {
	    g_tmp_indices[i] = vertex2_idx;
	}
	else {
	    g_tmp_indices[i] = vertex1_idx;
	}
	g_verts[i] = bsp->vertices[ g_tmp_indices[i] ];
    }

    uint32_t num_tris = 1 + ((uint32_t)num_edges - 3);
    uint16_t ancor_idx = g_tmp_indices[0];
    for (uint32_t i = 0; i < num_tris; ++i) {
	g_indices[i*3]   = ancor_idx;
	g_indices[i*3+1] = g_tmp_indices[i+1];
	g_indices[i*3+2] = g_tmp_indices[i+2];
    }
    for (uint32_t i = 0; i < 3*num_tris; ++i) {
	g_verts[i] = bsp->vertices[ g_indices[i] ];
    }
    
    Q2Tri tris = { 0 };
    tris.verts   = g_verts;
    tris.normal  = face_normal;
    tris.indices = g_indices;
    tris.vert_count = (uint32_t)num_edges;
    tris.idx_count  = 3*num_tris;
    tris.tri_count  = num_tris;
    
    return tris;
}

uint32_t register_texture(VkDescriptorSet descriptor_set, char * texture_name)
{
	uint32_t i = 0;
	for ( ; i <= g_worldmodel.texture_count; ++i) {
		if ( !strcmp(texture_name, g_worldmodel.textures[ i ].name) ) {
			break;
		}
	}
	if (i > g_worldmodel.texture_count) {
		i = g_worldmodel.texture_count;
		Image image = load_image_file_from_dir("textures", texture_name);
		Texture texture = vkal_create_texture(1, image.data, image.width, image.height, 4, 0,
			VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
			0, 1, 0, 1, VK_FILTER_NEAREST, VK_FILTER_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
		vkal_update_descriptor_set_texturearray(
			descriptor_set, 
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
			i,
			texture);
		strcpy( g_worldmodel.textures[ g_worldmodel.texture_count ].name, texture_name );
		g_worldmodel.textures[ g_worldmodel.texture_count ].texture = texture;
		g_worldmodel.texture_count++;
	}

	return i;
}

void load_faces(Q2Bsp bsp)
{
	g_worldmodel.map_vertices     = (Vertex*)malloc(MAX_MAP_VERTS * sizeof(Vertex));
	g_worldmodel.map_vertex_count = 0;
	BspFace * in  = bsp.faces;
	MapFace * out = (MapFace*)malloc(bsp.face_count * sizeof(MapFace));
	
	g_worldmodel.surfaces    = out;
	g_worldmodel.numsurfaces = bsp.face_count;

	for (uint32_t i = 0; i < g_worldmodel.numsurfaces; i++, in++, out++) {
		int16_t texinfo_idx = in->texture_info; // TODO: can be negative?
		BspTexinfo texinfo = bsp.texinfos[ texinfo_idx ];
		uint32_t tex_width, tex_height;

		out->texture_id = register_texture( g_worldmodel.descriptor_set, texinfo.texture_name );
		tex_width  = g_worldmodel.textures[ out->texture_id ].texture.width;
		tex_height = g_worldmodel.textures[ out->texture_id ].texture.height;
		uint32_t prev_map_vert_count = g_worldmodel.map_vertex_count;
		Q2Tri tris = q2bsp_triangulateFace(&bsp, *in);
		for (uint32_t idx = 0; idx < tris.idx_count; ++idx) {
			uint16_t vert_index = tris.indices[ idx ];
			vec3 pos = bsp.vertices[ vert_index ];
			vec3 normal = tris.normal;
			Vertex v = (Vertex){ 0 };
			float x = -pos.x;
			float y = pos.z;
			float z = pos.y;
			v.pos.x = x;
			v.pos.y = y;
			v.pos.z = z;
			v.normal.x = -normal.x;
			v.normal.y = normal.z;
			v.normal.z = normal.y;
			float scale = 1.0;
			v.uv.x = (-x * texinfo.u_axis.x + y * texinfo.u_axis.z + z * texinfo.u_axis.y + texinfo.u_offset)/(float)(scale*(float)tex_width);
			v.uv.y = (-x * texinfo.v_axis.x + y * texinfo.v_axis.z + z * texinfo.v_axis.y + texinfo.v_offset)/(float)(scale*(float)tex_height);
			v.texture_id = out->texture_id; // TODO: passing tex-index via vert-attribute probably not supported on all devices.
			v.surface_flags = texinfo.flags;
			g_worldmodel.map_vertices[ g_worldmodel.map_vertex_count++ ] = v;
		}

		out->side = in->plane_side;
		out->vk_vertex_buffer_offset = vkal_vertex_buffer_add( 
			g_worldmodel.map_vertices + prev_map_vert_count,
			sizeof(Vertex), tris.idx_count );
		out->vertex_count = tris.idx_count;
	}
}

void load_leaves(Q2Bsp bsp)
{
    BspLeaf * in  = bsp.leaves;
    Node * out = (Node*)malloc(bsp.leaf_count * sizeof(Node));

    g_worldmodel.leaves     = out;
    g_worldmodel.leaf_count = bsp.leaf_count;
    
    for (uint32_t i = 0; i < g_worldmodel.leaf_count; i++, in++, out++) {
		out->type = LEAF;
		out->bbox_min = in->bbox_min;
		out->bbox_max = in->bbox_max;
		out->content.leaf.cluster = in->cluster;
		out->content.leaf.area = in->area;
		out->content.leaf.firstmarksurface = g_worldmodel.marksurfaces + in->first_leaf_face;
		out->content.leaf.nummarksurfaces  = in->num_leaf_faces;
    }
}
