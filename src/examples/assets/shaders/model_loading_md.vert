#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec3  position;
layout (location = 1) in vec3  normal;
layout (location = 2) in vec3  color;
layout (location = 3) in uint  bone_indices[4];
layout (location = 7) in float bone_weights[4];

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

layout (set = 2, binding = 0) readonly buffer OffsetMatrices 
{ 
    mat4 m[];
} offset_matrices; 

layout (set = 3, binding = 0) readonly buffer SkeletonMatrices 
{ 
    mat4 m[];
} skeleton_matrices;

void main()
{
    out_color = vec3(bone_weights[0], bone_weights[1], (bone_weights[2] + bone_weights[3])/2.0);
    out_normal = (u_view_proj.proj * u_view_proj.view * u_model_data.model_mat * vec4(normal, 0.0)).xyz;
    
    mat4 skin_mat = 
        bone_weights[0]    *  (skeleton_matrices.m[bone_indices[0]]) +
        bone_weights[1]    *  (skeleton_matrices.m[bone_indices[1]]) +
        bone_weights[2]    *  (skeleton_matrices.m[bone_indices[2]]) +
        bone_weights[3]    *  (skeleton_matrices.m[bone_indices[3]]) ;
    vec4 pos_clipspace = u_view_proj.proj * u_view_proj.view * 
                         u_model_data.model_mat * skin_mat * vec4(position, 1.0);
	gl_Position = pos_clipspace;
    gl_Position.y = -gl_Position.y; // Hack: vulkan's y is down
}
