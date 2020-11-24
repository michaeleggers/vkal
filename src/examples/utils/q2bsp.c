#include <stdint.h>
#include <stdlib.h>

#include "q2bsp.h"

static vec3     g_verts[1024];
static uint16_t g_indices[1024];
static uint16_t g_tmp_indices[1024];

Q2Bsp q2bsp_init(uint8_t * data)
{
    Q2Bsp bsp = { 0 };
    bsp.data = data;

    bsp.header = (BspHeader*)bsp.data;
    bsp.entities = (char*)(bsp.data + bsp.header->lumps[LT_ENTITIES].offset);
    bsp.planes = (BspPlane*)(bsp.data + bsp.header->lumps[LT_PLANES].offset);
    bsp.plane_count = bsp.header->lumps[LT_PLANES].length / sizeof(BspPlane);
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
    bsp.texinfos = (BspTexinfo*)(bsp.data + bsp.header->lumps[LT_TEX_INFO].offset);
    bsp.texinfo_count = bsp.header->lumps[LT_TEX_INFO].length / sizeof(BspTexinfo);
    return bsp;
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
    tris.vert_count = 3*num_tris; //(uint32_t)num_edges;
    tris.idx_count  = 3*num_tris;
    tris.tri_count  = num_tris;
    
    return tris;
}
