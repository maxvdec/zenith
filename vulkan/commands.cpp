/*
* commands.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: 
* Copyright (c) 2025 Max Van den Eynde
*/

#ifdef ZENITH_VULKAN

#include <zenith/zenith_vulkan.h>
#include <vulkan/vulkan.hpp>

using namespace zen;

void CommandBuffer::begin() const {
    vkResetCommandBuffer(commandBuffer, 0);

    // We first check if the command buffer is valid
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

void CommandBuffer::end() {
    vkEndCommandBuffer(commandBuffer);
    inUse = false;
}


#endif
