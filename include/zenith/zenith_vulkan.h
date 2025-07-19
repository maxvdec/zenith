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

#define DEVICE_SCORE_FUNCTION(name) \
    float name(const VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface)

    DEVICE_SCORE_FUNCTION(defaultDeviceScoreFunction);

    class Device {
    public:
        static Device makeDefaultDevice();

        Device() = default;

    private:
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties physicalDeviceProperties{};
        VkPhysicalDeviceFeatures physicalDeviceFeatures{};
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
    };
};

#endif //ZENITH_ZENITH_VULKAN_H
