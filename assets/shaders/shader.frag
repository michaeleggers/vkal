#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 position;

void main() 
{
	
	outColor = vec4(position, 1.0);
}
