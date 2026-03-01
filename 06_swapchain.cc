#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <expected>
#include <functional>
#include <iostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <unordered_set>


using i32 = std::int32_t;
using u32 = std::uint32_t;
using f32 = float;

constexpr i32 WIDTH = 800;
constexpr i32 HEIGHT = 600;

#ifdef NDEBUG
constexpr bool isValidationLayersEnabled{ false };
#else
constexpr bool isValidationLayersEnabled{ true };
#endif

//Сишная залупа
void errorCallback(const i32 error, const char *description)
{
	std::cout << "Error " << error << ": " << description << std::endl;
}


class LifeDuringWartime
{
	template <typename T>
	[[nodiscard]] static auto unwrap(std::expected<T, const char *> &&expected) -> T
	{
		if (expected.has_value()) return std::move(expected.value());

		throw std::runtime_error{expected.error()};
	}


	template<class VecType, class Func>
	[[nodiscard]] static auto toUnorderedSet(std::vector<VecType> &&vec,  Func &&pred) -> std::unordered_set<std::string>
	{
		std::unordered_set<std::string> set;
		set.reserve(vec.size());

		for (auto &element : vec)
			set.emplace(pred(std::move(element)));

		return set;
	}


	template<std::size_t Size>
	[[nodiscard]] bool initValidationLayers(const std::array<const char *, Size> &requiredValidationLayers) const
	{
		std::vector properties{m_context.enumerateInstanceLayerProperties()};

		const auto availableLayersName{toUnorderedSet( std::move(properties),
			[](vk::LayerProperties &&layerProps)
			{
				return layerProps.layerName.data();
			}
		)};

		return std::ranges::all_of(requiredValidationLayers, [&](const char *validationLayer)
		{
			return availableLayersName.contains(validationLayer);
		});
	}


	static auto getRequiredExtensions() -> std::vector<const char *>
	{
		u32 glfwExtensionCount{};
		const char **glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

		std::vector<const char *> extensions{ glfwExtensions, glfwExtensions + glfwExtensionCount };

		if constexpr(isValidationLayersEnabled) extensions.emplace_back(vk::EXTDebugUtilsExtensionName);

		return extensions;
	}


	[[nodiscard]] auto isRequiredExtensionSupported(const std::vector<const char *> &requiredExtensions) const -> bool
	{
		std::vector properties{m_context.enumerateInstanceExtensionProperties()};

		const auto availableExtensionNames{toUnorderedSet(std::move(properties),
			[] (vk::ExtensionProperties &&property)
			{
			return property.extensionName.data();
			}
		)};

		return std::ranges::all_of(requiredExtensions, [availableExtensionNames](const auto requiredExtension)
		{
			return availableExtensionNames.contains(requiredExtension);
		});
	}


	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
		const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
		const vk::DebugUtilsMessageTypeFlagsEXT type,
		const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
	{
		std::cerr << "Validation layer: type" << vk::to_string(type) << " mgs: " << pCallbackData->pMessage << std::endl;

		return vk::False;
	}


	[[nodiscard]] auto setupDebugMessenger() const -> vk::raii::DebugUtilsMessengerEXT
	{
		if constexpr (not isValidationLayersEnabled) return {nullptr};

		using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
		using enum vk::DebugUtilsMessageTypeFlagBitsEXT;

		constexpr vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
			.messageSeverity = eVerbose | eWarning | eError,
			.messageType = eGeneral | ePerformance | eValidation,
			.pfnUserCallback = &debugCallback
		};

		return m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	[[nodiscard]] auto createInstance() const -> std::expected<vk::raii::Instance, const char *>
	{
		constexpr auto requiredValidationLayers{
			std::to_array<const char *>({"VK_LAYER_KHRONOS_validation"})
			};

		if constexpr(isValidationLayersEnabled)
		{
			if (not initValidationLayers(requiredValidationLayers))
				return std::unexpected{"Failed to initialize validation layers"};
		}

		const vk::ApplicationInfo appInfo{
			.pApplicationName = m_applicationName.data(),
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = m_applicationName.data(),
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = vk::ApiVersion14
		};

		const std::vector<const char*> requiredExtensions{getRequiredExtensions()};
		if (not isRequiredExtensionSupported(requiredExtensions))
			return std::unexpected{"Error: Required extension does not supported"};

		const vk::InstanceCreateInfo createInfo {
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = requiredValidationLayers.size(),
			.ppEnabledLayerNames = requiredValidationLayers.data(),
			.enabledExtensionCount = static_cast<u32>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data(),
		};

		return m_context.createInstance(createInfo);
	}

	[[nodiscard]] auto pickPhysicalDevice() const -> std::expected<vk::raii::PhysicalDevice, const char *>
	{
		std::vector physicalDevices{m_instance.enumeratePhysicalDevices()};

		if (physicalDevices.empty()) return std::unexpected{"Failed to find GPUs"};

		for (auto &device : physicalDevices)
		{
			const auto properties{device.getProperties()};
			if ( properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) continue;

			if (device.getFeatures().geometryShader and properties.apiVersion >= vk::ApiVersion14)
				return std::move(device);
		}

		return std::unexpected{"Failed to find a discrete GPU"};
	}

	[[nodiscard]] auto createLogicalDevice(const u32 queueIndex) const -> vk::raii::Device
	{
		constexpr f32 queuePriority{0.5f};

		vk::DeviceQueueCreateInfo devQueueCreateInfo{
			.queueFamilyIndex = queueIndex,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		};

		const vk::StructureChain featureChain{
			vk::PhysicalDeviceFeatures2{m_physicalDevice.getFeatures2()},
			vk::PhysicalDeviceVulkan13Features{.dynamicRendering = true},
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{.extendedDynamicState = true}
		};

		constexpr std::array deviceExtensions{ vk::KHRSwapchainExtensionName };

		const vk::DeviceCreateInfo devCreateInfo{
			.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &devQueueCreateInfo,
			.enabledExtensionCount = static_cast<u32>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
		};

		return m_physicalDevice.createDevice(devCreateInfo);
	}

	[[nodiscard]] auto findQueueIndex() const -> std::expected<u32, const char *>
	{
		const std::vector properties{m_physicalDevice.getQueueFamilyProperties()};

		for (u32 i{0}; i != static_cast<u32>(properties.size()); ++i)
		{
			using vk::QueueFlagBits::eGraphics;
			const bool isGraphics{(properties[i].queueFlags & eGraphics) == eGraphics};
			const bool hasSurfaceSupport{m_physicalDevice.getSurfaceSupportKHR(i, m_surface) == vk::Bool32{true}};

			if (isGraphics and hasSurfaceSupport)
				return i;
		}

		return std::unexpected{"Cannot find a suitable queue index"};
	}


	[[nodiscard]] auto createSurface() const -> std::expected<vk::raii::SurfaceKHR, const char *>
	{
		VkSurfaceKHR surface;
		const vk::Result result{glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface)};

		if (result == vk::Result::eSuccess)
			return vk::raii::SurfaceKHR{m_instance, surface};

		return std::unexpected{"Can't create surface"};
	}

	auto createSwapChain() const -> std::expected<vk::raii::SwapchainKHR, const char *>
	{
		const auto surfaceCapabilities{m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface)};

		u32 minImageCount{std::max(3u, surfaceCapabilities.minImageCount)};

		if (surfaceCapabilities.maxImageCount > 0 and minImageCount > surfaceCapabilities.maxImageCount)
			minImageCount = surfaceCapabilities.maxImageCount;
		else
			minImageCount = surfaceCapabilities.minImageCount;


	}

	[[nodiscard]] auto chooseSwapSurfaceFormat() const -> vk::SurfaceFormatKHR
	{
		const std::vector availableFormats{m_physicalDevice.getSurfaceFormatsKHR(m_surface)};

		for (const auto &format : availableFormats)
			if (format.format == vk::Format::eB8G8R8A8Srgb and format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				return format;

		return availableFormats.front();
	}


	[[nodiscard]] auto chooseSwapPresentationMode() const -> std::expected<vk::PresentModeKHR, const char *>
	{
		using expect = std::expected<vk::PresentModeKHR, const char *>;
		using error = std::unexpected<const char *>;
		using enum vk::PresentModeKHR;

		const std::vector modesVec{m_physicalDevice.getSurfacePresentModesKHR()};
		const std::unordered_set<vk::PresentModeKHR> availableModes{modesVec.begin(),modesVec.end()};


		if (availableModes.contains(eMailbox)) return eMailbox;

		return availableModes.contains(eFifo) ? expect{eFifo} : error{"Failed to choose presentatoin mode"};
	}


	[[nodiscard]] auto chooseSwapExtent() const -> vk::Extent2D
	{
		const auto capabilities{m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface)};

		if (capabilities.currentExtent.width != std::numeric_limits<u32>::max())
			return capabilities.currentExtent;

		i32 width, height;
		glfwGetFramebufferSize(m_window, &width, &height);

		const vk::Extent2D min{capabilities.minImageExtent};
		const vk::Extent2D max{capabilities.maxImageExtent};

		return {
			std::clamp<u32>(width, min.width, max.width),
			std::clamp<u32>(height, min.height, max.height)
		};
	}



	[[nodiscard]] auto createWindow(const i32 width, const i32 height) const noexcept
		-> std::expected<GLFWwindow *, const char *>
	{
		using expect = std::expected<GLFWwindow *, const char *>;
		using error = std::unexpected<const char *>;

		glfwSetErrorCallback(errorCallback);

		if (not glfwInit())
			return error{"Can't initialize GLFW"};

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		GLFWwindow *window{glfwCreateWindow(width, height, m_applicationName.data(), nullptr, nullptr)};
		return window != nullptr ? expect{window} : error{"Failed to create GLFW window"};
	}


	void initVulkan()
	{
		m_instance = unwrap(createInstance());
		m_debugMessenger = setupDebugMessenger();
		m_surface = unwrap(createSurface());
		m_physicalDevice = unwrap(pickPhysicalDevice());

		const u32 queueIndex{unwrap(findQueueIndex())};
		m_device = createLogicalDevice(queueIndex);
		m_deviceQueue = m_device.getQueue(queueIndex, 0);

		m_swapChainFormat = chooseSwapSurfaceFormat();
		m_swapChainExtent = chooseSwapExtent();



		createSwapChain();
	}

	void mainLoop() const
	{
		while (not glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		glfwTerminate();
	}

public:

	LifeDuringWartime(const char *appName) : m_applicationName{appName} {}

	void run(const i32 width, const i32 height)
	{
		m_window = unwrap(createWindow(width, height));
		initVulkan();
		mainLoop();
		cleanup();
	}
private:

	GLFWwindow *m_window{nullptr};

	vk::raii::Context m_context;
	vk::raii::Instance m_instance{nullptr}; //Дефолтный конструктор удален

	vk::raii::DebugUtilsMessengerEXT m_debugMessenger{nullptr};

	vk::raii::PhysicalDevice m_physicalDevice{nullptr};
	vk::raii::Device m_device{nullptr};
	vk::raii::Queue m_deviceQueue{nullptr};

	vk::raii::SurfaceKHR m_surface{nullptr};

	vk::SurfaceFormatKHR m_swapChainFormat;
	vk::Extent2D m_swapChainExtent;

	std::string m_applicationName;
};

int main()
{
	try
	{
		LifeDuringWartime app{"Life During Wartime"};
		app.run(WIDTH, HEIGHT);
	}
	catch (const std::runtime_error &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}