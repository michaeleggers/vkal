cmake_minimum_required(VERSION 3.24)
project(WIN32_Texture VERSION 1.0)

# Hello Triangle Example using GLFW

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE
${CMAKE_CURRENT_SOURCE_DIR} *.c)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE
${CMAKE_CURRENT_SOURCE_DIR} *.h)     


add_executable(WIN32_Texture
	${SRC_FILES}
	${HEADER_FILES}
	../utils/platform.cpp
	../utils/platform.h
	../utils/tr_math.c
	../utils/tr_math.h
	../assets/shaders/texture.vert
	../assets/shaders/texture.frag
)
target_include_directories(WIN32_Texture
	PUBLIC ../external
	PUBLIC ../utils
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)
target_link_libraries(WIN32_Texture
	PUBLIC vkal)

set_property(TARGET WIN32_Texture   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set_target_properties(WIN32_Texture PROPERTIES WIN32_EXECUTABLE TRUE) 