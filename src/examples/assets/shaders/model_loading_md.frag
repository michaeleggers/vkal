#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;

layout (set = 0, binding = 1) uniform u_viewport_t
{
    vec2 dimensions;
} u_viewport;


void main() 
{
    vec2 dimensions = u_viewport.dimensions;
    float dx = gl_FragCoord.x/dimensions.x;
    float dy = gl_FragCoord.y/dimensions.y;
    
    vec3 normal_color = 0.5*normalize(in_normal)+0.5;
    outColor = vec4(in_color, 1.0);
}
