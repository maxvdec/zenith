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

#include "../../../../../../../opt/homebrew/Cellar/glslang/15.4.0/include/glslang/Public/ShaderLang.h"

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

    template <int N>
    class InputDescriptor;

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

        [[nodiscard]] RenderPass makeRenderPass(std::vector<RenderAttachment>&) const;

        [[nodiscard]] ShaderModule makeShader(const std::string& source, ShaderType type) const;

        [[nodiscard]] ShaderModule makeShader(const std::vector<uint32_t>& code, ShaderType type) const;

        template <int N>
        void useInputDescriptor(InputDescriptor<N>& inputDescriptor) const;

    private:
        Instance instance;

        void findQueueFamilies();

        void initializeLogicalDevice();
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

        void create(const Device& device);
    };

    enum class ShaderType {
        Vertex,
        Fragment,
    };

    EShLanguage toGlslShaderType(ShaderType type);


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

        static ShaderModule loadFromSource(const std::string& source, const Device& device, ShaderType type);
        static ShaderModule loadFromCompiled(const std::vector<uint32_t>& code, const Device& device, ShaderType type);

        void compile(std::string entryPoint, ShaderSpecializationInformation info = {});
    };

    class ShaderProgram {
    public:
        std::vector<ShaderModule> shaderModules = {};

        ShaderProgram() = default;

        explicit ShaderProgram(std::vector<ShaderModule> shaderModules) : shaderModules(std::move(shaderModules)) {
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


    template <int N>
    class InputDescriptor {
    public:
        std::array<VkVertexInputAttributeDescription, N> attributes = {};
        VkVertexInputBindingDescription binding = {};
        std::vector<std::unique_ptr<InputDescriptorItemInterface>> items;

        InputDescriptor() = default;

        InputDescriptor(InputDescriptor&&) noexcept = default;

        InputDescriptor& operator=(InputDescriptor&&) noexcept = default;

        InputDescriptor(const InputDescriptor&) = delete;

        InputDescriptor& operator=(const InputDescriptor&) = delete;

        void buildInputLayout();

        template <typename T>
        inline void addItem(const InputDescriptorItem<T>& item) {
            if (items.size() < N) {
                items.push_back(std::make_unique<InputDescriptorItem<T>>(item));
            }
            else {
                throw std::runtime_error("InputDescriptor: Too many items added");
            }
        }

        [[nodiscard]] inline size_t getSize() const {
            size_t size = 0;
            for (const auto& item : items) {
                size += item->getSize();
            }
            return size;
        }
    };


    class RenderPipeline {
    public:
        VkPipeline pipeline = VK_NULL_HANDLE;
        ShaderProgram shaderProgram = ShaderProgram();
    };
};

#endif //ZENITH_ZENITH_VULKAN_H
