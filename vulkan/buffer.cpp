/*
* buffer.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: 
* Copyright (c) 2025 Max Van den Eynde
*/

#ifdef ZENITH_VULKAN

#include <zenith/zenith_vulkan.h>

using namespace zen;

void Buffer::pushData(const std::vector<uint8_t>& data, int size, Device& device) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateBuffer(device.logicalDevice, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer. Error: " + zen::getVulkanErrorString(result));
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.logicalDevice, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.vkFindMemoryType(memRequirements.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(device.logicalDevice, &allocInfo, nullptr, &memory);

    vkBindBufferMemory(device.logicalDevice, buffer, memory, 0);

    void* mappedData;
    vkMapMemory(device.logicalDevice, memory, 0, size, 0, &mappedData);
    std::memcpy(mappedData, data.data(), size);
    vkUnmapMemory(device.logicalDevice, memory);
}


void Buffer::destroy(const Device& device) {
    vkDestroyBuffer(device.logicalDevice, buffer, nullptr);
}


#endif
