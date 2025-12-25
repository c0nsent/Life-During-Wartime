cmake_minimum_required(VERSION 4.2)

project(DrawingTriangle
		LANGUAGES CXX
)

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

#find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME}
		main.cpp
)

target_link_libraries(${PROJECT_NAME}
		glfw
		Vulkan::Vulkan
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)