#pragma once

enum { LVK_MAX_COLOR_ATTACHMENTS = 8 };
enum { LVK_MAX_MIP_LEVELS = 16 };

struct VulkanContext;
struct VulkanImage
{
    VkImage vkImage_ = VK_NULL_HANDLE;
    VkImageUsageFlags vkUsageFlags_ = 0;
    VkDeviceMemory vkMemory_[3] = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
    VmaAllocation vmaAllocation_ = VK_NULL_HANDLE;
    VkFormatProperties vkFormatProperties_ = {};
    VkExtent3D vkExtent_ = {0, 0, 0};
    VkImageType vkType_ = VK_IMAGE_TYPE_MAX_ENUM;
    VkFormat vkImageFormat_ = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits vkSamples_ = VK_SAMPLE_COUNT_1_BIT;
    void* mappedPtr_ = nullptr;
    b32 isSwapchainImage_ = false;
    b32 isOwningVkImage_ = true;
    b32 isResolveAttachment = false; // autoset by cmdBeginRendering() for extra synchronization
    uint32_t numLevels_ = 1u;
    uint32_t numLayers_ = 1u;
    b32 isDepthFormat_ = false;
    b32 isStencilFormat_ = false;
    char debugName_[256] = {0};
    // current image layout
    mutable VkImageLayout vkImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    // precached image views - owned by this VulkanImage
    VkImageView imageView_ = VK_NULL_HANDLE; // default view with all mip-levels
    VkImageView imageViewStorage_ = VK_NULL_HANDLE; // default view with identity swizzle (all mip-levels)
    VkImageView imageViewForFramebuffer_[LVK_MAX_MIP_LEVELS][6] = {}; // max 6 faces for cubemap rendering
};



struct VulkanBuffer
{
  // clang-format off
  [[nodiscard]] inline uint8_t* getMappedPtr() const { return static_cast<uint8_t*>(mappedPtr_); }
  [[nodiscard]] inline b32 isMapped() const { return mappedPtr_ != nullptr;  }
  // clang-format on

  VkBuffer vkBuffer_ = VK_NULL_HANDLE;
  VkDeviceMemory vkMemory_ = VK_NULL_HANDLE;
  VmaAllocation vmaAllocation_ = VK_NULL_HANDLE;
  VkDeviceAddress vkDeviceAddress_ = 0;
  VkDeviceSize bufferSize_ = 0;
  VkBufferUsageFlags vkUsageFlags_ = 0;
  VkMemoryPropertyFlags vkMemFlags_ = 0;
  void* mappedPtr_ = nullptr;
  b32 isCoherentMemory_ = false;
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