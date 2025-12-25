#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>


constexpr uint32_t c_width{800};
constexpr uint32_t c_height{600};

static void glfwErrorCallback(const int error, const char *description)
{
	std::cerr << error << " GLFW Error: " << description << std::endl;
}

class LbgtTriangle
{
	void initWindow()
	{
		glfwSetErrorCallback(glfwErrorCallback);

		if (not glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

		m_window = glfwCreateWindow(c_width, c_height, "Sobaka", nullptr, nullptr);

		glfwShowWindow(m_window);

		if(m_window == nullptr) throw std::runtime_error("Failed to create GLFW window");
	}

	void initVulkan()
	{
	}

	void mainLoop()
	{
		while (not glfwWindowShouldClose(m_window))
			glfwPollEvents();
	}

	void cleanup()
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow *m_window{nullptr};
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