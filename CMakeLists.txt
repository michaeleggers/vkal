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
add_compile_definitions(VKAL_GLFW) 



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