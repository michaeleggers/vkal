#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../../vkal.h"
#include "q2bsp.h"
#include "q2_io.h"
#include "q2_common.h"

#include "../../platform.h"
#include "q2_r_local.h"
#include "q2_e_parser.h"

static Q2Bsp             bsp;
BspWorldModel			 g_worldmodel;

// Some buffers used in q2bsp_triangulateFace() function.
// TODO: Maybe replace thouse?
static vec3     g_verts[1024];
static uint16_t g_indices[1024];
static uint16_t g_tmp_indices[1024];

void q2bsp_init(uint8_t * data)
{
    bsp = (Q2Bsp){ 0 };
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
	bsp.leaf_face_count = bsp.header->lumps[LT_LEAF_FACE_TABLE].length / sizeof(uint16_t);
    bsp.face_edge_table = (int32_t*)(bsp.data + bsp.header->lumps[LT_FACE_EDGE_TABLE].offset);
    bsp.face_edge_count = bsp.header->lumps[LT_FACE_EDGE_TABLE].length / sizeof(int32_t);
    bsp.edges = (uint32_t*)(bsp.data + bsp.header->lumps[LT_EDGES].offset);
    bsp.edge_count = bsp.header->lumps[LT_EDGES].length / sizeof(uint32_t);
    bsp.vertices = (vec3*)(bsp.data + bsp.header->lumps[LT_VERTICES].offset);
    bsp.vertex_count = bsp.header->lumps[LT_VERTICES].length / sizeof(vec3);
    bsp.vis = (BspVis*)(bsp.data + bsp.header->lumps[LT_VISIBILITY].offset);
	bsp.vis_size = bsp.header->lumps[LT_VISIBILITY].length;
    bsp.vis_offsets = (BspVisOffset*)( ((uint8_t*)(bsp.vis)) + sizeof(uint32_t) );
    bsp.texinfos = (BspTexinfo*)(bsp.data + bsp.header->lumps[LT_TEX_INFO].offset);
    bsp.texinfo_count = bsp.header->lumps[LT_TEX_INFO].length / sizeof(BspTexinfo);

	init_worldmodel();
}

void init_worldmodel(void)
{
	g_worldmodel = (BspWorldModel){ 0 };	
	g_worldmodel.entities = q2_e_parse(bsp.entities);
	load_vis();
	load_planes();
	load_faces();
	load_sky();
	load_marksurfaces();
	load_leaves();
	load_nodes();
	
	// Int vertex buffers for drawing. Maybe they really should be somewhere else
	g_worldmodel.transient_vertex_buffer       = (Vertex*)malloc(MAX_MAP_VERTS*sizeof(Vertex));
	g_worldmodel.transient_vertex_buffer_sky   = (Vertex*)malloc(MAX_MAP_VERTS*sizeof(Vertex));
	g_worldmodel.transient_vertex_buffer_trans = (Vertex*)malloc(MAX_MAP_VERTS*sizeof(Vertex));
}

void deinit_worldmodel(void)
{

}

Q2Tri q2bsp_triangulateFace(BspFace face)
{
    uint16_t num_edges  = face.num_edges;
    uint16_t plane_idx = face.plane;
    uint16_t normal_is_flipped = face.plane_side;
    vec3 face_normal = bsp.planes[plane_idx].normal;
    if (normal_is_flipped) {
		face_normal = vec3_mul(-1.0f, face_normal);
    }
    
    for (uint32_t i = 0; i < (uint32_t)num_edges; ++i) {
		uint32_t face_edge_idx  = face.first_edge + i;
		int32_t edge_idx        = bsp.face_edge_table[face_edge_idx];
		int is_negative         = edge_idx & 0x08000000 ? 1 : 0;
		uint32_t abs_edge_idx;
		if (is_negative) {
			abs_edge_idx = (edge_idx - 1) ^ 0xFFFFFFFF; // assume 2's complement architecture
		}
		else {
			abs_edge_idx = (uint32_t)edge_idx;
		}

		uint32_t edge = bsp.edges[abs_edge_idx];
		uint32_t vertex1_idx = edge >> 16;
		uint32_t vertex2_idx = edge & 0x0000FFFF;
		if (is_negative) {
			g_tmp_indices[i] = vertex2_idx;
		}
		else {
			g_tmp_indices[i] = vertex1_idx;
		}
		g_verts[i] = bsp.vertices[ g_tmp_indices[i] ];
	}

	uint32_t num_tris = 1 + ((uint32_t)num_edges - 3);
	uint16_t ancor_idx = g_tmp_indices[0];
	for (uint32_t i = 0; i < num_tris; ++i) {
		g_indices[i*3]   = ancor_idx;
		g_indices[i*3+1] = g_tmp_indices[i+1];
		g_indices[i*3+2] = g_tmp_indices[i+2];
    }

    for (uint32_t i = 0; i < 3*num_tris; ++i) {
		g_verts[i] = bsp.vertices[ g_indices[i] ];
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

void load_sky(void)
{
	char * sky = g_worldmodel.entities.worldspawn.sky;
	if (sky != '\0') {
		Image images[ 6 ];
		char * env_names[ 6 ] = { "ft", "bk", "up", "dn", "rt", "lf" };
		char side[64];
		for (int i = 0; i < 6; ++i) {
			concat_str(sky, env_names[ i ], side);
			images[ i ] = load_image_file_from_dir("env", side);			
		}
		
		uint32_t img_width = images->width;
		uint32_t img_height = images->height;
		uint32_t img_size = img_width*img_height*4;
		unsigned char * cubemaps = (unsigned char*)malloc( 6 * img_size );
		unsigned char * img = cubemaps;
		for (int i = 0; i < 6; ++i) {
			memcpy( img + i*img_size, images[ i ].data, img_size );
		}
		
		create_cubemap( cubemaps, img_width, img_height, 4 );		

		free(cubemaps);
		for (int i = 0; i < 6; ++i) {
			q2_destroy_image( images + i );
		}
	} 
	else {
		// TODO: LOAD A DEFAULT CUBEMAP?
	}
}

void load_vis(void)
{
	uint32_t cluster_count  = bsp.vis->numclusters;
	//uint32_t vis_byte_count = (cluster_count >> 3) + ( (cluster_count & 7) & ~(cluster_count - 1) );

	uint32_t vis_size = bsp.vis_size;		
	uint8_t * in_vis  = (uint8_t*)(bsp.vis);
	uint8_t * out_vis  = (uint8_t*)malloc(vis_size);
	memcpy(out_vis, in_vis, vis_size);

	g_worldmodel.vis             = out_vis;
	g_worldmodel.cluster_count   = cluster_count;
	g_worldmodel.vis_for_cluster = (uint8_t**)malloc( cluster_count*sizeof(uint8_t*) );

	for (uint32_t i = 0; i < cluster_count; ++i) {
		uint32_t pvs_idx                  = bsp.vis_offsets[ i ].pvs;
		g_worldmodel.vis_for_cluster[ i ] = out_vis + pvs_idx;
	}

	// uint8_t * vis_for_cluster = g_worldmodel.vis[ cluster_id ];
}

void load_planes(void)
{
	BspPlane * out = (BspPlane*)malloc(bsp.plane_count * sizeof(BspPlane));

	g_worldmodel.planes    = out;
	g_worldmodel.numplanes = bsp.plane_count;

	memcpy(out, bsp.planes, bsp.plane_count * sizeof(BspPlane));
}

void load_faces(void)
{
	g_worldmodel.map_vertices     = (Vertex*)malloc(MAX_MAP_VERTS * sizeof(Vertex));
	g_worldmodel.map_vertex_count = 0;
	BspFace * in  = bsp.faces;
	MapFace * out = (MapFace*)malloc(bsp.face_count * sizeof(MapFace));
	memset(out, 0, bsp.face_count*sizeof(MapFace));

	g_worldmodel.surfaces    = out;
	g_worldmodel.numsurfaces = bsp.face_count;
	
	for (uint32_t i = 0; i < g_worldmodel.numsurfaces; i++, in++, out++) {
		int16_t texinfo_idx = in->texture_info; // TODO: can be negative?
		BspTexinfo texinfo = { 0 };
		texinfo = bsp.texinfos[ texinfo_idx ];
		uint32_t tex_width, tex_height;
		
		out->texture_id = register_texture( texinfo.texture_name, &tex_width, &tex_height );
		g_worldmodel.textures[ out->texture_id ].width  = tex_width;
		g_worldmodel.textures[ out->texture_id ].height = tex_height;
		strcpy(g_worldmodel.textures[ out->texture_id ].name, texinfo.texture_name);
		out->visframe   = r_framecount;
		out->plane      = g_worldmodel.planes + in->plane;
		out->plane_side = in->plane_side;
		out->type       = texinfo.flags;

		uint32_t prev_map_vert_count = g_worldmodel.map_vertex_count;
		Q2Tri tris = q2bsp_triangulateFace(*in);
		for (uint32_t idx = 0; idx < tris.idx_count; ++idx) {
			uint16_t vert_index = tris.indices[ idx ];
			vec3 pos = bsp.vertices[ vert_index ];
			vec3 normal = tris.normal;
			Vertex v = (Vertex){ 0 };
			float x = pos.x;
			float y = pos.y;
			float z = pos.z;
			v.pos.x = x;
			v.pos.y = y;
			v.pos.z = z;
			v.normal.x = normal.x;
			v.normal.y = normal.y;
			v.normal.z = normal.z; 
			float scale = 1.0;
			v.uv.x = (x * texinfo.u_axis.x + y * texinfo.u_axis.y + z * texinfo.u_axis.z + texinfo.u_offset)/(float)(scale*(float)tex_width);
			v.uv.y = (x * texinfo.v_axis.x + y * texinfo.v_axis.y + z * texinfo.v_axis.z + texinfo.v_offset)/(float)(scale*(float)tex_height);
			v.texture_id = out->texture_id; // TODO: passing tex-index via vert-attribute probably not supported on all devices.
			v.surface_flags = texinfo.flags;
			g_worldmodel.map_vertices[ g_worldmodel.map_vertex_count++ ] = v;
		}
		if ( (texinfo.flags & SURF_LIGHT) == SURF_LIGHT ) {			
			add_light( g_worldmodel.map_vertices + prev_map_vert_count );		
		}

		out->side = in->plane_side;
		out->vk_vertex_buffer_offset = vkal_vertex_buffer_add( g_worldmodel.map_vertices + prev_map_vert_count, sizeof(Vertex), tris.idx_count );
		out->vertex_buffer_offset = prev_map_vert_count;
		out->vertex_count = tris.idx_count;
	}
}

void load_marksurfaces(void)
{
	MapFace ** out;
	
	out = (MapFace**)malloc( bsp.leaf_face_count * sizeof(*out) );

	g_worldmodel.marksurfaces    = out;
	g_worldmodel.nummarksurfaces = bsp.leaf_face_count;

	for (uint32_t i = 0; i < bsp.leaf_face_count; ++i) {
		uint16_t j = bsp.leaf_face_table[ i ];
		out[ i ] = g_worldmodel.surfaces + j;
		out[ i ]->face_id = j;
	}
}

void load_leaves(void)
{
    BspLeaf * in   = bsp.leaves;
	uint32_t count = bsp.leaf_count;
    Node * out = (Node*)malloc(count * sizeof(Node));
	memset( out, 0, count * sizeof(Node) );

    g_worldmodel.leaves     = out;
    g_worldmodel.leaf_count = count;
    
    for (uint32_t i = 0; i < count; i++, in++, out++) {
		out->type = LEAF;
		
		// Node, Leaf common data
		out->bbox_min = in->bbox_min;
		out->bbox_max = in->bbox_max;
		out->visframe = -1;

		// Leaf specific data
		out->content.leaf.cluster = in->cluster;
		out->content.leaf.area = in->area;
		out->content.leaf.firstmarksurface = g_worldmodel.marksurfaces + in->first_leaf_face;
		out->content.leaf.nummarksurfaces  = in->num_leaf_faces;
    }
}

void set_parent_node(Node * node, Node * parent)
{
	node->parent = parent;
	if (node->type == LEAF) 
		return;
	set_parent_node(node->content.node.front, node);
	set_parent_node(node->content.node.back,  node);
}

void load_nodes(void)
{
	BspNode * in   = bsp.nodes;
	uint32_t count = bsp.node_count;

	Node * out = (Node*)malloc(count * sizeof(Node));
	memset( out, 0, count * sizeof(Node) );
	g_worldmodel.nodes    = out;
	g_worldmodel.numnodes = count;

	for (uint32_t i = 0; i < count; i++, in++, out++) {
		out->type = NODE;
		
		// Node, Leaf common data
		out->bbox_min = in->bbox_min;
		out->bbox_max = in->bbox_max;
		out->visframe = -1;

		// Node specific data
		out->content.node.plane        = g_worldmodel.planes + in->plane;
		out->content.node.firstsurface = in->first_face;
		out->content.node.numsurfaces  = in->num_faces;

		int32_t front_child = in->front_child;
		int32_t back_child  = in->back_child;
		if (front_child < 0) { // front child is a leaf
			int leaf_idx = (-front_child - 1);
			out->content.node.front = g_worldmodel.leaves + leaf_idx;
		}
		else {
			out->content.node.front = g_worldmodel.nodes + front_child;
		}
		if (back_child < 0) { // back child is a leaf
			int leaf_idx = (-back_child - 1);
			out->content.node.back = g_worldmodel.leaves + leaf_idx;
		}
		else {
			out->content.node.back = g_worldmodel.nodes + back_child;
		}
	}

	set_parent_node(g_worldmodel.nodes, NULL);
}

void add_bb(Node * node)
{
	vec3_16i bbox_min = node->bbox_min;
	vec3_16i bbox_max = node->bbox_max;
	vec3 minf = (vec3){ -(float)bbox_min.x, (float)bbox_min.z, (float)bbox_min.y };
	vec3 maxf = (vec3){ -(float)bbox_max.x, (float)bbox_max.z, (float)bbox_max.y };
	Vertex verts[ 8 ] = { 0 };
	// front
	verts[ 0 ].pos   = minf;
	verts[ 1 ].pos.x = minf.x; verts[ 1 ].pos.y = maxf.y; verts[ 1 ].pos.z = minf.z;
	verts[ 2 ].pos.x = maxf.x; verts[ 2 ].pos.y = maxf.y; verts[ 2 ].pos.z = minf.z;
	verts[ 3 ].pos.x = maxf.x; verts[ 3 ].pos.y = minf.y; verts[ 3 ].pos.z = minf.z;
	// back
	verts[ 4 ].pos.x = minf.x; verts[ 4 ].pos.y = minf.y; verts[ 4 ].pos.z = maxf.z;
	verts[ 5 ].pos.x = minf.x; verts[ 5 ].pos.y = maxf.y; verts[ 5 ].pos.z = maxf.z;
	verts[ 6 ].pos   = maxf;
	verts[ 7 ].pos.x = maxf.x; verts[ 7 ].pos.y = minf.y; verts[ 7 ].pos.z = maxf.z;

	uint16_t indices[48] = {
		// front
		0, 1,
		1, 2,
		2, 3,
		3, 4,
		// right
		3, 2,
		2, 6,
		6, 7,
		7, 3,
		// back
		7, 6,
		6, 5,
		5, 4,
		4, 7,
		// left
		4, 5,
		5, 1,
		1, 0,
		0, 4,
		// top
		1, 5,
		5, 6,
		6, 2,
		2, 1,
		// bottom
		0, 4,
		4, 7,
		7, 3,
		3, 0
	};
	for (int i = 0; i < 48; ++i) {
		indices[ i ] += g_worldmodel.transient_vertex_count_bb;
	}

	// TODO: update transient buffers (memcpy)
	//update_transient_vertex_buffer( &r_transient_vertex_buffer_bb, g_worldmodel.transient_vertex_count_bb, verts, 8 );
	//update_transient_index_buffer( &r_transient_index_buffer_bb, g_worldmodel.transient_index_count_bb, indices, 48 );

	g_worldmodel.transient_vertex_count_bb += 8;
	g_worldmodel.transient_index_count_bb  += 48;
}

uint8_t * pvs_for_cluster(int cluster)
{
	assert( cluster >= 0 );
	return g_worldmodel.vis_for_cluster[ cluster ];
}

// find the leaf the camera (or any other position) is located in
Leaf * point_in_leaf(vec3 pos)
{	
	Node * node = g_worldmodel.nodes;
	
	while (1) {
		if (node->type == LEAF) {
			add_bb(node);
			return &node->content.leaf;
		}
		BspPlane * plane = node->content.node.plane;
		vec3 plane_abc   = (vec3){ plane->normal.x, plane->normal.y, plane->normal.z };
		float distance   = plane->distance;
		if ( (vec3_dot(pos, plane_abc) - distance) > 0 ) { // pos located at front side of plane
			node = node->content.node.front;
		}
		else {  // pos located at back side of plane
			node = node->content.node.back;
		}
	}

	return NULL; // This cannot be reached.
}

// decompress a vis that has been acquired from a cluster
uint8_t * Mod_DecompressVis (uint8_t * in)
{
	static uint8_t	decompressed[MAX_MAP_LEAVES/8];
	int		c;
	uint8_t	* out;
	int		row;

	row = (bsp.vis->numclusters+7)>>3;	
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

// check if cluster i is visible in this pvs
// i >= 0!
int isVisible(uint8_t * pvs, int i)
{

	// i>>3 = i/8    
	// i&7  = i%8

	return pvs[i>>3] & (1<<(i&7));	    
}

// mark leaves and nodes
void mark_leaves(uint8_t * pvs)
{
	r_visframecount++;

	Node * leaf = g_worldmodel.leaves;
	for (uint32_t i = 0; i < g_worldmodel.leaf_count; ++i, leaf++) {
		int cluster = leaf->content.leaf.cluster;
		if (cluster == -1)
			continue;
		if ( isVisible(pvs, cluster) ) {
			Node * node = leaf;
			do {
				if (node->visframe == r_visframecount) // we have been here before while walking up from another branch -> no need to go further
					break;
				node->visframe = r_visframecount;
				node = node->parent;				
			} while(node);
		}
	}
}

void recursive_world_node(Node * node, vec3 pos)
{
	int c;
	Leaf leaf;
	MapFace ** mark;
		

	if (node->visframe != r_visframecount) 
		return;
	// TODO: Frustum Culling


	if (node->type == LEAF) { // Leaf Node -> Draw!
		leaf = node->content.leaf;
		mark = leaf.firstmarksurface;
		c = leaf.nummarksurfaces;
		if (c) { 
			do {
				(*mark)->visframe = r_framecount;				
				mark++;
			} while (--c);
		}

		return;
	}

	// Node is not a Leaf -> walk approporiate side
	// if pos is "behind" plane -> walk front, otherwise walk back. This gives back to front surface order

	BspPlane * plane = node->content.node.plane;
	vec3 plane_abc = (vec3){ plane->normal.x, plane->normal.y, plane->normal.z };
	float front_or_back = vec3_dot(pos, plane_abc) - plane->distance;
	
	int sidebit;
	if (front_or_back < 0) { // on back-side -> walk front side of plane
		sidebit = 1;
		recursive_world_node( node->content.node.back, pos );
	}
	else { // on front-side -> walk back side of plane
		sidebit = 0;
		recursive_world_node( node->content.node.front, pos );
	}
	
	// then draw the surfaces that have been marked previously (when node was a leaf)

	MapFace * surf = g_worldmodel.surfaces + node->content.node.firstsurface;
	for (c = node->content.node.numsurfaces; c; c--, surf++) {
		if (surf->visframe != r_framecount)
			continue;

		if ( surf->plane_side != sidebit )
			continue;

		if ( surf->type == SURF_TRANS66 ) {
			surf->transluscent_chain = g_worldmodel.transluscent_face_chain;
			g_worldmodel.transluscent_face_chain = surf;
		}
		else if ( surf->type == SURF_LIGHT ) {
			// TODO: add light face to uniform descriptor-array
			// TODO: maybe but into separate buffer for raytracing
			
			uint32_t vbuf_offset = surf->vertex_buffer_offset;			
			memcpy( g_worldmodel.transient_vertex_buffer + g_worldmodel.transient_vertex_count, g_worldmodel.map_vertices + vbuf_offset, surf->vertex_count*sizeof(Vertex) );
			g_worldmodel.transient_vertex_count += surf->vertex_count;		
		}
		else if ( surf->type == SURF_SKY ) {
			//uint32_t vbuf_offset = surf->vertex_buffer_offset;
			//update_transient_vertex_buffer(
			//	&r_transient_vertex_buffer_sky, g_worldmodel.trans_vertex_count_sky,
			//	g_worldmodel.map_vertices + vbuf_offset, surf->vertex_count );

			//g_worldmodel.trans_vertex_count_sky += surf->vertex_count;

			uint32_t vbuf_offset = surf->vertex_buffer_offset;			
			memcpy( g_worldmodel.transient_vertex_buffer_sky + g_worldmodel.transient_vertex_count_sky, g_worldmodel.map_vertices + vbuf_offset, surf->vertex_count*sizeof(Vertex) );
			g_worldmodel.transient_vertex_count_sky += surf->vertex_count;
		}
		else {
			//vkal_draw(r_image_id, r_graphics_pipeline, surf->vk_vertex_buffer_offset, surf->vertex_count);
			
			uint32_t vbuf_offset = surf->vertex_buffer_offset;			
			memcpy( g_worldmodel.transient_vertex_buffer + g_worldmodel.transient_vertex_count, g_worldmodel.map_vertices + vbuf_offset, surf->vertex_count*sizeof(Vertex) );
			g_worldmodel.transient_vertex_count += surf->vertex_count;
		}
	}

	// Recurse down the other side
	if (sidebit) {		
		recursive_world_node( node->content.node.front, pos );
	}
	else {
		recursive_world_node( node->content.node.back, pos );
	}
}

void draw_world(vec3 pos)
{	
	g_worldmodel.transluscent_face_chain      = NULL;
	g_worldmodel.transient_vertex_count       = 0;
	g_worldmodel.transient_vertex_count_sky   = 0;
	g_worldmodel.transient_vertex_count_bb    = 0;
	g_worldmodel.transient_index_count_bb     = 0;
	g_worldmodel.transient_vertex_count_trans = 0;

	Leaf * leaf = point_in_leaf( pos );
	int cluster_id = leaf->cluster;
	//printf("cluster id: %d\n", cluster_id);
	if (cluster_id >= 0) {		
		uint8_t * compressed_pvs    = pvs_for_cluster(cluster_id);
		uint8_t * decompressed_pvs  = Mod_DecompressVis(compressed_pvs);

		mark_leaves(decompressed_pvs);
 		recursive_world_node(g_worldmodel.nodes, pos);		
	}
}

