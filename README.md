# VKAL

The Vulkan Abstraction Layer (short: VKAL) is being created in order to help setting up a Vulkan based
renderer. VKAL is still in development and the API is not final. It is written in C and compiles with
MSVC, CLANG and GCC.

## The reason for VKAL

Despite its name, VKAL does not completely abstract away the Vulkan API. It rather is a set of functions that wrap
many Vulkan-calls in order to make the developer's life easier. It does not prevent the user
to mix in Vulkan calls directly. In fact, VKAL expects the user to do this. If you need a
renderering API that is completely API-agnostic, VKAL is not for you. Have a look at
sokol_gfx which  is part of the Sokol single-header files if you need a clean rendering API that
does not require you to learn a specific graphics API such as DX11/12, OpenGL or Vulkan: 
[Sokol-Headers](https://github.com/floooh/sokol) created by Andre Weissflog.

If you're starting to learn Vulkan and you do want to take a more top-down approach in order to
boost your learning experience, then VKAL  might be worth a look at. That being said, VKAL
is being developed simply because I want to get more familiar with the Vulkan-API myself.

## Dependencies

In order to initialize VKAL, the OS-specific Window-Handle is needed. You can do this via:
- [GLFW3](https://www.glfw.org/)
- [SDL2](https://libsdl.org)
- [Native Win32 (on Windows)](https://docs.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window)

Otherwise, VKAL does depends on the C Standard Library.

## Using VKAL
Clone this repo with:
```
git clone --recurse-submodules git@github.com:michaeleggers/vkal.git
```

Then, in the repo's main folder make a subdirectory called ```build``` and ```cd``` into it.
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

## Examples

The following libraries are required:

* [stb_image](https://github.com/nothings/stb) for loading image-data (PNG, JPG, ...)
* [tinyobjloader-c](https://github.com/syoyo/tinyobjloader-c) for loading OBJ model files

Thanks to the authors of those libs, creating the examples was much easier. Thank you!





