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
#include <utility>
#include <vulkan/vulkan.hpp>
#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <optional>

using namespace zen;

DevicePicker DevicePicker::makeDefaultPicker() {
    // We make a default device picker that sorts devices by their score
    return DevicePicker([](const Device& device) -> float
    {
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
        }
        else if (device.physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            score += 500.0f; // Integrated GPUs are still good
        }
        else {
            score += 100.0f; // Other types of devices
        }

        score += (float)device.physicalDeviceProperties.limits.maxImageDimension2D; // Higher resolution is better

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
    for (const auto& device : devices) {
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

    // We need to find the queue families that support graphics, compute, and transfer operations
    findQueueFamilies();

    // We initialize the logical device with the queue families we found
    initializeLogicalDevice();
}

void Device::findQueueFamilies() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    queues.clear(); // Make sure it's empty

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        const auto& queueFamily = queueFamilies[i];
        CoreQueue queue;
        queue.familyIndex = i; // Set the family index

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue.capabilities.push_back(DeviceCapabilities::Graphics);
        }
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queue.capabilities.push_back(DeviceCapabilities::Compute);
        }
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            queue.capabilities.push_back(DeviceCapabilities::Transfer);
        }

        VkBool32 present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, instance.surface, &present);
        if (present) {
            queue.capabilities.push_back(DeviceCapabilities::Present);
        }

        queues.push_back(queue);
    }

    if (queueFamilies.empty()) {
        throw std::runtime_error("No suitable queue families found for the physical device");
    }

    if (std::none_of(queues.begin(), queues.end(), [](const CoreQueue& q)
    {
        return std::find(q.capabilities.begin(), q.capabilities.end(), DeviceCapabilities::Graphics) !=
            q.capabilities.end();
    })) {
        throw std::runtime_error("No graphics queue found for the physical device");
    }

    if (std::none_of(queues.begin(), queues.end(), [](const CoreQueue& q)
    {
        return std::find(q.capabilities.begin(), q.capabilities.end(), DeviceCapabilities::Present) !=
            q.capabilities.end();
    })) {
        throw std::runtime_error("No present queue found for the physical device");
    }
}

void Device::initializeLogicalDevice() {
    std::set<uint32_t> uniqueQueueFamilies;
    for (const auto& queue : queues) {
        for (const auto& capability : queue.capabilities) {
            if (capability == DeviceCapabilities::Graphics ||
                capability == DeviceCapabilities::Compute ||
                capability == DeviceCapabilities::Transfer ||
                capability == DeviceCapabilities::Present) {
                uniqueQueueFamilies.insert(queue.familyIndex);
            }
        }
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f; // We set the queue priority to 1.0f

    for (uint32_t familyIndex : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = familyIndex;
        queueCreateInfo.queueCount = 1; // We create one queue per family
        queueCreateInfo.pQueuePriorities = &queuePriority; // We set the queue priority
        queueCreateInfos.push_back(queueCreateInfo);
    }

    physicalDeviceFeatures.samplerAnisotropy = VK_TRUE; // Enable anisotropic filtering

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures; // We set the physical device features
    deviceCreateInfo.enabledExtensionCount = extensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device. Error: " + zen::getVulkanErrorString(result));
    }

    for (auto& queue : queues) {
        if (queue.capabilities.empty()) {
            continue; // Skip queues without capabilities
        }

        VkQueue vkQueue;
        vkGetDeviceQueue(logicalDevice, queue.familyIndex, 0, &vkQueue); // We get the first queue of the family
        queue.queue = vkQueue; // Set the Vulkan queue handle
    }
}

std::vector<CoreQueue> Device::getQueueFromCapability(zen::DeviceCapabilities capability) {
    std::vector<CoreQueue> result;
    for (const auto& queue : queues) {
        if (std::find(queue.capabilities.begin(), queue.capabilities.end(), capability) != queue.capabilities.end()) {
            result.push_back(queue);
        }
    }
    return result;
}


bool Device::supportsExtensions(const std::vector<std::string>& requiredExtensions) const {
    std::vector<std::string> extensionsToCheck = requiredExtensions;
    if (extensionsToCheck.empty()) {
        for (const char* ext : extensions) {
            extensionsToCheck.push_back(std::string(ext));
        }
    }

    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> available;
    for (const auto& ext : availableExtensions) {
        available.insert(ext.extensionName);
    }

    for (const auto& req : extensionsToCheck) {
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
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    if (queueFamilyCount == 0) {
        return false;
    }

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

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

Presentable Device::makePresentable() const {
    // We create a presentable object that can be used to present images to the swapchain
    return {*this, instance};
}

Format Device::makeDepthFormat() const {
    // We create a depth format that can be used for depth testing
    Format depthFormat;
    depthFormat.format = VK_FORMAT_D32_SFLOAT;
    if (!depthFormat.isSupportedDepthAttachment(*this)) {
        throw std::runtime_error("Depth format VK_FORMAT_D32_SFLOAT is not supported by the device");
    }
    return depthFormat;
}

Format Device::makeColorFormat() const {
    // We create a color format that can be used for color attachments
    Format colorFormat;
    colorFormat.format = VK_FORMAT_B8G8R8A8_SRGB; // This is a common format for color attachments
    if (!colorFormat.isSupportedColorAttachment(*this)) {
        throw std::runtime_error("Color format VK_FORMAT_B8G8R8A8_SRGB is not supported by the device");
    }
    return colorFormat;
}

RenderPass Device::makeRenderPass(std::vector<RenderAttachment>& attachments, Presentable& presentable) {
    RenderPass renderPass{};
    renderPass.attachments = std::move(attachments);
    renderPass.create(*this, presentable); // We create the render pass with the current device
    return renderPass;
}

ShaderModule Device::makeShader(const std::string& source, ShaderType type) const {
    // We create a shader module from the source code
    return ShaderModule::loadFromSource(source, *this, type);
}

ShaderModule Device::makeShader(const std::vector<uint32_t>& code, ShaderType type) const {
    // We create a shader module from the compiled code
    return ShaderModule::loadFromCompiled(code, *this, type);
}

void Device::useInputDescriptor(InputDescriptor& inputDescriptor) const {
    inputDescriptor.buildInputLayout();
}

RenderPipeline Device::makeRenderPipeline() const {
    return RenderPipeline(*this); // We create a render pipeline with the current device
}

void Device::makeCommandPool() {
    // If we don't have a command pool or command buffer, we create them
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CoreQueue graphicsQueue = getQueueFromCapability(DeviceCapabilities::Graphics).front();
    poolInfo.queueFamilyIndex = graphicsQueue.familyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // We allow resetting command buffers

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkResult result = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool. Error: " + zen::getVulkanErrorString(result));
    }

    this->commandPool = commandPool;
}

std::shared_ptr<CommandBuffer> Device::requestCommandBuffer(RenderPipeline pipeline, Presentable& presentable) {
    if (!commandPool.has_value()) {
        makeCommandPool();
    }

    for (const auto& cmdBuffer : commandBuffers) {
        if (!cmdBuffer->inUse) {
            return cmdBuffer;
        }
    }

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool.value();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // We create a primary command buffer
    allocInfo.commandBufferCount = 1; // We allocate one command buffer
    VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer. Error: " + zen::getVulkanErrorString(result));
    }
    CommandBuffer buffer = {pipeline, commandPool.value(), commandBuffer, *this, presentable};
    buffer.inUse = true;
    commandBuffers.push_back(std::make_shared<CommandBuffer>(buffer));
    return commandBuffers.back();
}

CoreQueue Device::getGraphicsQueue() const {
    // We return the first queue that supports graphics operations
    for (const auto& queue : queues) {
        if (std::ranges::find(queue.capabilities, DeviceCapabilities::Graphics) !=
            queue.capabilities.end()) {
            return queue;
        }
    }
    throw std::runtime_error("No graphics queue found for the device");
}

CoreQueue Device::getPresentQueue() const {
    // We return the first queue that supports present operations
    for (const auto& queue : queues) {
        if (std::ranges::find(queue.capabilities, DeviceCapabilities::Present) !=
            queue.capabilities.end()) {
            return queue;
        }
    }
    throw std::runtime_error("No present queue found for the device");
}

uint32_t Device::vkFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

UniformBlock Device::makeUniformBlock(size_t size) {
    UniformBlock block(*this);
    block.create(*this, size);
    return block;
}

Texture Device::createTexture(size_t width, size_t height, size_t channels, std::shared_ptr<void> data) {
    Texture texture;
    texture.load(std::move(data), width * height * channels, *this, width, height);
    return texture;
}

#ifdef ZENITH_EXT_TEXTURE

#include <zenith/texture.h>

Texture Device::createTexture(zen::texture::TextureData data) {
    Texture texture;
    texture.load(std::move(data.data), data.size, *this, data.width, data.height);
    return texture;
}

#endif


#endif
