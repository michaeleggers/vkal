cmake_minimum_required(VERSION 3.10)
project(GLFW_ImGUI VERSION 1.0)

# Hello Triangle Example using GLFW

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.h)     

add_executable(GLFW_ImGUI
	${SRC_FILES}
    ${HEADER_FILES}
	../utils/platform.c
	../utils/platform.h
    ../assets/shaders/imgui.vert
    ../assets/shaders/imgui.frag
    ../external/imgui/backends/imgui_impl_glfw.cpp
    ../external/imgui/backends/imgui_impl_glfw.h
    ../external/imgui/backends/imgui_impl_vulkan.cpp
    ../external/imgui/backends/imgui_impl_vulkan.h
    ../external/imgui/imgui.cpp
    ../external/imgui/imgui.h
    ../external/imgui/imgui_draw.cpp
    ../external/imgui/imgui_tables.cpp
    ../external/imgui/imgui_widgets.cpp
)
target_include_directories(GLFW_ImGUI
    PUBLIC ../external
    PUBLIC ../external/imgui
    PUBLIC ../utils
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)
target_link_libraries(GLFW_ImGUI
	PUBLIC glfw
	PUBLIC vkal)

set_property(TARGET GLFW_ImGUI   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
