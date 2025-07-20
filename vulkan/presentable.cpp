/*
* presentable.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: 
* Copyright (c) 2025 Max Van den Eynde
*/

#ifdef ZENITH_VULKAN

#include <zenith/zenith.h>
#include <vulkan/vulkan.hpp>

using namespace zen;

Presentable::Presentable(zen::Device device, zen::Instance instance) : device(std::move(device)), instance(instance) {
    create();
}

Presentable::~Presentable() {
    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device.logicalDevice, swapchain, nullptr);
    }
    for (const auto &image: images) {
        if (image.view != VK_NULL_HANDLE) {
            vkDestroyImageView(device.logicalDevice, image.view, nullptr);
        }
        if (image.image != VK_NULL_HANDLE) {
            vkDestroyImage(device.logicalDevice, image.image, nullptr);
        }
    }
}

void Presentable::create() {
    // First, we need to get the surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicalDevice, instance.surface, &capabilities);

    // Then, we need to get the available surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicalDevice, instance.surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicalDevice, instance.surface, &formatCount,
                                         availableFormats.data());

    // Then, we need to get the available present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, instance.surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, instance.surface, &presentModeCount,
                                              availablePresentModes.data());

    // Finally, we choose the surface format, present mode, and extent
    auto surfaceFormat = chooseSurfaceFormat(availableFormats);
    auto presentMode = choosePresentMode(availablePresentModes);
    extent = chooseSwapExtent(capabilities, instance.extent);

    // We create the swapchain and get the images
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = instance.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device.logicalDevice, &createInfo, nullptr, &swapchain);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain. Error: " + zen::getVulkanErrorString(result));
    }

    format = surfaceFormat.format;

    // Now we get the swapchain images
    uint32_t count = 0;
    vkGetSwapchainImagesKHR(device.logicalDevice, swapchain, &count, nullptr);
    std::vector<VkImage> swapchainImages(count);
    vkGetSwapchainImagesKHR(device.logicalDevice, swapchain, &count, swapchainImages.data());

    // And we can create the views
    for (auto image: swapchainImages) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components = {};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView view;
        result = vkCreateImageView(device.logicalDevice, &viewInfo, nullptr, &view);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view. Error: " + zen::getVulkanErrorString(result));
        }
        images.push_back({image, view});
    }
}

VkSurfaceFormatKHR Presentable::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &f: availableFormats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f; // This is the most common format
        }
    }
    // If we don't find the preferred format, we return the first available format
    if (!availableFormats.empty()) {
        return availableFormats[0];
    } else {
        throw std::runtime_error("No available surface formats found.");
    }
}

VkPresentModeKHR Presentable::choosePresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &mode: availablePresentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode; // This is the preferred mode for low latency
        }
    }
    // If we don't find the preferred mode, we return FIFO which is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Presentable::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D windowExtent) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent; // The surface has a fixed extent
    } else {
        // The surface has a variable extent, we use the window extent
        VkExtent2D extent = windowExtent;
        extent.width = std::max(capabilities.minImageExtent.width,
                                std::min(capabilities.maxImageExtent.width, extent.width));
        extent.height = std::max(capabilities.minImageExtent.height,
                                 std::min(capabilities.maxImageExtent.height, extent.height));
        return extent;
    }
}

#endif