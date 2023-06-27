# VKAL

The Vulkan Abstraction Layer (short: VKAL) is being created in order to help setting up a Vulkan based
renderer. VKAL is still in development and the API is not final. It is written in C and compiles with
MSVC, CLANG and GCC.

## Operating Systems supported: 
- **Windows 10/11**
- **Ubuntu LTS 20.04. It _should_ work on other Linux distributions as well, but this is the one we tested it on.**

I got **MacOS** to work but is not maintained and not a priority at this point.

## The reason for VKAL

Despite its name, VKAL does not completely abstract away the Vulkan API. It rather is a set of functions that wrap
many Vulkan-calls in order to make the developer's life easier. It does not prevent the user
to mix in Vulkan calls directly. In fact, VKAL expects the user to do this.

VKAL is used as part of the template program at the [Munich University of Applied Sciences](https://www.cs.hm.edu/en/home/index.en.html) for the Computer Graphics course.

## Prerequisites

In order to initialize VKAL, the OS-specific Window-Handle is needed. You can do this via:
- [GLFW3](https://www.glfw.org/)
- [SDL2](https://libsdl.org)

On windows you can also use [plain WIN32 to get the window](https://docs.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window). VKAL can use the Window Handle to setup
everything. In that case you do not depend on any third-party library apart from the implementation of the C standard library that comes with your operating system.

- [LunarG Vulkan SDK](https://vulkan.lunarg.com/) The SDK is needed! VKAL was tested with version 1.3.216.0
- [CMake](https://cmake.org/download/) In order to build the examples easily or to integrate VKAL easily into your current CMake project CMake version 3.24 is needed.

# Using VKAL
Clone this repo with:
```
git clone --recurse-submodules git@github.com:michaeleggers/vkal.git
```

If you already cloned, but forgot the recursive part, no worries. Just init the submodules afterwards with:
```
git submodule update --init --recursive
```

## CMake

In the repo's main folder make a subdirectory called ```build``` and ```cd``` into it.
### Build using **GLFW**
```
cmake -G "<your target here>" -DWINDOWING=VKAL_GFLW ..
```
### Build using **SDL2** (currently not working on MacOS)
```
cmake -G "<your target here>" -DWINDOWING=VKAL_SDL ..
```
### Build using **WIN32** (only on Windows)
```
cmake -G "<your target here>" -DWINDOWING=VKAL_WIN32 ..
```

## Without CMake

Just drop in ```vkal.h``` and ```vkal.h``` into your project and set a compiler define depending on what backend to use. The following are available at the moment:
```
for GLFW:   VKAL_GLFW
for SDL2:   VKAL_SDL 
for WIN32:  VKAL_WIN32
```
Also, of course, you have to link against Vulkan loader.

# Examples

You have to tell CMake if you want to generate project files for the examples:
```
-DBUILD_EXAMPLES=ON
```

for example:
```
cmake -G "Visual Studio 17 2022" -DWINDOWING=VKAL_GLFW -DBUILD_EXAMPLES=ON ..
```
which will generate Visual Studio 2022 project files and a solution with GLFW as a backend and with the examples available for GLFW.


Some of the examples use some further libraries from other authors, specifically:

* [stb_image](https://github.com/nothings/stb) for loading image-data (PNG, JPG, ...).
* [tinyobjloader-c](https://github.com/syoyo/tinyobjloader-c) for loading OBJ model files.
* [Dear ImGUI](https://github.com/ocornut/imgui) probably the best (c++)library out there to build user interfaces.

Thanks to the authors of those libs, creating the examples was much easier. Thank you!





