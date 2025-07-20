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

void InputDescriptor::buildInputLayout() {
    int size = 0;
    for (const auto& item : items) {
        size += static_cast<int>(item->getSize());
    }
    binding.binding = 0;
    binding.stride = size;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // We use vertex input rate for now

    attributes.resize(items.size());

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        attributes[i].location = item->getLocation();
        attributes[i].binding = binding.binding;
        attributes[i].format = zen::toVulkanFormat(item->getFormat());
        attributes[i].offset = static_cast<uint32_t>(i * item->getSize());
    }
}

void RenderPipeline::makePipeline() {
    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // First, we need to set the shader stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(shaderProgram.shaderModules.size());
    for (const auto& shader : shaderProgram.shaderModules) {
        shaderStages.emplace_back(shader.get().shaderStageInfo);
    }
    info.stageCount = static_cast<uint32_t>(shaderStages.size());
    info.pStages = shaderStages.data();

    // Now, we need to set the vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &inputDescriptor.binding;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(inputDescriptor.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = inputDescriptor.attributes.data();

    info.pVertexInputState = &vertexInputInfo;

    // We set the input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // We use triangle list for now
    inputAssembly.primitiveRestartEnable = VK_FALSE; // We don't use primitive restart for now

    info.pInputAssemblyState = &inputAssembly;

    // We set the viewport and scissor state
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(device.instance.extent.width);
    viewport.height = static_cast<float>(device.instance.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {device.instance.extent.width, device.instance.extent.height};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    info.pViewportState = &viewportState;

    // We set the rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // <- wireframe for debug?
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    info.pRasterizationState = &rasterizer;

    // We set the multisample state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // We use 1 sample for now

    info.pMultisampleState = &multisampling;

    // We set the depth stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    info.pDepthStencilState = &depthStencil;

    // We set the color blend state
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    info.pColorBlendState = &colorBlending;

    // We set the layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // no descriptor sets
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // no push constants
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device.logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create empty pipeline layout!");
    }

    info.layout = pipelineLayout;
    info.renderPass = renderPass.renderPass;
    info.subpass = 0; // We use the first subpass

    VkResult result = vkCreateGraphicsPipelines(device.logicalDevice, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline. Error: " + zen::getVulkanErrorString(result));
    }
}


#endif
