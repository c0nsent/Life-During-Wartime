#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <ranges>
#include <cstdint>
#include <stdexcept>


using i32 = std::int32_t;
using u32 = std::uint32_t;


constexpr i32 WIDTH = 800;
constexpr i32 HEIGHT = 600;

class HelloTriangleApplication
{
	void createInstance()
	{
		constexpr vk::ApplicationInfo appInfo{
			.pApplicationName = "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "Life During Wartime",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = vk::ApiVersion14
		};

		u32 glfwExtensionCount{};
		const auto glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

		const auto extensionProperties{ context.enumerateInstanceExtensionProperties() };

		for (u32 i{0}; i < glfwExtensionCount; ++i)
		{
			if (std::ranges::none_of(extensionProperties,
				[glfwExtension{glfwExtensions[i]}](auto const& extensionProperty)
				{
					return std::strcmp(extensionProperty.extensionName, glfwExtension) == 0;
				}))
				throw std::runtime_error{"Required GLFW extension not supported: " + std::string(glfwExtensions[i])};
		}

		const vk::InstanceCreateInfo createInfo {
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = glfwExtensionCount,
			.ppEnabledExtensionNames = glfwExtensions
		};

		instance = vk::raii::Instance(context, createInfo);

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
	vk::raii::Instance instance{nullptr}; //Дефолтный конструктор удален
};

int main()
{
	try
	{
		HelloTriangleApplication app;
		app.run(WIDTH, HEIGHT);
	}
	catch (const std::runtime_error& e)
	{
		throw std::runtime_error{std::string{e.what()}};
	}

}