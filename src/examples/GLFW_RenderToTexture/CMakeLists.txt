cmake_minimum_required(VERSION 3.23)
project(GLFW_RenderToTexture VERSION 1.0)

# Hello Triangle Example using GLFW

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.h)     

add_executable(GLFW_RenderToTexture
	${SRC_FILES}
    ${HEADER_FILES}
	../utils/platform.cpp
	../utils/platform.h
    ../utils/tr_math.c
	../utils/tr_math.h
	../assets/shaders/rendertexture.vert
	../assets/shaders/rendertexture.frag
	../assets/shaders/rendertexture_composite.frag
)
target_include_directories(GLFW_RenderToTexture
    PUBLIC ../external
    PUBLIC ../utils
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)
target_link_libraries(GLFW_RenderToTexture
	PUBLIC glfw
	PUBLIC vkal)

set_property(TARGET GLFW_RenderToTexture   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property(TARGET GLFW_RenderToTexture   PROPERTY CXX_STANDARD 11)
