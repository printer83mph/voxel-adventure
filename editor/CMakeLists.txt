include(FetchContent)

project(voxel-editor)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
   PRIVATE src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    libs::vxng
    SDL3
    libglew_shared
)
