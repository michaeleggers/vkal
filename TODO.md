### TODOs

- [ ] Move duplicate image loading code from examples to /utils/*.c.
- [x] Make shaders compile automatically in Visual Studio.
      NOTE: Googles shaderc (https://github.com/google/shaderc) is used. In order create eg vs-files with cmake that
      already links to shaderc_combined cmake v3.24 is needed!
- [ ] Fix formatting of sourcecode.
- [ ] Correct description-comments in examples.
- [x] Use C-Library to load files instead of native OS-specific APIs.
- [x] Replace some of the (unlicensed) textures for example programs.
- [x] Create DearImGUI example.
- [ ] Define VKAL_TRUE / VKAL_FALSE instead of using uint32_t for true / false.
- [ ] Free malloc'd memory.
- [x] Print info about available swapchain present modes and which one was selected.