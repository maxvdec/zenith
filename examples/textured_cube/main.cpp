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
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
} ubo;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPosition, 1.0);
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

struct alignas(16) Uniforms {
    float time;
    bool enabled;
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

struct Model {
    glm::vec3 position = {0, 0, 0};
    glm::vec3 scale = {1, 1, 1};
    glm::vec3 rotation = {0, 0, 0};

    [[nodiscard]] glm::mat4 makeMatrix() const {
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, scale);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        return model;
    }
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
    Uniforms uniforms{0.0f, true};
    uniformBlock.uploadData(&uniforms);
    pipeline.attachUniformBlock(uniformBlock);

    auto textureData = texture::TextureData();
    std::string projectRoot = PROJECT_ROOT_PATH;
    textureData.load(projectRoot + "/examples/textured_cube/texture.jpg");
    auto texture = device->createTexture(textureData);

    device->activateTexture(texture);
    pipeline.attachTexture(texture);

    pipeline.makePipeline();

    std::vector<Vertex> vertices = {
        // Front face
        {{-0.5f, -0.5f, 0.5f}, {1, 0, 0, 1}, {0.0f, 0.0f}}, // 0
        {{0.5f, -0.5f, 0.5f}, {0, 1, 0, 1}, {1.0f, 0.0f}}, // 1
        {{0.5f, 0.5f, 0.5f}, {0, 0, 1, 1}, {1.0f, 1.0f}}, // 2
        {{-0.5f, 0.5f, 0.5f}, {1, 1, 0, 1}, {0.0f, 1.0f}}, // 3

        // Back face
        {{-0.5f, -0.5f, -0.5f}, {1, 0, 1, 1}, {1.0f, 0.0f}}, // 4
        {{0.5f, -0.5f, -0.5f}, {0, 1, 1, 1}, {0.0f, 0.0f}}, // 5
        {{0.5f, 0.5f, -0.5f}, {1, 1, 1, 1}, {0.0f, 1.0f}}, // 6
        {{-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f, 1}, {1.0f, 1.0f}}, // 7
    };

    Buffer vertexBuffer = device->makeBuffer(vertices);

    std::vector<uint32_t> indices = {
        // Front face
        0, 1, 2, 2, 3, 0,

        // Right face
        1, 5, 6, 6, 2, 1,

        // Back face
        5, 4, 7, 7, 6, 5,

        // Left face
        4, 0, 3, 3, 7, 4,

        // Top face
        3, 2, 6, 6, 7, 3,

        // Bottom face
        4, 5, 1, 1, 0, 4,
    };

    Buffer indexBuffer = device->makeBuffer(indices);

    auto model = Model();

    uniforms.viewMatrix = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f), // camera pos
        glm::vec3(0.0f, 0.0f, 0.0f), // target
        glm::vec3(0.0f, 1.0f, 0.0f) // up
    );

    uniforms.projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(config.width) / static_cast<float>(config.height),
        0.1f,
        10.0f
    );
    uniforms.projectionMatrix[1][1] *= -1;

    while (!window.shouldClose()) {
        uniforms.time = glfwGetTime();
        uniforms.modelMatrix = model.makeMatrix();
        uniformBlock.uploadData(&uniforms);
        auto commandBuffer = device->requestCommandBuffer(pipeline, presentable);
        commandBuffer->begin();
        commandBuffer->beginRendering();
        commandBuffer->bindUniforms(pipeline);

        commandBuffer->bindVertexBuffer(vertexBuffer);
        commandBuffer->bindIndexBuffer(indexBuffer, IndexType::UInt32);

        commandBuffer->bindTexture(pipeline);

        commandBuffer->draw(static_cast<int>(indices.size()), true);

        commandBuffer->endRendering();
        commandBuffer->end();
        commandBuffer->submit();
        commandBuffer->present();
        window.allEvents();
    }
    return 0;
}
