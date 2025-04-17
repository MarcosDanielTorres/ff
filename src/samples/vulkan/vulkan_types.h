#pragma once

struct VulkanImmediateCommands
{
    // the maximum number of command buffers which can similtaneously exist in the system; when we run out of buffers, we stall and wait until
    // an existing buffer becomes available
    static constexpr uint32_t kMaxCommandBuffers = 64;
    VkDevice device;
    VkQueue queue;
    VkCommandPool commandPool;
    u32 queue_family_index;

    CommandBufferWrapper buffers[kMaxCommandBuffers];

    const char *debug_name;
    SubmitHandle lastSubmitHandle_ = SubmitHandle();
    SubmitHandle nextSubmitHandle_ = SubmitHandle();
    VkSemaphoreSubmitInfo lastSubmitSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                              .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
    VkSemaphoreSubmitInfo waitSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                            .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}; // extra "wait" semaphore
    VkSemaphoreSubmitInfo signalSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                            .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}; // extra "signal" semaphore
    u32 numAvailableCommandBuffers_ = kMaxCommandBuffers;
    u32 submitCounter_ = 1;

};

struct VulkanSwapchain
{
    enum { LVK_MAX_SWAPCHAIN_IMAGES = 16 };

    VulkanContext *ctx_;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t numSwapchainImages_ = 0;
    uint32_t currentImageIndex_ = 0; // [0...numSwapchainImages_)
    uint64_t currentFrameIndex_ = 0; // [0...+inf)
    bool getNextImage_ = true;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkSurfaceFormatKHR surfaceFormat_ = {.format = VK_FORMAT_UNDEFINED};
    TextureHandle swapchainTextures_[LVK_MAX_SWAPCHAIN_IMAGES] = {};
    VkSemaphore acquireSemaphore_[LVK_MAX_SWAPCHAIN_IMAGES] = {};
    uint64_t timelineWaitValues_[LVK_MAX_SWAPCHAIN_IMAGES] = {};
};


struct VulkanContext;
struct MemoryRegionDesc
{
    u32 offset_ = 0;
    u32 size_ = 0;
    SubmitHandle handle_ = {};
};

struct VulkanStagingDevice
{

    VulkanContext& ctx_;
    //Holder<BufferHandle> stagingBuffer_;
    u32 stagingBufferSize_ = 0;
    u32 stagingBufferCounter_ = 0;
    u32 maxBufferSize_ = 0;
    const u32 minBufferSize_ = 4u * 2048u * 2048u;
    std::deque<MemoryRegionDesc> regions_;
};

// pimpl
struct YcbcrConversionData 
{
    VkSamplerYcbcrConversionInfo info;
    //Holder<SamplerHandle> sampler;
    SamplerHandle sampler;
};

struct DeferredTask 
{
  DeferredTask(std::packaged_task<void()>&& task, SubmitHandle handle) : task_(std::move(task)), handle_(handle) {}
  std::packaged_task<void()> task_;
  SubmitHandle handle_;
};

// pimpl

struct DeviceQueues
{
    const static uint32_t INVALID = 0xFFFFFFFF;
    uint32_t graphicsQueueFamilyIndex = INVALID;
    uint32_t computeQueueFamilyIndex = INVALID;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
};

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
    Pool_ShaderModule shaderModulesPool_;
    Pool_RenderPipeline renderPipelinesPool_;


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



// vulkan_buffer_sub_data
void bufferSubData(VulkanBuffer *buffer, VulkanContext& ctx, size_t offset, size_t size, const void* data)
{

}

// vulkan_buffer_get_sub_data
void getBufferSubData(VulkanBuffer *buffer, VulkanContext& ctx, size_t offset, size_t size, void* data)
{

}

// vulkan_buffer_mapped_memory_flush
void flushMappedMemory(VulkanBuffer *buffer, VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size)
{

}

// vulkan_buffer_mapped_memory_invalidate
void invalidateMappedMemory(VulkanBuffer *buffer, VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size)
{

}


struct ShaderModuleHandle;