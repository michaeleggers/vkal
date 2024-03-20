#version 460
#extension GL_EXT_ray_tracing : enable

//layout(location = 0) rayPayloadInEXT vec3 hitValue;

struct HitValue
{
	vec3 diffuseColor;
	vec3 pos;
};
layout(location = 0) rayPayloadInEXT HitValue hitValue;

layout(location = 1) rayPayloadInEXT bool isLit;

void main()
{
    // hitValue.diffuseColor = vec3(0.0f, 0.0f, 1.0f);
    isLit = false;
}