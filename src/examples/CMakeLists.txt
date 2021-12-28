cmake_minimum_required(VERSION 3.10)
project(GLFW_Texture VERSION 1.0)

# Hello Triangle Example using GLFW
add_executable(GLFW_HelloTriangle
	hello_triangle.cpp
	utils/platform.c
	utils/platform.h
    assets/shaders/hello_triangle.vert
    assets/shaders/hello_triangle.frag
)
target_include_directories(GLFW_HelloTriangle
    PUBLIC external
)
target_link_libraries(GLFW_HelloTriangle
	PUBLIC vkal)

# Hello Triangle Example using SDL2
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL)
add_executable(SDL_HelloTriangle
	hello_triangle_sdl.cpp
	utils/platform.c
	utils/platform.h
	assets/shaders/hello_triangle.vert
	assets/shaders/hello_triangle.frag
)
target_include_directories(SDL_HelloTriangle
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL/include
	PUBLIC external
)
target_link_libraries(SDL_HelloTriangle
	PUBLIC vkal
	# if (${WINDOWING} STREQUAL "VKAL_SDL")
	PUBLIC SDL2
	PUBLIC SDL2main
	# endif()
)

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
	
set_property(TARGET GLFW_Texture      PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property(TARGET GLFW_ModelLoading PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property(TARGET GLFW_Primitives   PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")


set_property(TARGET GLFW_Primitives     PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property(TARGET SDL_HelloTriangle   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
