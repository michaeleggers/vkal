#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

// layout(location = 0) in vec3 in_position;
// layout(location = 1) in vec2 in_uv;

void main() 
{
	outColor = vec4(vec3(1, 0, 0), 1.0);
}
