# Render a ton of Quads using bindless techniques.

- Resources used: https://jorenjoestar.github.io/post/modern_sprite_batch/

## TODOS
- Check why updating the current frame on the GPU is super slow.
    ```C++
    gpuSprite->metaData[0].y = sprite->sequence.current;
    ```
- Revisit buffer alignment and offset rules when writing into a host-mapped buffer.

- Buffer mapping: Make sure vkFlusMappedMemoryRanges is called when memory is not HOST_COHERENT.
- The buffer used for storing framedata of a spritesheet could (and should?) be DEVICE_LOCAL.