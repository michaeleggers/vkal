#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_EXT_nonuniform_qualifier    : enable


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D u_texture_1;
layout(set = 0, binding = 1) uniform sampler2D u_texture_2;

vec2 offsets3x3[9] = vec2[9](
    vec2(-1, -1), vec2(0, -1), vec2(1, -1),
    vec2(-1,  0), vec2(0,  0), vec2(1,  0),
    vec2(-1,  1), vec2(0,  1), vec2(1,  1)
);

float sobel3x3_X[9] = float[9](
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
);

float sobel3x3_Y[9] = float[9](
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
);

void main() 
{
    vec4 texel1 = texture(u_texture_1, in_uv);
    vec4 texel2 = texture(u_texture_2, in_uv);
    vec2 texture2_size = textureSize(u_texture_2, 0);
    vec2 dxdy = vec2(1.f/texture2_size.x, 1.f/texture2_size.y);
    
    float sobel_x = 0.0f;
    float sobel_y = 0.0f;
    for (int i = 0; i < 9; ++i) {
        vec4 texel2 = sobel3x3_X[i] * texture( u_texture_2, in_uv + vec2(dxdy.x*offsets3x3[i].x, dxdy.y*offsets3x3[i].y) );
        sobel_x += (texel2.x + texel2.y + texel2.z)/3.0f;
        texel2      = sobel3x3_Y[i] * texture( u_texture_2, in_uv + vec2(dxdy.x*offsets3x3[i].x, dxdy.y*offsets3x3[i].y) );
        sobel_y += (texel2.x + texel2.y + texel2.z)/3.0f;       
    }
    float final_texel = sqrt(sobel_x*sobel_x + sobel_y*sobel_y);
    //texel2_sobelX /= 9.f; // normalization not needed here!
    final_texel = smoothstep(0.0, 1.0, final_texel);
    
	//outColor = vec4(texel1.rgb * texel2.rgb, 1.0);
    outColor = vec4( vec3(final_texel) * in_color.xyz, 1.0 );
    
}
