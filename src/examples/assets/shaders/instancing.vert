#version 450
#extension GL_ARB_separate_shader_objects : enable

struct GPUSprite {
    mat4  transform;
    mat4  metaData;
};

struct GPUFrame {
    vec2 uv;
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

layout (set = 0, binding = 0) uniform PerFrameData
{
    mat4  view;
    mat4  proj;
    float screenWidth;
    float screenHeight;
} perFrameData;

layout (std430, set = 0, binding = 1) buffer readonly GPUSprites {
    GPUSprite in_GPUSprites[];
};

layout (std430, set = 0, binding = 3) buffer readonly GPUFrames {
    GPUFrame in_GPUFrames[];
};

layout (location = 0) out uint out_textureID;
layout (location = 1) out vec2 out_UV;

vec2 getUV(GPUSprite spriteData) {
    uint frameIndex = uint(spriteData.metaData[0].y);
    return in_GPUFrames[frameIndex].uv;
}

void main()
{
    uint vertexIndex = gl_VertexIndex % 6;
    //int instanceIndex = gl_VertexIndex / 6;
    int instanceIndex = gl_InstanceIndex;

    GPUSprite spriteData = in_GPUSprites[instanceIndex];

    vec3 pos = quad[vertexIndex];
    vec2 uv  = quadUVs[vertexIndex];
    // vec2 uv = getUV(spriteData);

    mat4 transform = spriteData.transform;
    transform[3].y = perFrameData.screenHeight-transform[3].y;

    float scale = 20.0;
    vec4 worldPos = transform * vec4(scale*pos, 1.0);
    
    out_textureID = uint(spriteData.metaData[0].x);
    out_UV = uv;
	gl_Position = perFrameData.proj * worldPos;
}
