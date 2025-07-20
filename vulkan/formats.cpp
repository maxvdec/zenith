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

VkFormat zen::toVulkanFormat(InputFormat format) {
    switch (format) {
    case InputFormat::Vector3:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case InputFormat::Vector2:
        return VK_FORMAT_R32G32_SFLOAT;
    case InputFormat::Vector4:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case InputFormat::Color:
        return VK_FORMAT_R32G32B32A32_SFLOAT; // Assuming color is represented as a vec4
    case InputFormat::Float:
        return VK_FORMAT_R32_SFLOAT;
    case InputFormat::Int:
        return VK_FORMAT_R32_SINT;
    case InputFormat::Uint:
        return VK_FORMAT_R32_UINT;
    case InputFormat::Bool:
        return VK_FORMAT_R8_UINT; // No direct boolean format, using unsigned byte
    case InputFormat::Mat3:
        return VK_FORMAT_R32G32B32_SFLOAT; // No direct mat3 format, using vec3
    case InputFormat::Mat4:
        return VK_FORMAT_R32G32B32A32_SFLOAT; // No direct mat4 format, using vec4
    default:
        throw std::runtime_error("Unsupported input format");
    }
}


#endif
