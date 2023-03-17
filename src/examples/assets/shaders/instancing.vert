#version 450
#extension GL_ARB_separate_shader_objects : enable

// struct GPUSprite {
//     float transform[16];
//     uint textureID;
// };

struct GPUSprite {
    mat4  transform;
    mat4  textureID;
};

vec3 quad[6] = vec3[6](
    // Lower left tri
    vec3(-1, 1, 0),
    vec3(-1, -1, 0),
    vec3(1, -1, 0),
    // Upper right tri
    vec3(1, -1, 0),
    vec3(1, 1, 0),
    vec3(-1, 1, 0)
);

vec2 quadUVs[6] = vec2[6](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1),
    vec2(1, 1),
    vec2(1, 0),
    vec2(0, 0)
);

// layout (location = 0) in vec3 position;
// layout (location = 1) in vec2 uv;

// layout (location = 0) out vec3 out_position;
// layout (location = 1) out vec2 out_uv;

layout (set = 0, binding = 0) uniform ViewProj_t
{
    mat4  view;
    mat4  proj;
} u_view_proj;

layout (std430, set = 0, binding = 1) buffer readonly GPUSprites {
    GPUSprite in_GPUSprites[];
};

// mat4 getTransform(uint id) {
//     mat4 transform = mat4(
//         in_GPUSprites[id].transform[0], in_GPUSprites[id].transform[1], in_GPUSprites[id].transform[2], in_GPUSprites[id].transform[3],
//         in_GPUSprites[id].transform[4], in_GPUSprites[id].transform[5], in_GPUSprites[id].transform[6], in_GPUSprites[id].transform[7],
//         in_GPUSprites[id].transform[8], in_GPUSprites[id].transform[9], in_GPUSprites[id].transform[10], in_GPUSprites[id].transform[11],
//         in_GPUSprites[id].transform[12], in_GPUSprites[id].transform[13], in_GPUSprites[id].transform[14], in_GPUSprites[id].transform[15]
//     );
//     return transform;
// }

layout (location = 0) out uint out_textureID;
layout (location = 1) out vec2 out_UV;

void main()
{
    uint vertexIndex = gl_VertexIndex % 6;
    int instanceIndex = gl_VertexIndex / 6;
    //int instanceIndex = gl_InstanceIndex;

    vec3 pos = quad[vertexIndex];
    vec2 uv  = quadUVs[vertexIndex];
    GPUSprite spriteData = in_GPUSprites[instanceIndex];
    //mat4 transform = getTransform(instanceIndex);

    mat4 transform = spriteData.transform;
    //mat4 transform = spriteData.xTransform;
    //out_position = (u_view_proj.proj * vec4(pos, 1.0)).xyz;
    //out_uv = uv;
    vec4 worldPos = transform * vec4(pos, 1.0);
    //worldPos.x += xTransform;
    
    out_textureID = uint(spriteData.textureID[0].x);
    out_UV = uv;
	gl_Position = u_view_proj.proj * u_view_proj.view * worldPos;
}
