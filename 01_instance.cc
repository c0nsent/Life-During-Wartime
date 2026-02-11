#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <ostream>



using i32 = std::int32_t;
using u32 = std::uint32_t;


constexpr i32 WIDTH = 800;
constexpr i32 HEIGHT = 600;

class HelloTriangleApplication
{
	void createInstance()
	{

	}

	void initWindow(const i32 width, const i32 height)
	{
		if (not glfwInit())
			std::cerr << "Failed to initialize GLFW" << std::endl;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(width, height, "Hello Triangle", nullptr, nullptr);
		if (not window)
			std::cerr << "Failed to create GLFW window" << std::endl;
	}

	void initVulkan()
	{
		createInstance();
	}

	void mainLoop()
	{
		while (not glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

public:

	void run(const i32 width, const i32 height)
	{
		initWindow(width, height);
		initVulkan();
		mainLoop();
		cleanup();
	}

private:

	GLFWwindow* window;

	vk::raii::Context context;
	vk::raii::Instance instance;
};

int main()
{
	HelloTriangleApplication app;

	app.run(WIDTH, HEIGHT);
}