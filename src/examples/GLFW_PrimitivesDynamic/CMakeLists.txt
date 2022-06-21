cmake_minimum_required(VERSION 3.10)
project(GLFW_PrimitivesDynamic VERSION 1.0)

# Hello Triangle Example using GLFW

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.c)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.h)     

add_executable(GLFW_PrimitivesDynamic
	${SRC_FILES}
    ${HEADER_FILES}
	../utils/platform.c
	../utils/platform.h
    ../utils/tr_math.c
	../utils/tr_math.h
	../assets/shaders/primitives.vert
	../assets/shaders/primitives.frag
)
target_include_directories(GLFW_PrimitivesDynamic
    PUBLIC ../external
    PUBLIC ../utils
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../vkal/
)
target_link_libraries(GLFW_PrimitivesDynamic
	PUBLIC glfw
	PUBLIC vkal)

set_property(TARGET GLFW_PrimitivesDynamic   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
