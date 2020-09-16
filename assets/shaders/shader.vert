#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
//layout (location = 1) in vec2 uv;
//layout (location = 2) in vec3 normal;
//layout (location = 4) in vec3 tangent;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_color;

void main()
{
    out_position = 0.5*position + 0.5;
    out_color = color;
	gl_Position = vec4(position, 1.0);
}
