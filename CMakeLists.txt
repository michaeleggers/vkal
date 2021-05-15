#make a build folder, go into it and call
#cmake -G "Visual Studio 15 2017 Win64" -DWINDOWING=VKAL_GLFW ..
#to use GLFW as a windowing backend.


cmake_minimum_required(VERSION 3.10)
project(VKAL VERSION 1.0)
cmake_minimum_required(VERSION 3.12)
find_package(Vulkan REQUIRED)



# GLFW, https://www.glfw.org/docs/latest/build.html#build_link_cmake_source
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(src/external/glfw332src)


# Vulkan Abstraction Layer
add_library(libvkal
    src/vkal.c
    src/vkal.h
)
target_include_directories(libvkal
    PUBLIC Vulkan::Vulkan
)
target_link_libraries(libvkal PUBLIC
    Vulkan::Vulkan 
    glfw
)
target_compile_definitions(libvkal PUBLIC ${WINDOWING})


# Basic Texture Example
add_executable(Texture
    src/examples/texture.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/texture.vert
    src/examples/assets/shaders/texture.frag
)
target_include_directories(Texture
    PUBLIC external
)
target_link_libraries(Texture
    PUBLIC libvkal
    PUBLIC glfw)
	
# Basic Texture Example without GLFW
add_executable(TexturePureWin32 WIN32
    src/examples/texture_win32.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/texture.vert
    src/examples/assets/shaders/texture.frag
)
target_include_directories(TexturePureWin32
    PUBLIC external
)
target_link_libraries(TexturePureWin32
    PUBLIC libvkal)
	
# Mesh Skinning
add_executable(MeshSkinning
    src/examples/mesh_skinning.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/model.c
	src/examples/utils/model.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/texture.vert
    src/examples/assets/shaders/texture.frag
)
target_include_directories(MeshSkinning
    PUBLIC external
)
target_link_libraries(MeshSkinning
    PUBLIC libvkal
    PUBLIC glfw)
	
# Model Loading
add_executable(ModelLoading
    src/examples/model_loading.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/examples/utils/model.c
	src/examples/utils/model.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/model_loading.vert
    src/examples/assets/shaders/model_loading.frag
)
target_include_directories(ModelLoading
    PUBLIC external
)
target_link_libraries(ModelLoading
    PUBLIC libvkal
    PUBLIC glfw)
	
# Primitives
add_executable(Primitives
    src/examples/primitives.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/primitives.vert
    src/examples/assets/shaders/primitives.frag
)
target_include_directories(Primitives
    PUBLIC external
)
target_link_libraries(Primitives
    PUBLIC libvkal
    PUBLIC glfw)
	
# Primitives Dynamic
add_executable(PrimitivesDynamic
    src/examples/primitives_dynamic.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/primitives.vert
    src/examples/assets/shaders/primitives.frag
)
target_include_directories(PrimitivesDynamic
    PUBLIC external
)
target_link_libraries(PrimitivesDynamic
    PUBLIC libvkal
    PUBLIC glfw)
	
# Render Texture
add_executable(RenderToTexture
    src/examples/rendertexture.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/rendertexture.vert
    src/examples/assets/shaders/rendertexture.frag
)
target_include_directories(RenderToTexture
    PUBLIC external
)
target_link_libraries(RenderToTexture
    PUBLIC libvkal
    PUBLIC glfw)
	
# Multiple Textures
add_executable(MultipleTextures
    src/examples/textures.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/textures.vert
    src/examples/assets/shaders/textures.frag
)
target_include_directories(MultipleTextures
    PUBLIC external
)
target_link_libraries(MultipleTextures
    PUBLIC libvkal
    PUBLIC glfw)
	
# Descriptor Array and Push Constant
add_executable(DescriptorArrayPushConstant
    src/examples/textures_descriptorarray_push_constant.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/textures_descriptorarray_push_constant.vert
    src/examples/assets/shaders/textures_descriptorarray_push_constant.frag
)
target_include_directories(DescriptorArrayPushConstant
    PUBLIC external
)
target_link_libraries(DescriptorArrayPushConstant
    PUBLIC libvkal
    PUBLIC glfw)
	
# Dynamic Uniform Buffer
add_executable(DynamicUniformBuffer
    src/examples/textures_dynamic_descriptor.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/textures_dynamic_descriptor.vert
    src/examples/assets/shaders/textures_dynamic_descriptor.frag
)
target_include_directories(DynamicUniformBuffer
    PUBLIC external
)
target_link_libraries(DynamicUniformBuffer
    PUBLIC libvkal
    PUBLIC glfw)
	
# TTF Drawing
add_executable(TTFDrawing
    src/examples/ttf_drawing.c
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/ttf_drawing.vert
    src/examples/assets/shaders/ttf_drawing.frag
)
target_include_directories(TTFDrawing
    PUBLIC external
)
target_link_libraries(TTFDrawing
    PUBLIC libvkal
    PUBLIC glfw)
	
# Mesh Skinning with GLM
add_executable(MeshSkinningGLM
    src/examples/mesh_skinning_glm.cpp
	src/examples/utils/tr_math.c
	src/examples/utils/tr_math.h
	src/platform.c
	src/platform.h
    src/examples/assets/shaders/model_loading_md.vert
    src/examples/assets/shaders/model_loading_md.frag
)
target_include_directories(MeshSkinningGLM
    PUBLIC external src/examples/external/
)
target_link_libraries(MeshSkinningGLM
    PUBLIC libvkal
    PUBLIC glfw)