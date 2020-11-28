#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout(location = 0)      in vec3 in_position;
layout(location = 1)      in vec3 in_normal;
layout(location = 2)      in vec2 in_uv;
layout(location = 3) flat in uint in_texture_id;
layout(location = 4)      in vec3 in_eye_cam_pos_distance;
layout(location = 5)      in vec3 in_cam_pos;

layout(set = 0, binding = 1) uniform samplerCube u_cubemap_sampler[];

void main() 
{
    vec3 cam_to_pos = normalize( in_cam_pos - in_position );
    cam_to_pos = 100 * cam_to_pos;
    cam_to_pos.x = cam_to_pos.x;
    cam_to_pos.y = cam_to_pos.y;
    cam_to_pos.z = cam_to_pos.z;
    vec4 texel = texture( u_cubemap_sampler[ in_texture_id ], cam_to_pos );

    outColor = vec4( texel.rgb, 1.0);
    
}
