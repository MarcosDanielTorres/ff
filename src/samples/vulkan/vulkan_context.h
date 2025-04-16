#pragma once
struct VulkanContext
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;

    VkDebugUtilsMessengerEXT vkDebugUtilsMessenger_ = VK_NULL_HANDLE;

    VulkanSwapchain *swapchain_;
    VkSemaphore timelineSemaphore_;

    VulkanImmediateCommands *immediate_;
    VulkanStagingDevice *stagingDevice_;

    // pimpl
    // NOTE unique_ptr<VulkanContextImpl> pimpl_;
    VmaAllocator vma;
    YcbcrConversionData ycbcrConversionData_[256]; // indexed by lvk::Format
    u32 numYcbcrSamplers_ = 0;
    mutable std::deque<DeferredTask> deferredTasks_;
    CommandBuffer currentCommandBuffer_;
    // pimpl



    // Originalmente en una config
    b32 enabled_validation_layers;
    size_t pipelineCacheDataSize;
    const void* pipelineCacheData;
    VkPipelineCache pipelineCache_;

    // a texture/sampler was created since the last descriptor set update
    mutable bool awaitingCreation_ = false;
    mutable bool awaitingNewImmutableSamplers_ = false;

    // Originalmente en una config

    DeviceQueues queues;
    // NOTE instead of having a TextureHandle I'm going to have a Handle and know at runtime what to do. This is provisiory!
    // The same applies to Holder
    TextureHandle dummyTexture_;

    // TODO borrar algunos de estos porque no todo es necesario para esta demo
    VkResolveModeFlagBits depthResolveMode_ = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
    std::vector<VkFormat> deviceDepthFormats_;
    std::vector<VkSurfaceFormatKHR> deviceSurfaceFormats_;
    VkSurfaceCapabilitiesKHR deviceSurfaceCaps_;
    std::vector<VkPresentModeKHR> devicePresentModes_;
    VkSurfaceKHR vkSurface_;
    // TODO borrar algunos de estos porque no todo es necesario para esta demo

    // TODO If I do u32 currentMaxTextures_ = 16; and then zii the struct it gets set to 0. Inspect the rest of the code. Inspect Casey's code
    // Because this means you can have default values.
    // Okay it seems there are no default values

    u32 currentMaxTextures_ = 16;
    u32 currentMaxSamplers_ = 16;
    u32 currentMaxAccelStructs_ = 1;
    VkDescriptorSetLayout vkDSL_ = VK_NULL_HANDLE;
    VkDescriptorPool vkDPool_ = VK_NULL_HANDLE;
    VkDescriptorSet vkDSet_ = VK_NULL_HANDLE;


    // TODO
    //Pool<Sampler, VkSampler> samplersPool_;
    //Pool<Texture, VulkanImage> texturesPool_;
    Pool_Sampler samplersPool_;
    Pool_Texture texturesPool_;
    Pool_Buffer buffersPool_;


    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties;

    #if 1
    // provided by Vulkan 1.2
    VkPhysicalDeviceDepthStencilResolveProperties vkPhysicalDeviceDepthStencilResolveProperties;
	VkPhysicalDeviceDriverProperties vkPhysicalDeviceDriverProperties;
	VkPhysicalDeviceVulkan12Properties vkPhysicalDeviceVulkan12Properties;
    // provided by Vulkan 1.1
    VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2;
    
    VkPhysicalDeviceVulkan13Features vkFeatures13;
    VkPhysicalDeviceVulkan12Features vkFeatures12;
    VkPhysicalDeviceVulkan11Features vkFeatures11;
    VkPhysicalDeviceFeatures2 vkFeatures10;
    #endif
    
};