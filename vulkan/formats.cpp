/*
* formats.cpp
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

bool Format::isSupportedColorAttachment(const zen::Device& device) const {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &props);

    return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0;
}

bool Format::isSupportedDepthAttachment(const zen::Device& device) const {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &props);

    return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
}


#endif
