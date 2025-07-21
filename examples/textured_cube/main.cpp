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
    glm::vec2 texCoord;
};

constexpr const char* vertexShaderSource = R"(
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform Uniforms {
    bool enabled;
    float time;
} ubo;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    if (ubo.enabled) {
        fragColor = inColor * sin(ubo.time);
    } else {
        fragColor = inColor;
    }
    fragTexCoord = inTexCoord;
}
)";

constexpr const char* fragmentShaderSource = R"(
#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 1) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;
void main() {
    vec4 textureColor = texture(textureSampler, fragTexCoord) * fragColor.a;
    outColor = textureColor;
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
    inputDescriptor.addItem(InputDescriptorItem<glm::vec2>(2, InputFormat::Vector2));

    device->useInputDescriptor(inputDescriptor);

    RenderPipeline pipeline = device->makeRenderPipeline();
    pipeline.inputDescriptor = inputDescriptor;
    pipeline.renderPass = renderPass;
    pipeline.shaderProgram = program;

    UniformBlock uniformBlock = device->makeUniformBlock(sizeof(Uniforms));
    Uniforms uniforms{true, 0.0f};
    uniformBlock.uploadData(&uniforms);
    pipeline.attachUniformBlock(uniformBlock);

    auto textureData = texture::TextureData();
    std::string projectRoot = PROJECT_ROOT_PATH;
    textureData.load(projectRoot + "/examples/textured_cube/texture.jpg");
    auto texture = device->createTexture(textureData);

    device->activateTexture(texture);
    pipeline.attachTexture(texture);

    pipeline.makePipeline();

    // We make a square
    std::vector<Vertex> vertices(4);
    vertices[0] = {glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)};
    vertices[1] = {glm::vec3(0.5f, -0.5f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)};
    vertices[2] = {glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f)};
    vertices[3] = {glm::vec3(0.5f, 0.5f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)};

    Buffer vertexBuffer = device->makeBuffer(vertices);

    std::vector<uint32_t> indices = {
        0, 1, 2, // First triangle
        1, 3, 2 // Second triangle
    };

    Buffer indexBuffer = device->makeBuffer(indices);

    while (!window.shouldClose()) {
        uniforms.time = glfwGetTime();
        uniformBlock.uploadData(&uniforms);
        auto commandBuffer = device->requestCommandBuffer(pipeline, presentable);
        commandBuffer->begin();
        commandBuffer->beginRendering();
        commandBuffer->bindUniforms(pipeline);

        commandBuffer->bindVertexBuffer(vertexBuffer);
        commandBuffer->bindIndexBuffer(indexBuffer, IndexType::UInt32);

        commandBuffer->bindTexture(pipeline);

        commandBuffer->draw(6, true);

        commandBuffer->endRendering();
        commandBuffer->end();
        commandBuffer->submit();
        commandBuffer->present();
        window.allEvents();
    }
    return 0;
}
