include(FetchContent)

project(voxel-editor)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
   PRIVATE src/main.cpp
)

find_package(GLEW 2.0 REQUIRED)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-3.4.0
)
FetchContent_MakeAvailable(SDL3)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    libs::vxng
    SDL3
    GLEW::GLEW
)
