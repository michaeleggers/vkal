#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable


#define	SURF_LIGHT		0x1		// value will hold the light strength
#define	SURF_SLICK		0x2		// effects game physics
#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	        // scroll towards angle
#define	SURF_NODRAW		0x80	        // don't bother referencing the texture


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) flat in uint in_texture_id;
layout(location = 4) in vec3 in_eye_cam_pos_distance;
layout(location = 5) flat in uint in_surface_flags;

layout(set = 0, binding = 1) uniform sampler2D u_textures[];

void main() 
{
#if 1
    vec4 texel = texture(u_textures[ in_texture_id ], in_uv);
    float z = in_position.z / in_position.w;
	vec3 normal_color = 0.5*in_normal+0.5;
    vec3 ambient = vec3(.01, .01, .01);
    
    /* Fog */
    //vec3 fog_color = vec3(0.761, 0.698, 0.502); // sand color
    vec3 fog_color = vec3(0.161, 0.118, 0.112);
    float min_fog_radius = 50.0;
    float max_fog_radius = 2500.0;
    float dist = length(in_eye_cam_pos_distance);
    float fog_fact = (dist - min_fog_radius) / (max_fog_radius - min_fog_radius);
    fog_fact = clamp(fog_fact, 0.0, 1.0);
    vec3 final_color = mix(texel.rgb, fog_color, fog_fact);
    
    if ( (in_surface_flags & SURF_TRANS66) == SURF_TRANS66) {
        outColor = vec4( final_color, 0.66);
    }
    else {
        outColor = vec4( final_color, 1.0);
    }
    #endif
    //outColor = vec4( 0.5*in_normal + 0.5, 1.0);
}
