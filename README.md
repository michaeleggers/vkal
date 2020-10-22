# VKAL

The Vulkan Abstraction Layer (short: VKAL) is being created in order to help setting up a Vulkan based
renderer. VKAL is still in development and the API is not final.

## The reason for VKAL

Despite its name, VKAL does not abstract away the Vulkan API. It rather is a set of functions that wrap
many Vulkan-calls in order to make the developer's life easier. It does not prevent the user
to mix in Vulkan calls directly. In fact, VKAL expects the user to do this. If you need a
renderering API that is completely API-agnostic, VKAL is not for you. Have a look at
sokol_gfx which  is part of the Sokol single-header files: [Sokol-Headers](https://github.com/floooh/sokol)
created by Andre Weissflog.
If you're starting to learn Vulkan and you do want to take a more top-down approach in order to
boost your learning experience, then Vulkan  might be worth a look at. That being said, VKAL
is being developed simply because I want to get more familiar with the Vulkan-API myself.
