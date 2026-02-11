/*#if defined(__INTELLISENSE__) and not defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif*/

#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <ostream>
#include <GLFW/glfw3.h>


using i32 = std::int32_t;
using u32 = std::uint32_t;


constexpr i32 WIDTH = 800;
constexpr i32 HEIGHT = 600;

class HelloTriangleApplication
{
	void initWindow(const i32 width, const i32 height)
	{
		if (not glfwInit())
			std::cerr << "Failed to initialize GLFW" << std::endl;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(width, height, "Hello Triangle", nullptr, nullptr);
		if (not m_window)
			std::cerr << "Failed to create GLFW window" << std::endl;
	}

	void initVulkan()
	{
	}

	void mainLoop()
	{
		while (not glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		glfwDestroyWindow(m_window);
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

	GLFWwindow* m_window;
};

int main()
{
	HelloTriangleApplication app;

	app.run(WIDTH, HEIGHT);
}