#ifndef _GLSL_COMPILE_H_
#define _GLSL_COMPILE_H_

#include <stdint.h>

typedef enum ShaderType
{
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_FRAGMENT
} ShaderType;

#ifdef __cplusplus
extern "C" {
#endif


void load_glsl_and_compile(char const* glsl_source, uint8_t** out_spirv, int* out_spirv_size, ShaderType shader_type);


#ifdef __cplusplus
}
#endif 

#endif