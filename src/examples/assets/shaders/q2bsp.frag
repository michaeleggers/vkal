#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) flat in uint in_texture_id;
layout(location = 4) in vec3 in_eye_cam_pos_distance;

layout(set = 0, binding = 1) uniform sampler2D u_textures[];

void main() 
{
    vec4 texel = texture(u_textures[ in_texture_id ], in_uv);
    float z = in_position.z / in_position.w;
	vec3 normal_color = 0.5*in_normal+0.5;
    vec3 ambient = vec3(.01, .01, .01);
    
    /* Fog */
    vec3 fog_color = vec3(0.761, 0.698, 0.502);
    float min_fog_radius = 25.0;
    float max_fog_radius = 1000.0;
    float dist = length(in_eye_cam_pos_distance);
    float fog_fact = (dist - min_fog_radius) / (max_fog_radius - min_fog_radius);
    fog_fact = clamp(fog_fact, 0.0, 1.0);
    vec3 final_color = mix(texel.rgb, fog_color, fog_fact);
    
    outColor = vec4( final_color, 1.0);
}
