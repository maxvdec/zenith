/*
* glfw.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: GLFW windowing utilities to speed up development.
* Copyright (c) 2025 Max Van den Eynde
*/

#include <exception>
#include <iostream>

#ifdef ZENITH_EXT_WINDOWING
#ifdef ZENITH_GLFW

#include <zenith/window.h>

using namespace zen::glfw;

#ifdef ZENITH_VULKAN

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <zenith/zenith.h>

void VkInitializerConfiguration::ensureIntegrity() const {
    // We basically ensure that all the required fields are set
    if (name.empty()) {
        throw std::runtime_error("Window name must be set");
    }

    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Window width and height must be greater than 0");
    }
}

void VkInitializer::initialize() {
    // We first ensure the configuration is valid
    config.ensureIntegrity();

    // First, we need to initialize GLFW
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    // Disable the OpenGL context creation for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // We set other window hints based on the configuration
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
    if (monitorCount == 0) {
        glfwTerminate();
        throw std::runtime_error("No monitors found");
    }
    if (config.monitorIndex < 0 || config.monitorIndex >= monitorCount) {
        glfwTerminate();
        throw std::runtime_error("Invalid monitor index: " + std::to_string(config.monitorIndex));
    }
    GLFWmonitor *monitor = monitors[config.monitorIndex];
    if (config.monitorIndex == 0) {
        monitor = glfwGetPrimaryMonitor();
    }
    if (monitor == nullptr) {
        std::cerr << "No monitor found at index " << config.monitorIndex << ". Using default one." << std::endl;
    }

    if (!config.fullscreen) {
        monitor = nullptr; // Set to nullptr for windowed mode
    }

    window = std::make_optional<GLFWwindow *>(glfwCreateWindow(config.width, config.height, config.name.c_str(),
                                                               monitor,
                                                               nullptr));

    // Then, we check if the window was created successfully
    if (!*window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    initializeVulkan(); // We pass the window to the Vulkan initialization function
}

void VkInitializer::initializeVulkan() {
    // We first check if MoltenVK is available
    if (!isMoltenVkAvailable()) {
        glfwDestroyWindow(*window);
        glfwTerminate();
        throw std::runtime_error("MoltenVK is not available. Please ensure it is installed correctly.");
    }
    // First, we need to check if the Vulkan API is supported
    if (!glfwVulkanSupported()) {
        glfwTerminate();
        throw std::runtime_error("Vulkan is not supported on this system");
    }

    // Then, we get the required extensions GLFW needs to create a Vulkan instance
    uint32_t extensionCount = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    // Then, we create the Vulkan application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = config.name.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(config.applicationVersion[0],
                                                 config.applicationVersion[1],
                                                 config.applicationVersion[2]);
    appInfo.pEngineName = config.engineName.c_str();
    appInfo.engineVersion = VK_MAKE_VERSION(config.engineVersion[0],
                                            config.engineVersion[1],
                                            config.engineVersion[2]);
    appInfo.apiVersion = config.vulkanVersion;

    // We must load the extensions now.
    std::vector<const char *> enabledExtensions(extensions, extensions + extensionCount);
    if (!config.extraExtensions.empty()) {
        for (const auto &ext: config.extraExtensions) {
            enabledExtensions.push_back(ext.c_str());
        }
    }

#ifdef __APPLE__
    // On Apple platforms, we need to make sure that the VK_KHR_portability_subset extension is enabled
    enabledExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    enabledExtensions.push_back("VK_KHR_get_physical_device_properties2"); // Often needed for portability

    // Check if the portability subset extension is available
    uint32_t availableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

    bool portabilityEnumerationFound = false;
    for (const auto &ext: availableExtensions) {
        if (strcmp(ext.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
            portabilityEnumerationFound = true;
            break;
        }
    }

    if (!portabilityEnumerationFound) {
        std::cerr << "Warning: VK_KHR_portability_enumeration extension not found. This may cause issues on macOS."
                  << std::endl;
    }
#endif

    // Then, we create the Vulkan instance create info to create the final instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = enabledExtensions.size();
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // We check if validation layers are enabled
    if (config.enableValidationLayers) {
        CoreValidationLayer validationLayer{"VK_LAYER_KHRONOS_validation"};
        if (validationLayer.exists()) {
            // If the validation layer exists, we enable it
            validationLayer.enable(&createInfo);
        } else {
            std::cerr << "Validation layer " << validationLayer.name << " does not exist. Skipping." << std::endl;
        }
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    // And finally, we create the Vulkan instance
    if (result != VK_SUCCESS) {
        glfwDestroyWindow(*window);
        glfwTerminate();
        throw std::runtime_error("Failed to create Vulkan instance. Error: " + zen::getVulkanErrorString(result));
    }

    result = glfwCreateWindowSurface(instance, *window, nullptr, &surface);

    // Now we can create the Vulkan surface for the window
    if (result != VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(*window);
        glfwTerminate();
        throw std::runtime_error("Failed to create Vulkan surface. Error: " + zen::getVulkanErrorString(result));
    }
}

#endif


#endif
#endif
