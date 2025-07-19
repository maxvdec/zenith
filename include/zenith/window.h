/*
* window.h
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: Window utilities to speed up development.
* Copyright (c) 2025 Max Van den Eynde
*/

#ifndef ZENITH_WINDOW_H
#define ZENITH_WINDOW_H

#include <memory>
#include <string>
#include <array>
#include <vector>
#include <optional>

#ifndef ZENITH_EXT_WINDOWING
#error "To use this header, you must enable the Zenith Windowing extension when compiling or downloading the toolkit."
#endif

#ifdef ZENITH_GLFW

#ifdef ZENITH_VULKAN

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

namespace zen::glfw {
    struct VkInitializerConfiguration {
        std::string name;
        int width;
        int height;
        std::string engineName = "Zenith";
        bool resizable = true;
        bool fullscreen = false;
        int monitorIndex = 0;
        const int vulkanVersion = VK_API_VERSION_1_0;
        std::vector<std::string> extraExtensions = {};
        bool enableValidationLayers = false;
        std::array<int, 3> applicationVersion = {1, 0, 0};
        std::array<int, 3> engineVersion = {1, 0, 0};
        bool enableDebugMessenger = false;

        void ensureIntegrity() const;
    };

    class VkInitializer {
    public:
        std::optional<GLFWwindow *> window = std::nullopt;
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        const VkInitializerConfiguration &config;

        explicit VkInitializer(const VkInitializerConfiguration &config) : config(config) {}

        void initialize();

        [[nodiscard]] bool shouldClose() const {
            return glfwWindowShouldClose(*window);
        }

    private:
        void initializeVulkan();

        void initializeDebugMessenger() const;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                VkDebugUtilsMessageTypeFlagsEXT type,
                const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                void *userData);
    };
}

#endif

namespace zen::glfw {
    class Window {
    public:
#ifdef ZENITH_VULKAN
        VkInitializer vulkan;

        explicit Window(const VkInitializerConfiguration &config) : vulkan(config) {
            vulkan.initialize();
        }

        inline void init() {
            vulkan.initialize();
        }

        [[nodiscard]] inline bool shouldClose() const {
            return vulkan.shouldClose();
        }

#endif
    };

#ifdef ZENITH_VULKAN
    typedef zen::glfw::VkInitializerConfiguration WindowConfiguration;
#endif
}

#endif

#endif //ZENITH_WINDOW_H
