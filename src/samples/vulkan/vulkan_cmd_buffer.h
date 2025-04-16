#pragma once

struct VulkanContext;
struct SubmitHandle {
  uint32_t bufferIndex_ = 0;
  uint32_t submitId_ = 0;
  SubmitHandle() = default;
  explicit SubmitHandle(uint64_t handle) : bufferIndex_(uint32_t(handle & 0xffffffff)), submitId_(uint32_t(handle >> 32)) {
    assert(submitId_);
  }
  bool empty() const {
    return submitId_ == 0;
  }
  uint64_t handle() const {
    return (uint64_t(submitId_) << 32) + bufferIndex_;
  }
};

struct CommandBufferWrapper 
{
    VkCommandBuffer cmdBuf_ = VK_NULL_HANDLE;
    VkCommandBuffer cmdBufAllocated_ = VK_NULL_HANDLE;
    SubmitHandle handle_;
    VkFence fence_ = VK_NULL_HANDLE;
    VkSemaphore semaphore_ = VK_NULL_HANDLE;
    bool isEncoding_ = false;
};

struct CommandBuffer
{
    VulkanContext* ctx_ = nullptr;
    CommandBufferWrapper* wrapper_ = nullptr;

    Framebuffer framebuffer_ = {};
    SubmitHandle lastSubmitHandle_ = {};

    VkPipeline lastPipelineBound_ = VK_NULL_HANDLE;

    bool isRendering_ = false;

    RenderPipelineHandle currentPipelineGraphics_ = {};
};