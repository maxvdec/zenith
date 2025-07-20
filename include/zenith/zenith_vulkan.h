/*
* zenith_vulkan.h
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: Main header for the Zenith Vulkan backend.
* Copyright (c) 2025 Max Van den Eynde
*/

#ifndef ZENITH_ZENITH_VULKAN_H
#define ZENITH_ZENITH_VULKAN_H

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <functional>

namespace zen {
    struct Instance {
        VkInstance instance;
        VkSurfaceKHR surface;
        VkExtent2D extent = {0, 0};

        Instance(VkInstance instance, VkSurfaceKHR surface, VkExtent2D extent) : instance(instance), surface(surface),
                                                                                 extent(extent) {};
    };

    class CoreValidationLayer {
    public:
        std::string name;

        [[nodiscard]] bool exists() const;

        void enable(VkInstanceCreateInfo *createInfo) const;

        static std::vector<CoreValidationLayer> getDeviceLayers();
    };

    class CoreVulkanExtension {
    public:
        std::string name;

        static std::vector<CoreVulkanExtension> getDeviceExtensions();

        [[nodiscard]] bool exists() const;
    };

    std::string getVulkanErrorString(VkResult error);

    bool isMoltenVkAvailable();

    class Device;

    using DeviceSelector = std::function<float(const Device &)>;

    class DevicePicker {
    public:
        DeviceSelector selector;

        static DevicePicker makeDefaultPicker();

        explicit DevicePicker(DeviceSelector selector) : selector(std::move(selector)) {};
    };

    enum class DeviceCapabilities {
        Graphics,
        Compute,
        Transfer,
        Present,
    };

    class CoreQueue {
    public:
        VkQueue queue = VK_NULL_HANDLE;
        uint32_t familyIndex = 0;
        std::vector<DeviceCapabilities> capabilities = {};
    };

    class Presentable;

    class Device {
    public:
        DevicePicker picker = DevicePicker::makeDefaultPicker();

        static std::unique_ptr<Device> makeDefaultDevice(Instance instance);

        explicit Device(Instance instance, DevicePicker picker = DevicePicker::makeDefaultPicker())
                : picker(std::move(picker)), instance(instance) {}

        [[nodiscard]] bool supportsExtensions(const std::vector<std::string> &requiredExtensions = {}) const;

        [[nodiscard]] bool supportsSwapchain() const;

        [[nodiscard]] bool hasRequiredQueues() const;

        [[nodiscard]] bool supportsRaytracing() const;

        void init();

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties physicalDeviceProperties{};
        VkPhysicalDeviceFeatures physicalDeviceFeatures{};
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};

        VkDevice logicalDevice = VK_NULL_HANDLE;
        std::vector<CoreQueue> queues;

        std::vector<const char *> extensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        std::vector<CoreQueue> getQueueFromCapability(DeviceCapabilities capability);

        [[nodiscard]] Presentable makePresentable() const;

    private:
        Instance instance;

        void findQueueFamilies();

        void initializeLogicalDevice();
    };

    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    class Presentable {
    public:
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        std::vector<Image> images = {};
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent = {0, 0};

        Presentable(Device device, Instance instance);

        ~Presentable();

    private:
        Device device;
        Instance instance;

        void create();

        static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D windowExtent);
    };
};

#endif //ZENITH_ZENITH_VULKAN_H
