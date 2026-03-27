project(opengametools)

add_library(${PROJECT_NAME}
    src/ogt_vox.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(${PROJECT_NAME}
    PUBLIC
        cxx_std_17
)
