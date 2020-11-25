#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) flat in uint in_texture_id;

layout(set = 0, binding = 1) uniform sampler2D u_textures[];

void main() 
{
    vec4 texel = texture(u_textures[ in_texture_id ], in_uv/256.0f);
    float z = in_position.z / in_position.w;
	vec3 normal_color = 0.5*in_normal+0.5;
    outColor = vec4( normal_color, 1.0);
}
