project(voxel-editor)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        src/editor.cpp
        src/main.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        libs::vxng
        glm::glm
        SDL3::SDL3
        webgpu
        sdl3webgpu
)

target_copy_webgpu_binaries(${PROJECT_NAME})
