/*
* zenith.h
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: The main header file for the Zenith graphics engine.
* Copyright (c) 2025 Max Van den Eynde
*/

#ifndef ZENITH_ZENITH_H
#define ZENITH_ZENITH_H

#ifdef ZENITH_VULKAN

#include <vulkan/vulkan.hpp>

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
    };

    std::string getVulkanErrorString(VkResult error);

    bool isMoltenVkAvailable();
};

#endif

#endif //ZENITH_ZENITH_H
