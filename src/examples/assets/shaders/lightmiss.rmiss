#version 460
#extension GL_EXT_ray_tracing : enable

//layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout(location = 1) rayPayloadInEXT bool isLit;

void main()
{
    isLit = true;
}