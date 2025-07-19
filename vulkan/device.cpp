/*
* device.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: 
* Copyright (c) 2025 Max Van den Eynde
*/

#ifdef ZENITH_VULKAN

#include <zenith/zenith.h>
#include <vulkan/vulkan.hpp>
#include <set>
#include <string>
#include <vector>

using namespace zen;

DevicePicker DevicePicker::makeDefaultPicker() {
    // We make a default device picker that sorts devices by their score
    return DevicePicker([](const Device &device) -> float {
        if (!device.supportsExtensions()) {
            return 0.0f;
        }

        if (!device.supportsSwapchain()) {
            return 0.0f;
        }

        if (!device.hasRequiredQueues()) {
            return 0.0f;
        }

        float score = 0.0f;

        if (device.physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000.0f; // Discrete GPUs are preferred
        } else if (device.physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            score += 500.0f; // Integrated GPUs are still good
        } else {
            score += 100.0f; // Other types of devices
        }

        score += (float) device.physicalDeviceProperties.limits.maxImageDimension2D; // Higher resolution is better

        if (device.physicalDeviceFeatures.samplerAnisotropy) {
            score += 200.0f; // Anisotropic filtering support is a plus
        }

        if (device.supportsRaytracing()) {
            score += 500.0f; // Ray tracing support is a big plus
        }

        if (device.physicalDeviceFeatures.geometryShader) {
            score += 300.0f; // Geometry shaders are a plus
        }

        return score;
    });
}

std::unique_ptr<Device> Device::makeDefaultDevice(Instance instance) {
    // We create a default device with the default picker
    auto device = std::make_unique<Device>(Device(instance, DevicePicker::makeDefaultPicker()));
    device->init();
    return device;
}

void Device::init() {
    // First, we need to enumerate the physical devices available
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan-compatible devices found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    devices.reserve(deviceCount);
    vkEnumeratePhysicalDevices(instance.instance, &deviceCount, devices.data());

    std::optional<std::pair<VkPhysicalDevice, float>> bestDevice = std::nullopt;
    for (const auto &device: devices) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device, &properties);
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(device, &features);

        Device tempDevice{instance};
        tempDevice.physicalDevice = device;
        tempDevice.physicalDeviceProperties = properties;
        tempDevice.physicalDeviceFeatures = features;

        float score = picker.selector(tempDevice);
        if (!bestDevice || score > bestDevice->second) {
            bestDevice = {std::make_pair(device, score)};
        }
    }

    if (!bestDevice) {
        throw std::runtime_error("No suitable Vulkan device found");
    }

    physicalDevice = bestDevice->first;

    // Once initialized the physical device, we can retrieve its properties and features
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
}

bool Device::supportsExtensions(const std::vector<std::string> &requiredExtensions) const {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> available;
    for (const auto &ext: availableExtensions) {
        available.insert(ext.extensionName);
    }

    for (const auto &req: requiredExtensions) {
        if (available.find(req) == available.end()) {
            return false;
        }
    }
    return true;
}

bool Device::supportsSwapchain() const {
    // We check if the device supports the swapchain extension
    std::vector<std::string> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    return supportsExtensions(requiredExtensions);
}

bool Device::hasRequiredQueues() const {
    // We check if the device has the required queues for graphics, compute, and transfer
    VkQueueFamilyProperties queueFamilies[3];
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount < 3) {
        return false; // Not enough queue families
    }
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    bool hasGraphics = false;
    bool hasCompute = false;
    bool hasTransfer = false;

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            hasGraphics = true;
        }
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            hasCompute = true;
        }
        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            hasTransfer = true;
        }
    }

    return hasGraphics && hasCompute && hasTransfer;
}

bool Device::supportsRaytracing() const {
    // We check if the device supports ray tracing
    if (!zen::CoreVulkanExtension{"VK_KHR_ray_tracing_pipeline"}.exists()) {
        return false; // Ray tracing is not supported
    }

    // Check if the device supports the ray tracing pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures{};
    rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &rayTracingFeatures;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

    return rayTracingFeatures.rayTracingPipeline;
}


#endif