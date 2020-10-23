#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout (set = 0, binding = 1) uniform MaterialData_t
{
    vec3 position;
    float aspect;
    uint index;
} u_material_data;

layout(set = 0, binding = 2) uniform u_dummy_t
{
    uint a;
    uint b;
} u_dummy;

layout(set = 0, binding = 3) uniform u_dummy_large_t
{
    uint a;
    uint b;
    uint c;
    uint d;
} u_dummy_large;

void main() 
{
    vec4 texel = texture(textures[u_material_data.index], in_uv);
    
    if (u_dummy.a == 998 && u_dummy_large.b == 1811)
        outColor = vec4(texel.rgb, 1.0);
    else
        outColor = vec4(0, 0, 1, 1);
}
