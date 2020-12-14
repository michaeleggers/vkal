#ifndef Q2_COMMON_H
#define Q2_COMMON_H

#include <stdint.h>

#include "utils/tr_math.h"

typedef struct Q2Tri
{
	vec3     * verts;
	vec3     normal;
	uint16_t * indices;
	uint32_t vert_count;
	uint32_t idx_count;
	uint32_t tri_count;
} Q2Tri;

typedef struct Vertex
{
	vec3     pos;
	vec3     normal;
	vec2     uv;
	uint32_t texture_id;
	uint32_t surface_flags;
} Vertex;

typedef struct Image
{
	uint32_t width, height, channels;
	unsigned char * data;
} Image;


int string_length(char * string);

#endif