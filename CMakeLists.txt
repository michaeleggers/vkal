#make a build folder, go into it and call
#cmake -G "Visual Studio 15 2017 Win64" -DWINDOWING=VKAL_GLFW ..
#to use GLFW as a windowing backend.


cmake_minimum_required(VERSION 3.10)
project(VKAL VERSION 1.0)
cmake_minimum_required(VERSION 3.12)
find_package(Vulkan REQUIRED)

if (${WINDOWING} STREQUAL "VKAL_GLFW")
	# GLFW, https://www.glfw.org/docs/latest/build.html#build_link_cmake_source
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	add_subdirectory(src/external/glfw332src)
endif()

add_subdirectory(src/vkal)

# Basic Texture Example
add_executable(GLFW_Texture
    src/examples/texture.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/texture.vert
    src/examples/assets/shaders/texture.frag
)
target_include_directories(GLFW_Texture
    PUBLIC external
)
target_link_libraries(GLFW_Texture
	PUBLIC libvkal)
	
# Basic Texture Example without GLFW, just WinMain...
add_executable(Win32_Texture WIN32
    src/examples/texture_win32.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/texture.vert
    src/examples/assets/shaders/texture.frag
)
target_include_directories(Win32_Texture
    PUBLIC external
)
target_link_libraries(Win32_Texture
    PUBLIC libvkal)
	
# Mesh Skinning
add_executable(GLFW_MeshSkinning
    src/examples/mesh_skinning.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/model.c
	src/examples/utils/model.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/texture.vert
    src/examples/assets/shaders/texture.frag
)
target_include_directories(GLFW_MeshSkinning
    PUBLIC external
)
target_link_libraries(GLFW_MeshSkinning
    PUBLIC libvkal
    PUBLIC glfw)
	
# Model Loading
add_executable(GLFW_ModelLoading
    src/examples/model_loading.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/model.c
	src/examples/utils/model.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/model_loading.vert
    src/examples/assets/shaders/model_loading.frag
)
target_include_directories(GLFW_ModelLoading
    PUBLIC external
)
target_link_libraries(GLFW_ModelLoading
    PUBLIC libvkal
    PUBLIC glfw)
	
# Primitives
add_executable(GLFW_Primitives
    src/examples/primitives.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/primitives.vert
    src/examples/assets/shaders/primitives.frag
)
target_include_directories(GLFW_Primitives
    PUBLIC external
)
target_link_libraries(GLFW_Primitives
    PUBLIC libvkal
    PUBLIC glfw)
	
# Primitives Dynamic
add_executable(GLFW_PrimitivesDynamic
    src/examples/primitives_dynamic.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/primitives.vert
    src/examples/assets/shaders/primitives.frag
)
target_include_directories(GLFW_PrimitivesDynamic
    PUBLIC external
)
target_link_libraries(GLFW_PrimitivesDynamic
    PUBLIC libvkal
    PUBLIC glfw)
	
# Render Texture
add_executable(GLFW_RenderToTexture
    src/examples/rendertexture.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/rendertexture.vert
    src/examples/assets/shaders/rendertexture.frag
)
target_include_directories(GLFW_RenderToTexture
    PUBLIC external
)
target_link_libraries(GLFW_RenderToTexture
    PUBLIC libvkal
    PUBLIC glfw)
	
# Multiple Textures
add_executable(GLFW_MultipleTextures
    src/examples/textures.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/textures.vert
    src/examples/assets/shaders/textures.frag
)
target_include_directories(GLFW_MultipleTextures
    PUBLIC external
)
target_link_libraries(GLFW_MultipleTextures
    PUBLIC libvkal
    PUBLIC glfw)
	
# Descriptor Array and Push Constant
add_executable(GLFW_DescriptorArrayPushConstant
    src/examples/textures_descriptorarray_push_constant.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/textures_descriptorarray_push_constant.vert
    src/examples/assets/shaders/textures_descriptorarray_push_constant.frag
)
target_include_directories(GLFW_DescriptorArrayPushConstant
    PUBLIC external
)
target_link_libraries(GLFW_DescriptorArrayPushConstant
    PUBLIC libvkal
    PUBLIC glfw)
	
# Dynamic Uniform Buffer
add_executable(GLFW_DynamicUniformBuffer
    src/examples/textures_dynamic_descriptor.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/textures_dynamic_descriptor.vert
    src/examples/assets/shaders/textures_dynamic_descriptor.frag
)
target_include_directories(GLFW_DynamicUniformBuffer
    PUBLIC external
)
target_link_libraries(GLFW_DynamicUniformBuffer
    PUBLIC libvkal
    PUBLIC glfw)
	
# TTF Drawing
add_executable(GLFW_TTFDrawing
    src/examples/ttf_drawing.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/ttf_drawing.vert
    src/examples/assets/shaders/ttf_drawing.frag
)
target_include_directories(GLFW_TTFDrawing
    PUBLIC external
)
target_link_libraries(GLFW_TTFDrawing
    PUBLIC libvkal
    PUBLIC glfw)
	
# Mesh Skinning with GLM
add_executable(GLFW_MeshSkinningGLM
    src/examples/mesh_skinning_glm.cpp
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/platform.c
	src/examples/utils/platform.h
    src/examples/assets/shaders/model_loading_md.vert
    src/examples/assets/shaders/model_loading_md.frag
)
target_include_directories(GLFW_MeshSkinningGLM
    PUBLIC external src/examples/external/
)
target_link_libraries(GLFW_MeshSkinningGLM
    PUBLIC libvkal
    PUBLIC glfw)
