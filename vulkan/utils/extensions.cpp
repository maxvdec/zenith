/*
* validationLayer.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: Validation layer utilities to speed up development.
* Copyright (c) 2025 Max Van den Eynde
*/

#ifdef ZENITH_VULKAN

#include <zenith/zenith.h>
#include <vulkan/vulkan.hpp>

using namespace zen;

bool CoreValidationLayer::exists() const {
    // We first check if the layer name is empty
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // If the layer name is empty, we return false
    for (const auto &layer: availableLayers) {
        if (name == layer.layerName) {
            return true;
        }
    }
    return false;
}

void CoreValidationLayer::enable(VkInstanceCreateInfo *createInfo) const {
    if (!exists()) {
        throw std::runtime_error("Validation layer " + name + " does not exist");
    }

    createInfo->enabledLayerCount++;
    // We sort of 'append' the layer name to the enabled layers
    const char *const *enabledLayers = createInfo->ppEnabledLayerNames;
    std::vector<const char *> newEnabledLayers(enabledLayers, enabledLayers + createInfo->enabledLayerCount);
    newEnabledLayers.push_back(name.c_str());
    // And then we set the new enabled layers
    createInfo->ppEnabledLayerNames = newEnabledLayers.data();
}

std::vector<CoreVulkanExtension> CoreVulkanExtension::getDeviceExtensions() {
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    std::vector<CoreVulkanExtension> extensions;
    extensions.reserve(extensionCount);
    for (const auto &ext: availableExtensions) {
        CoreVulkanExtension coreExt;
        coreExt.name = ext.extensionName;
        extensions.push_back(coreExt);
    }
    return extensions;
}

std::vector<CoreValidationLayer> CoreValidationLayer::getDeviceLayers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::vector<CoreValidationLayer> layers;
    layers.reserve(layerCount);
    for (const auto &layer: availableLayers) {
        CoreValidationLayer coreLayer;
        coreLayer.name = layer.layerName;
        layers.push_back(coreLayer);
    }
    return layers;
}

bool zen::isMoltenVkAvailable() {
#ifndef __APPLE__
    return true; // We assume MoltenVK is available on non-Apple platforms
#endif

    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    if (layerCount > 0) {
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        for (const auto &layer: layers) {
            if (std::string(layer.layerName).find("MoltenVK") != std::string::npos) {
                return true;
            }
        }
    }

    // Also check for MoltenVK via extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    if (extensionCount > 0) {
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        for (const auto &ext: extensions) {
            if (std::string(ext.extensionName).find("VK_MVK") != std::string::npos) {
                return true;
            }
        }
    }

    return false;
}

bool CoreVulkanExtension::exists() const {
    if (name.empty()) {
        return false; // If the name is empty, we return false
    }

    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    for (const auto &ext: availableExtensions) {
        if (name == ext.extensionName) {
            return true;
        }
    }
    return false;
}

#endif

