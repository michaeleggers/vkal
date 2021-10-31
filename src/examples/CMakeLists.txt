cmake_minimum_required(VERSION 3.10)
project(GLFW_Texture VERSION 1.0)

# Basic Texture Example
add_executable(GLFW_Texture
    texture.c
	utils/tr_math.c
	utils/tr_math.h
	utils/platform.c
	utils/platform.h
    assets/shaders/texture.vert
    assets/shaders/texture.frag
)
target_include_directories(GLFW_Texture
    PUBLIC external
)
target_link_libraries(GLFW_Texture
	PUBLIC vkal)

# Model Loading
add_executable(GLFW_ModelLoading
    model_loading.c
	utils/tr_math.c
	utils/tr_math.h
	utils/model.c
	utils/model.h
	utils/platform.c
	utils/platform.h
    assets/shaders/model_loading.vert
    assets/shaders/model_loading.frag
)
target_include_directories(GLFW_ModelLoading
    PUBLIC external
)
target_link_libraries(GLFW_ModelLoading
    PUBLIC vkal
    PUBLIC glfw)

# Primitives
add_executable(GLFW_Primitives
	primitives.c
	utils/tr_math.c
	utils/tr_math.h
	utils/platform.c
	utils/platform.h
	assets/shaders/primitives.vert
	assets/shaders/primitives.frag
)
target_include_directories(GLFW_Primitives
	PUBLIC external
)
target_link_libraries(GLFW_Primitives
	PUBLIC vkal
	PUBLIC glfw)