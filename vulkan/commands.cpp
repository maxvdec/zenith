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
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
    submitInfo.pWaitDstStageMask = waitStages;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    submitInfo.pWaitSemaphores = waitSemaphores;

    VkQueue graphicsQueue = device.getGraphicsQueue().queue;

    vkQueueSubmit(graphicsQueue, 1,
                  &submitInfo, VK_NULL_HANDLE
    );
}


#endif
