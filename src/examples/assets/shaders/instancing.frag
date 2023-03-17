#version 450
// #extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout (set = 0, binding = 2) uniform sampler2D textures[];

layout(location = 0) flat in uint in_TextureID;
layout(location = 1)      in vec2 in_UV;

layout(location = 0) out vec4 outColor;


void main() 
{
	vec4 color = texture(textures[nonuniformEXT(in_TextureID)], in_UV);
	outColor = vec4(color.rgb, 1.0);
}
