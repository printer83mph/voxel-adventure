# Voxel Adventure

`vxng`: a sparse voxel octree engine, with a voxel editor demo app

## Building

Dependencies:

- CMake 4.2 or higher
- (Linux) `libx11-xcb-dev` or distro equivalent

This project uses:

- Dawn (WebGPU) as a render hardware interface
- SDL3 for window/io management
- glm for linear algebra etc.
- opengametools for

First, initialize submodules (not recursive):

```sh
git submodule update --init
```

We use Ninja multi-config with clangd. To configure CMake, run:

```sh
mkdir build && cd build/
cmake .. -G "Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                                 -DDAWN_FETCH_DEPENDENCIES=ON
#                                ^ Automatically fetch dawn dependencies
```

Then, to build the editor app:

```sh
cmake --build . --target voxel-editor --config "Release" -j 8 # builds to ./Release/voxel-editor
# or for a debug build:
cmake --build . --target voxel-editor --config "Debug" -j 8 # builds to ./Debug/voxel-editor
```

## Development

We use `clangd` and `clang-format`. To create `compile_commands.json`, make sure `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` is in your CMake configuration.

The VSCode/zed integrations assume you are using the Ninja Multi-Config generator.

### Using devcontainers (Linux)

Make sure to add Docker to the x11 ACL.

```sh
xhost +local:docker
```

Everything else should work out-of-the-box.
