cmake_minimum_required(VERSION 3.24)
project(GLFW_Raytracing VERSION 1.0)

# Hello Triangle Example using GLFW

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR} *.h)     

add_executable(GLFW_Raytracing
	${SRC_FILES}
    ${HEADER_FILES}
	../utils/platform.cpp
	../utils/platform.h
    ../utils/tr_math.c
	../utils/tr_math.h
    ../utils/camera.h
    ../utils/camera.cpp
    ../utils/model_v2.h
    ../utils/model_v2.cpp
	../assets/shaders/raygen.rgen
	../assets/shaders/closesthit.rchit
	../assets/shaders/lightmiss.rmiss
	../assets/shaders/miss.rmiss
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
target_include_directories(GLFW_Raytracing
    PUBLIC ../external
    PUBLIC ../utils
    PUBLIC ../external/imgui
    PUBLIC ../external/assimp/include/
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)
target_link_libraries(GLFW_Raytracing
	PUBLIC glfw
    PUBLIC assimp
	PUBLIC vkal)

set_property(TARGET GLFW_Raytracing   PROPERTY CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_property(TARGET GLFW_Raytracing   PROPERTY CXX_STANDARD 11)
