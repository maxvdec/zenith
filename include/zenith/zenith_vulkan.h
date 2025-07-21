/*
* zenith_vulkan.h
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: Main header for the Zenith Vulkan backend.
* Copyright (c) 2025 Max Van den Eynde
*/

#ifndef ZENITH_ZENITH_VULKAN_H
#define ZENITH_ZENITH_VULKAN_H

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <functional>

#ifdef ZENITH_EXT_TEXTURE
#include <zenith/texture.h>
#endif

#include "texture.h"
#include "glslang/Public/ShaderLang.h"

#ifdef ZENITH_EXT_TEXTURE
namespace zen::texture {
    class TextureData;
}
#endif

namespace zen {
    struct Instance {
        VkInstance instance;
        VkSurfaceKHR surface;
        VkExtent2D extent = {0, 0};

        Instance(VkInstance instance, VkSurfaceKHR surface, VkExtent2D extent) : instance(instance), surface(surface),
            extent(extent) {
        };
    };

    class CoreValidationLayer {
    public:
        std::string name;

        [[nodiscard]] bool exists() const;

        void enable(VkInstanceCreateInfo* createInfo) const;

        static std::vector<CoreValidationLayer> getDeviceLayers();
    };

    class CoreVulkanExtension {
    public:
        std::string name;

        static std::vector<CoreVulkanExtension> getDeviceExtensions();

        [[nodiscard]] bool exists() const;
    };

    std::string getVulkanErrorString(VkResult error);

    bool isMoltenVkAvailable();

    class Device;

    using DeviceSelector = std::function<float(const Device&)>;

    class DevicePicker {
    public:
        DeviceSelector selector;

        static DevicePicker makeDefaultPicker();

        explicit DevicePicker(DeviceSelector selector) : selector(std::move(selector)) {
        };
    };

    enum class DeviceCapabilities {
        Graphics,
        Compute,
        Transfer,
        Present,
    };

    class CoreQueue {
    public:
        VkQueue queue = VK_NULL_HANDLE;
        uint32_t familyIndex = 0;
        std::vector<DeviceCapabilities> capabilities = {};
    };

    class Presentable;

    struct Format;

    class RenderPass;

    class RenderAttachment;

    class ShaderModule;

    enum class ShaderType;

    class InputDescriptor;

    class RenderPipeline;

    class Buffer {
    public:
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        template <typename T>
        void uploadData(const std::vector<T>& data, Device& device) {
            pushData(
                std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data.data()),
                                     reinterpret_cast<const uint8_t*>(data.data() + data.size())),
                sizeof(T) * data.size(),
                device
            );
        }

        void destroy(const Device& device);

        [[nodiscard]] bool isValid() const {
            return buffer != VK_NULL_HANDLE && memory != VK_NULL_HANDLE;
        }

    private:
        void pushData(const std::vector<uint8_t>& data, int size, Device& device);
    };

    enum class IndexType {
        UInt32,
        UInt16,
        UInt8,
    };

    class Texture;

    VkIndexType getIndexType(IndexType type);

    class CommandBuffer {
    public:
        CommandBuffer(const RenderPipeline& pipeline, VkCommandPool commandPool,
                      VkCommandBuffer commandBuffer, Device& device, Presentable& presentable
        );

        void begin() const;
        void end();

        void beginRendering();
        void endRendering() const;

        void present() const;
        void submit() const;

        void bindVertexBuffer(const Buffer& buffer) const;
        void bindIndexBuffer(const Buffer& buffer, IndexType type) const;
        void bindUniforms(const RenderPipeline& pipeline) const;
        void activateTexture(Texture& texture);
        void draw(int vertexCount, bool indexed) const;

        bool inUse;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    private:
        [[maybe_unused]] VkCommandPool commandPool = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        Presentable& presentable;
        int imageIndex = 0;
        Device& device;
    };

    struct Framebuffer {
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
    };

    class UniformBlock;


    class Device {
    public:
        DevicePicker picker = DevicePicker::makeDefaultPicker();

        static std::unique_ptr<Device> makeDefaultDevice(Instance instance);

        explicit Device(const Instance& instance, DevicePicker picker = DevicePicker::makeDefaultPicker())
            : picker(std::move(picker)), instance(instance) {
        }

        [[nodiscard]] bool supportsExtensions(const std::vector<std::string>& requiredExtensions = {}) const;

        [[nodiscard]] bool supportsSwapchain() const;

        [[nodiscard]] bool hasRequiredQueues() const;

        [[nodiscard]] bool supportsRaytracing() const;

        void init();

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties physicalDeviceProperties{};
        VkPhysicalDeviceFeatures physicalDeviceFeatures{};
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};

        VkDevice logicalDevice = VK_NULL_HANDLE;
        std::vector<CoreQueue> queues;

        std::vector<const char*> extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        std::vector<CoreQueue> getQueueFromCapability(DeviceCapabilities capability);

        [[nodiscard]] Presentable makePresentable() const;

        [[nodiscard]] Format makeDepthFormat() const;

        [[nodiscard]] Format makeColorFormat() const;

        [[nodiscard]] RenderPass makeRenderPass(std::vector<RenderAttachment>&, Presentable&);

        [[nodiscard]] ShaderModule makeShader(const std::string& source, ShaderType type) const;

        [[nodiscard]] ShaderModule makeShader(const std::vector<uint32_t>& code, ShaderType type) const;

        [[nodiscard]] RenderPipeline makeRenderPipeline() const;

        [[nodiscard]] UniformBlock makeUniformBlock(size_t size);

        [[nodiscard]] Texture createTexture(size_t width, size_t height, size_t channels,
                                            std::shared_ptr<void> data);
#ifdef ZENITH_EXT_TEXTURE
#ifdef ZENITH_VULKAN
        [[nodiscard]] Texture createTexture(zen::texture::TextureData data);
#endif
#endif

        void useInputDescriptor(InputDescriptor& inputDescriptor) const;

        [[nodiscard]] std::shared_ptr<CommandBuffer> requestCommandBuffer(
            RenderPipeline pipeline, Presentable& presentable);

        void makeCommandPool();

        template <typename T>
        Buffer makeBuffer(std::vector<T>& data) {
            Buffer buffer;
            buffer.uploadData(data, *this);
            return buffer;
        }

        [[nodiscard]] CoreQueue getGraphicsQueue() const;
        [[nodiscard]] CoreQueue getPresentQueue() const;

        [[nodiscard]] uint32_t vkFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        Instance instance;

        std::vector<Framebuffer> framebuffers = {};

    private:
        void findQueueFamilies();

        void initializeLogicalDevice();

        std::optional<VkCommandPool> commandPool = std::nullopt;

        std::vector<std::shared_ptr<CommandBuffer>> commandBuffers;
    };

    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    class Presentable {
    public:
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        std::vector<Image> images = {};
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent = {0, 0};

        Presentable(Device device, Instance instance);

        ~Presentable();

    private:
        Device device;
        Instance instance;

        void create();

        static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

        static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

        static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D windowExtent);
    };

    struct Format {
        VkFormat format = VK_FORMAT_UNDEFINED;

        [[nodiscard]] bool isSupportedColorAttachment(const Device& device) const;
        [[nodiscard]] bool isSupportedDepthAttachment(const Device& device) const;
    };

    enum class Operation {
        Store,
        Clear,
        DontCare,
    };

    VkAttachmentStoreOp toVulkanStoreOp(Operation operation);

    VkAttachmentLoadOp toVulkanLoadOp(Operation operation);

    enum class AttachmentLayout {
        ColorAttachment,
        DepthAttachment,
    };

    class RenderAttachment {
    public:
        VkAttachmentDescription description{};
        VkAttachmentReference reference{};
        Operation loadOperation = Operation::Clear;
        Operation storeOperation = Operation::Store;
        Format format{};
        AttachmentLayout layout = AttachmentLayout::ColorAttachment;
        int attachmentIndex = -1;

        void makeRenderAttachment();
    };

    class RenderPass {
    public:
        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::vector<RenderAttachment> attachments = {};

        void addAttachment(const RenderAttachment& attachment) {
            attachments.push_back(attachment);
        }

        void create(Device& device, const Presentable& presentable);
    };

    enum class ShaderType {
        Vertex,
        Fragment,
    };

    EShLanguage toGlslShaderType(ShaderType type);

    VkShaderStageFlagBits toVulkanShaderStage(ShaderType type);


    struct CoreShaderSpecializationValueInterface {
        virtual ~CoreShaderSpecializationValueInterface() = default;
        [[nodiscard]] virtual size_t getSize() const = 0;
        [[nodiscard]] virtual int getId() const = 0;
        [[nodiscard]] virtual const void* getValue() const = 0;
    };

    template <typename T>
    struct ShaderSpecializationValue final : public CoreShaderSpecializationValueInterface {
        T value;
        int id;

        ShaderSpecializationValue(const int id, const T& v) : value(v), id(id) {
        }

        [[nodiscard]] size_t getSize() const override { return sizeof(T); }
        [[nodiscard]] const void* getValue() const override { return &value; }
        [[nodiscard]] int getId() const override { return id; }
    };

    class ShaderSpecializationInformation {
    public:
        ShaderSpecializationInformation() = default;

        ShaderSpecializationInformation(const ShaderSpecializationInformation&) = delete;
        ShaderSpecializationInformation& operator=(const ShaderSpecializationInformation&) = delete;

        ShaderSpecializationInformation(ShaderSpecializationInformation&&) noexcept = default;
        ShaderSpecializationInformation& operator=(ShaderSpecializationInformation&&) noexcept = default;

        template <typename T>
        void addValue(int id, const T& value) {
            values.push_back(std::make_unique<ShaderSpecializationValue<T>>(id, value));
        }

        VkSpecializationInfo specializationInfo = {};
        void createSpecializationInfo();

    private:
        std::vector<std::unique_ptr<CoreShaderSpecializationValueInterface>> values{};
        std::vector<VkSpecializationMapEntry> specializations{};
        std::vector<uint8_t> specializationData{};
    };

    class ShaderModule {
    public:
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        ShaderType type = ShaderType::Vertex;
        std::string entryPoint;
        ShaderSpecializationInformation specializationInfo;
        VkPipelineShaderStageCreateInfo shaderStageInfo = {};

        static ShaderModule loadFromSource(const std::string& source, const Device& device, ShaderType type);
        static ShaderModule loadFromCompiled(const std::vector<uint32_t>& code, const Device& device, ShaderType type);

        void compile(std::string entryPoint, ShaderSpecializationInformation info = {});
    };

    class ShaderProgram {
    public:
        std::vector<std::reference_wrapper<ShaderModule>> shaderModules = {};

        template <typename... ShaderModules>
        explicit ShaderProgram(ShaderModules&... modules) {
            (shaderModules.emplace_back(std::ref(modules)), ...);
        }

        explicit ShaderProgram(const std::vector<std::reference_wrapper<ShaderModule>>& modules)
            : shaderModules(modules) {
            if (modules.empty()) {
                throw std::runtime_error("ShaderProgram: No shader modules provided");
            }
        }
    };

    enum class InputFormat {
        Vector3,
        Vector2,
        Vector4,
        Color,
        Float,
        Int,
        Uint,
        Bool,
        Mat3,
        Mat4,
    };

    VkFormat toVulkanFormat(InputFormat format);

    struct InputDescriptorItemInterface {
        virtual ~InputDescriptorItemInterface() = default;
        [[nodiscard]] virtual int getLocation() const = 0;
        [[nodiscard]] virtual InputFormat getFormat() const = 0;
        [[nodiscard]] virtual size_t getSize() const = 0;
    };

    template <typename T>
    struct InputDescriptorItem final : public InputDescriptorItemInterface {
        int location = 0;
        InputFormat format = InputFormat::Vector3;

        InputDescriptorItem(const int location, const InputFormat format) : location(location), format(format) {
        }

        InputDescriptorItem() = default;

        [[nodiscard]] int getLocation() const override { return location; }
        [[nodiscard]] InputFormat getFormat() const override { return format; }
        [[nodiscard]] size_t getSize() const override { return sizeof(T); }
    };


    class InputDescriptor {
    public:
        std::vector<VkVertexInputAttributeDescription> attributes;
        VkVertexInputBindingDescription binding = {};
        std::vector<std::shared_ptr<InputDescriptorItemInterface>> items;

        InputDescriptor() = default;

        void buildInputLayout();

        template <typename T>
        inline void addItem(const InputDescriptorItem<T>& item) {
            items.push_back(std::make_shared<InputDescriptorItem<T>>(item));
        }

        [[nodiscard]] inline size_t getSize() const {
            size_t size = 0;
            for (const auto& item : items) {
                size += item->getSize();
            }
            return size;
        }
    };

    class UniformBlock;

    class RenderPipeline {
    public:
        VkPipeline pipeline = VK_NULL_HANDLE;
        ShaderProgram shaderProgram = ShaderProgram();
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        InputDescriptor inputDescriptor = {};
        RenderPass renderPass = RenderPass();
        const Device& device;

        explicit RenderPipeline(const Device& device) : device(device) {
        }

        void makePipeline();
        void bindUniformBlock(UniformBlock& uniformBlock);

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    private:
        std::vector<std::shared_ptr<UniformBlock>> uniformBlocks;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        void recalculateUniforms();
    };

    class UniformBlock {
    public:
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo descriptorBufferInfo = {};

        explicit UniformBlock(Device& device) : device(device) {
        }

        void create(const Device& device, size_t size);
        void destroy(const Device& device);
        void uploadData(void* data) const;

    private:
        size_t size = 0;
        Device& device;
    };

    class Texture {
    public:
        VkBuffer imageBuffer = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        std::shared_ptr<void> imageData = nullptr;
        VkDeviceMemory imageMemory = VK_NULL_HANDLE;
        VkDeviceSize imageSize = 0;
        Image image = {};

        void load(std::shared_ptr<void> imageData, VkDeviceSize imageSize, Device& device, uint32_t width,
                  uint32_t height);

        void activateTexture(CommandBuffer& commandBuffer) const;
        void createSampler(Device& device);
        void createDescriptorSet(Device& device, VkDescriptorSetLayout);

        uint32_t width = 0;
        uint32_t height = 0;

    private:
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Device& device);
    };
};

#endif //ZENITH_ZENITH_VULKAN_H
