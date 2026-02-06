# Voxel Engine and Editor

Project name WIP

## Development

```sh
git submodule init
```

We use `clangd` and `clang-format`. To create `compile_commands.json`, run:

```sh
mkdir build && cd build/
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```
