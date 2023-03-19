#version 450
// #extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable

layout (set = 0, binding = 0) uniform PerFrameData
{
    mat4  view;
    mat4  proj;
    float screenWidth;
    float screenHeight;
} perFrameData;

layout (set = 0, binding = 2) uniform sampler2D textures[];

layout(location = 0) flat in uint in_TextureID;
layout(location = 1)      in vec2 in_UV;

layout(location = 0) out vec4 outColor;


void main() 
{
	float screenWidth = perFrameData.screenWidth;
	float screenHeight = perFrameData.screenHeight;
	vec4 color = texture(textures[nonuniformEXT(in_TextureID)], in_UV);
	vec3 tint = vec3(gl_FragCoord.x / screenWidth, gl_FragCoord.y / screenHeight, 0.2);
	outColor = vec4(color.rgb, color.a);
}
