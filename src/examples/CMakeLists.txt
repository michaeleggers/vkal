cmake_minimum_required(VERSION 3.10)
project(Examples VERSION 1.0)

if (${WINDOWING} STREQUAL "VKAL_GLFW")
    add_subdirectory(GLFW_HelloTriangle)
    add_subdirectory(GLFW_Primitives)
    add_subdirectory(GLFW_PrimitivesDynamic)
    add_subdirectory(GLFW_TrueType)
    add_subdirectory(GLFW_Texture)
elseif(${WINDOWING} STREQUAL "VKAL_SDL")
    add_subdirectory(SDL_HelloTriangle)
endif()


