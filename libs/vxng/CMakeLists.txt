project(vxng)

add_library(${PROJECT_NAME}
    src/wgsl/fullscreen.wgsl.cpp
    # src/camera/camera.cpp
    # src/camera/orbit-camera.cpp
    src/scene/scene.cpp
    src/renderer.cpp
)

add_library(libs::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        dawn::webgpu_dawn
        glm::glm
)

target_compile_features(${PROJECT_NAME}
    PUBLIC
        cxx_std_17
)
