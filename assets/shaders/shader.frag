#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;
//layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 0) uniform sampler2D u_texture;

void main() 
{
    vec4 texel2 = texture(u_texture, in_uv);
//	vec4 texel = texture(textures[0], in_uv);
    
	outColor = vec4(texel2.rgb, 1.0);
}
