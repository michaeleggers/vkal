/* Copyright (c) 2019-2020, Sascha Willems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#version 460 core

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

//layout(binding = 3, set = 0) uniform sampler2D textures[];

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

struct AreaLight
{
  vec3 pos;
  vec3 dir;
  vec2 size;
};

struct Material
{
  vec4 emissive;
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  uint texture_id;
  uint is_textured;
  uint normal_id;
  uint has_normal_map;
};
layout(binding = 5, set = 0) buffer materials
{
  Material m;
} in_Materials[];

layout(binding = 6, set = 0) buffer instance_metadata
{
  uint md;
} in_Metadata[];

layout(binding = 7, set = 0) uniform sampler2D textures[];

struct Vertex
{
  vec3 pos;
  vec2 uv;
  vec3 normal;
  vec4 color;
  vec3 tangent;
  float dummy;
};
layout(binding = 3, set = 0) buffer vertices 
{
  vec4 v[];
} in_Vertices[];


layout(binding = 4, set = 0) buffer indices
{
  uint i[];
} in_Indices[];


//layout(location = 0) rayPayloadInEXT vec3 hitValue;
struct HitValue
{
	vec3 diffuseColor;
	vec3 pos;
};
layout(location = 0) rayPayloadInEXT HitValue hitValue;
hitAttributeEXT vec3 attribs;

layout(location = 1) rayPayloadEXT bool isLit;


// vec3 getPosition(uint i) {
//   return vec3(in_Vertices[i].pos[0], in_Vertices[i].pos[1], in_Vertices[i].pos[2]);  
// }

// vec3 getNormal(uint indexX, uint indexY, uint indexZ) {
//   return vec3(in_Vertices[indexX].normal[0], in_Vertices[i].normal[1], in_Vertices[i].normal[2]);
// }


Vertex unpack(uint index)
{
  vec4 data0 = in_Vertices[nonuniformEXT(gl_InstanceCustomIndexEXT)].v[4 * index + 0];
  vec4 data1 = in_Vertices[nonuniformEXT(gl_InstanceCustomIndexEXT)].v[4 * index + 1];
  vec4 data2 = in_Vertices[nonuniformEXT(gl_InstanceCustomIndexEXT)].v[4 * index + 2];
  vec4 data3 = in_Vertices[nonuniformEXT(gl_InstanceCustomIndexEXT)].v[4 * index + 3];

  Vertex  v;
  v.pos = vec3(data0.x, data0.y, data0.z);
  v.uv = vec2(data0.w, data1.x);
  v.normal = vec3(data1.y, data1.z, data1.w);

  return v;
}

void main()
{
  uint i0 = in_Indices[gl_InstanceCustomIndexEXT].i[3*gl_PrimitiveID + 0];
  uint i1 = in_Indices[gl_InstanceCustomIndexEXT].i[3*gl_PrimitiveID + 1];
  uint i2 = in_Indices[gl_InstanceCustomIndexEXT].i[3*gl_PrimitiveID + 2];
  Vertex v0 = unpack(i0);
  Vertex v1 = unpack(i1);
  Vertex v2 = unpack(i2);

  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

  uint material_info_index = in_Metadata[nonuniformEXT(gl_InstanceCustomIndexEXT)].md;
  Material m = in_Materials[nonuniformEXT(material_info_index)].m;
  vec4 textureColor = m.diffuse;
  if (m.is_textured == 1) {
    textureColor = texture(textures[nonuniformEXT(m.texture_id)], (barycentricCoords.x*v0.uv + barycentricCoords.y*v1.uv + barycentricCoords.z*v2.uv));
  }

	AreaLight areaLight;
	areaLight.pos = vec3(6, 10, 6);

	AreaLight areaLight2;
	areaLight2.pos = vec3(0, 10, 0);

  float shadowStrength0 = 0.6;
  vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  vec3 direction =   normalize(areaLight.pos - origin);
  float lightDistance = length(areaLight.pos - origin);
	float tmin = 0.001;
	float tmax = lightDistance;
  isLit = false;
  traceRayEXT(topLevelAS, gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xff, 0, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);
  if (isLit) {
    shadowStrength0 = 0.0;
  }

  float shadowStrength1 = 0.6;
  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  direction =   normalize(areaLight2.pos - origin);
  lightDistance = length(areaLight2.pos - origin);
	tmax = lightDistance;
  isLit = false;
  traceRayEXT(topLevelAS, gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xff, 0, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);
  if (isLit) {
    shadowStrength1 = 0.0;
  }

  float shadowStrength = clamp(shadowStrength0 + shadowStrength1, 0.0, 1.0);
  shadowStrength = shadowStrength0;
  
  vec3 normal = normalize(v0.normal + v1.normal + v2.normal);

  hitValue.diffuseColor = (1.0 - shadowStrength) * textureColor.bgr;
  //hitValue.diffuseColor = (1.0 - shadowStrength) * m.diffuse.rgb;
}
