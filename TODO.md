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
- [ ] Should ```render_pass``` not also be called ```default_render_pass```?
- [ ] Rename vkal_update_descriptor_set_uniform. Everything is a buffer. The descriptor type has to be
      passed to the function anyway, which is redundant information in case of a uniform buffer (and confusing otherwise!).
- [ ] read_file: Better error message when file not found.
- [ ] vkal_end_command_buffer -> vkal_end_default_command_buffer
- [ ] descriptor set layout creation: * layout param should be in plural form.
- [ ] Function that can bind multiple descriptor sets at once? Or just use original vk* command for this?
- [ ] Fix VKAL_ASSERT macro.
- [ ] Examples: Create a proper Vulkan projection matrix so that flipping y in shader is
      not necessary.