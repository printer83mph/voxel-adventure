project(vxng)

add_library(${PROJECT_NAME}
  src/vxng-pub.cpp
)

add_library(libs::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_17
)
