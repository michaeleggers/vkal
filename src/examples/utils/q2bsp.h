#ifndef Q2BSP_H
#define Q2BSP_H

#include "../../vkal.h"

#include "tr_math.h"
#include "../q2_common.h"

#define MAX_QPATH              128

#define MAX_MAP_TEXTURES       1024
#define MAX_MAP_VERTS          65536 
#define	MAX_MAP_FACES	       65536
#define MAX_MAP_LEAVES         65536

/* Flags of BspTexinfo */
#define	SURF_LIGHT		0x1		// value will hold the light strength
#define	SURF_SLICK		0x2		// effects game physics
#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	        0x10
#define	SURF_TRANS66	        0x20
#define	SURF_FLOWING	        0x40	        // scroll towards angle
#define	SURF_NODRAW		0x80	        // don't bother referencing the texture

//////////////////////////////////////////////////////////
// File BSP representation
//////////////////////////////////////////////////////////

typedef struct vec3_16i
{
    int16_t x, y, z;
} vec3_16i;

typedef enum LumpType {
    LT_ENTITIES,                // MAP entity text buffer	
    LT_PLANES,                  // Plane array	
    LT_VERTICES,                // Vertex array	
    LT_VISIBILITY,              // Compressed PVS data and directory for all clusters	
    LT_NODES,                   // Internal node array for the BSP tree	
    LT_TEX_INFO,                // Face texture application array	
    LT_FACES,                   // Face array	
    LT_LIGHTMAPS,               // Lightmaps	
    LT_LEAVES,                  // Internal leaf array of the BSP tree	
    LT_LEAF_FACE_TABLE,         // Index lookup table for referencing the face array from a leaf	
    LT_LEAF_BRUSH_TABLE,        // ?	
    LT_EDGES,                   // Edge array	
    LT_FACE_EDGE_TABLE,         // Index lookup table for referencing the edge array from a face	
    LT_MODELS,                  // Brush Models that get indexed by an entity from the LT_ENTITIES char array, eg: "model" "*17* means that at index no 17, the brush model is to be found.	
    LT_BRUSHES,                 // ?	
    LT_BRUSH_SIDES,             // ?	
    LT_POP,                     // ?	
    LT_AREAS,                   // ?      	
    LT_AREA_PORTALS,            // ?
    LT_MAX_LUMP_TYPES
} LumpType;

typedef struct BspLump
{

    uint32_t    offset;     // offset (in bytes) of the data from the beginning of the file
    uint32_t    length;     // length (in bytes) of the data

} BspLump;

typedef struct BspHeader
{

    uint32_t    magic;      // magic number ("IBSP")
    uint32_t    version;    // version of the BSP format (38)

    BspLump  lumps[LT_MAX_LUMP_TYPES];   // directory of the lumps

} BspHeader;

#pragma pack(push, 1)

typedef struct BspNode
{

    uint32_t   plane;             // index of the splitting plane (in the plane array)
    
    int32_t    front_child;       // index of the front child node or leaf
    int32_t    back_child;        // index of the back child node or leaf
   
    vec3_16i  bbox_min;          // minimum x, y and z of the bounding box
    vec3_16i  bbox_max;          // maximum x, y and z of the bounding box
	
    uint16_t   first_face;        // index of the first face (in the face array)
    uint16_t   num_faces;         // number of consecutive edges (in the face array)
} BspNode;

typedef struct BspLeaf
{
   
    uint32_t   brush_or;          // ?
	
    int16_t    cluster;           // -1 for cluster indicates no visibility information
    uint16_t   area;              // ?

    vec3_16i   bbox_min;          // bounding box minimums
    vec3_16i   bbox_max;          // bounding box maximums

    uint16_t   first_leaf_face;   // index of the first face (in the face leaf array)
    uint16_t   num_leaf_faces;    // number of consecutive faces (in the face leaf array)

    uint16_t   first_leaf_brush;  // ?
    uint16_t   num_leaf_brushes;  // ?

} BspLeaf;

typedef struct BspVisOffset
{
    uint32_t pvs;
    uint32_t phs;
} BspVisOffset;

typedef struct BspVis
{
    uint32_t	   numclusters;
} BspVis;

typedef struct BspPlane
{

    vec3      normal;      // A, B, C components of the plane equation
    float     distance;    // D component of the plane equation
    uint32_t  type;        // ?

} BspPlane;

typedef struct BspSubModel
{
    float	min[3], max[3];
    float	origin[3];		// for sounds or lights
    int		headnode;
    int		firstface, numfaces;	// submodels just draw faces TODO: Can the faceindex be negative?
					// without walking the bsp tree
} BspSubModel;

typedef struct BspTexinfo
{

    vec3     u_axis;
    float    u_offset;
   
    vec3     v_axis;
    float    v_offset;

    uint32_t flags;
    uint32_t value;

    char     texture_name[32];

    uint32_t next_texinfo;

} BspTexinfo;

typedef struct BspFace
{

    uint16_t   plane;             // index of the plane the face is parallel to
    uint16_t   plane_side;        // set if the normal is parallel to the plane normal

    uint32_t   first_edge;        // index of the first edge (in the face edge array)
    uint16_t   num_edges;         // number of consecutive edges (in the face edge array)
	
    int16_t    texture_info;      // index of the texture info structure	
   
    uint8_t    lightmap_syles[4]; // styles (bit flags) for the lightmaps
    uint32_t   lightmap_offset;   // offset of the lightmap (in bytes) in the lightmap lump
} BspFace;


#pragma pack(pop)

typedef struct Q2Bsp
{
    uint8_t   * data;
    int         size;

    BspHeader * header;

    BspNode   * nodes;
    uint32_t    node_count;
    
    char      * entities;

    BspPlane  * planes;
    uint32_t    plane_count;

    BspSubModel * sub_models;
    uint32_t    sub_model_count;
    BspFace   * faces;
    uint32_t    face_count;

    BspLeaf   * leaves;
    uint32_t    leaf_count;

    uint16_t  * leaf_face_table;
	uint32_t    leaf_face_count;

    // Array of indices into the edges array. The index can be negative but the absolute
    // value is used as the index. A negative index means that the vertex-order is flipped.
    int32_t   * face_edge_table;
    uint32_t    face_edge_count;

    uint32_t  * edges;
    uint32_t    edge_count;

    vec3      * vertices;
    uint32_t    vertex_count;

    BspTexinfo * texinfos;
    uint32_t     texinfo_count;

    BspVis         * vis;
    BspVisOffset   * vis_offsets; // there are as many of those as numclusters
} Q2Bsp;


//////////////////////////////////////////////////////////
// Runtime BSP representation
//////////////////////////////////////////////////////////

typedef struct MapTexture
{
	Texture texture;
	char    name[32];
} MapTexture;

typedef enum MapFaceType
{
    REGULAR,
    LIGHT,
    SKY,
    TRANS33,
    TRANS66,
    NODRAW
} MapFaceType;

typedef struct MapFace
{
    MapFaceType type;
    uint32_t    vertex_buffer_offset;
    uint64_t    vk_vertex_buffer_offset;
    uint32_t    vertex_count;
    uint32_t    texture_id;
    uint32_t    side;
    int         visframe;
} MapFace;

typedef struct Leaf
{
    int cluster;
    int area;

    MapFace ** firstmarksurface;
    int        nummarksurfaces;
} Leaf;

typedef struct Node Node;
typedef struct _Node
{
    BspPlane * plane;
    Node * front;
    Node * back;
    uint16_t firstsurface;
    uint16_t numsurfaces;
} _Node;

typedef enum NodeType { NODE, LEAF } NodeType;
struct Node
{    
    int visframe;
    vec3_16i bbox_min;
    vec3_16i bbox_max;
    
    Node * parent;
 
    NodeType type;
    union content {
	_Node node;
	Leaf  leaf;
    } content;
};


typedef enum {mod_bad, mod_brush, mod_sprite, mod_alias } modtype_t;
typedef struct BspWorldModel
{
    char		name[MAX_QPATH];

    int			registration_sequence;

    int			flags;

//
// volume occupied by the model graphics
//		
    vec3		mins, maxs;
    float		radius;


//
// brush model
//
    int			firstmodelsurface, nummodelsurfaces;

    int		        numsubmodels;
    BspSubModel	        *submodels;

    int			numplanes;
    BspPlane    	*planes;

    uint32_t		leaf_count;		// number of visible leafs, not counting 0
    Node 		*leaves;

    int		        numvertexes;
    vec3	        *vertices;

    int		        numedges;
    uint32_t	        *edges;

    int		        numnodes;
    int		        firstnode;
    Node                *nodes;

    int		        numtexinfo;
    BspTexinfo          *texinfos;
    
    uint32_t        numsurfaces;
    MapFace         	*surfaces;

    int			numsurfedges;
    int			*surfedges;
    
    int			nummarksurfaces;
    MapFace             **marksurfaces;
    
    BspVis              *vis;
    BspVisOffset        *vis_offsets; // there are as many of those as numclusters
    
    uint8_t		*lightdata;

	// Renderer Data, TODO: maybe this should be part of another "thing" in the code.
	VkDescriptorSet descriptor_set;
	VkDescriptorSet skybox_descriptor_set;

	MapTexture      textures[MAX_MAP_TEXTURES];
	uint32_t        texture_count;

	Vertex          * map_vertices;
	uint32_t        map_vertex_count;
} BspWorldModel;


Q2Bsp q2bsp_init(uint8_t * data);
void init_worldmodel(Q2Bsp bsp, VkDescriptorSet descriptor_set);
void deinit_worldmodel(void);
void load_faces(Q2Bsp bsp);
void load_marksurfaces(Q2Bsp bsp);
void load_leaves(Q2Bsp bsp);
Q2Tri q2bsp_triangulateFace(Q2Bsp * bsp, BspFace face);

uint32_t register_texture(VkDescriptorSet descriptor_set, char * texture_name);

#endif
