#ifndef TR_MATH_H
#define TR_MATH_H

#define TR_PI 3.14159265358979323846f

typedef struct vec2
{
    float x, y;
} vec2;

typedef struct vec3
{
    float x, y, z;
} vec3;

typedef struct mat3
{
    float d[3][3];
} mat3;

typedef struct vec4
{
    float x, y, z, w;
} vec4;

typedef struct mat4
{
    float d[4][4];
} mat4;

float vec2_length(vec2 v);
vec2  vec2_mul(float s, vec2 v);
vec2  vec2_normalize(vec2 v);
float det_mat2(float c00, float c01, float c10, float c11);

float vec3_length(vec3 v);
vec3  vec3_normalize(vec3 v);
float vec3_dot(vec3 a, vec3 b);
vec3  vec3_cross(vec3 a, vec3 b);
vec3  vec3_div(vec3 v, float s);
vec3  vec3_add(vec3 a, vec3 b);
vec3  vec3_sub(vec3 a, vec3 b);
vec3  vec3_mul(float s, vec3 v);
float det_mat3(mat3 m);

float vec4_length(vec4 v);
vec4  vec4_normalize(vec4 v);
float vec4_dot(vec4 a, vec4 b);
vec4  vec4_div(vec4 v, float s);

mat4  mat4_identity(void);
mat4  mat4_inverse(mat4 m);
mat4  mat4_x_mat4(mat4 a, mat4 b);
vec4  mat4_x_vec4(mat4 m, vec4 v);
float det_mat4(mat4 m);

mat4  translate(mat4 m, vec3 v);
mat4  tr_scale(mat4 m, vec3 v);
mat4  rotate_x(float angle);
mat4  rotate_y(float angle);
mat4  rotate_z(float angle);
mat4  rotate(vec3 r, float angle);
mat4  perspective_gl(float fov, float aspect, float z_near, float z_far);
mat4  perspective_vk(float fov, float aspect, float z_near, float z_far);
mat4  perspective_quake2_vk(float fov, float aspect, float y_near, float y_far);
mat4  ortho(float left, float right, float bottom, float top, float z_near, float z_far);
mat4  look_at(vec3 eye_pos, vec3 center, vec3 up);
mat4  look_at_quake2(vec3 eye, vec3 center, vec3 up);
float tr_radians(float deg);
float rand_between(float min, float max);

vec4 vec3_to_vec4(vec3 v3, float w);
vec3 vec4_as_vec3(vec4 v4);
//#define VEC3_TO_VEC4(v3, w) ( (vec4){ *( (vec4*)&v3), w } )
//#define VEC4_AS_VEC3(v4)    ( *( (vec3*)&v4) )

#ifdef  TRM_NDC_ZERO_TO_ONE
	#ifdef TRM_QUAKE2
		#define perspective perspective_quake2_vk
		#define look_at look_at_quake2
	#else
		#define perspective perspective_vk
		#define look_at look_at
	#endif
#else
	#define perspective perspective_gl
#endif

#endif
