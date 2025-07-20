/*
* main.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: A simple example that renders a triangle with Zenith.
* Copyright (c) 2025 Max Van den Eynde
*/

#include <zenith/zenith.h>
#include <zenith/window.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

using namespace zen;

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
};

int main() {
    zen::glfw::WindowConfiguration config{};
    config.name = "Zenith Triangle Example";
    config.width = 800;
    config.height = 600;
    config.enableDebugMessenger = true;
    zen::glfw::Window window(config);
    window.init();

    auto device = zen::Device::makeDefaultDevice(window.acquireInstance());
    auto presentable = device->makePresentable();

    std::vector<zen::RenderAttachment> attachments;

    zen::RenderAttachment colorAttachment;
    colorAttachment.layout = zen::AttachmentLayout::ColorAttachment;
    colorAttachment.format = device->makeColorFormat();
    colorAttachment.attachmentIndex = 0;

    attachments.emplace_back(colorAttachment);

    zen::RenderPass renderPass = device->makeRenderPass(attachments, presentable);

    zen::ShaderModule vertexShader = device->makeShader(R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
)", zen::ShaderType::Vertex);

    zen::ShaderModule fragmentShader = device->makeShader(R"(
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}
)", zen::ShaderType::Fragment);

    vertexShader.compile("main");
    fragmentShader.compile("main");

    ShaderProgram shaderProgram({vertexShader, fragmentShader});

    InputDescriptor inputDescriptor;

    InputDescriptorItem<glm::vec3> position;
    position.location = 0;
    position.format = InputFormat::Vector3;
    inputDescriptor.addItem(position);

    InputDescriptorItem<glm::vec4> color;
    color.location = 1;
    color.format = InputFormat::Vector4;
    inputDescriptor.addItem(color);

    device->useInputDescriptor(inputDescriptor);

    RenderPipeline pipeline = device->makeRenderPipeline();
    pipeline.inputDescriptor = inputDescriptor;
    pipeline.renderPass = renderPass;
    pipeline.shaderProgram = shaderProgram;

    pipeline.makePipeline();

    std::vector<Vertex> vertices(3);
    vertices[0] = {glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)};
    vertices[1] = {glm::vec3(0.5f, -0.5f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)};
    vertices[2] = {glm::vec3(0.0f, 0.5f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)};

    Buffer vertexBuffer = device->makeBuffer(vertices);

    while (!window.shouldClose()) {
        auto commandBuffer = device->requestCommandBuffer(pipeline, presentable);
        commandBuffer->begin();
        commandBuffer->beginRendering();

        commandBuffer->bindVertexBuffer(vertexBuffer);
        commandBuffer->draw(3, false);

        commandBuffer->endRendering();
        commandBuffer->end();
        commandBuffer->submit();
        commandBuffer->present();

        window.allEvents();
    }
    return 0;
}
