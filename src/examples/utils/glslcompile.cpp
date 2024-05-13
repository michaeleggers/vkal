#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>

#include <shaderc/shaderc.h>

#include "platform.h"

#include "glslcompile.h"


void load_glsl_and_compile(char const* glsl_source_file, uint8_t** out_spirv, int* out_spirv_size, ShaderType shader_type)
{
	std::string abs_path = concat_paths(get_shaders_dir(), std::string(glsl_source_file));
	std::string glsl_source = read_text_file(abs_path.c_str());
	size_t glsl_source_size = glsl_source.size();

	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	
	shaderc_shader_kind shader_kind = shader_type == SHADER_TYPE_VERTEX ? shaderc_glsl_vertex_shader : shaderc_glsl_fragment_shader;
	shaderc_compilation_result_t result = shaderc_compile_into_spv(
			compiler, glsl_source.c_str(), glsl_source_size, shader_kind,
			glsl_source_file, "main", NULL);
	shaderc_compilation_status status = shaderc_result_get_compilation_status(result);
	printf("[SHADER-UTILS] Shader compilation info\n");
	printf("[SHADER-UTILS]     file:   %s\n", glsl_source_file);
	printf("[SHADER-UTILS]     status: %d\n", (int)status);
	if (status != shaderc_compilation_status_success) {
		printf("[SHADER-UTILS]     error: %s", shaderc_result_get_error_message(result));
	}
	printf("\n");

	size_t spirv_size = shaderc_result_get_length(result);
	*out_spirv = (uint8_t*)malloc(spirv_size);
	uint32_t* code = (uint32_t*)shaderc_result_get_bytes(result);
	memcpy(*out_spirv, (uint8_t*)code, spirv_size); 
	*out_spirv_size = spirv_size;

	shaderc_result_release(result);
	shaderc_compiler_release(compiler);
}

