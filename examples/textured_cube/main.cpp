/*
* main.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: 
* Copyright (c) 2025 Max Van den Eynde
*/

#define ZENITH_VULKAN
#include <zenith/zenith.h>
#include <glm/glm.hpp>
#include <zenith/window.h>

using namespace zen;

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
};

constexpr const char* vertexShaderSource = R"(
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(set = 0, binding = 0) uniform Uniforms {
    bool enabled;
    float time;
} ubo;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    if (ubo.enabled) {
        fragColor = inColor * sin(ubo.time);
    } else {
        fragColor = inColor;
    }
}
)";

constexpr const char* fragmentShaderSource = R"(
#version 450
layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;
void main() {
    outColor = fragColor;
}
)";

struct Uniforms {
    bool enabled;
    float time;
};

int main() {
    glfw::WindowConfiguration config;
    config.name = "Zenith Triangle Example";
    config.width = 800;
    config.height = 600;
    config.enableDebugMessenger = true;
    glfw::Window window(config);
    window.init();

    auto device = Device::makeDefaultDevice(window.acquireInstance());
    auto presentable = device->makePresentable();

    std::vector<RenderAttachment> attachments;
    RenderAttachment colorAttachment;
    colorAttachment.layout = AttachmentLayout::ColorAttachment;
    colorAttachment.format = device->makeColorFormat();
    colorAttachment.attachmentIndex = 0;
    attachments.emplace_back(colorAttachment);

    auto renderPass = device->makeRenderPass(attachments, presentable);

    ShaderModule vertexShader = device->makeShader(vertexShaderSource, ShaderType::Vertex);
    ShaderModule fragmentShader = device->makeShader(fragmentShaderSource, ShaderType::Fragment);

    vertexShader.compile("main");
    fragmentShader.compile("main");

    ShaderProgram program(vertexShader, fragmentShader);

    InputDescriptor inputDescriptor;
    inputDescriptor.addItem(InputDescriptorItem<glm::vec3>(0, InputFormat::Vector3));
    inputDescriptor.addItem(InputDescriptorItem<glm::vec4>(1, InputFormat::Vector4));

    device->useInputDescriptor(inputDescriptor);

    RenderPipeline pipeline = device->makeRenderPipeline();
    pipeline.inputDescriptor = inputDescriptor;
    pipeline.renderPass = renderPass;
    pipeline.shaderProgram = program;

    UniformBlock uniformBlock = device->makeUniformBlock(sizeof(Uniforms));
    Uniforms uniforms{true, 0.0f};
    uniformBlock.uploadData(&uniforms);
    pipeline.bindUniformBlock(uniformBlock);

    pipeline.makePipeline();

    // We make a square
    std::vector<Vertex> vertices(4);
    vertices[0] = {glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)};
    vertices[1] = {glm::vec3(0.5f, -0.5f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)};
    vertices[2] = {glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)};
    vertices[3] = {glm::vec3(0.5f, 0.5f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)};

    Buffer vertexBuffer = device->makeBuffer(vertices);

    std::vector<uint32_t> indices = {
        0, 1, 2, // First triangle
        1, 3, 2 // Second triangle
    };

    Buffer indexBuffer = device->makeBuffer(indices);

    while (!window.shouldClose()) {
        auto commandBuffer = device->requestCommandBuffer(pipeline, presentable);
        commandBuffer->begin();
        commandBuffer->beginRendering();
        commandBuffer->bindUniforms(pipeline);

        commandBuffer->bindVertexBuffer(vertexBuffer);
        commandBuffer->bindIndexBuffer(indexBuffer, IndexType::UInt32);

        commandBuffer->draw(6, true);

        commandBuffer->endRendering();
        commandBuffer->end();
        commandBuffer->submit();
        commandBuffer->present();
        window.allEvents();
    }
    return 0;
}
