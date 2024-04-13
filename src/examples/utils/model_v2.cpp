#include "model_v2.h"

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

Model create_model_from_file_indexed(char const* file)
{
	char exe_path[256];
	get_exe_path(exe_path, 256 * sizeof(char));
	std::string final_path = concat_paths(std::string(exe_path), std::string(file));

	Model model = {};
	/* Find out how many vertices and indices this model has in total.
	Assimp makes sure to use multiple vertices per face by the flag 'aiProcess_JoinIdenticalVertices'.
	So we allocate exactly as much memory as is required for the vertex-buffer.
	*/
	aiScene const* scene = aiImportFile(final_path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		model.vertex_count += scene->mMeshes[i]->mNumVertices;
		for (int f = 0; f < scene->mMeshes[i]->mNumFaces; ++f) {
			model.index_count += scene->mMeshes[i]->mFaces[f].mNumIndices;
			model.face_count++;
		}
	}
	model.vertices = (Vertex*)malloc(model.vertex_count * sizeof(Vertex));
	model.indices = (uint32_t*)malloc(model.index_count * sizeof(uint32_t));

	/* Go through all faces of meshes and copy. */
	uint32_t indices_copied = 0;
	uint32_t vertices_copied = 0;
	for (int m = 0; m < scene->mNumMeshes; ++m) {
		for (int f = 0; f < scene->mMeshes[m]->mNumFaces; ++f) {
			for (int i = 0; i < scene->mMeshes[m]->mFaces[f].mNumIndices; ++i) {
				/* Get current index/vertex/normal from model file */
				uint32_t* index = (uint32_t*)&(scene->mMeshes[m]->mFaces[f].mIndices[i]);
				aiVector3D vertex = scene->mMeshes[m]->mVertices[*index];

				/* copy into Model data */
				model.indices[indices_copied] = *index;
				Vertex* current_vertex = &model.vertices[*index];
				current_vertex->pos.x = vertex.x;
				current_vertex->pos.y = vertex.y;
				current_vertex->pos.z = vertex.z;

				if (scene->mMeshes[m]->HasTextureCoords(0)) {
					aiVector3D uv = scene->mMeshes[m]->mTextureCoords[0][*index];
					current_vertex->uv.x = uv.x;
					current_vertex->uv.y = uv.y;
				}

				if (scene->mMeshes[m]->HasNormals()) {
					aiVector3D normal = scene->mMeshes[m]->mNormals[*index];
					current_vertex->normal.x = normal.x;
					current_vertex->normal.y = normal.y;
					current_vertex->normal.z = normal.z;
				}

				/* add debug color */
				{
					current_vertex->color = glm::vec4(1.f, 0.84f, 0.f, 0.f);
				}

				indices_copied++;
			}
		}

		aiAABB aabb = scene->mMeshes[m]->mAABB;
		model.bounding_box.min_xyz = glm::vec4(aabb.mMin.x, aabb.mMin.y, aabb.mMin.z, 1.0f);
		model.bounding_box.max_xyz = glm::vec4(aabb.mMax.x, aabb.mMax.y, aabb.mMax.z, 1.0f);
	}

	return model;
}

void assign_texture_to_model(Model * model, VkalTexture texture, uint32_t id, TextureType texture_type)
{
	assert( (texture_type < MAX_TEXTURE_TYPES) && "Unknown TextureType!" );
	if (texture_type == TEXTURE_TYPE_DIFFUSE) {
		model->material.texture_id = id;
		model->texture = texture;
		model->material.is_textured = 1;
	}
	else if (texture_type == TEXTURE_TYPE_NORMAL) {
		model->material.normal_id = id;
		model->normal_map = texture;
		model->material.has_normal_map = 1;
	}
}