/*
* shaders.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: 
* Copyright (c) 2025 Max Van den Eynde
*/

#ifdef ZENITH_VULKAN

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>
#include <zenith/zenith_vulkan.h>

using namespace zen;

EShLanguage toGlslShaderType(const ShaderType type) {
    switch (type) {
    case ShaderType::Vertex:
        return EShLangVertex;
    case ShaderType::Fragment:
        return EShLangFragment;
    default:
        throw std::runtime_error("Unsupported shader type");
    }
}

ShaderModule ShaderModule::loadFromSource(const std::string& source, const Device& device, ShaderType type) {
    glslang::InitializeProcess();

    const char* shaderStrings[1];
    shaderStrings[0] = source.c_str();

    glslang::TShader shader(toGlslShaderType(type));
    shader.setStrings(shaderStrings, 1);

    shader.setEnvInput(glslang::EShSourceGlsl, toGlslShaderType(type), glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

    TBuiltInResource resources = {};
    resources.maxLights = 32;
    resources.maxClipPlanes = 6;
    resources.maxTextureUnits = 32;
    resources.maxTextureCoords = 32;
    resources.maxVertexAttribs = 64;
    resources.maxVertexUniformComponents = 4096;
    resources.maxVaryingFloats = 64;
    resources.maxVertexTextureImageUnits = 32;
    resources.maxCombinedTextureImageUnits = 80;
    resources.maxTextureImageUnits = 32;
    resources.maxFragmentUniformComponents = 4096;
    resources.maxDrawBuffers = 32;
    resources.maxVertexUniformVectors = 128;
    resources.maxVaryingVectors = 8;
    resources.maxFragmentUniformVectors = 16;
    resources.maxVertexOutputVectors = 16;
    resources.maxFragmentInputVectors = 15;
    resources.minProgramTexelOffset = -8;
    resources.maxProgramTexelOffset = 7;
    resources.maxClipDistances = 8;
    resources.maxComputeWorkGroupCountX = 65535;
    resources.maxComputeWorkGroupCountY = 65535;
    resources.maxComputeWorkGroupCountZ = 65535;
    resources.maxComputeWorkGroupSizeX = 1024;
    resources.maxComputeWorkGroupSizeY = 1024;
    resources.maxComputeWorkGroupSizeZ = 64;
    resources.maxComputeUniformComponents = 1024;
    resources.maxComputeTextureImageUnits = 16;
    resources.maxComputeImageUniforms = 8;
    resources.maxComputeAtomicCounters = 8;
    resources.maxComputeAtomicCounterBuffers = 1;
    resources.maxVaryingComponents = 60;
    resources.maxVertexOutputComponents = 64;
    resources.maxGeometryInputComponents = 64;
    resources.maxGeometryOutputComponents = 128;
    resources.maxFragmentInputComponents = 128;
    resources.maxImageUnits = 8;
    resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    resources.maxCombinedShaderOutputResources = 8;
    resources.maxImageSamples = 0;
    resources.maxVertexImageUniforms = 0;
    resources.maxTessControlImageUniforms = 0;
    resources.maxTessEvaluationImageUniforms = 0;
    resources.maxGeometryImageUniforms = 0;
    resources.maxFragmentImageUniforms = 8;
    resources.maxCombinedImageUniforms = 8;
    resources.maxGeometryTextureImageUnits = 16;
    resources.maxGeometryOutputVertices = 256;
    resources.maxGeometryTotalOutputComponents = 1024;
    resources.maxGeometryUniformComponents = 1024;
    resources.maxGeometryVaryingComponents = 64;
    resources.maxTessControlInputComponents = 128;
    resources.maxTessControlOutputComponents = 128;
    resources.maxTessControlTextureImageUnits = 16;
    resources.maxTessControlUniformComponents = 1024;
    resources.maxTessControlTotalOutputComponents = 4096;
    resources.maxTessEvaluationInputComponents = 128;
    resources.maxTessEvaluationOutputComponents = 128;
    resources.maxTessEvaluationTextureImageUnits = 16;
    resources.maxTessEvaluationUniformComponents = 1024;
    resources.maxTessPatchComponents = 120;
    resources.maxPatchVertices = 32;
    resources.maxTessGenLevel = 64;
    resources.maxViewports = 16;
    resources.maxVertexAtomicCounters = 0;
    resources.maxTessControlAtomicCounters = 0;
    resources.maxTessEvaluationAtomicCounters = 0;
    resources.maxGeometryAtomicCounters = 0;
    resources.maxFragmentAtomicCounters = 8;
    resources.maxCombinedAtomicCounters = 8;
    resources.maxAtomicCounterBindings = 1;
    resources.maxVertexAtomicCounterBuffers = 0;
    resources.maxTessControlAtomicCounterBuffers = 0;
    resources.maxTessEvaluationAtomicCounterBuffers = 0;
    resources.maxGeometryAtomicCounterBuffers = 0;
    resources.maxFragmentAtomicCounterBuffers = 1;
    resources.maxCombinedAtomicCounterBuffers = 1;
    resources.maxAtomicCounterBufferSize = 16384;
    resources.maxTransformFeedbackBuffers = 4;
    resources.maxTransformFeedbackInterleavedComponents = 64;
    resources.maxCullDistances = 8;
    resources.maxCombinedClipAndCullDistances = 8;
    resources.maxSamples = 4;
    resources.limits.nonInductiveForLoops = true;
    resources.limits.whileLoops = true;
    resources.limits.doWhileLoops = true;
    resources.limits.generalUniformIndexing = true;
    resources.limits.generalAttributeMatrixVectorIndexing = true;
    resources.limits.generalVaryingIndexing = true;
    resources.limits.generalSamplerIndexing = true;
    resources.limits.generalVariableIndexing = true;
    resources.limits.generalConstantMatrixVectorIndexing = true;

    auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(&resources, 100, false, messages)) {
        glslang::FinalizeProcess();
        std::string errorMessage = std::string(shader.getInfoLog()) + "\n" + shader.getInfoDebugLog();
        throw std::runtime_error("Failed to parse shader: " + errorMessage);
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        glslang::FinalizeProcess();
        std::string errorMessage = std::string(program.getInfoLog()) + "\n" + program.getInfoDebugLog();
        throw std::runtime_error("Failed to link shader program: " + errorMessage);
    }

    std::vector<uint32_t> spirv;

    glslang::GlslangToSpv(*program.getIntermediate(toGlslShaderType(type)), spirv);
    glslang::FinalizeProcess();

    return ShaderModule::loadFromCompiled(spirv, device, type);
}

ShaderModule ShaderModule::loadFromCompiled(const std::vector<uint32_t>& code, const Device& device, ShaderType type) {
    ShaderModule module;
    module.type = type;

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    VkResult result = vkCreateShaderModule(device.logicalDevice, &createInfo, nullptr, &module.shaderModule);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module. Error: " + zen::getVulkanErrorString(result));
    }
    return module;
}


#endif
