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

CommandBuffer::CommandBuffer(const RenderPipeline& pipeline, VkCommandPool commandPool, VkCommandBuffer commandBuffer,
                             Device& device, Presentable& presentable) : presentable(presentable), device(device) {
    this->commandPool = commandPool;
    this->commandBuffer = commandBuffer;
    this->inUse = true;
    this->pipeline = pipeline.pipeline;
    this->renderPass = pipeline.renderPass.renderPass;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(device.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
}

void CommandBuffer::beginRendering() {
    uint32_t imageIndexLocal = 0;
    vkAcquireNextImageKHR(device.logicalDevice, presentable.swapchain, UINT64_MAX,
                          imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndexLocal);
    imageIndex = static_cast<int>(imageIndexLocal);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = device.framebuffers[imageIndex].framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = device.instance.extent;

    VkClearValue clearColor = {};
    clearColor.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandBuffer::endRendering() const {
    vkCmdEndRenderPass(commandBuffer);
}

void CommandBuffer::present() const {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    VkSemaphore waitSemaphores[] = {renderFinishedSemaphore};
    presentInfo.pWaitSemaphores = waitSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &presentable.swapchain;
    auto imageIndexLocal = static_cast<uint32_t>(imageIndex);
    presentInfo.pImageIndices = &imageIndexLocal;
    presentInfo.pResults = nullptr;

    VkQueue queue = device.getPresentQueue().queue;

    vkQueuePresentKHR(queue, &presentInfo);
}

void CommandBuffer::submit() const {
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.waitSemaphoreCount = 1;
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;

    assert(imageAvailableSemaphore != VK_NULL_HANDLE);
    assert(renderFinishedSemaphore != VK_NULL_HANDLE);

    assert(presentable.swapchain != VK_NULL_HANDLE);
    assert(commandBuffer != VK_NULL_HANDLE);
    assert(device.logicalDevice != VK_NULL_HANDLE);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    submitInfo.pWaitSemaphores = waitSemaphores;

    VkQueue graphicsQueue = device.getGraphicsQueue().queue;
    assert(graphicsQueue != VK_NULL_HANDLE);

    vkQueueSubmit(graphicsQueue, 1,
                  &submitInfo, VK_NULL_HANDLE
    );
}

void CommandBuffer::bindVertexBuffer(const Buffer& buffer) const {
    VkBuffer buffers[] = {buffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

VkIndexType zen::getIndexType(IndexType type) {
    switch (type) {
    case IndexType::UInt32:
        return VK_INDEX_TYPE_UINT32;
    case IndexType::UInt16:
        return VK_INDEX_TYPE_UINT16;
    case IndexType::UInt8:
        return VK_INDEX_TYPE_UINT8_EXT; // VK_INDEX_TYPE_UINT8_EXT is used for 8-bit indices
    default:
        throw std::runtime_error("Unsupported index type");
    }
}


void CommandBuffer::bindIndexBuffer(const Buffer& buffer, IndexType type) const {
    vkCmdBindIndexBuffer(commandBuffer, buffer.buffer, 0, getIndexType(type));
}


void CommandBuffer::draw(const int vertexCount, const bool indexed) const {
    if (indexed) {
        vkCmdDrawIndexed(commandBuffer, vertexCount, 1, 0, 0, 0);
    }
    else {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
}


#endif
