cmake_minimum_required(VERSION 4.2)

project(DrawingTriangle
		LANGUAGES CXX
)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME}
		main.cpp
)

target_link_libraries(${PROJECT_NAME}
		glfw
		${VULKAN_LIB_LIST}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)