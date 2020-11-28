#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in uint texture_id;

//layout (location = 2) in vec3 normal;
//layout (location = 4) in vec3 tangent;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv;
layout (location = 3) flat out uint out_texture_id;
layout (location = 4) out vec3 out_eye_cam_pos_distance;
layout (location = 5) out vec3 out_cam_pos;

layout (set = 0, binding = 0) uniform ViewProj_t
{
    mat4  view;
    mat4  proj;
    vec3  cam_pos;
} u_view_proj;

void main()
{
    out_normal = normal;
    
    out_uv = uv;
	
    gl_Position = u_view_proj.proj * u_view_proj.view * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y; // Hack: vulkan's y is down
    
    out_position = position;
    out_position.y = -out_position.y;   
    
    
    out_cam_pos = u_view_proj.cam_pos;
    out_cam_pos.y = -out_cam_pos.y;    
    
    out_texture_id = texture_id;
    
    
    
    vec4 pos_eye  = u_view_proj.view * vec4(position, 1.0);
    pos_eye.y = -pos_eye.y;    
    out_eye_cam_pos_distance = -pos_eye.xyz;    
    
}
