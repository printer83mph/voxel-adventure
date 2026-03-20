project(imgui)

set(IMGUI_SOURCES
    src/imgui.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_impl_sdl3.cpp
    src/imgui_impl_wgpu.cpp
    src/imgui_tables.cpp
    src/imgui_widgets.cpp
)

# On Apple, imgui_impl_wgpu.cpp pulls in Cocoa headers,
# so it must be compiled as Objective-C++.
if(APPLE)
    set_source_files_properties(src/imgui_impl_wgpu.cpp
        PROPERTIES LANGUAGE OBJCXX
    )
endif()

add_library(${PROJECT_NAME} STATIC ${IMGUI_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ./include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    dawn::webgpu_dawn
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WEBGPU_BACKEND_DAWN
    IMGUI_IMPL_WEBGPU_BACKEND_DAWN
)
