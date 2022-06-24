cmake_minimum_required(VERSION 3.10)
project(SDL_HelloTriangle VERSION 1.0)

# Hello Triangle Example using SDL

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.h)     

add_executable(SDL_HelloTriangle
	${SRC_FILES}
    ${HEADER_FILES}
	../utils/platform.c
	../utils/platform.h
    ../assets/shaders/hello_triangle.vert
    ../assets/shaders/hello_triangle.frag
)
target_include_directories(SDL_HelloTriangle
    PUBLIC ../external
    PUBLIC ../utils
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL/include
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)
target_link_libraries(SDL_HelloTriangle
    PUBLIC SDL2
    PUBLIC SDL2main
	PUBLIC vkal)

set_property(TARGET SDL_HelloTriangle   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
