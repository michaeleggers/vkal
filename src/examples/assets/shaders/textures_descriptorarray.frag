#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;
layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 1) uniform u_texture_info_t
{
    uint index;
} u_texture_info;

void main() 
{
	vec4 texel = texture(textures[u_texture_info.index], in_uv);
    
	outColor = vec4(texel.rgb, 1.0);
}
