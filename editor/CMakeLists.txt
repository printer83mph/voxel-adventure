project(voxel-editor)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        src/tools/shared/brush-kernel.cpp
        src/tools/draggable-tool.cpp
        src/tools/voxel-brush.cpp
        src/cursors.cpp
        src/editor.cpp
        src/palette.cpp
        src/main.cpp
        src/tool.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        libs::vxng
        glm::glm
        SDL3::SDL3
        dawn::webgpu_dawn
        sdl3webgpu
        imgui
)
