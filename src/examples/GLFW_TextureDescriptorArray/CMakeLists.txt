cmake_minimum_required(VERSION 3.24)
project(GLFW_TextureDescriptorArray VERSION 1.0)

# Hello Triangle Example using GLFW

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.h)     

add_executable(GLFW_TextureDescriptorArray
	${SRC_FILES}
    ${HEADER_FILES}
	../utils/platform.cpp
	../utils/platform.h
    ../utils/tr_math.c
	../utils/tr_math.h
	../assets/shaders/textures_descriptorarray_push_constant.vert
	../assets/shaders/textures_descriptorarray_push_constant.frag	
)
target_include_directories(GLFW_TextureDescriptorArray
    PUBLIC ../external
    PUBLIC ../utils
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)
target_link_libraries(GLFW_TextureDescriptorArray
	PUBLIC glfw
	PUBLIC vkal)

set_property(TARGET GLFW_TextureDescriptorArray   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
