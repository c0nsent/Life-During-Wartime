#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <expected>
#include <functional>
#include <iostream>
#include <optional>
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


class HelloTriangleApplication
{
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
		std::cerr << "Validation layer: type" << to_string(type) << " mgs: " << pCallbackData->pMessage << std::endl;

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
		const auto devices{instance.enumeratePhysicalDevices()};

		if (devices.empty()) return std::unexpected{"Failed to find GPUs"};

		vk::raii::PhysicalDevice iGpu{nullptr};

		for (const auto &device : devices)
		{
			const auto &deviceProps{ device.getProperties() };

			if (not (device.getFeatures().geometryShader and deviceProps.apiVersion >= VK_API_VERSION_1_3) ) continue;

			using vk::PhysicalDeviceType::eDiscreteGpu;
			if (deviceProps.deviceType == eDiscreteGpu) return device;

			iGpu = device;
		}

		return iGpu;
	}

	auto createLogicalDeviceInfo(const vk::raii::PhysicalDevice &physDev, const u32 graphicsIndex)
		-> std::optional<vk::DeviceCreateInfo>
	{
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

		return devCreateInfo;
	}

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
		createInstance();
		setupDebugMessenger();

		if (const auto pickedDevice{pickPhysicalDevice(m_instance)})
			m_physicalDevice = pickedDevice.value();
		else
			throw std::runtime_error{pickedDevice.error().data()};

		const u32 graphicsIndex{std::invoke([qfp = m_physicalDevice.getQueueFamilyProperties()]
		{
			u32 index{static_cast<u32>(qfp.size())};

			for (u32 i{0}; i < qfp.size(); ++i)
			{
				using vk::QueueFlagBits::eGraphics;
				if ((qfp[i].queueFlags & eGraphics) == eGraphics) index = i;
			}

			if (index == qfp.size()) throw std::runtime_error{"Cannot find a graphics in device queue"};

			return index;
		})};


		if (const auto devCreateInfo{createLogicalDeviceInfo(m_physicalDevice, graphicsIndex)})
			m_device = vk::raii::Device{m_physicalDevice, devCreateInfo.value()};
		else
			throw std::runtime_error{"Can't create logical device"};

		m_graphicsQueue = vk::raii::Queue{m_device, graphicsIndex, 0};
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