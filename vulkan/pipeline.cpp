/*
* pipeline.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: 
* Copyright (c) 2025 Max Van den Eynde
*/

#ifdef ZENITH_VULKAN

#include <zenith/zenith_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <string>
#include <stdexcept>
#include <iostream>

using namespace zen;

void RenderAttachment::makeRenderAttachment() {
    // We must make sure the index is not -1
    if (attachmentIndex < 0) {
        throw std::runtime_error("Attachment index must be set before making a render attachment");
    }

    description.format = format.format;
    description.samples = VK_SAMPLE_COUNT_1_BIT; // We use 1 sample for now
    description.loadOp = toVulkanLoadOp(loadOperation);
    description.storeOp = toVulkanStoreOp(storeOperation);
    if (layout == AttachmentLayout::ColorAttachment) {
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // We set the reference for color attachments
        reference.attachment = attachmentIndex;
        reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else if (layout == AttachmentLayout::DepthAttachment) {
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // We set the reference for depth attachments
        reference.attachment = attachmentIndex;
        reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
}

VkAttachmentStoreOp zen::toVulkanStoreOp(const zen::Operation operation) {
    switch (operation) {
    case zen::Operation::Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case zen::Operation::Clear:
        throw std::runtime_error("Clear operation is not supported for Vulkan store op");
    case zen::Operation::DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        throw std::runtime_error("Unknown operation for Vulkan store op");
    }
}

VkAttachmentLoadOp zen::toVulkanLoadOp(zen::Operation operation) {
    switch (operation) {
    case zen::Operation::Store:
        throw std::runtime_error("Store operation is not supported for Vulkan load op");
    case zen::Operation::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case zen::Operation::DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        throw std::runtime_error("Unknown operation for Vulkan load op");
    }
}

void RenderPass::create(const Device& device) {
    if (attachments.empty()) {
        throw std::runtime_error("No attachments specified to create the Render Pass");
    }
    // First, we need to create a subpass. We're going to use a single subpass for now
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachments[0].reference; // We assume the first attachment is a color attachment
    if (attachments.size() > 1 && attachments[1].layout == AttachmentLayout::DepthAttachment) {
        subpass.pDepthStencilAttachment = &attachments[1].reference;
    }
    else {
        subpass.pDepthStencilAttachment = nullptr; // No depth attachment
    }

    // We create the subpass dependencies
    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // This is the first subpass
    subpassDependency.dstSubpass = 0; // This is the subpass we created

    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;

    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    std::vector<VkAttachmentDescription> vulkanAttachments{};
    for (auto& attachment : attachments) {
        attachment.makeRenderAttachment();
        vulkanAttachments.push_back(attachment.description);
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(vulkanAttachments.size());
    renderPassInfo.pAttachments = vulkanAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;

    if (const VkResult result = vkCreateRenderPass(device.logicalDevice, &renderPassInfo, nullptr, &renderPass); result
        !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass. Error: " + zen::getVulkanErrorString(result));
    }
}

template <int N>
void InputDescriptor<N>::buildInputLayout() {
    int size = 0;
    for (const auto& item : items) {
        size += static_cast<int>(item->getSize());
    }
    binding.binding = 0;
    binding.stride = size;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // We use vertex input rate for now

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        attributes[i].location = item->getLocation();
        attributes[i].binding = binding.binding;
        attributes[i].format = zen::toVulkanFormat(item->getFormat());
        attributes[i].offset = static_cast<uint32_t>(i * item->getSize());
    }
}

#endif
