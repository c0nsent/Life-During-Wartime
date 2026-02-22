#define VK_USE_PLATFORM_WAYLAND_KHR
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3native.h>

#include <array>
#include <cstdint>
#include <expected>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <stdfloat>
#include <unordered_set>

using i32 = std::int32_t;
using u32 = std::uint32_t;

constexpr i32 WIDTH = 800;
constexpr i32 HEIGHT = 600;


constexpr auto REQUIRED_VALIDATION_LAYERS{
	std::to_array<const char *>({"VK_LAYER_KHRONOS_validation"})
};

#ifdef NDEBUG
constexpr bool IS_VALIDATION_LAYERS_ENABLED{ false };
#else
constexpr bool IS_VALIDATION_LAYERS_ENABLED{ true };
#endif


class HelloTriangleApplication
{
	template <std::size_t Size>
	[[nodiscard]] auto checkValidationLayersSupport(const std::array<const char *, Size> requiredValidationLayerNames )
		-> std::optional<std::string_view>
	{
		if (Size == 0) return {};

		const auto availableLayersName{ std::invoke( [&]
		{
			const auto layerPropertiesResult{ m_context.enumerateInstanceLayerProperties()};
			std::unordered_set<std::string_view> layerNames;

			if (layerPropertiesResult.result != vk::Result::eSuccess) return layerNames;

			const auto &layerProperties{ layerPropertiesResult.value };

			layerNames.reserve(layerProperties.size());

			for (const auto &layerProp : layerProperties)
				layerNames.insert(layerProp.layerName);

			return layerNames;
		} )};

		if (availableLayersName.empty()) return "Failed to enumerate instance's layer properties";

		const bool isAllRequiredLayersAvailable{ std::ranges::all_of(requiredValidationLayerNames,
			[&](const std::string_view validationLayer)
			{
				return availableLayersName.contains(validationLayer);
			}
		)};

		return isAllRequiredLayersAvailable ? std::nullopt : std::optional{"Failed to find all required validation layers"};
	}


	static auto getRequiredExtensions() -> std::vector<const char *>
	{
		u32 glfwExtensionCount{};
		const auto glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

		std::vector<const char *> extensions{ glfwExtensions, glfwExtensions + glfwExtensionCount };

		if constexpr(IS_VALIDATION_LAYERS_ENABLED) extensions.emplace_back(vk::EXTDebugUtilsExtensionName);

		return extensions;
	}

	auto checkExtensionSupport(const std::vector<const char *> &requiredExtensions) -> std::optional<std::string_view>
	{
		if (requiredExtensions.empty()) return "No required extension provided";

		const auto availableExtensionNames{ std::invoke([&]
		{
			const auto extensionPropertiesResult{ m_context.enumerateInstanceExtensionProperties() };
			std::unordered_set<std::string_view> extensionNames;

			if (extensionPropertiesResult.result != vk::Result::eSuccess) return extensionNames;

			const auto &extensionProperties{ extensionPropertiesResult.value };
			extensionNames.reserve(extensionProperties.size());

			for (const auto &[extensionName, specVersion] : extensionProperties)
				extensionNames.insert(extensionName);

			return extensionNames;
		})};

		if (availableExtensionNames.empty()) return "No extensions available";

		const bool isAllRequiredExtensionsAvailable{std::ranges::all_of(requiredExtensions, [&](const auto requiredExtension)
		{
			return availableExtensionNames.contains(requiredExtension);
		})};

		return isAllRequiredExtensionsAvailable ? std::nullopt : std::optional{"Not all required extensions available"};
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
		const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
		const vk::DebugUtilsMessageTypeFlagsEXT type,
		const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
	{
		std::cerr << "Validation layer: type" << to_string(type) << " mgs: " << pCallbackData->pMessage << std::endl;

		return vk::False;
	}

	[[nodiscard]] auto createDebugMessenger() const noexcept -> std::optional<vk::raii::DebugUtilsMessengerEXT>
	{
		if constexpr (not IS_VALIDATION_LAYERS_ENABLED) return std::nullopt;

		using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
		using enum vk::DebugUtilsMessageTypeFlagBitsEXT;

		constexpr vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
			.messageSeverity = eVerbose | eWarning | eError,
			.messageType = eGeneral | ePerformance | eValidation,
			.pfnUserCallback = &debugCallback
		};

		auto result{m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT)};

		return result.result == vk::Result::eSuccess ? std::optional{std::move(result.value)} : std::nullopt;
	}


	auto createInstance() -> std::expected<vk::raii::Instance, std::string_view>
	{
		constexpr vk::ApplicationInfo appInfo{
			.pApplicationName = "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "Life During Wartime",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = vk::ApiVersion14
		};

		if constexpr(IS_VALIDATION_LAYERS_ENABLED)
		{
			const auto isSupported{checkValidationLayersSupport(REQUIRED_VALIDATION_LAYERS)};

			if ( not isSupported)
				return std::unexpected{isSupported.value()};
		}

		const auto requiredExtensions{ getRequiredExtensions() };
		if (requiredExtensions.empty())
			return std::unexpected{"Required extensions not found"};

		if (const auto isSupported{checkExtensionSupport(requiredExtensions)}; not isSupported)
			return std::unexpected{isSupported.value()};

		const vk::InstanceCreateInfo createInfo {
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = REQUIRED_VALIDATION_LAYERS.size(),
			.ppEnabledLayerNames = REQUIRED_VALIDATION_LAYERS.data(),
			.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data(),
		};

		auto instanceResult{ m_context.createInstance(createInfo) };

		if (instanceResult.result == vk::Result::eSuccess) [[likely]]
			return std::move(instanceResult.value);

		return std::unexpected{"Failed to create an instance"};
	}

	static auto pickPhysicalDevice(const vk::raii::Instance &instance)
		-> std::expected<vk::raii::PhysicalDevice, std::string_view>
	{
		const auto devices{std::invoke([&instance]
		{
			auto physicalDevicesResult{instance.enumeratePhysicalDevices()};
			if (physicalDevicesResult.result == vk::Result::eSuccess) [[likely]]
				return std::move(physicalDevicesResult.value);

			return std::vector<vk::raii::PhysicalDevice>{};
		} )};

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

	static auto createLogicalDeviceInfo(const vk::raii::PhysicalDevice &physDev, const u32 graphicsIndex)
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


	static auto createWindow(const i32 width, const i32 height, const std::string_view title) noexcept
		-> std::expected<GLFWwindow *, std::string_view>
	{
		if (not glfwInit()) return std::unexpected{"Failed to initialize GLFW"};

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		const auto window{
			glfwCreateWindow(width, height, title.data(), nullptr, nullptr)
		};

		if (window == nullptr) return std::unexpected{"Failed to create GLFW window"};

		return window;;
	}

	auto initVulkan() -> std::optional<std::string_view>
	{

		if (auto instanceExpected{createInstance()}; instanceExpected.has_value()) [[likely]]
			m_instance = std::move(instanceExpected.value());
		else [[unlikely]]
			return instanceExpected.error();

		if (auto DebugMessengerOpt{createDebugMessenger()}; DebugMessengerOpt.has_value()) [[likely]]
			m_debugMessenger = std::move(DebugMessengerOpt.value());
		else [[unlikely]]
			return "Failed to create Vulkan debug messenger";




		if (auto pickedDevice{pickPhysicalDevice(m_instance)})
			m_physicalDevice = std::move(pickedDevice.value());
		else
			return pickedDevice.error();

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
			{
				auto deviceResult{m_physicalDevice.createDevice(devCreateInfo.value())};
				if (deviceResult.result == vk::Result::eSuccess)
					m_device = std::move(deviceResult.value);
				else
					return "Failed to create logical device";
			}
		else
			return "Cannot create logical device initialization struct";


		if (auto queueResult{vk::Queue};)
		m_graphicsQueue = vk::raii::Queue{m_device, graphicsIndex, 0};


		constexpr vk::WaylandSurfaceCreateInfoKHR surfaceCreateInfo {
			.sType = vk::StructureType::eWaylandSurfaceCreateInfoKHR,
		};

		m_surface = m_instance.createWaylandSurfaceKHR(surfaceCreateInfo);
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
		m_window = initWindow(width, height);
		initVulkan();
		mainLoop();
		cleanup();
	}

private:

	GLFWwindow *m_window;

	vk::raii::Context m_context;
	vk::raii::Instance m_instance{nullptr}; //Дефолтный конструктор удален

	vk::raii::DebugUtilsMessengerEXT m_debugMessenger{nullptr};
	vk::raii::SurfaceKHR surface{nullptr};

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