#ifndef Q2BSP_H
#define Q2BSP_H

#include "tr_math.h"

typedef struct Q2Tri
{
    vec3     * verts;
    vec3     normal;
    uint16_t * indices;
    uint32_t vert_count;
    uint32_t idx_count;
    uint32_t tri_count;
} Q2Tri;


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
    LT_MODELS,                  // ?	
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
typedef struct BspPlane
{

    vec3      normal;      // A, B, C components of the plane equation
    float     distance;    // D component of the plane equation
    uint32_t  type;        // ?

} BspPlane;

typedef struct BspFace
{

    uint16_t   plane;             // index of the plane the face is parallel to
    uint16_t   plane_side;        // set if the normal is parallel to the plane normal

    uint32_t   first_edge;        // index of the first edge (in the face edge array)
    uint16_t   num_edges;         // number of consecutive edges (in the face edge array)
	
    uint16_t   texture_info;      // index of the texture info structure	
   
    uint8_t    lightmap_syles[4]; // styles (bit flags) for the lightmaps
    uint32_t   lightmap_offset;   // offset of the lightmap (in bytes) in the lightmap lump
} BspFace;

typedef struct BspLeaf
{
   
    uint32_t   brush_or;          // ?
	
    uint16_t   cluster;           // -1 for cluster indicates no visibility information
    uint16_t   area;              // ?

    vec3_16i   bbox_min;          // bounding box minimums
    vec3_16i   bbox_max;          // bounding box maximums

    uint16_t   first_leaf_face;   // index of the first face (in the face leaf array)
    uint16_t   num_leaf_faces;    // number of consecutive faces (in the face leaf array)

    uint16_t   first_leaf_brush;  // ?
    uint16_t   num_leaf_brushes;  // ?

} BspLeaf;

#pragma pack(pop)

typedef struct Q2Bsp
{
    uint8_t   * data;
    int         size;

    BspHeader * header;

    char      * entities;

    BspPlane  * planes;
    uint32_t    plane_count;

    BspFace   * faces;
    uint32_t    face_count;

    BspLeaf   * leaves;
    uint32_t    leaf_count;

    uint16_t  * leaf_face_table;

    // Array of indices into the edges array. The index can be negative but the absolute
    // value is used as the index. A negative index means that the vertex-order is flipped.
    int32_t   * face_edge_table;
    uint32_t    face_edge_count;

    uint32_t  * edges;
    uint32_t    edge_count;

    vec3      * vertices;
    uint32_t    vertex_count;
} Q2Bsp;

Q2Bsp q2bsp_init(uint8_t * data);
Q2Tri q2bsp_triangulateFace(Q2Bsp * bsp, BspFace face);

#endif
