#ifndef Q2_COMMON_H
#define Q2_COMMON_H

#include <stdint.h>

#include "utils/tr_math.h"
#include "../platform.h"

#define MAX_MAP_TEXTURES       1024
#define MAX_MAP_VERTS          65536 
#define	MAX_MAP_FACES	       65536
#define MAX_MAP_LEAVES         65536

extern Platform          p;
extern char              g_exe_dir[128];

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

typedef struct Material
{
	uint32_t diffuse;
} Material;

typedef struct Image
{
	uint32_t width, height, channels;
	unsigned char * data;
} Image;

typedef struct Camera
{
	vec3 pos;
	vec3 center;
	vec3 up;
	vec3 right;
	float velocity;
} Camera;

int  string_length(char * string);
void concat_str(uint8_t * str1, uint8_t * str2, uint8_t * out_result);
#endif