# Voxel Engine and Editor

Project name WIP

## Development

```sh
git submodule init
```

We use `clangd` and `clang-format`. To create `compile_commands.json`, run:

```sh
mkdir build && cd build/
cmake .. -G "Ninja Multi-Config" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

The vscode/zed integrations assume you are using the Ninja Multi-Config generator.

### Using devcontainers (Linux)

Make sure to add docker to the x11 ACL.

```sh
xhost +local:docker
```

Everything else should work out-of-the-box.
