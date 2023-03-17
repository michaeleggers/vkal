cmake_minimum_required(VERSION 3.24)
project(SDL_Instancing VERSION 1.0)

# Hello Triangle Example using SDL

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.h)     

add_executable(SDL_Instancing
	${SRC_FILES}
    ${HEADER_FILES}
	../utils/platform.cpp
	../utils/platform.h
    ../assets/shaders/instancing.vert
    ../assets/shaders/instancing.frag
)
target_include_directories(SDL_Instancing
    PUBLIC ../external
    PUBLIC ../utils
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL/include
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)
target_link_libraries(SDL_Instancing
    PUBLIC SDL2
    PUBLIC SDL2main
	PUBLIC vkal)

set_property(TARGET SDL_Instancing   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property(TARGET SDL_Instancing   PROPERTY CXX_STANDARD 11)
