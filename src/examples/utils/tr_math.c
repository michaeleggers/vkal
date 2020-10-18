
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
    vec3 result;
    result.x = v.x/length;
    result.y = v.y/length;
    result.z = v.z/length;
    return result;
}

float vec3_dot(vec3 a, vec3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;

}

vec3 vec3_cross(vec3 a, vec3 b)
{
    vec3 result;
    result.x = a.y*b.z - a.z*b.y;
    result.y = a.z*b.x - a.x*b.z;
    result.z = a.x*b.y - a.y*b.x;
    return result;
}

vec3 vec3_div(vec3 v, float s)
{
    vec3 result;
    result.x = v.x/s;
    result.y = v.y/s;
    result.z = v.z/s; 
    return result;
}

vec3  vec3_sub(vec3 a, vec3 b)
{
    vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

vec3  vec3_mul(float s, vec3 v)
{
    vec3 result;
    result.x = s*v.x;
    result.y = s*v.y;
    result.z = s*v.z;
    return result;
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
    vec4 result;
    result.x = v.x/length;
    result.y = v.y/length;
    result.z = v.z/length;
    result.w = v.w/length;
    return result;
}

float vec4_dot(vec4 a, vec4 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

vec4 vec4_div(vec4 v, float s)
{
    vec4 result;
    result.x = v.x/s;
    result.y = v.y/s;
    result.z = v.z/s;
    result.w = v.w/s;
    return result;
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
    vec4 result;
    result.x = m.d[0][0]*v.x + m.d[1][0]*v.y + m.d[2][0]*v.z + m.d[3][0]*v.w;
    result.y = m.d[0][1]*v.x + m.d[1][1]*v.y + m.d[2][1]*v.z + m.d[3][1]*v.w;
    result.z = m.d[0][2]*v.x + m.d[1][2]*v.y + m.d[2][2]*v.z + m.d[3][2]*v.w;
    result.w = m.d[0][3]*v.x + m.d[1][3]*v.y + m.d[2][3]*v.z + m.d[3][3]*v.w;
    return result;
}

mat4 translate(mat4 m, vec3 v)
{
    m.d[3][0] = v.x;
    m.d[3][1] = v.y;
    m.d[3][2] = v.z;
    m.d[3][3] = 1.0f;
    return m;
}

mat4 tr_scale(mat4 m, vec3 v)
{
    m.d[0][0] *= v.x;
    m.d[1][1] *= v.y;
    m.d[2][2] *= v.z;
    m.d[3][3] = 1.0f;
    return m;
}

mat4 rotate_x(float angle)
{
    mat4 result = { 0 };
    result.d[0][0] = 1;
    result.d[0][1] = result.d[0][2] = result.d[0][3] = 0;
    
    result.d[1][0] = 0;
    result.d[1][1] = cosf(angle);
    result.d[1][2] = sinf(angle);
    result.d[1][3] = 0;
    
    result.d[2][0] = 0;
    result.d[2][1] = -sinf(angle);
    result.d[2][2] = cosf(angle);
    result.d[2][3] = 0;
    
    result.d[3][0] = result.d[3][1] = result.d[3][2] = 0;
    result.d[3][3] = 1;
    return result;
}

mat4 rotate_y(float angle)
{
    mat4 result = { 0 };
    result.d[0][0] = cosf(angle);
    result.d[0][1] = 0;
    result.d[0][2] = -sinf(angle);
    result.d[0][3] = 0;
    
    result.d[1][0] = 0;
    result.d[1][1] = 1;
    result.d[1][2] = 0;
    result.d[1][3] = 0;
    
    result.d[2][0] = sinf(angle);
    result.d[2][1] = 0;
    result.d[2][2] = cosf(angle);
    result.d[2][3] = 0;
    
    result.d[3][0] = result.d[3][1] = result.d[3][2] = 0;
    result.d[3][3] = 1;
    return result;
}

mat4 rotate_z(float angle)
{
    mat4 result = { 0 };
    result.d[0][0] = cosf(angle);
    result.d[0][1] = sinf(angle);
    result.d[0][2] = 0;
    result.d[0][3] = 0;
	
    result.d[1][0] = -sinf(angle);
    result.d[1][1] = cosf(angle);
    result.d[1][2] = result.d[1][3] = 0;

    result.d[2][0] = 0;
    result.d[2][1] = 0;
    result.d[2][2] = 1;
    result.d[2][3] = 0;
	
    result.d[3][0] = result.d[3][1] = result.d[3][2] =  0;
    result.d[3][3] = 1;
    return result;
}

/* OpenGL friendly: this persp proj. will result in z from [-1,1] after persp. divide. */
mat4 perspective_gl(float fov, float aspect, float z_near, float z_far)
{
    float g = 1.0f / tanf(fov*0.5f);
    float k = 1.0f / (z_near-z_far);
    mat4 result = { 0 };
    result.d[0][0] = g/aspect;
    result.d[0][1] = result.d[0][2] = result.d[0][3] = 0;

    result.d[1][0] = 0;
    result.d[1][1] = g;
    result.d[1][2] = result.d[1][3] = 0;
	
    result.d[2][0] = result.d[2][1] = 0;
    result.d[2][2] = (z_near+z_far)*k;
    result.d[2][3] = -1.0f;
	
    result.d[3][0] = result.d[3][1] = 0;
    result.d[3][2] = 2*z_near*z_far*k;
    result.d[3][3] = 0;
    return result;
}

/* Vulkan friendly: this persp proj. will result in z from [0,1] after persp. divide. */
mat4 perspective_vk(float fov, float aspect, float z_near, float z_far)
{
    float g = 1.0f / tanf(fov*0.5f);
    
    mat4 result = { 0 };
    result.d[0][0] = g/aspect;
    result.d[0][1] = result.d[0][2] = result.d[0][3] = 0;
	
    result.d[1][0] = 0;
    result.d[1][1] = g;
    result.d[1][2] = result.d[1][3] = 0;
	
    result.d[2][0] = result.d[2][1] = 0;
    result.d[2][2] = z_far / (z_near - z_far);
    result.d[2][3] = -1.0f;
	
    result.d[3][0] = result.d[3][1] = 0;
    result.d[3][2] = -(z_far * z_near) / (z_far - z_near);
    result.d[3][3] = 0;
    return result;
}

mat4  look_at(vec3 eye, vec3 center, vec3 up)
{
    vec3 f = vec3_normalize( vec3_sub(center, eye) );
    vec3 s = vec3_normalize( vec3_cross(f, up) );
    vec3 u = vec3_cross(s, f);

    mat4 result = mat4_identity();
    result.d[0][0] = s.x;
    result.d[1][0] = s.y;
    result.d[2][0] = s.z;
    result.d[0][1] = u.x;
    result.d[1][1] = u.y;
    result.d[2][1] = u.z;
    result.d[0][2] =-f.x;
    result.d[1][2] =-f.y;
    result.d[2][2] =-f.z;
    result.d[3][0] =-vec3_dot(s, eye);
    result.d[3][1] =-vec3_dot(u, eye);
    result.d[3][2] = vec3_dot(f, eye);

    return result;
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
