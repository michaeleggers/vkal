#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 color;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_color;

layout (set = 0, binding = 0) uniform u_view_proj_t
{
    mat4 view;
    mat4 proj;
} u_view_proj;

layout (set = 1, binding = 0) uniform u_model_data_t
{
    mat4 model_mat;
} u_model_data;

void main()
{
    out_color = color;
    out_normal = (u_view_proj.proj * u_view_proj.view * u_model_data.model_mat * vec4(normal, 0.0)).xyz;
    vec4 pos_clipspace = u_view_proj.proj * u_view_proj.view * u_model_data.model_mat * vec4(position, 1.0);
	gl_Position = pos_clipspace;
    //gl_Position.y = -gl_Position.y; // Hack: vulkan's y is down
}
