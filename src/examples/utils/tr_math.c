
#include <math.h>
#include <stdlib.h>

#define TR_PI 3.14159265358979323846f

#include "tr_math.h"

float vec3_length(vec3 v)
{
    return sqrtf(v.x*v.x +
                v.y*v.y +
                v.z*v.z);
}

vec3 vec3_normalize(vec3 v)
{
    float length = vec3_length(v);
    return (vec3){v.x/length, v.y/length, v.z/length};
}

float vec3_dot(vec3 a, vec3 b)
{
    return (float){
        a.x*b.x + a.y*b.y + a.z*b.z
    };
}

vec3 vec3_cross(vec3 a, vec3 b)
{
    return (vec3){
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
	a.x*b.y - a.y*b.x
    };
}

vec3 vec3_div(vec3 v, float s)
{
    return (vec3){v.x/s, v.y/s, v.z/s};
}

float vec4_length(vec4 v)
{
    return sqrtf(v.x*v.x +
                v.y*v.y +
                v.z*v.z +
                v.w*v.w);
}

vec4 vec4_normalize(vec4 v)
{
    float length = vec4_length(v);
    return (vec4){v.x/length, v.y/length, v.z/length, v.w/length};
}

float vec4_dot(vec4 a, vec4 b)
{
    return (float){
        a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w
    };
}

vec4 vec4_div(vec4 v, float s)
{
    return (vec4){v.x/s, v.y/s, v.z/s, v.w/s};
}

mat4 mat4_x_mat4(mat4 a, mat4 b)
{
    mat4 result = {0};
    
    for (int i=0; i<4; ++i) // compute columns of result
    {
        result.d[0][i] = a.d[0][i]*b.d[0][0] + a.d[1][i]*b.d[0][1] + a.d[2][i]*b.d[0][2] + a.d[3][i]*b.d[0][3];
        result.d[1][i] = a.d[0][i]*b.d[1][0] + a.d[1][i]*b.d[1][1] + a.d[2][i]*b.d[1][2] + a.d[3][i]*b.d[1][3];
        result.d[2][i] = a.d[0][i]*b.d[2][0] + a.d[1][i]*b.d[2][1] + a.d[2][i]*b.d[2][2] + a.d[3][i]*b.d[2][3];
        result.d[3][i] = a.d[0][i]*b.d[3][0] + a.d[1][i]*b.d[3][1] + a.d[2][i]*b.d[3][2] + a.d[3][i]*b.d[3][3];        
    }
    return result;
}

mat4 mat4_identity()
{
    return (mat4) {
        .d[0]={1, 0, 0, 0},
	.d[1]={0, 1, 0, 0},
        .d[2]={0, 0, 1, 0},
        .d[3]={0, 0, 0, 1}
    };
}

vec4 mat4_x_vec4(mat4 m, vec4 v)
{
    return (vec4) {
        m.d[0][0]*v.x + m.d[1][0]*v.y + m.d[2][0]*v.z + m.d[3][0]*v.w,
        m.d[0][1]*v.x + m.d[1][1]*v.y + m.d[2][1]*v.z + m.d[3][1]*v.w,
        m.d[0][2]*v.x + m.d[1][2]*v.y + m.d[2][2]*v.z + m.d[3][2]*v.w,
        m.d[0][3]*v.x + m.d[1][3]*v.y + m.d[2][3]*v.z + m.d[3][3]*v.w
    };
}

mat4 translate(mat4 m, vec3 v)
{
    m.d[3][0] = v.x;
    m.d[3][1] = v.y;
    m.d[3][2] = v.z;
    m.d[3][3] = 1.0f;
    return m;
}

mat4 trm_scale(mat4 m, vec3 v)
{
    m.d[0][0] *= v.x;
    m.d[1][1] *= v.y;
    m.d[2][2] *= v.z;
    m.d[3][3] = 1.0f;
    return m;
}

mat4 rotate_x(float angle)
{
    return (mat4) {
	.d[0]={1, 0, 0, 0},
        .d[1]={0, cosf(angle), sinf(angle), 0},
        .d[2]={0, -sinf(angle), cosf(angle), 0},
        .d[3]={0, 0, 0, 1}
    };	    
}

mat4 rotate_y(float angle)
{
    return (mat4) {
	.d[0]={cosf(angle), 0, -sinf(angle), 0},
        .d[1]={0, 1, 0, 0},
        .d[2]={sinf(angle), 0, cosf(angle), 0},
        .d[3]={0, 0, 0, 1}
    };	    
}

mat4 rotate_z(float angle)
{
    return (mat4) {
	.d[0]={cosf(angle), sinf(angle), 0, 0},
 	.d[1]={-sinf(angle), cosf(angle), 0, 0},
	.d[2]={0, 0, 1, 0},
	.d[3]={0, 0, 0, 1},
    };    
}

/* OpenGL friendly: this persp proj. will result in z from [-1,1] after persp. divide. */
mat4 perspective_gl(float fov, float aspect, float z_near, float z_far)
{
    float g = 1.0f / tanf(fov*0.5f);
    float k = 1.0f / (z_near-z_far);
    return (mat4) {
	.d[0]={g/aspect, 0, 0, 0},
        .d[1]={0, g, 0, 0},
        .d[2]={0, 0, (z_near+z_far)*k, -1.0f},
        .d[3]={0, 0, 2*z_near*z_far*k, 0}
    };
}

/* Vulkan friendly: this persp proj. will result in z from [0,1] after persp. divide. */
mat4 perspective_vk(float fov, float aspect, float z_near, float z_far)
{
    float g = 1.0f / tanf(fov*0.5f);
    float k = 1.0f / (z_far-z_near);
    return (mat4)
    {
	.d[0]={g/aspect, 0, 0, 0},
	.d[1]={0, g, 0, 0},
	.d[2]={0, 0, -z_far*k, -1.0f},
	.d[3]={0, 0, -(z_near*z_far)*k, 0}
    };
}

float tr_radians(float deg)
{
    return (TR_PI*deg)/180.0f;
}

float rand_between(float min, float max)
{
	float range = max - min;
	float step = range / RAND_MAX;
	return (step * rand()) + min;
}
