#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan_raii.hpp>
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <GLFW/glfw3native.h>

#include <array>
#include <cstdint>
#include <expected>
#include <functional>
#include <iostream>
#include <optional>
#include <print>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <unordered_set>


using i32 = std::int32_t;
using u32 = std::uint32_t;

constexpr i32 WIDTH = 800;
constexpr i32 HEIGHT = 600;


constexpr auto validationLayers{
	std::to_array<const char *>({"VK_LAYER_KHRONOS_validation"})
};

#ifdef NDEBUG
constexpr bool isValidationLayersEnabled{ false };
#else
constexpr bool isValidationLayersEnabled{ true };
#endif

void errorCallback(const i32 error, const char *description)
{
	std::cout << "Error " << error << ": " << description << std::endl;
}

class HelloTriangleApplication
{
	template <typename T>
	[[nodiscard]] static auto unwrap(const std::expected<T, std::string_view> &expected) -> T
	{
		if (expected.has_value()) return expected.value();

		throw std::runtime_error{expected.error().data()};
	}

	[[nodiscard]] bool initValidationLayers() const
	{
		const auto availableLayersName{ std::invoke( [&]
			{
				const auto availableLayerProps{ m_context.enumerateInstanceLayerProperties() };

				std::unordered_set<std::string_view> result;
				result.reserve(availableLayerProps.size());

				for (const auto &layerProp : availableLayerProps) result.insert(layerProp.layerName);

				return result;
			}
		)};

		const bool isAllRequiredLayersAvailable{ std::ranges::all_of(validationLayers,
			[&](const std::string_view validationLayer)
			{
				return availableLayersName.contains(validationLayer);
			}
		)};

		return isAllRequiredLayersAvailable;
	}


	static auto getRequiredExtensions() -> std::vector<const char *>
	{
		u32 glfwExtensionCount{};
		const auto glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

		std::vector<const char *> extensions{ glfwExtensions, glfwExtensions + glfwExtensionCount };

		if constexpr(isValidationLayersEnabled) extensions.emplace_back(vk::EXTDebugUtilsExtensionName);

		return extensions;
	}

	std::optional<std::vector<const char *>> checkIfRequiredExtensionSupported(const std::vector<const char *> &requiredExtensions)
	{
		const auto availableExtensions{ std::invoke([&]
		{
			const auto extensionPropertiesVec{ m_context.enumerateInstanceExtensionProperties() };

			std::unordered_set<std::string_view> result;
			result.reserve(requiredExtensions.size());

			for (const auto &[extensionName, specVersion] : extensionPropertiesVec)
				result.insert(extensionName);

			return result;
		})};

		std::vector<const char *> unsupportedExtensions;
		unsupportedExtensions.reserve(requiredExtensions.size());

		std::ranges::all_of(requiredExtensions, [&](const auto requiredExtension)
		{
			if (availableExtensions.contains(requiredExtension)) return true;

			unsupportedExtensions.emplace_back(requiredExtension);
			return false;
		});

		return unsupportedExtensions.empty() ? std::nullopt : std::optional{std::move(unsupportedExtensions)};
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
		const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
		const vk::DebugUtilsMessageTypeFlagsEXT type,
		const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
	{
		std::cerr << "Validation layer: type" << vk::to_string(type) << " mgs: " << pCallbackData->pMessage << std::endl;

		return vk::False;
	}

	void setupDebugMessenger()
	{
		if constexpr (not isValidationLayersEnabled) return;

		using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
		using enum vk::DebugUtilsMessageTypeFlagBitsEXT;

		constexpr vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
			.messageSeverity = eVerbose | eWarning | eError,
			.messageType = eGeneral | ePerformance | eValidation,
			.pfnUserCallback = &debugCallback
		};

		m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	void createInstance()
	{
		constexpr vk::ApplicationInfo appInfo{
			.pApplicationName = "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "Life During Wartime",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = vk::ApiVersion14
		};

		if constexpr(isValidationLayersEnabled)
		{
			if (not initValidationLayers())
				throw std::runtime_error{"Failed to initialize validation layers"};
		}

		const auto requiredExtensions{ getRequiredExtensions() };

		if (checkIfRequiredExtensionSupported(requiredExtensions).has_value())
		{
			std::ostringstream ss;

			ss << "Error: required extension are not supported:  \"" << requiredExtensions.data() << "\" " << std::endl;

			throw std::runtime_error{ss.str()};
		}

		const vk::InstanceCreateInfo createInfo {
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = validationLayers.size(),
			.ppEnabledLayerNames = validationLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data(),
		};

		m_instance = vk::raii::Instance{m_context, createInfo};
	}

	static auto pickPhysicalDevice(const vk::raii::Instance &instance)
		-> std::expected<vk::raii::PhysicalDevice, std::string_view>
	{
		std::vector<vk::raii::PhysicalDevice> physDevices{instance.enumeratePhysicalDevices()};

		if (physDevices.empty()) return std::unexpected{"Failed to find GPUs"};

		vk::raii::PhysicalDevice iGpu{nullptr};

		for (auto &device : physDevices)
		{
			const auto &deviceProps{ device.getProperties() };

			if (not (device.getFeatures().geometryShader and deviceProps.apiVersion >= VK_API_VERSION_1_3) ) continue;

			using vk::PhysicalDeviceType::eDiscreteGpu;
			if (deviceProps.deviceType == eDiscreteGpu) return device;

			iGpu = std::move(device);
		}

		return iGpu;
	}

	auto createLogicalDevice(const u32 graphicsIndex) -> vk::raii::Device
	{
		const auto presentIndex{m_physicalDevice.getSurfaceSupportKHR(graphicsIndex, m_surface)
			? graphicsIndex
			: static_cast<u32>(m_physicalDevice.getQueueFamilyProperties().size())};


		constexpr auto queuePriority{0.5f};

		vk::DeviceQueueCreateInfo devQueueCreateInfo{
			.queueFamilyIndex = graphicsIndex,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		};

		vk::PhysicalDeviceFeatures physDevFeatures;

		const vk::StructureChain featureChain{
			vk::PhysicalDeviceFeatures2{},
			vk::PhysicalDeviceVulkan13Features{.dynamicRendering = true},
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{.extendedDynamicState = true}
		};

		constexpr std::array deviceExtensions {
			vk::KHRSwapchainExtensionName
		};

		const vk::DeviceCreateInfo devCreateInfo{
			.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &devQueueCreateInfo,
			.enabledExtensionCount = static_cast<u32>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
		};

		return vk::raii::Device{m_physicalDevice, devCreateInfo};
	}

	static auto findGraphicsIndex(const vk::raii::PhysicalDevice &physDev) -> u32
	{
		const std::vector<vk::QueueFamilyProperties> queueFamilyProperties{physDev.getQueueFamilyProperties()};

		u32 graphicsIndex{static_cast<u32>(queueFamilyProperties.size())};

		for (u32 i{0}; i < queueFamilyProperties.size(); ++i)
		{
			using vk::QueueFlagBits::eGraphics;
			if ((queueFamilyProperties[i].queueFlags & eGraphics) == eGraphics)
				graphicsIndex = i;
		}

		if (graphicsIndex == queueFamilyProperties.size())
			throw std::runtime_error{"Can't find graphics index"};

		return graphicsIndex;
	}


	static auto createSurface(const vk::raii::Instance &instance, GLFWwindow *window) -> vk::raii::SurfaceKHR
	{
		VkSurfaceKHR surface;
		if (glfwCreateWindowSurface(*instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error{"Can't create surface"};
		}

		return {instance, surface};
	}


	void initWindow(const i32 width, const i32 height)
	{
		glfwSetErrorCallback(errorCallback);

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
		createInstance();
		setupDebugMessenger();
		m_surface = createSurface(m_instance, m_window);
		m_physicalDevice = unwrap(pickPhysicalDevice(m_instance));

		const u32 graphicsIndex{findGraphicsIndex(m_physicalDevice)};

		m_device = createLogicalDevice(m_physicalDevice, graphicsIndex);
		m_graphicsQueue = m_device.getQueue(graphicsIndex, 0);

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

	GLFWwindow *m_window;

	vk::raii::Context m_context;
	vk::raii::Instance m_instance{nullptr}; //Дефолтный конструктор удален

	vk::raii::DebugUtilsMessengerEXT m_debugMessenger{nullptr};

	vk::raii::PhysicalDevice m_physicalDevice{nullptr};
	vk::raii::Device m_device{nullptr};
	vk::raii::Queue m_graphicsQueue{nullptr};

	vk::raii::SurfaceKHR m_surface{nullptr};
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
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}