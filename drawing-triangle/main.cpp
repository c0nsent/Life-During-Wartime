#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <print>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

constexpr uint32_t c_width{800};
constexpr uint32_t c_height{600};

class LbgtTriangle
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(c_width, c_height, "Sobaka", nullptr, nullptr);
	}

	void initVulkan()
	{
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_window)) glfwPollEvents();
	}

	void cleanup()
	{
		glfwDestroyWindow(m_window);

		glfwTerminate();
	}

	GLFWwindow* m_window;
};

int main()
{
	LbgtTriangle triangle;

	try
	{
		triangle.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}