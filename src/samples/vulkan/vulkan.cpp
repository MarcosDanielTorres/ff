#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <deque>
#include <set>
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "os/os_core.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "os/os_core.cpp"

global_variable bool hasAccelerationStructure_ = false;
global_variable bool hasRayQuery_ = false;
global_variable bool hasRayTracingPipeline_ = false;
global_variable bool has8BitIndices_ = false;

// TODO the fuck is this
#define VMA_VULKAN_VERSION 1003000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#define VK_USE_PLATFORM_WIN32_KHR
#define LVK_VULKAN_USE_VMA 1
#define VMA_IMPLEMENTATION

#define VOLK_IMPLEMENTATION
#define VK_NO_PROTOTYPES 1
#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_win32.h>


#include "generated/handles.h"
#include "vulkan_base.h"
#include "vulkan_cmd_buffer.h"
#include "generated/generated.h"
#include "vulkan_types.h"

const char* k_def_validation_layer[] = {"VK_LAYER_KHRONOS_validation"};
// These bindings should match GLSL declarations injected into shaders in VulkanContext::createShaderModule().
enum Bindings {
  kBinding_Textures = 0,
  kBinding_Samplers = 1,
  kBinding_StorageImages = 2,
  kBinding_YUVImages = 3,
  kBinding_AccelerationStructures = 4,
  kBinding_NumBindings = 5,
};




/*

texturespool
samplerspool
bufferspools
swapchain this creates 3 textures out of the texture pools so for up until this point is the only one with 4 idx

then submit and getCurrentswapchainformat() get called in a loop and execute

```
  while (!glfwWindowShouldClose(window)) 
  {
    glfwPollEvents();
    glfwGetFramebufferSize(window, &width, &height);
    if (!width || !height)
      continue;
    ICommandBuffer& buf = ctx->acquireCommandBuffer();
    ctx->submit(buf, ctx->getCurrentSwapchainTexture());
  }
```

There is a weird fucking function called `ensureStagingBuffer` that destroys the holder for whatever reason.
its called only 2 times and i guess it does nothing in this example!

There is no call to destroy() inside Handle (the only function related destruction)

*/


enum TextureUsageBits : u8 {
  TextureUsageBits_Sampled = 1 << 0,
  TextureUsageBits_Storage = 1 << 1,
  TextureUsageBits_Attachment = 1 << 2,
};



struct TextureFormatProperties {
  Format format = Format_Invalid;
  u8 bytesPerBlock : 5 = 1;
  u8 blockWidth : 3 = 1;
  u8 blockHeight : 3 = 1;
  u8 minBlocksX : 2 = 1;
  u8 minBlocksY : 2 = 1;
  bool depth : 1 = false;
  bool stencil : 1 = false;
  bool compressed : 1 = false;
  u8 numPlanes : 2 = 1;
};


#define PROPS(fmt, bpb, ...) \
  TextureFormatProperties { .format = Format_##fmt, .bytesPerBlock = bpb, ##__VA_ARGS__ }

static constexpr TextureFormatProperties properties[] = {
    PROPS(Invalid, 1),
    PROPS(R_UN8, 1),
    PROPS(R_UI16, 2),
    PROPS(R_UI32, 4),
    PROPS(R_UN16, 2),
    PROPS(R_F16, 2),
    PROPS(R_F32, 4),
    PROPS(RG_UN8, 2),
    PROPS(RG_UI16, 4),
    PROPS(RG_UI32, 8),
    PROPS(RG_UN16, 4),
    PROPS(RG_F16, 4),
    PROPS(RG_F32, 8),
    PROPS(RGBA_UN8, 4),
    PROPS(RGBA_UI32, 16),
    PROPS(RGBA_F16, 8),
    PROPS(RGBA_F32, 16),
    PROPS(RGBA_SRGB8, 4),
    PROPS(BGRA_UN8, 4),
    PROPS(BGRA_SRGB8, 4),
    PROPS(ETC2_RGB8, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(ETC2_SRGB8, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC7_RGBA, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(Z_UN16, 2, .depth = true),
    PROPS(Z_UN24, 3, .depth = true),
    PROPS(Z_F32, 4, .depth = true),
    PROPS(Z_UN24_S_UI8, 4, .depth = true, .stencil = true),
    PROPS(Z_F32_S_UI8, 5, .depth = true, .stencil = true),
    PROPS(YUV_NV12, 24, .blockWidth = 4, .blockHeight = 4, .compressed = true, .numPlanes = 2), // Subsampled 420
    PROPS(YUV_420p, 24, .blockWidth = 4, .blockHeight = 4, .compressed = true, .numPlanes = 3), // Subsampled 420
};



VkResult setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, const char* name) {
  if (!name || !*name || !vkSetDebugUtilsObjectNameEXT) {
    return VK_SUCCESS;
  }
  VkDebugUtilsObjectNameInfoEXT ni = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .objectType = type,
      .objectHandle = handle,
      .pObjectName = name,
  };
  return vkSetDebugUtilsObjectNameEXT(device, &ni);
}


const char* getVulkanResultString(VkResult result) {
#define RESULT_CASE(res) \
  case res:              \
    return #res
  switch (result) {
    RESULT_CASE(VK_SUCCESS);
    RESULT_CASE(VK_NOT_READY);
    RESULT_CASE(VK_TIMEOUT);
    RESULT_CASE(VK_EVENT_SET);
    RESULT_CASE(VK_EVENT_RESET);
    RESULT_CASE(VK_INCOMPLETE);
    RESULT_CASE(VK_ERROR_OUT_OF_HOST_MEMORY);
    RESULT_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    RESULT_CASE(VK_ERROR_INITIALIZATION_FAILED);
    RESULT_CASE(VK_ERROR_DEVICE_LOST);
    RESULT_CASE(VK_ERROR_MEMORY_MAP_FAILED);
    RESULT_CASE(VK_ERROR_LAYER_NOT_PRESENT);
    RESULT_CASE(VK_ERROR_EXTENSION_NOT_PRESENT);
    RESULT_CASE(VK_ERROR_FEATURE_NOT_PRESENT);
    RESULT_CASE(VK_ERROR_INCOMPATIBLE_DRIVER);
    RESULT_CASE(VK_ERROR_TOO_MANY_OBJECTS);
    RESULT_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
    RESULT_CASE(VK_ERROR_SURFACE_LOST_KHR);
    RESULT_CASE(VK_ERROR_OUT_OF_DATE_KHR);
    RESULT_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
    RESULT_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
    RESULT_CASE(VK_ERROR_VALIDATION_FAILED_EXT);
    RESULT_CASE(VK_ERROR_FRAGMENTED_POOL);
    RESULT_CASE(VK_ERROR_UNKNOWN);
    // Provided by VK_VERSION_1_1
    RESULT_CASE(VK_ERROR_OUT_OF_POOL_MEMORY);
    // Provided by VK_VERSION_1_1
    RESULT_CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE);
    // Provided by VK_VERSION_1_2
    RESULT_CASE(VK_ERROR_FRAGMENTATION);
    // Provided by VK_VERSION_1_2
    RESULT_CASE(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
    // Provided by VK_KHR_swapchain
    RESULT_CASE(VK_SUBOPTIMAL_KHR);
    // Provided by VK_NV_glsl_shader
    RESULT_CASE(VK_ERROR_INVALID_SHADER_NV);
#ifdef VK_ENABLE_BETA_EXTENSIONS
    // Provided by VK_KHR_video_queue
    RESULT_CASE(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    // Provided by VK_KHR_video_queue
    RESULT_CASE(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    // Provided by VK_KHR_video_queue
    RESULT_CASE(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    // Provided by VK_KHR_video_queue
    RESULT_CASE(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    // Provided by VK_KHR_video_queue
    RESULT_CASE(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    // Provided by VK_KHR_video_queue
    RESULT_CASE(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR);
#endif
    // Provided by VK_EXT_image_drm_format_modifier
    RESULT_CASE(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
    // Provided by VK_KHR_global_priority
    RESULT_CASE(VK_ERROR_NOT_PERMITTED_KHR);
    // Provided by VK_EXT_full_screen_exclusive
    RESULT_CASE(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
    // Provided by VK_KHR_deferred_host_operations
    RESULT_CASE(VK_THREAD_IDLE_KHR);
    // Provided by VK_KHR_deferred_host_operations
    RESULT_CASE(VK_THREAD_DONE_KHR);
    // Provided by VK_KHR_deferred_host_operations
    RESULT_CASE(VK_OPERATION_DEFERRED_KHR);
    // Provided by VK_KHR_deferred_host_operations
    RESULT_CASE(VK_OPERATION_NOT_DEFERRED_KHR);
  default:
    return "Unknown VkResult Value";
  }
#undef RESULT_CASE
}


#define VK_ASSERT(func)                                            \
  {                                                                \
    const VkResult vk_assert_result = func;                        \
    if (vk_assert_result != VK_SUCCESS) {                          \
      printf("Vulkan API call failed: %s:%i\n  %s\n  %s\n",         \
                    __FILE__,                                      \
                    __LINE__,                                      \
                    #func,                                         \
                    getVulkanResultString(vk_assert_result));      \
      assert(false);                                               \
    }                                                              \
  }

#define VK_ASSERT_RETURN(func)                                     \
  {                                                                \
    const VkResult vk_assert_result = func;                        \
    if (vk_assert_result != VK_SUCCESS) {                          \
      printf("Vulkan API call failed: %s:%i\n  %s\n  %s\n",         \
                    __FILE__,                                      \
                    __LINE__,                                      \
                    #func,                                         \
                    getVulkanResultString(vk_assert_result));      \
      assert(false);                                               \
    }                                                              \
  }



global_variable OS_Window global_w32_window;

LRESULT CALLBACK win32_main_callback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) 
{
    LRESULT result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            global_w32_window.is_running = false;
        } break;
        default:
        {
            result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    return result;
}

void Win32ProcessPendingMessages() {
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        switch(Message.message)
        {
            case WM_QUIT:
            {
                global_w32_window.is_running = false;
            } break;
            default:
            {
                DispatchMessageA(&Message);
            } break;
        }
    }
}



bool hasExtension(const char* ext, std::vector<VkExtensionProperties>& props) 
{
    for (VkExtensionProperties& p : props) {
        if (strcmp(ext, p.extensionName) == 0)
        return true;
    }
    return false;
}

void getDeviceExtensionProps(VkPhysicalDevice dev, std::vector<VkExtensionProperties>& props, const char* validationLayer = nullptr) 
{
    u32 numExtensions = 0;
    vkEnumerateDeviceExtensionProperties(dev, validationLayer, &numExtensions, nullptr);
    std::vector<VkExtensionProperties> p(numExtensions);
    vkEnumerateDeviceExtensionProperties(dev, validationLayer, &numExtensions, p.data());
    props.insert(props.end(), p.begin(), p.end());
}




internal b32
vulkan_has_extension(const char *ext, VkExtensionProperties *props, u32 count) {
    b32 result = 0;
    for (u32 i = 0; i < count; i++) {
        VkExtensionProperties *p = props + i;
        if (strcmp(ext, p->extensionName) == 0)
        {
            result = 1;
            break;
        }
    }
    return result;
}

struct VulkanDeviceHWDesc
{
    char name[200] = {0};
    uintptr_t guid = 0;
    VkPhysicalDeviceType type;
};

struct VulkanQueryDeviceResult
{
    u32 num_compatible_devices;
    VulkanDeviceHWDesc out_devices[8];
};

//global_variable  VkPhysicalDeviceVulkan13Features vkFeatures13_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
//global_variable  VkPhysicalDeviceVulkan12Features vkFeatures12_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
//                                                    .pNext = &vkFeatures13_};
//global_variable  VkPhysicalDeviceVulkan11Features vkFeatures11_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
//                                                    .pNext = &vkFeatures12_};
//global_variable  VkPhysicalDeviceFeatures2 vkFeatures10_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &vkFeatures11_};


 //   global_variable VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties_ = {
 //     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
 // global_variable VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties_ = {
 //     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
 // // provided by Vulkan 1.2
 // global_variable VkPhysicalDeviceDepthStencilResolveProperties vkPhysicalDeviceDepthStencilResolveProperties_ = {
 //     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES,
 //     nullptr,
 // };
 // global_variable VkPhysicalDeviceDriverProperties vkPhysicalDeviceDriverProperties_ = {
 //     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
 //     &vkPhysicalDeviceDepthStencilResolveProperties_,
 // };
 // global_variable VkPhysicalDeviceVulkan12Properties vkPhysicalDeviceVulkan12Properties_ = {
 //     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
 //     &vkPhysicalDeviceDriverProperties_,
 // };
 // // provided by Vulkan 1.1
 // global_variable VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2_ = {
 //     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
 //     &vkPhysicalDeviceVulkan12Properties_,
 //     VkPhysicalDeviceProperties{},
 // };

internal VmaAllocator
vulkan_vma_allocator_create(
    VkPhysicalDevice phys_dev,
    VkDevice device,
    VkInstance instance,
    u32 api_version) 
{
    VmaAllocator vma = VK_NULL_HANDLE;
    const VmaVulkanFunctions funcs = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = vkAllocateMemory,
        .vkFreeMemory = vkFreeMemory,
        .vkMapMemory = vkMapMemory,
        .vkUnmapMemory = vkUnmapMemory,
        .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = vkBindBufferMemory,
        .vkBindImageMemory = vkBindImageMemory,
        .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
        .vkCreateBuffer = vkCreateBuffer,
        .vkDestroyBuffer = vkDestroyBuffer,
        .vkCreateImage = vkCreateImage,
        .vkDestroyImage = vkDestroyImage,
        .vkCmdCopyBuffer = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2,
        .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2,
        .vkBindBufferMemory2KHR = vkBindBufferMemory2,
        .vkBindImageMemory2KHR = vkBindImageMemory2,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
        .vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements,
    };

    const VmaAllocatorCreateInfo ci = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = phys_dev,
        .device = device,
        .preferredLargeHeapBlockSize = 0,
        .pAllocationCallbacks = nullptr,
        .pDeviceMemoryCallbacks = nullptr,
        .pHeapSizeLimit = nullptr,
        .pVulkanFunctions = &funcs,
        .instance = instance,
        .vulkanApiVersion = api_version,
    };
    VK_ASSERT(vmaCreateAllocator(&ci, &vma));
    return vma;
}





struct StageAccess {
  VkPipelineStageFlags2 stage;
  VkAccessFlags2 access;
};

internal StageAccess
getPipelineStageAccess(VkImageLayout layout)
{
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return {
            .stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .access = VK_ACCESS_2_NONE,
        };
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        };
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        };
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
            .access = VK_ACCESS_2_SHADER_READ_BIT,
        };
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .access = VK_ACCESS_2_TRANSFER_READ_BIT,
        };
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        };
    case VK_IMAGE_LAYOUT_GENERAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
        };
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return {
            .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .access = VK_ACCESS_2_NONE | VK_ACCESS_2_SHADER_WRITE_BIT,
        };
    default:
        gui_assert(false, "Unsupported image layout transition!");
        return {
            .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
        };
    }
}

[[nodiscard]] inline b32 vulkan_image_is_sampled_image(VkImageUsageFlags vkUsageFlags_) { return (vkUsageFlags_ & VK_IMAGE_USAGE_SAMPLED_BIT) > 0; }
[[nodiscard]] inline b32 vulkan_image_is_storage_image(VkImageUsageFlags vkUsageFlags_) { return (vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT) > 0; }
[[nodiscard]] inline b32 vulkan_image_is_color_attachment(VkImageUsageFlags vkUsageFlags_) { return (vkUsageFlags_ & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) > 0; }
[[nodiscard]] inline b32 vulkan_image_is_depth_attachment(VkImageUsageFlags vkUsageFlags_) { return (vkUsageFlags_ & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) > 0; }
[[nodiscard]] inline b32 vulkan_image_is_attachment(VkImageUsageFlags vkUsageFlags_) { return (vkUsageFlags_ & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) > 0; }

internal void
vulkan_image_transition_layout(VulkanImage *image, VkCommandBuffer commandBuffer, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange)
{
    //LVK_PROFILER_FUNCTION_COLOR(LVK_PROFILER_COLOR_BARRIER);

    VkImageLayout oldImageLayout =
        image->vkImageLayout_ == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
            ? (vulkan_image_is_depth_attachment(image->vkUsageFlags_) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            : image->vkImageLayout_;

    if (newImageLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL) {
        newImageLayout = vulkan_image_is_depth_attachment(image->vkUsageFlags_) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    StageAccess src = getPipelineStageAccess(oldImageLayout);
    StageAccess dst = getPipelineStageAccess(newImageLayout);

    if (vulkan_image_is_depth_attachment(image->vkUsageFlags_) && image->isResolveAttachment) {
        // https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#renderpass-resolve-operations
        src.stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        dst.stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        src.access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        dst.access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }

    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = src.stage,
        .srcAccessMask = src.access,
        .dstStageMask = dst.stage,
        .dstAccessMask = dst.access,
        .oldLayout = image->vkImageLayout_,
        .newLayout = newImageLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->vkImage_,
        .subresourceRange = subresourceRange,
    };

    VkDependencyInfo depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(commandBuffer, &depInfo);

    image->vkImageLayout_ = newImageLayout;
}

internal VkImageAspectFlags
vulkan_image_get_image_aspect_flags(VulkanImage *image)
{
    VkImageAspectFlags flags = 0;

    flags |= image->isDepthFormat_ ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
    flags |= image->isStencilFormat_ ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
    flags |= !(image->isDepthFormat_ || image->isStencilFormat_) ? VK_IMAGE_ASPECT_COLOR_BIT : 0;

    return flags;

}

internal VkDescriptorSetLayoutBinding getDSLBinding(u32 binding,
                                                VkDescriptorType descriptorType,
                                                u32 descriptorCount,
                                                VkShaderStageFlags stageFlags,
                                                const VkSampler* immutableSamplers = nullptr) {
  return VkDescriptorSetLayoutBinding{
      .binding = binding,
      .descriptorType = descriptorType,
      .descriptorCount = descriptorCount,
      .stageFlags = stageFlags,
      .pImmutableSamplers = immutableSamplers,
  };
}


internal void
imageMemoryBarrier2(VkCommandBuffer buffer,
                    VkImage image,
                    StageAccess src,
                    StageAccess dst,
                    VkImageLayout oldImageLayout,
                    VkImageLayout newImageLayout,
                    VkImageSubresourceRange subresourceRange)
{
    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = src.stage,
        .srcAccessMask = src.access,
        .dstStageMask = dst.stage,
        .dstAccessMask = dst.access,
        .oldLayout = oldImageLayout,
        .newLayout = newImageLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange,
    };

    VkDependencyInfo depInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(buffer, &depInfo);
}

internal void
vulkan_image_generate_mipmap(VulkanImage *image, VkCommandBuffer commandBuffer)
{
    //LVK_PROFILER_FUNCTION();

    // Check if device supports downscaling for color or depth/stencil buffer based on image format
    {
        u32 formatFeatureMask = (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT);

        bool hardwareDownscalingSupported = (image->vkFormatProperties_.optimalTilingFeatures & formatFeatureMask) == formatFeatureMask;

        if (!hardwareDownscalingSupported) 
        {
            printf("Doesn't support hardware downscaling of this image format: %d", image->vkImageFormat_);
            return;
        }
    }

    // Choose linear filter for color formats if supported by the device, else use nearest filter
    // Choose nearest filter by default for depth/stencil formats
     VkFilter blitFilter = [](bool isDepthOrStencilFormat, bool imageFilterLinear) {
        if (isDepthOrStencilFormat) {
        return VK_FILTER_NEAREST;
        }
        if (imageFilterLinear) {
        return VK_FILTER_LINEAR;
        }
        return VK_FILTER_NEAREST;
    }(image->isDepthFormat_ || image->isStencilFormat_, image->vkFormatProperties_.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkImageAspectFlags imageAspectFlags = vulkan_image_get_image_aspect_flags(image);

    if (vkCmdBeginDebugUtilsLabelEXT) {
        VkDebugUtilsLabelEXT utilsLabel = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
            .pLabelName = "Generate mipmaps",
            .color = {1.0f, 0.75f, 1.0f, 1.0f},
        };
        vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &utilsLabel);
    }

    VkImageLayout originalImageLayout = image->vkImageLayout_;

    assert(originalImageLayout != VK_IMAGE_LAYOUT_UNDEFINED);

    // 0: Transition the first level and all layers into VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    vulkan_image_transition_layout(image, commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageSubresourceRange{imageAspectFlags, 0, 1, 0, image->numLayers_});

    for (u32 layer = 0; layer < image->numLayers_; ++layer) {
        i32 mipWidth = (i32)image->vkExtent_.width;
        i32 mipHeight = (i32)image->vkExtent_.height;

        for (u32 i = 1; i < image->numLevels_; ++i) {
        // 1: Transition the i-th level to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; it will be copied into from the (i-1)-th layer
        imageMemoryBarrier2(commandBuffer,
                                image->vkImage_,
                                StageAccess{.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .access = VK_ACCESS_2_NONE},
                                StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT},
                                VK_IMAGE_LAYOUT_UNDEFINED, // oldImageLayout
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newImageLayout
                                VkImageSubresourceRange{imageAspectFlags, i, 1, layer, 1});

        i32 nextLevelWidth = mipWidth > 1 ? mipWidth / 2 : 1;
        i32 nextLevelHeight = mipHeight > 1 ? mipHeight / 2 : 1;

        VkOffset3D srcOffsets[2] = {
            VkOffset3D{0, 0, 0},
            VkOffset3D{mipWidth, mipHeight, 1},
        };
        VkOffset3D dstOffsets[2] = {
            VkOffset3D{0, 0, 0},
            VkOffset3D{nextLevelWidth, nextLevelHeight, 1},
        };

        // 2: Blit the image from the prev mip-level (i-1) (VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) to the current mip-level (i)
        // (VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    #if LVK_VULKAN_PRINT_COMMANDS
        LLOGL("%p vkCmdBlitImage()\n", commandBuffer);
    #endif // LVK_VULKAN_PRINT_COMMANDS
         VkImageBlit blit = {
            .srcSubresource = VkImageSubresourceLayers{imageAspectFlags, i - 1, layer, 1},
            .srcOffsets = {srcOffsets[0], srcOffsets[1]},
            .dstSubresource = VkImageSubresourceLayers{imageAspectFlags, i, layer, 1},
            .dstOffsets = {dstOffsets[0], dstOffsets[1]},
        };
        vkCmdBlitImage(commandBuffer,
                        image->vkImage_,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        image->vkImage_,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1,
                        &blit,
                        blitFilter);
        // 3: Transition i-th level to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL as it will be read from in the next iteration
        imageMemoryBarrier2(commandBuffer,
                                image->vkImage_,
                                StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT},
                                StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT},
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, /* oldImageLayout */
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, /* newImageLayout */
                                VkImageSubresourceRange{imageAspectFlags, i, 1, layer, 1});

        // Compute the size of the next mip-level
        mipWidth = nextLevelWidth;
        mipHeight = nextLevelHeight;
        }
    }

    // 4: Transition all levels and layers (faces) to their final layout
    imageMemoryBarrier2(
        commandBuffer,
        image->vkImage_,
        StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT},
        StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT},
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // oldImageLayout
        originalImageLayout, // newImageLayout
        VkImageSubresourceRange{imageAspectFlags, 0, image->numLevels_, 0, image->numLayers_});

    if (vkCmdEndDebugUtilsLabelEXT) {
        vkCmdEndDebugUtilsLabelEXT(commandBuffer);
    }

    image->vkImageLayout_ = originalImageLayout;
}


internal b32
vulkan_image_is_depth_format(VkFormat format)
{
    b32 result = format == VK_FORMAT_D16_UNORM ||
                 format == VK_FORMAT_X8_D24_UNORM_PACK32 || 
                 format == VK_FORMAT_D32_SFLOAT || 
                 format == VK_FORMAT_D16_UNORM_S8_UINT ||
                 format == VK_FORMAT_D24_UNORM_S8_UINT ||
                 format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    return result; 
}

internal b32
vulkan_image_is_stencil_format(VkFormat format)
{
    b32 result = format == VK_FORMAT_S8_UINT ||
                 format == VK_FORMAT_D16_UNORM_S8_UINT ||
                 format == VK_FORMAT_D24_UNORM_S8_UINT ||
                 format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    return result; 
}

struct Handle
{
    // NOTE I can try using a zero index for the invalid one!
    // ZII
    u32 idx = 0;
    u32 gen = 0;
};

b32 handle_is_empty(Handle *handle)
{
    b32 result = false;
    if (handle->gen == 0)
    {
        result = true;
    }
    return result;
}

b32 handle_is_valid(Handle *handle)
{
    b32 result = false;
    if (handle->gen != 0)
    {
        result = true;
    }
    return result;
}


#if 0
//Do these versions:
//- boilerplate
//- runtime checks
//- macro
//- metaprogramming
//- template (imaginary)

/*
enum PoolType
{
    TexturesPool,
    SamplersPool,
};

struct PoolEntry
{
    //void* data;
    u32 gen = 1;
    VulkanImage image;
};

struct Pool
{
    PoolType type;

    u32 count = 1;
    PoolEntry entries[15];
};

/*
    TODO Como hace Casey para lidiar con esto? 
    Cuando quiere instanciar una struct que tiene una union, segun el type el objecto va a ser distinto. Se me ocurre que al no usar funciones es mas simple.
    Sino se tiene que hacer una funcion para cada combinacion diferente de type y dato. Deberia exitir un ejemplo de esto en HandmadeHero!
    La otra es tener un Pool por cada typo alltogether asi:
        struct PoolTexture
        {
            struct PoolEntry
            {
                VulkanImage image;
                u32 gen = 1;
            };
            u32 count = 1;
            PoolEntry entries[15];
        }

        struct PoolSamplers
        {
            struct PoolEntry
            {
                VkSampler sampler;
                u32 gen = 1;
            };
            u32 count = 1;
            PoolEntry entries[15];
        }


    Otra variante es: internal Handle pool_create(Pool *pool, PoolType type, void *data)
                            if type == TexturesPool
                                (VulkanImage*) data

    Otra variante es:
        Pool pool = {0}
        pool->type = TexturesPool
        pool->data = (VulkanImage*)data;
*/
internal Handle
pool_create(Pool *pool, VulkanImage image)
{
    Handle result;
    assert(pool->count < array_count(pool->entries));
    result.idx = pool->count++;
    result.gen = 1;
    PoolEntry new_entry = {.gen = result.gen, .image = image};
    pool->entries[result.idx] = new_entry;
    return result; 
}

internal VulkanImage *
pool_get(Pool *pool, Handle handle)
{
    VulkanImage *result = 0;
    if(handle_is_valid(&handle))
    {
        PoolEntry *entry = &pool->entries[handle.idx];
        result = &entry->image;
    }
    return result; 
}

#endif


// struct Holder -> Handle (it knows what pool it has, i guess)
// Holder calls the respective destruction method for this

// Right know I dont have to delete the resources nor remove them from the Pool



VkSemaphore createSemaphoreTimeline(VkDevice device, uint64_t initialValue, const char* debugName) {
   VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      .initialValue = initialValue,
  };
   VkSemaphoreCreateInfo ci = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &semaphoreTypeCreateInfo,
      .flags = 0,
  };
  VkSemaphore semaphore = VK_NULL_HANDLE;
  VK_ASSERT(vkCreateSemaphore(device, &ci, nullptr, &semaphore));
  VK_ASSERT(setDebugObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, debugName));
  return semaphore;
}


internal VkSemaphore
vulkan_create_semaphore(VkDevice device, const char *debug_name)
{
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = 0,
    };
    VK_ASSERT(vkCreateSemaphore(device, &ci, nullptr, &semaphore));
    VK_ASSERT(setDebugObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, debug_name));
    return semaphore;
}

internal VkFence
vulkan_create_fence(VkDevice device, const char *debug_name)
{
    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = 0,
    };
    VK_ASSERT(vkCreateFence(device, &ci, nullptr, &fence));
    VK_ASSERT(setDebugObjectName(device, VK_OBJECT_TYPE_FENCE, (uint64_t)fence, debug_name));
    return fence;
}

internal b32
vulkan_immediate_commands_is_ready(VulkanImmediateCommands *immediate, SubmitHandle handle, b32 fast_check_no_vulkan)
{
    assert(handle.bufferIndex_ < immediate->kMaxCommandBuffers);
    if(handle.empty())
    {
        return true;
    }
    CommandBufferWrapper *buf = &immediate->buffers[handle.bufferIndex_];

    if(buf->cmdBuf_ == VK_NULL_HANDLE)
    {
        return true;
    }

    if(buf->handle_.submitId_ != handle.submitId_)
    {
        return true;
    }

    if(fast_check_no_vulkan)
    {
        return false;
    }
    return vkWaitForFences(immediate->device, 1, &buf->fence_, VK_TRUE, 0) == VK_SUCCESS;
}

internal void
vulkan_immediate_commands_create(VulkanImmediateCommands *immediate, VkDevice device, u32 queue_family_index, const char *debug_name)
{
    immediate->device = device;
    immediate->queue_family_index = queue_family_index;
    immediate->debug_name = debug_name;
    immediate->lastSubmitSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                              .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
    immediate->waitSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                            .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}; // extra "wait" semaphore
    immediate->signalSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                            .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}; // extra "signal" semaphore
    immediate->numAvailableCommandBuffers_ = immediate->kMaxCommandBuffers;
    immediate->submitCounter_ = 1;
    // aca

    vkGetDeviceQueue(device, queue_family_index, 0, &immediate->queue);

    VkCommandPoolCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = queue_family_index,
    };
    VK_ASSERT(vkCreateCommandPool(device, &ci, nullptr, &immediate->commandPool));
    setDebugObjectName(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)immediate->commandPool, immediate->debug_name);

    VkCommandBufferAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = immediate->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    for (u32 i = 0; i != immediate->kMaxCommandBuffers; i++) {
        CommandBufferWrapper& buf = immediate->buffers[i];
        char fenceName[256] = {0};
        char semaphoreName[256] = {0};
        if (immediate->debug_name) {
            snprintf(fenceName, sizeof(fenceName) - 1, "Fence: %s (cmdbuf %u)", immediate->debug_name, i);
            snprintf(semaphoreName, sizeof(semaphoreName) - 1, "Semaphore: %s (cmdbuf %u)", immediate->debug_name, i);
        }
        buf.semaphore_ = vulkan_create_semaphore(device, semaphoreName);
        buf.fence_ = vulkan_create_fence(device, fenceName);
        VK_ASSERT(vkAllocateCommandBuffers(device, &ai, &buf.cmdBufAllocated_));
        immediate->buffers[i].handle_.bufferIndex_ = i;
    }
}

internal void
vulkan_immediate_commands_purge(VulkanImmediateCommands *immediate)
{
    //LVK_PROFILER_FUNCTION();

    u32 numBuffers = static_cast<u32>array_count(immediate->buffers);

    for (u32 i = 0; i != numBuffers; i++) {
        // always start checking with the oldest submitted buffer, then wrap around
        CommandBufferWrapper& buf = immediate->buffers[(i + immediate->lastSubmitHandle_.bufferIndex_ + 1) % numBuffers];

        if (buf.cmdBuf_ == VK_NULL_HANDLE || buf.isEncoding_) 
        {
            continue;
        }

        VkResult result = vkWaitForFences(immediate->device, 1, &buf.fence_, VK_TRUE, 0);

        if (result == VK_SUCCESS) 
        {
            VK_ASSERT(vkResetCommandBuffer(buf.cmdBuf_, VkCommandBufferResetFlags{0}));
            VK_ASSERT(vkResetFences(immediate->device, 1, &buf.fence_));
            buf.cmdBuf_ = VK_NULL_HANDLE;
            immediate->numAvailableCommandBuffers_++;
        } 
        else 
        {
            if (result != VK_TIMEOUT) 
            {
                VK_ASSERT(result);
            }
        }
    }
}

internal VkSemaphore
vulkan_immediate_commands_acquire_last_submit_semaphore(VulkanImmediateCommands *immediate)
{
    VkSemaphore result = immediate->lastSubmitSemaphore_.semaphore;
    immediate->lastSubmitSemaphore_.semaphore = VK_NULL_HANDLE;
    return result;
}
internal void
vulkan_immediate_commands_signal_semaphore(VulkanImmediateCommands *immediate, VkSemaphore semaphore, u64 signal_value)
{
    assert(immediate->signalSemaphore_.semaphore == VK_NULL_HANDLE);
    immediate->signalSemaphore_.semaphore = semaphore;
    immediate->signalSemaphore_.value = signal_value;
}

internal CommandBufferWrapper *
vulkan_immediate_commands_acquire(VulkanImmediateCommands *immediate)
{
    if(!immediate->numAvailableCommandBuffers_)
    {
        vulkan_immediate_commands_purge(immediate);
    }

    while(!immediate->numAvailableCommandBuffers_)
    {
        printf("waiting for command buffers...\n");
        vulkan_immediate_commands_purge(immediate);
    }

      CommandBufferWrapper* current = nullptr;

    // we are ok with any available buffer
    //for (CommandBufferWrapper& buf : immediate->buffers) {
    for (u32 i = 0; i != immediate->kMaxCommandBuffers; i++) {
        if (immediate->buffers[i].cmdBuf_ == VK_NULL_HANDLE) 
        {
            current = &immediate->buffers[i];
            break;
        }
    }

    // make clang happy
    assert(current);

    gui_assert(immediate->numAvailableCommandBuffers_, "No available command buffers");
    gui_assert(current, "No available command buffers");
    assert(current->cmdBufAllocated_ != VK_NULL_HANDLE);

    current->handle_.submitId_ = immediate->submitCounter_;
    immediate->numAvailableCommandBuffers_--;

    current->cmdBuf_ = current->cmdBufAllocated_;
    current->isEncoding_ = true;
    VkCommandBufferBeginInfo bi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_ASSERT(vkBeginCommandBuffer(current->cmdBuf_, &bi));

    immediate->nextSubmitHandle_ = current->handle_;

    return current;
}



internal void
vulkan_immediate_commands_wait_semaphore(VulkanImmediateCommands *immediate, VkSemaphore semaphore)
{
    assert(immediate->waitSemaphore_.semaphore == VK_NULL_HANDLE);
    immediate->waitSemaphore_.semaphore = semaphore;
}

internal SubmitHandle
vulkan_immediate_commands_submit(VulkanImmediateCommands *immediate, CommandBufferWrapper *wrapper)
{
    //LVK_PROFILER_FUNCTION_COLOR(LVK_PROFILER_COLOR_SUBMIT);
    assert(wrapper->isEncoding_);
    VK_ASSERT(vkEndCommandBuffer(wrapper->cmdBuf_));

    VkSemaphoreSubmitInfo waitSemaphores[] = {{}, {}};
    u32 numWaitSemaphores = 0;
    if (immediate->waitSemaphore_.semaphore) {
        waitSemaphores[numWaitSemaphores++] = immediate->waitSemaphore_;
    }
    if (immediate->lastSubmitSemaphore_.semaphore) {
        waitSemaphores[numWaitSemaphores++] = immediate->lastSubmitSemaphore_;
    }
    VkSemaphoreSubmitInfo signalSemaphores[] = {
        VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                .semaphore = wrapper->semaphore_,
                                .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT},
        {},
    };
    u32 numSignalSemaphores = 1;
    if (immediate->signalSemaphore_.semaphore) {
        signalSemaphores[numSignalSemaphores++] = immediate->signalSemaphore_;
    }

    //LVK_PROFILER_ZONE("vkQueueSubmit2()", LVK_PROFILER_COLOR_SUBMIT);
    #if LVK_VULKAN_PRINT_COMMANDS
    //LLOGL("%p vkQueueSubmit2()\n\n", wrapper.cmdBuf_);
    #endif // LVK_VULKAN_PRINT_COMMANDS
    VkCommandBufferSubmitInfo bufferSI = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = wrapper->cmdBuf_,
    };
    VkSubmitInfo2 si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = numWaitSemaphores,
        .pWaitSemaphoreInfos = waitSemaphores,
        .commandBufferInfoCount = 1u,
        .pCommandBufferInfos = &bufferSI,
        .signalSemaphoreInfoCount = numSignalSemaphores,
        .pSignalSemaphoreInfos = signalSemaphores,
    };
    VK_ASSERT(vkQueueSubmit2(immediate->queue, 1u, &si, wrapper->fence_));
    //LVK_PROFILER_ZONE_END();

    immediate->lastSubmitSemaphore_.semaphore = wrapper->semaphore_;
    immediate->lastSubmitHandle_ = wrapper->handle_;
    immediate->waitSemaphore_.semaphore = VK_NULL_HANDLE;
    immediate->signalSemaphore_.semaphore = VK_NULL_HANDLE;

    // reset
    //const_cast<CommandBufferWrapper&>(wrapper).isEncoding_ = false;
    wrapper->isEncoding_ = false;
    immediate->submitCounter_++;

    if (!immediate->submitCounter_) {
        // skip the 0 value - when u32 wraps around (null SubmitHandle)
        immediate->submitCounter_++;
    }

    return immediate->lastSubmitHandle_;
}









void vulkan_swapchain_present(VulkanSwapchain *swapchain, VkSemaphore wait_semaphore)
{
    //LVK_PROFILER_FUNCTION();

    //LVK_PROFILER_ZONE("vkQueuePresent()", LVK_PROFILER_COLOR_PRESENT);
    VkPresentInfoKHR pi = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &wait_semaphore,
        .swapchainCount = 1u,
        .pSwapchains = &swapchain->swapchain_,
        .pImageIndices = &swapchain->currentImageIndex_,
    };
    VkResult r = vkQueuePresentKHR(swapchain->graphicsQueue_, &pi);
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR && r != VK_ERROR_OUT_OF_DATE_KHR) 
    {
        VK_ASSERT(r);
    }
    //LVK_PROFILER_ZONE_END();

    // Ready to call acquireNextImage() on the next getCurrentVulkanTexture();
    swapchain->getNextImage_ = true;
    swapchain->currentFrameIndex_++;

    //LVK_PROFILER_FRAME(nullptr);
}




TextureHandle vulkan_swapchain_get_current_texture(VulkanSwapchain *swapchain)
{
    //LVK_PROFILER_FUNCTION();

    if (swapchain->getNextImage_) {
        VkSemaphoreWaitInfo waitInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores = &swapchain->ctx_->timelineSemaphore_,
            .pValues = &swapchain->timelineWaitValues_[swapchain->currentImageIndex_],
        };
        VK_ASSERT(vkWaitSemaphores(swapchain->device_, &waitInfo, UINT64_MAX));
        // when timeout is set to UINT64_MAX, we wait until the next image has been acquired
        VkSemaphore acquireSemaphore = swapchain->acquireSemaphore_[swapchain->currentImageIndex_];
        VkResult r = vkAcquireNextImageKHR(swapchain->device_, swapchain->swapchain_, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &swapchain->currentImageIndex_);
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR && r != VK_ERROR_OUT_OF_DATE_KHR) 
        {
            VK_ASSERT(r);
        }
        swapchain->getNextImage_ = false;
        //swapchain->ctx_->immediate_->waitSemaphore(acquireSemaphore);
        vulkan_immediate_commands_wait_semaphore(swapchain->ctx_->immediate_, acquireSemaphore);
    }

    //if (LVK_VERIFY(swapchain->currentImageIndex_ < swapchain->numSwapchainImages_)) {
    if (swapchain->currentImageIndex_ < swapchain->numSwapchainImages_) {
        return swapchain->swapchainTextures_[swapchain->currentImageIndex_];
    }

    return {};
}

// weird names between these two: `vulkan_swapchain_get_current_texture` and `vulkan_context_get_current_swapchain_texture`
TextureHandle vulkan_context_get_current_swapchain_texture(VulkanContext *ctx)
{
    // fucking complicated! i thought it was easier than that!
    // return swapchain->swapchainTextures_[swapchain->currentImageIndex];
    //LVK_PROFILER_FUNCTION();

    //if (!hasSwapchain()) {
    if (!ctx->swapchain_) {
        return {};
    }

    //TextureHandle tex = ctx->swapchain_->getCurrentTexture();
    TextureHandle tex = vulkan_swapchain_get_current_texture(ctx->swapchain_);
    //if (!LVK_VERIFY(tex.valid())) {
    if (!handle_is_valid(tex)) {
        gui_assert(false, "Swapchain has no valid texture");
        return {};
    }

    //LVK_ASSERT_MSG(texturesPool_.get(tex)->vkImageFormat_ != VK_FORMAT_UNDEFINED, "Invalid image format");
    gui_assert(pool_get(&ctx->texturesPool_, tex)->vkImageFormat_ != VK_FORMAT_UNDEFINED, "Invalid image format");

    return tex;
}

internal VkPhysicalDeviceProperties
vulkan_get_physical_device_props(VulkanContext *context)
{
    // getVkPhysicalDeviceProperties()
    return context->vkPhysicalDeviceProperties2.properties;
}

internal VkImageView
vulkan_image_view_create(VulkanImage *image,
    VkDevice device,
    VkImageViewType type,
    VkFormat format,
    VkImageAspectFlags aspectMask,
                        u32 baseLevel,
                        u32 numLevels,
                        u32 baseLayer,
                        u32 numLayers,
                        VkComponentMapping mapping,
                        VkSamplerYcbcrConversionInfo* ycbcr,
                        const char* debugName
                    )
{
    VkImageViewCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = ycbcr,
        .image = image->vkImage_,
        .viewType = type,
        .format = format,
        .components = mapping,
        .subresourceRange = {aspectMask, baseLevel, numLevels ? numLevels : image->numLevels_, baseLayer, numLayers},
    };
    VkImageView vkView = VK_NULL_HANDLE;
    VK_ASSERT(vkCreateImageView(device, &ci, nullptr, &vkView));
    VK_ASSERT(setDebugObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)vkView, debugName));

    return vkView;
}

internal VkFormat
formatToVkFormat(Format format)
{
  using TextureFormat = Format;
  switch (format) {
  case Format_Invalid:
    return VK_FORMAT_UNDEFINED;
  case Format_R_UN8:
    return VK_FORMAT_R8_UNORM;
  case Format_R_UN16:
    return VK_FORMAT_R16_UNORM;
  case Format_R_F16:
    return VK_FORMAT_R16_SFLOAT;
  case Format_R_UI16:
    return VK_FORMAT_R16_UINT;
  case Format_R_UI32:
    return VK_FORMAT_R32_UINT;
  case Format_RG_UN8:
    return VK_FORMAT_R8G8_UNORM;
  case Format_RG_UI16:
    return VK_FORMAT_R16G16_UINT;
  case Format_RG_UI32:
    return VK_FORMAT_R32G32_UINT;
  case Format_RG_UN16:
    return VK_FORMAT_R16G16_UNORM;
  case Format_BGRA_UN8:
    return VK_FORMAT_B8G8R8A8_UNORM;
  case Format_RGBA_UN8:
    return VK_FORMAT_R8G8B8A8_UNORM;
  case Format_RGBA_SRGB8:
    return VK_FORMAT_R8G8B8A8_SRGB;
  case Format_BGRA_SRGB8:
    return VK_FORMAT_B8G8R8A8_SRGB;
  case Format_RG_F16:
    return VK_FORMAT_R16G16_SFLOAT;
  case Format_RG_F32:
    return VK_FORMAT_R32G32_SFLOAT;
  case Format_R_F32:
    return VK_FORMAT_R32_SFLOAT;
  case Format_RGBA_F16:
    return VK_FORMAT_R16G16B16A16_SFLOAT;
  case Format_RGBA_UI32:
    return VK_FORMAT_R32G32B32A32_UINT;
  case Format_RGBA_F32:
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  case Format_ETC2_RGB8:
    return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
  case Format_ETC2_SRGB8:
    return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
  case Format_BC7_RGBA:
    return VK_FORMAT_BC7_UNORM_BLOCK;
  case Format_Z_UN16:
    return VK_FORMAT_D16_UNORM;
  case Format_Z_UN24:
    return VK_FORMAT_D24_UNORM_S8_UINT;
  case Format_Z_F32:
    return VK_FORMAT_D32_SFLOAT;
  case Format_Z_UN24_S_UI8:
    return VK_FORMAT_D24_UNORM_S8_UINT;
  case Format_Z_F32_S_UI8:
    return VK_FORMAT_D32_SFLOAT_S8_UINT;
  case Format_YUV_NV12:
    return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
  case Format_YUV_420p:
    return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    default: 
    return VK_FORMAT_UNDEFINED;
  }

}

enum SamplerFilter : u8 { SamplerFilter_Nearest = 0, SamplerFilter_Linear };
enum SamplerMip : u8 { SamplerMip_Disabled = 0, SamplerMip_Nearest, SamplerMip_Linear };
enum SamplerWrap : u8 { SamplerWrap_Repeat = 0, SamplerWrap_Clamp, SamplerWrap_MirrorRepeat };


VkFilter samplerFilterToVkFilter(SamplerFilter filter) {
    switch (filter) 
    {
        case SamplerFilter_Nearest:
            return VK_FILTER_NEAREST;
        case SamplerFilter_Linear:
            return VK_FILTER_LINEAR;
    }
    gui_assert(false, "SamplerFilter value not handled: %d", (int)filter);
    return VK_FILTER_LINEAR;
}

VkSamplerMipmapMode samplerMipMapToVkSamplerMipmapMode(SamplerMip filter) {
    switch (filter) {
        case SamplerMip_Disabled:
        case SamplerMip_Nearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case SamplerMip_Linear:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    gui_assert(false, "SamplerMipMap value not handled: %d", (int)filter);
    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
}

VkSamplerAddressMode samplerWrapModeToVkSamplerAddressMode(SamplerWrap mode) {
    switch (mode) 
    {
        case SamplerWrap_Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case SamplerWrap_Clamp:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case SamplerWrap_MirrorRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
    gui_assert(false, "SamplerWrapMode value not handled: %d", (int)mode);
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}


struct SamplerStateDesc {
  SamplerFilter minFilter = SamplerFilter_Linear;
  SamplerFilter magFilter = SamplerFilter_Linear;
  SamplerMip mipMap = SamplerMip_Disabled;
  SamplerWrap wrapU = SamplerWrap_Repeat;
  SamplerWrap wrapV = SamplerWrap_Repeat;
  SamplerWrap wrapW = SamplerWrap_Repeat;
  CompareOp depthCompareOp = CompareOp_LessEqual;
  u8 mipLodMin = 0;
  u8 mipLodMax = 15;
  u8 maxAnisotropic = 1;
  b32 depthCompareEnabled = false;
  const char* debugName = "";
};

internal VkCompareOp
compareOpToVkCompareOp(CompareOp func)
{
 switch (func) 
 {
    case CompareOp_Never:
        return VK_COMPARE_OP_NEVER;
    case CompareOp_Less:
        return VK_COMPARE_OP_LESS;
    case CompareOp_Equal:
        return VK_COMPARE_OP_EQUAL;
    case CompareOp_LessEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp_Greater:
        return VK_COMPARE_OP_GREATER;
    case CompareOp_NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp_GreaterEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp_AlwaysPass:
        return VK_COMPARE_OP_ALWAYS;
 }
    gui_assert(false, "CompareFunction value not handled: %d", (int)func);
    return VK_COMPARE_OP_ALWAYS;
}

internal VkSamplerCreateInfo
samplerStateDescToVkSamplerCreateInfo(const SamplerStateDesc& desc, const VkPhysicalDeviceLimits& limits)
{
    gui_assert(desc.mipLodMax >= desc.mipLodMin,
               "mipLodMax (%d) must be greater than or equal to mipLodMin (%d)",
               (int)desc.mipLodMax,
               (int)desc.mipLodMin);

    VkSamplerCreateInfo ci = 
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = samplerFilterToVkFilter(desc.magFilter),
        .minFilter = samplerFilterToVkFilter(desc.minFilter),
        .mipmapMode = samplerMipMapToVkSamplerMipmapMode(desc.mipMap),
        .addressModeU = samplerWrapModeToVkSamplerAddressMode(desc.wrapU),
        .addressModeV = samplerWrapModeToVkSamplerAddressMode(desc.wrapV),
        .addressModeW = samplerWrapModeToVkSamplerAddressMode(desc.wrapW),
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = desc.depthCompareEnabled ? VK_TRUE : VK_FALSE,
        .compareOp = desc.depthCompareEnabled ? compareOpToVkCompareOp(desc.depthCompareOp) : VK_COMPARE_OP_ALWAYS,
        .minLod = float(desc.mipLodMin),
        .maxLod = desc.mipMap == SamplerMip_Disabled ? float(desc.mipLodMin) : float(desc.mipLodMax),
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if (desc.maxAnisotropic > 1) 
    {
        const bool isAnisotropicFilteringSupported = limits.maxSamplerAnisotropy > 1;
        gui_assert(isAnisotropicFilteringSupported, "Anisotropic filtering is not supported by the device.");
        ci.anisotropyEnable = isAnisotropicFilteringSupported ? VK_TRUE : VK_FALSE;

        if (limits.maxSamplerAnisotropy < desc.maxAnisotropic) 
        {
            printf( "Supplied sampler anisotropic value greater than max supported by the device, setting to %.0f\n", static_cast<double>(limits.maxSamplerAnisotropy));
        }
        ci.maxAnisotropy = min((float)limits.maxSamplerAnisotropy, (float)desc.maxAnisotropic);
    }
    return ci;
}


//TOOD see this cyclick thing!
internal SamplerHandle vulkan_sampler_create(VulkanContext *context, const VkSamplerCreateInfo& ci, Format yuvFormat, const char* debugName);

internal VkSamplerYcbcrConversionInfo *
getOrCreateYcbcrConversionInfo(VulkanContext *context, Format format)
{
    if (context->ycbcrConversionData_[format].info.sType) 
    {
        return &context->ycbcrConversionData_[format].info;
    }

    if (!(context->vkFeatures11.samplerYcbcrConversion)) 
    {
        gui_assert(false, "Ycbcr samplers are not supported");
        return nullptr;
    }

    const VkFormat vkFormat = formatToVkFormat(format);

    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(context->physical_device, vkFormat, &props);

    const bool cosited = (props.optimalTilingFeatures & VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT) != 0;
    const bool midpoint = (props.optimalTilingFeatures & VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT) != 0;

    if (!(cosited || midpoint)) 
    {
        gui_assert(cosited || midpoint, "Unsupported Ycbcr feature");
        return nullptr;
    }

    const VkSamplerYcbcrConversionCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
        .format = vkFormat,
        .ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709,
        .ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_FULL,
        .components =
            {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .xChromaOffset = midpoint ? VK_CHROMA_LOCATION_MIDPOINT : VK_CHROMA_LOCATION_COSITED_EVEN,
        .yChromaOffset = midpoint ? VK_CHROMA_LOCATION_MIDPOINT : VK_CHROMA_LOCATION_COSITED_EVEN,
        .chromaFilter = VK_FILTER_LINEAR,
        .forceExplicitReconstruction = VK_FALSE,
    };

    VkSamplerYcbcrConversionInfo info = 
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
        .pNext = nullptr,
    };
    vkCreateSamplerYcbcrConversion(context->device, &ci, nullptr, &info.conversion);

    // check properties
    VkSamplerYcbcrConversionImageFormatProperties samplerYcbcrConversionImageFormatProps = 
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES,
    };
    VkImageFormatProperties2 imageFormatProps = 
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
        .pNext = &samplerYcbcrConversionImageFormatProps,
    };
    VkPhysicalDeviceImageFormatInfo2 imageFormatInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
        .format = vkFormat,
        .type = VK_IMAGE_TYPE_2D,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags = VK_IMAGE_CREATE_DISJOINT_BIT,
    };
    vkGetPhysicalDeviceImageFormatProperties2(context->physical_device, &imageFormatInfo, &imageFormatProps);

    assert(samplerYcbcrConversionImageFormatProps.combinedImageSamplerDescriptorCount <= 3);

    VkSamplerCreateInfo cinfo = samplerStateDescToVkSamplerCreateInfo({}, vulkan_get_physical_device_props(context).limits);

    context->ycbcrConversionData_[format].info = info;
    // NOTE this is Holder<SamplerHandle>
    // IMPORTANT prestar atencion a esto es Holder
    // Supongo que si no borro esto igual el validation layer va a romper las bolas
    //context->ycbcrConversionData_[format].sampler = {context, vulkan_sampler_create(context, cinfo, format, "YUV sampler")};
    // Le saque el context por ahora
    context->ycbcrConversionData_[format].sampler = vulkan_sampler_create(context, cinfo, format, "YUV sampler");
    context->numYcbcrSamplers_++;
    context->awaitingNewImmutableSamplers_ = true;

    return &context->ycbcrConversionData_[format].info;
}

internal SamplerHandle
vulkan_sampler_create(VulkanContext *context, const VkSamplerCreateInfo& ci, Format yuvFormat, const char* debugName) 
{
    //LVK_PROFILER_FUNCTION_COLOR(LVK_PROFILER_COLOR_CREATE);
    VkSamplerCreateInfo cinfo = ci;

    if (yuvFormat != Format_Invalid) {
        cinfo.pNext = getOrCreateYcbcrConversionInfo(context, yuvFormat);
        // must be CLAMP_TO_EDGE
        // https://vulkan.lunarg.com/doc/view/1.3.268.0/windows/1.3-extensions/vkspec.html#VUID-VkSamplerCreateInfo-addressModeU-01646
        cinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        cinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        cinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        cinfo.anisotropyEnable = VK_FALSE;
        cinfo.unnormalizedCoordinates = VK_FALSE;
    }

    VkSampler sampler = VK_NULL_HANDLE;
    VK_ASSERT(vkCreateSampler(context->device, &cinfo, nullptr, &sampler));
    VK_ASSERT(setDebugObjectName(context->device, VK_OBJECT_TYPE_SAMPLER, (uint64_t)sampler, debugName));

    //Handle handle = context->samplersPool_.create(VkSampler(sampler));
    SamplerHandle handle = pool_create(&context->samplersPool_, VkSampler(sampler));

    context->awaitingCreation_ = true;

    return handle;
}







#if 0

its not yet used!!!!!!!!

internal VkSampler
getOrCreateYcbcrSampler(VulkanContext *context, Format format)
{
    const VkSamplerYcbcrConversionInfo* info = getOrCreateYcbcrConversionInfo(context, format);

    if (!info) {
        return VK_NULL_HANDLE;
    }

    return *samplersPool_.get(pimpl_->ycbcrConversionData_[format].sampler);
}
#endif

internal void
vulkan_staging_device_create(VulkanStagingDevice *staging_device, VulkanContext *context)
{
    VkPhysicalDeviceLimits limits = vulkan_get_physical_device_props(context).limits;

    // use default value of 128Mb clamped to the max limits
    // TODO see if this min works
    staging_device->maxBufferSize_ = min(limits.maxStorageBufferRange, 128u * 1024u * 1024u);

    assert(staging_device->minBufferSize_ <= staging_device->maxBufferSize_);
}


//void deferredTask(std::packaged_task<void()>&& task, SubmitHandle handle)  {
//    if (handle.empty()) {
//        handle = context->immediate_->nextSubmitHandle_;
//    }
//    context->deferredTasks_.emplace_back(std::move(task), handle);
//}

void growDescriptorPool(VulkanContext *context, u32 maxTextures, u32 maxSamplers, u32 maxAccelStructs) {
  context->currentMaxTextures_ = maxTextures;
  context->currentMaxSamplers_ = maxSamplers;
  context->currentMaxAccelStructs_ = maxAccelStructs;

#if LVK_VULKAN_PRINT_COMMANDS
  //LLOGL("growDescriptorPool(%u, %u)\n", maxTextures, maxSamplers);
#endif // LVK_VULKAN_PRINT_COMMANDS

  gui_assert(maxTextures <= context->vkPhysicalDeviceVulkan12Properties.maxDescriptorSetUpdateAfterBindSampledImages,
     "Max Textures exceeded: %u (max %u)", maxTextures, context->vkPhysicalDeviceVulkan12Properties.maxDescriptorSetUpdateAfterBindSampledImages);

#if 0
  if (!LVK_VERIFY(maxSamplers <= context->vkPhysicalDeviceVulkan12Properties.maxDescriptorSetUpdateAfterBindSamplers)) {
    LLOGW("Max Samplers exceeded %u (max %u)", maxSamplers, context->vkPhysicalDeviceVulkan12Properties.maxDescriptorSetUpdateAfterBindSamplers);
  }
#endif


  // IMPORTANT these two uses of the function defferedTask is not used at least not even in the HelloTriangle!
  if (context->vkDSL_ != VK_NULL_HANDLE) {
    //deferredTask(std::packaged_task<void()>([device = context->device, dsl = context->vkDSL_]() { vkDestroyDescriptorSetLayout(device, dsl, nullptr); }));
  }
  if (context->vkDPool_ != VK_NULL_HANDLE) {
    //deferredTask(std::packaged_task<void()>([device = context->device, dp = context->vkDPool_]() { vkDestroyDescriptorPool(device, dp, nullptr); }));
  }

  bool hasYUVImages = false;

/* IMPORTANT Is not in:
  - Swapchain
  - HelloTriangle

  Also I did some modifications in the code to be able to enclose all in and #if 0 block. Be careful!
*/
std::vector<VkSampler> immutableSamplers;
const VkSampler* immutableSamplersData = nullptr;
#if 0
  // check if we have any YUV images
  for (const auto& obj : texturesPool_.objects_) {
    const VulkanImage* img = &obj.obj_;
    // multisampled images cannot be directly accessed from shaders
    const bool isTextureAvailable = (img->vkSamples_ & VK_SAMPLE_COUNT_1_BIT) == VK_SAMPLE_COUNT_1_BIT;
    hasYUVImages = isTextureAvailable && img->isSampledImage() && getNumImagePlanes(img->vkImageFormat_) > 1;
    if (hasYUVImages) {
      break;
    }
  }


  if (hasYUVImages) {
    VkSampler dummySampler = samplersPool_.objects_[0].obj_;
    immutableSamplers.reserve(texturesPool_.objects_.size());
    for (const auto& obj : texturesPool_.objects_) {
      const VulkanImage* img = &obj.obj_;
      // multisampled images cannot be directly accessed from shaders
      const bool isTextureAvailable = (img->vkSamples_ & VK_SAMPLE_COUNT_1_BIT) == VK_SAMPLE_COUNT_1_BIT;
      const bool isYUVImage = isTextureAvailable && img->isSampledImage() && getNumImagePlanes(img->vkImageFormat_) > 1;
      immutableSamplers.push_back(isYUVImage ? getOrCreateYcbcrSampler(vkFormatToFormat(img->vkImageFormat_)) : dummySampler);
    }
    immutableSamplersData = immutableSamplers.data();
  }
  #endif

  // create default descriptor set layout which is going to be shared by graphics pipelines
  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
                                  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
  if (hasRayTracingPipeline_) {
    stageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  }
  const VkDescriptorSetLayoutBinding bindings[kBinding_NumBindings] = {
      getDSLBinding(kBinding_Textures, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTextures, stageFlags),
      getDSLBinding(kBinding_Samplers, VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers, stageFlags),
      getDSLBinding(kBinding_StorageImages, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxTextures, stageFlags),
      getDSLBinding(
          kBinding_YUVImages, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, immutableSamplers.size(), stageFlags, immutableSamplersData),
      getDSLBinding(kBinding_AccelerationStructures, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, maxAccelStructs, stageFlags),
  };
  const u32 flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
                         VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
  VkDescriptorBindingFlags bindingFlags[kBinding_NumBindings];
  for (int i = 0; i < kBinding_NumBindings; ++i) {
    bindingFlags[i] = flags;
  }
  const VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlagsCI = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
      .bindingCount = u32(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1),
      .pBindingFlags = bindingFlags,
  };
  const VkDescriptorSetLayoutCreateInfo dslci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = &setLayoutBindingFlagsCI,
      .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
      .bindingCount = u32(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1),
      .pBindings = bindings,
  };
  VK_ASSERT(vkCreateDescriptorSetLayout(context->device, &dslci, nullptr, &context->vkDSL_));
  VK_ASSERT(setDebugObjectName(
      context->device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)context->vkDSL_, "Descriptor Set Layout: VulkanContext::vkDSL_"));

  {
    // create default descriptor pool and allocate 1 descriptor set
    const VkDescriptorPoolSize poolSizes[kBinding_NumBindings]{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTextures},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxTextures},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, maxAccelStructs},
    };
    const VkDescriptorPoolCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 1,
        .poolSizeCount = u32(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1),
        .pPoolSizes = poolSizes,
    };
    VK_ASSERT_RETURN(vkCreateDescriptorPool(context->device, &ci, nullptr, &context->vkDPool_));
    const VkDescriptorSetAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = context->vkDPool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &context->vkDSL_,
    };
    VK_ASSERT_RETURN(vkAllocateDescriptorSets(context->device, &ai, &context->vkDSet_));
  }

  context->awaitingNewImmutableSamplers_ = false;
}

enum ColorSpace : u8 {
  ColorSpace_SRGB_LINEAR,
  ColorSpace_SRGB_NONLINEAR,
};


enum TextureType : u8 {
  TextureType_2D,
  TextureType_3D,
  TextureType_Cube,
};

enum Swizzle : u8 {
  Swizzle_Default = 0,
  Swizzle_0,
  Swizzle_1,
  Swizzle_R,
  Swizzle_G,
  Swizzle_B,
  Swizzle_A,
};


struct ComponentMapping {
  Swizzle r = Swizzle_Default;
  Swizzle g = Swizzle_Default;
  Swizzle b = Swizzle_Default;
  Swizzle a = Swizzle_Default;
  bool identity() const {
    return r == Swizzle_Default && g == Swizzle_Default && b == Swizzle_Default && a == Swizzle_Default;
  }
};

enum StorageType {
  StorageType_Device,
  StorageType_HostVisible,
  StorageType_Memoryless
};

struct Dimensions
{
    u32 width = 1;
    u32 height = 1;
    u32 depth = 1;
    inline Dimensions divide1D(u32 v) const {
        return {.width = width / v, .height = height, .depth = depth};
    }
    inline Dimensions divide2D(u32 v) const {
        return {.width = width / v, .height = height / v, .depth = depth};
    }
    inline Dimensions divide3D(u32 v) const {
        return {.width = width / v, .height = height / v, .depth = depth / v};
    }
    inline bool operator==(const Dimensions& other) const {
        return width == other.width && height == other.height && depth == other.depth;
    }

};

struct TextureDesc
{
  TextureType type = TextureType_2D;
  Format format = Format_Invalid;

  Dimensions dimensions = {1, 1, 1};
  u32 numLayers = 1;
  u32 numSamples = 1;
  u8 usage = TextureUsageBits_Sampled;
  u32 numMipLevels = 1;
  StorageType storage = StorageType_Device;
  ComponentMapping swizzle = {};
  const void* data = nullptr;
  u32 dataNumMipLevels = 1; // how many mip-levels we want to upload
  bool generateMipmaps = false; // generate mip-levels immediately, valid only with non-null data
  const char* debugName = "";
};


u32 getNumImagePlanes(Format format) {
    switch (format) {
  case VK_FORMAT_UNDEFINED:
    return 0;
  case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
  case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
  case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
  case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
  case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
    return 3;
  case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
  case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
  case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
  case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
    return 2;
  default:
    return 1;
  }
}

internal VkMemoryPropertyFlags
storageTypeToVkMemoryPropertyFlags(StorageType storage)
{
    VkMemoryPropertyFlags memFlags{0};

    switch (storage) {
    case StorageType_Device:
        memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case StorageType_HostVisible:
        memFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case StorageType_Memoryless:
        memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        break;
    }
    return memFlags;
}


internal VkSampleCountFlagBits
getVulkanSampleCountFlags(u32 numSamples, VkSampleCountFlags maxSamplesMask) {
  if (numSamples <= 1 || VK_SAMPLE_COUNT_2_BIT > maxSamplesMask) {
    return VK_SAMPLE_COUNT_1_BIT;
  }
  if (numSamples <= 2 || VK_SAMPLE_COUNT_4_BIT > maxSamplesMask) {
    return VK_SAMPLE_COUNT_2_BIT;
  }
  if (numSamples <= 4 || VK_SAMPLE_COUNT_8_BIT > maxSamplesMask) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (numSamples <= 8 || VK_SAMPLE_COUNT_16_BIT > maxSamplesMask) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (numSamples <= 16 || VK_SAMPLE_COUNT_32_BIT > maxSamplesMask) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (numSamples <= 32 || VK_SAMPLE_COUNT_64_BIT > maxSamplesMask) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  return VK_SAMPLE_COUNT_64_BIT;
}


internal u32
getFramebufferMSAABitMask(VulkanContext *context)
{
    // NOTE it was a reference
    //VkPhysicalDeviceLimits& limits = vulkan_get_physical_device_props().limits;
    VkPhysicalDeviceLimits limits = vulkan_get_physical_device_props(context).limits;
    return limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
}


internal b32
isDepthOrStencilFormat(Format format)
{
    return properties[format].depth || properties[format].stencil;

}

internal b32
validateImageLimits(VkImageType imageType, VkSampleCountFlagBits samples, VkExtent3D extent, VkPhysicalDeviceLimits limits)
{
    if (samples != VK_SAMPLE_COUNT_1_BIT && !(imageType == VK_IMAGE_TYPE_2D)) {
        printf("Multisampling is supported only for 2D images\n");
        return false;
    }

    if (imageType == VK_IMAGE_TYPE_2D &&
        !(extent.width <= limits.maxImageDimension2D && extent.height <= limits.maxImageDimension2D)) {
        printf("2D texture size exceeded\n");
        return false;
    }
    if (imageType == VK_IMAGE_TYPE_3D &&
        !(extent.width <= limits.maxImageDimension3D && extent.height <= limits.maxImageDimension3D &&
                    extent.depth <= limits.maxImageDimension3D)) {
        printf("3D texture size exceeded\n");
        return false;
    }

    return true;
}

internal u32
findMemoryType(VkPhysicalDevice physDev, u32 memoryTypeBits, VkMemoryPropertyFlags flags)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physDev, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        const bool hasProperties = (memProperties.memoryTypes[i].propertyFlags & flags) == flags;
        if ((memoryTypeBits & (1 << i)) && hasProperties) {
        return i;
        }
    }

    assert(false);

    return 0;

}

VkResult allocateMemory2(VkPhysicalDevice physDev, VkDevice device, const VkMemoryRequirements2* memRequirements, VkMemoryPropertyFlags props, VkDeviceMemory* outMemory) 
{
  assert(memRequirements);

  const VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
      .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
  };
  const VkMemoryAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = memRequirements->memoryRequirements.size,
      .memoryTypeIndex = findMemoryType(physDev, memRequirements->memoryRequirements.memoryTypeBits, props),
  };

  return vkAllocateMemory(device, &ai, NULL, outMemory);
}


internal VkBindImageMemoryInfo
getBindImageMemoryInfo(const VkBindImagePlaneMemoryInfo* next, VkImage image, VkDeviceMemory memory) {
  return VkBindImageMemoryInfo{
      .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
      .pNext = next,
      .image = image,
      .memory = memory,
      .memoryOffset = 0,
  };
}

internal void
generateMipmap(VulkanContext *context, TextureHandle handle)
{
    if(handle_is_empty(handle))
    {
        return;
    }

    //VulkanImage* tex = context->texturesPool_.get(handle);
    VulkanImage* tex = pool_get(&context->texturesPool_, handle);
    if(tex->numLevels_ <= 1)
    {
        return;
    }
    assert(tex->vkImageLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);
    CommandBufferWrapper *wrapper = vulkan_immediate_commands_acquire(context->immediate_);
    vulkan_image_generate_mipmap(tex, wrapper->cmdBuf_);
    //context->immediate_->submit(wrapper);
    vulkan_immediate_commands_submit(context->immediate_, wrapper);
}

internal u32
calcNumMipLevels(u32 width, u32 height)
{
    u32 levels = 1;
    while((width | height) >> 1)
    {
        levels++;
    }
    return levels;
}

internal std::vector<VkFormat>
getCompatibleDepthStencilFormats(Format format)
{
    switch (format) {
    case Format_Z_UN16:
        return {VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT};
    case Format_Z_UN24:
        return {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM_S8_UINT};
    case Format_Z_F32:
        return {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    case Format_Z_UN24_S_UI8:
        return {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT};
    case Format_Z_F32_S_UI8:
        return {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT};
    default:
        return {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT};
    }
    return {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT};
}

internal VkFormat
getClosestDepthStencilFormat(Format desiredFormat, VulkanContext *context)
{
    // TODO dont send the context here 
    // get a list of compatible depth formats for a given desired format
    // The list will contain depth format that are ordered from most to least closest
    const std::vector<VkFormat> compatibleDepthStencilFormatList = getCompatibleDepthStencilFormats(desiredFormat);

    // Generate a set of device supported formats
    std::set<VkFormat> availableFormats;
    for (VkFormat format : context->deviceDepthFormats_) {
        availableFormats.insert(format);
    }

    // check if any of the format in compatible list is supported
    for (VkFormat depthStencilFormat : compatibleDepthStencilFormatList) {
        if (availableFormats.count(depthStencilFormat) != 0) {
        return depthStencilFormat;
        }
    }

    // no matching found, choose the first supported format
    return !context->deviceDepthFormats_.empty() ? context->deviceDepthFormats_[0] : VK_FORMAT_D24_UNORM_S8_UINT;
}


#if 0
internal void
vulkan_staging_device()
{

}

// in createBuffer and createTexture
internal void
vulkan_upload(VulkanContext *context, Handle buffer_handle, const void *data, size_t size, size_t offset)
{
    b32 result = true;
    if (!(data)) {
        result = true;
    }

    gui_assert(size, "Data size should be non-zero");

    lvk::VulkanBuffer* buf = buffersPool_.get(handle);

    if (!(buf)) {
        result = true;
    }

    if (!(offset + size <= buf->bufferSize_)) {
        result = false;
    }

    context->stagingDevice_->bufferSubData(*buf, offset, size, data);

    return lvk::Result();
}
#endif

// NOTE replaced by just a Handle
//internal Holder<TextureHandle> 
internal TextureHandle
vulkan_create_texture(VulkanContext *context, const TextureDesc& requestedDesc, const char* debugName) 
{
    //LVK_PROFILER_FUNCTION_COLOR(LVK_PROFILER_COLOR_CREATE);

    TextureDesc desc(requestedDesc);

    if (debugName && *debugName) {
        desc.debugName = debugName;
    }

     VkFormat vkFormat = isDepthOrStencilFormat(desc.format) ? getClosestDepthStencilFormat(desc.format, context)
                                                                     : formatToVkFormat(desc.format);

    

    //assert_msg(vkFormat != VK_FORMAT_UNDEFINED, "Invalid VkFormat value");

    TextureType type = desc.type;
    if (!(type == TextureType_2D || type == TextureType_Cube || type == TextureType_3D)) {
        printf("Only 2D, 3D and Cube textures are supported");
        return {};
    }

    if (desc.numMipLevels == 0) {
        printf("The number of mip levels specified must be greater than 0");
        desc.numMipLevels = 1;
    }

    if (desc.numSamples > 1 && desc.numMipLevels != 1) {
        printf("The number of mip levels for multisampled images should be 1");
        return {};
    }

    if (desc.numSamples > 1 && type == TextureType_3D) {
        printf("Multisampled 3D images are not supported");
        return {};
    }

    if (!(desc.numMipLevels <= calcNumMipLevels(desc.dimensions.width, desc.dimensions.height))) {
        printf("The number of specified mip-levels is greater than the maximum possible number of mip-levels.");
        return {};
    }

    if (desc.usage == 0) {
        printf("Texture usage flags are not set");
        desc.usage = TextureUsageBits_Sampled;
    }

    /* Use staging device to transfer data into the image when the storage is private to the device */
    VkImageUsageFlags usageFlags = (desc.storage == StorageType_Device) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;

    if (desc.usage & TextureUsageBits_Sampled) {
        usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.usage & TextureUsageBits_Storage) {
        if(desc.numSamples > 1) printf("Storage images cannot be multisampled\n");
        usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.usage & TextureUsageBits_Attachment) {
        usageFlags |= isDepthOrStencilFormat(desc.format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                                            : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (desc.storage == StorageType_Memoryless) {
        usageFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        }
    }

    if (desc.storage != StorageType_Memoryless) {
        // For now, always set this flag so we can read it back
        usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    //assert(usageFlags != 0, "Invalid usage flags");

    const VkMemoryPropertyFlags memFlags = storageTypeToVkMemoryPropertyFlags(desc.storage);

    const bool hasDebugName = desc.debugName && *desc.debugName;

    char debugNameImage[256] = {0};
    char debugNameImageView[256] = {0};

    if (hasDebugName) {
        snprintf(debugNameImage, sizeof(debugNameImage) - 1, "Image: %s", desc.debugName);
        snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: %s", desc.debugName);
    }

    VkImageCreateFlags vkCreateFlags = 0;
    VkImageViewType vkImageViewType;
    VkImageType vkImageType;
    VkSampleCountFlagBits vkSamples = VK_SAMPLE_COUNT_1_BIT;
    u32 numLayers = desc.numLayers;
    switch (desc.type) {
    case TextureType_2D:
        vkImageViewType = numLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        vkImageType = VK_IMAGE_TYPE_2D;
        vkSamples = getVulkanSampleCountFlags(desc.numSamples, getFramebufferMSAABitMask(context));
        break;
    case TextureType_3D:
        vkImageViewType = VK_IMAGE_VIEW_TYPE_3D;
        vkImageType = VK_IMAGE_TYPE_3D;
        break;
    case TextureType_Cube:
        vkImageViewType = numLayers > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        vkImageType = VK_IMAGE_TYPE_2D;
        vkCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        numLayers *= 6;
        break;
    default:
        printf("Code should NOT be reached\n");
        // Unreacheable();
        //Result::setResult(outResult, Result::Code::RuntimeError, "Unsupported texture type");
        return {};
    }

    VkExtent3D vkExtent{desc.dimensions.width, desc.dimensions.height, desc.dimensions.depth};
    u32 numLevels = desc.numMipLevels;

    if (!(validateImageLimits(vkImageType, vkSamples, vkExtent, vulkan_get_physical_device_props(context).limits))) {
        return {};
    }

    gui_assert(numLevels > 0, "The image must contain at least one mip-level");
    gui_assert(numLayers > 0, "The image must contain at least one layer");
    gui_assert(vkSamples > 0, "The image must contain at least one sample");
    assert(vkExtent.width > 0);
    assert(vkExtent.height > 0);
    assert(vkExtent.depth > 0);

    VulkanImage image = {
        .vkUsageFlags_ = usageFlags,
        .vkExtent_ = vkExtent,
        .vkType_ = vkImageType,
        .vkImageFormat_ = vkFormat,
        .vkSamples_ = vkSamples,
        .numLevels_ = numLevels,
        .numLayers_ = numLayers,
        .isDepthFormat_ = vulkan_image_is_depth_format(vkFormat),
        .isStencilFormat_ = vulkan_image_is_stencil_format(vkFormat),
    };

    if (hasDebugName) {
        // store debug name
        snprintf(image.debugName_, sizeof(image.debugName_) - 1, "%s", desc.debugName);
    }

    const u32 numPlanes = getNumImagePlanes(desc.format);
    const bool isDisjoint = numPlanes > 1;


    if (isDisjoint) {
        // some constraints for multiplanar image formats
        assert(vkImageType == VK_IMAGE_TYPE_2D);
        assert(vkSamples == VK_SAMPLE_COUNT_1_BIT);
        assert(numLayers == 1);
        assert(numLevels == 1);
        vkCreateFlags |= VK_IMAGE_CREATE_DISJOINT_BIT | VK_IMAGE_CREATE_ALIAS_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
        context->awaitingNewImmutableSamplers_ = true;
    }

    const VkImageCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = vkCreateFlags,
        .imageType = vkImageType,
        .format = vkFormat,
        .extent = vkExtent,
        .mipLevels = numLevels,
        .arrayLayers = numLayers,
        .samples = vkSamples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (LVK_VULKAN_USE_VMA && numPlanes == 1) {
        VmaAllocationCreateInfo vmaAllocInfo = {
            .usage = memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_AUTO,
        };

        VkResult result = vmaCreateImage(context->vma, &ci, &vmaAllocInfo, &image.vkImage_, &image.vmaAllocation_, nullptr);

        if (!(result == VK_SUCCESS)) 
        {
            printf("Failed: error result: %d, memflags: %d,  imageformat: %d\n", result, memFlags, image.vkImageFormat_);
            return {};
        }

        // handle memory-mapped buffers
        if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        vmaMapMemory(context->vma, image.vmaAllocation_, &image.mappedPtr_);
        }
    } else {
        // create image
        VK_ASSERT(vkCreateImage(context->device, &ci, nullptr, &image.vkImage_));

        // back the image with some memory
        constexpr u32 kNumMaxImagePlanes = array_count(image.vkMemory_);

        VkMemoryRequirements2 memRequirements[kNumMaxImagePlanes] = {
            {.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2},
            {.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2},
            {.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2},
        };

        const VkImagePlaneMemoryRequirementsInfo planes[kNumMaxImagePlanes] = {
            {.sType = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO, .planeAspect = VK_IMAGE_ASPECT_PLANE_0_BIT},
            {.sType = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO, .planeAspect = VK_IMAGE_ASPECT_PLANE_1_BIT},
            {.sType = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO, .planeAspect = VK_IMAGE_ASPECT_PLANE_2_BIT},
        };

        const VkImage img = image.vkImage_;

        const VkImageMemoryRequirementsInfo2 imgRequirements[kNumMaxImagePlanes] = {
            {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, .pNext = numPlanes > 0 ? &planes[0] : nullptr, .image = img},
            {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, .pNext = numPlanes > 1 ? &planes[1] : nullptr, .image = img},
            {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, .pNext = numPlanes > 2 ? &planes[2] : nullptr, .image = img},
        };

        for (u32 p = 0; p != numPlanes; p++) {
        vkGetImageMemoryRequirements2(context->device, &imgRequirements[p], &memRequirements[p]);
        VK_ASSERT(allocateMemory2(context->physical_device, context->device, &memRequirements[p], memFlags, &image.vkMemory_[p]));
        }

        const VkBindImagePlaneMemoryInfo bindImagePlaneMemoryInfo[kNumMaxImagePlanes] = {
            {VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO, nullptr, VK_IMAGE_ASPECT_PLANE_0_BIT},
            {VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO, nullptr, VK_IMAGE_ASPECT_PLANE_1_BIT},
            {VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO, nullptr, VK_IMAGE_ASPECT_PLANE_2_BIT},
        };
        const VkBindImageMemoryInfo bindInfo[kNumMaxImagePlanes] = {
            getBindImageMemoryInfo(isDisjoint ? &bindImagePlaneMemoryInfo[0] : nullptr, img, image.vkMemory_[0]),
            getBindImageMemoryInfo(&bindImagePlaneMemoryInfo[1], img, image.vkMemory_[1]),
            getBindImageMemoryInfo(&bindImagePlaneMemoryInfo[2], img, image.vkMemory_[2]),
        };
        VK_ASSERT(vkBindImageMemory2(context->device, numPlanes, bindInfo));

        // handle memory-mapped images
        if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && numPlanes == 1) {
        VK_ASSERT(vkMapMemory(context->device, image.vkMemory_[0], 0, VK_WHOLE_SIZE, 0, &image.mappedPtr_));
        }
    }


    VK_ASSERT(setDebugObjectName(context->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)image.vkImage_, debugNameImage));

    // Get physical device's properties for the image's format
    vkGetPhysicalDeviceFormatProperties(context->physical_device, image.vkImageFormat_, &image.vkFormatProperties_);

    VkImageAspectFlags aspect = 0;
    if (image.isDepthFormat_ || image.isStencilFormat_) {
        if (image.isDepthFormat_) {
        aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        } else if (image.isStencilFormat_) {
        aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkComponentMapping mapping = {
        .r = VkComponentSwizzle(desc.swizzle.r),
        .g = VkComponentSwizzle(desc.swizzle.g),
        .b = VkComponentSwizzle(desc.swizzle.b),
        .a = VkComponentSwizzle(desc.swizzle.a),
    };

    VkSamplerYcbcrConversionInfo* ycbcrInfo = isDisjoint ? getOrCreateYcbcrConversionInfo(context, desc.format) : nullptr;

    image.imageView_ = vulkan_image_view_create(&image,
        context->device, vkImageViewType, vkFormat, aspect, 0, VK_REMAINING_MIP_LEVELS, 0, numLayers, mapping, ycbcrInfo, debugNameImageView);

    if (image.vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT) {
        if (!desc.swizzle.identity()) {
        // use identity swizzle for storage images
        image.imageViewStorage_ = vulkan_image_view_create(&image, 
            context->device, vkImageViewType, vkFormat, aspect, 0, VK_REMAINING_MIP_LEVELS, 0, numLayers, {}, ycbcrInfo, debugNameImageView);
        assert(image.imageViewStorage_ != VK_NULL_HANDLE);
        }
    }

    if (!(image.imageView_ != VK_NULL_HANDLE)) {
        return {};
    }

    //TextureHandle handle = texturesPool_.create(std::move(image));
    //Handle handle = texturesPool_.create(std::move(image));
    TextureHandle handle = pool_create(&context->texturesPool_, image);

    context->awaitingCreation_ = true;

    if (desc.data) {
        assert(desc.type == TextureType_2D || desc.type == TextureType_Cube);
        assert(desc.dataNumMipLevels <= desc.numMipLevels);
        u32 numLayers = desc.type == TextureType_Cube ? 6 : 1;
        //b32 res = upload(handle, {.dimensions = desc.dimensions, .numLayers = numLayers, .numMipLevels = desc.dataNumMipLevels}, desc.data);
        b32 res = true;
        if (!res) {
        return {};
        }
        if (desc.generateMipmaps) {
            generateMipmap(context, handle);
        }
    }

    return handle;
}

void querySurfaceCapabilities(VulkanContext *context) {
  // enumerate only the formats we are using
  const VkFormat depthFormats[] = {
      VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM};
  for (VkFormat depthFormat : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(context->physical_device, depthFormat, &formatProps);

    if (formatProps.optimalTilingFeatures) {
      context->deviceDepthFormats_.push_back(depthFormat);
    }
  }

  if (context->vkSurface_ == VK_NULL_HANDLE) {
    return;
  }

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device, context->vkSurface_, &context->deviceSurfaceCaps_);

  u32 formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->vkSurface_, &formatCount, nullptr);

  if (formatCount) {
    context->deviceSurfaceFormats_.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->vkSurface_, &formatCount, context->deviceSurfaceFormats_.data());
  }

  u32 presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->vkSurface_, &presentModeCount, nullptr);

  if (presentModeCount) {
    context->devicePresentModes_.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->vkSurface_, &presentModeCount, context->devicePresentModes_.data());
  }
}


internal VulkanQueryDeviceResult
vulkan_query_devices(Arena *arena, VulkanContext* context, VkPhysicalDeviceType desired_type)
{
    VulkanQueryDeviceResult result = {0};
    u32 device_count = 0;
    TempArena temp_arena = temp_begin(arena);
    vkEnumeratePhysicalDevices(context->instance, &device_count, 0);
    VkPhysicalDevice *phys_devices = arena_push_size(temp_arena.arena, VkPhysicalDevice, device_count);
    vkEnumeratePhysicalDevices(context->instance, &device_count, phys_devices);

    /*
    VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
    VK_PHYSICAL_DEVICE_TYPE_CPU = 4,
    VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM = 0x7FFFFFFF
    // TODO what about cpu here, interesting...?? 
    */

    u32 num_compatible_devices = 0;
    for (u32 i = 0; i < device_count; ++i) {
        VkPhysicalDevice phys_device = phys_devices[i];
        VkPhysicalDeviceProperties device_properties;
        // TODO see how vkGetPhysicalDeviceProperties correlates to vulkan_get_physical_device_props
        vkGetPhysicalDeviceProperties(phys_device, &device_properties);

        VkPhysicalDeviceType device_type = device_properties.deviceType;

        // filter non-suitable hardware devices
        if (device_type !=  desired_type) 
        {
            continue;
        }

        result.out_devices[result.num_compatible_devices] = {.guid = (uintptr_t)phys_devices[i], .type = device_type};
        strncpy(result.out_devices[result.num_compatible_devices].name, device_properties.deviceName, strlen(device_properties.deviceName));
        result.num_compatible_devices++;
    }

    temp_end(temp_arena);
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
                                                   [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT msgType,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* cbData,
                                                   void* userData) {
  if (msgSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    return VK_FALSE;
  }

  const bool isError = (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0;
  const bool isWarning = (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0;

  const size_t len = cbData->pMessage ? strlen(cbData->pMessage) : 128u;

  assert(len < 65536);

  char* errorName = (char*)alloca(len + 1);
  int object = 0;
  void* handle = nullptr;
  char typeName[128] = {};
  void* messageID = nullptr;

//  minilog::eLogLevel level = minilog::Log;
  if (isError) {
    VulkanContext* ctx = (VulkanContext*)userData;
    //level = ctx->config_.terminateOnValidationError ? minilog::FatalError : minilog::Warning;
  }

  if (!isError && !isWarning && cbData->pMessageIdName) {
    if (strcmp(cbData->pMessageIdName, "Loader Message") == 0) {
      return VK_FALSE;
    }
  }

  if (sscanf(cbData->pMessage,
             "Validation Error: [ %[^]] ] Object %i: handle = %p, type = %127s | MessageID = %p",
             errorName,
             &object,
             &handle,
             typeName,
             &messageID) >= 2) {
    const char* message = strrchr(cbData->pMessage, '|') + 1;

    printf("%sValidation layer:\n Validation Error: %s \n Object %i: handle = %p, type = %s\n "
                     "MessageID = %p \n%s \n",
                     isError ? "\nERROR:\n" : "",
                     errorName,
                     object,
                     handle,
                     typeName,
                     messageID,
                     message);
  } else {
    printf("%sValidation layer:\n%s\n", isError ? "\nERROR:\n" : "", cbData->pMessage);
  }

  #if 0
  if (isError) {
    VulkanContext* ctx = (VulkanContext*)userData;

    if (ctx->config_.shaderModuleErrorCallback != nullptr) {
      // retrieve source code references - this is very experimental and depends a lot on the validation layer output
      int line = 0;
      int col = 0;
      const char* substr1 = strstr(cbData->pMessage, "Shader validation error occurred at line ");
      if (substr1 && sscanf(substr1, "Shader validation error occurred at line %d, column %d.", &line, &col) >= 1) {
        const char* substr2 = strstr(cbData->pMessage, "Shader Module (Shader Module: ");
        char* shaderModuleDebugName = (char*)alloca(len + 1);
        VkShaderModule shaderModule = VK_NULL_HANDLE;
#if VK_USE_64_BIT_PTR_DEFINES
        if (substr2 && sscanf(substr2, "Shader Module (Shader Module: %[^)])(%p)", shaderModuleDebugName, &shaderModule) == 2) {
#else
        if (substr2 && sscanf(substr2, "Shader Module (Shader Module: %[^)])(%llu)", shaderModuleDebugName, &shaderModule) == 2) {
#endif // VK_USE_64_BIT_PTR_DEFINES
          ctx->invokeShaderModuleErrorCallback(line, col, shaderModuleDebugName, shaderModule);
        }
      }
    }
  }
  #endif

  return VK_FALSE;
}


internal VulkanContext
vulkan_create_context_inside(Arena *transient, HWND handle)
{
    VulkanContext context = {0};
    if(volkInitialize() != VK_SUCCESS)
    {
        abort();
    }
    // Instance creation
    TempArena temp_arena = temp_begin(transient);
    u32 num_layer_props = 0;
    vkEnumerateInstanceLayerProperties(&num_layer_props, 0);
    printf("layer props: %d\n", num_layer_props);

    VkLayerProperties *layer_props = arena_push_size(temp_arena.arena, VkLayerProperties, num_layer_props);
    vkEnumerateInstanceLayerProperties(&num_layer_props, layer_props);

    b32 enabled_validation_layers = false;
    {
        u32 layer_idx = 0;
        while(!enabled_validation_layers && layer_idx < num_layer_props)
        {
            VkLayerProperties *layer_prop = layer_props + layer_idx;
            if(!strcmp(layer_prop->layerName, k_def_validation_layer[0]))
            {
                enabled_validation_layers = true;
            }
            layer_idx++;
        }
    }

    // NOTE here i decided to create a big enough stack array
    VkExtensionProperties all_instance_extensions[100];
    u32 all_instance_extensions_count = 0;
    {
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(0, &all_instance_extensions_count, 0));
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(0, &all_instance_extensions_count, all_instance_extensions));
        printf("all instance Count is %d\n", all_instance_extensions_count);
    }

    // collect instance extensions from all validation layers
    if (enabled_validation_layers) 
    {
        u32 count = 0;
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(k_def_validation_layer[0], &count, 0));
        if (count > 0) 
        {
            printf("Count is %d\n", count);
            const size_t sz = all_instance_extensions_count;
            VK_ASSERT(vkEnumerateInstanceExtensionProperties(k_def_validation_layer[0], &count, all_instance_extensions + sz));
            all_instance_extensions_count += count;
        }
    }

    //printf("%d %s %s\n: ", all_instance_extensions_count, all_instance_extensions[0].extensionName, all_instance_extensions[all_instance_extensions_count - 1].extensionName);
    const char **instance_extension_names = arena_push_size(temp_arena.arena, const char *, 50);
    const char **instance_extension_name = instance_extension_names;
    *instance_extension_name++ = VK_KHR_SURFACE_EXTENSION_NAME;
    #if defined(_WIN32)
        *instance_extension_name++ = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        *instance_extension_name++ = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
    #elif defined(__linux__)
    #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        *instance_extension_name++ = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    #else
        *instance_extension_name++ = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
    #endif
    #elif defined(__APPLE__)
        *instance_extension_name++ = VK_EXT_LAYER_SETTINGS_EXTENSION_NAME;
        *instance_extension_name++ = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
    #endif
    #if defined(LVK_WITH_VULKAN_PORTABILITY)
        *instance_extension_name++ = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    #endif

    b32 has_debug_utils = vulkan_has_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, all_instance_extensions, all_instance_extensions_count);

    if (has_debug_utils) 
    {
        *instance_extension_name++ = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    if (enabled_validation_layers) {
        *instance_extension_name++ = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;
    }

    //if (config_.enableHeadlessSurface) {
    //    instanceExtensionNames.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
    //}

    //for (const char* ext : config_.extensionsInstance) {
    //    if (ext) {
    //    instanceExtensionNames.push_back(ext);
    //    }
    //}


    #if !defined(ANDROID)
        // GPU Assisted Validation doesn't work on Android.
        // It implicitly requires vertexPipelineStoresAndAtomics feature that's not supported even on high-end devices.
        VkValidationFeatureEnableEXT validationFeaturesEnabled[] = {
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        };
    #endif // ANDROID

    #if defined(__APPLE__)
        // Shader validation doesn't work in MoltenVK for SPIR-V 1.6 under Vulkan 1.3:
        // "Invalid SPIR-V binary version 1.6 for target environment SPIR-V 1.5 (under Vulkan 1.2 semantics)."
        VkValidationFeatureDisableEXT validationFeaturesDisabled[] = {
            VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT,
            VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT,
        };
    #endif // __APPLE__

    VkValidationFeaturesEXT features = 
    {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext = nullptr,
    #if !defined(ANDROID)
        .enabledValidationFeatureCount = enabled_validation_layers ? array_count(validationFeaturesEnabled) : 0u,
        .pEnabledValidationFeatures = enabled_validation_layers ? validationFeaturesEnabled : nullptr,
    #endif
    #if defined(__APPLE__)
        .disabledValidationFeatureCount = enabled_validation_layers ? array_count(validationFeaturesDisabled) : 0u,
        .pDisabledValidationFeatures = enabled_validation_layers ? validationFeaturesDisabled : nullptr,
    #endif
    };

    #if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
        // https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Configuration_Parameters.md
        int useMetalArgumentBuffers = 1;
        VkBool32 gpuav_descriptor_checks = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8688
        VkBool32 gpuav_indirect_draws_buffers = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8579
        VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/9222
        #define LAYER_SETTINGS_BOOL32(name, var)                                                                                        \
            VkLayerSettingEXT {                                                                                                           \
                .pLayerName = k_def_validation_layer[0], .pSettingName = name, .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT, .valueCount = 1, \
                .pValues = var,                                                                                                             \
            }

        VkLayerSettingEXT settings[] = {
            LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks", &gpuav_descriptor_checks),
            LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers", &gpuav_indirect_draws_buffers),
            LAYER_SETTINGS_BOOL32("gpuav_post_process_descriptor_indexing", &gpuav_post_process_descriptor_indexing),
            {"MoltenVK", "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &useMetalArgumentBuffers},
        };
        #undef LAYER_SETTINGS_BOOL32
        VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
            .pNext = enabled_validation_layers ? &features : nullptr,
            .settingCount = array_count(settings),
            .pSettings = settings,
        };
    #endif // defined(VK_EXT_layer_settings) && VK_EXT_layer_settings


    VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = 0,
    .pApplicationName = "Vulkan",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Vulkan",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateFlags flags = 0;
    #if defined(LVK_WITH_VULKAN_PORTABILITY)
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
    VkInstanceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    #if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
        .pNext = &layerSettingsCreateInfo,
    #else
        .pNext = enabled_validation_layers ? &features : nullptr,
    #endif // defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
        .flags = flags,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = enabled_validation_layers ? 1 : 0u,
        .ppEnabledLayerNames = enabled_validation_layers ? k_def_validation_layer : nullptr,
        .enabledExtensionCount = u32(instance_extension_name - instance_extension_names),
        .ppEnabledExtensionNames = instance_extension_names,
    };
    VK_ASSERT(vkCreateInstance(&ci, nullptr, &context.instance));


    // TODO: see what does it do
    volkLoadInstance(context.instance);

    // debug messenger
    if (has_debug_utils) {
        const VkDebugUtilsMessengerCreateInfoEXT ci = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = &vulkanDebugCallback,
            .pUserData = (void*)&context,
        };
        VK_ASSERT(vkCreateDebugUtilsMessengerEXT(context.instance, &ci, nullptr, &context.vkDebugUtilsMessenger_));
    }


    printf("\nVulkan instance extensions:\n");

    for (u32 i = 0; i < all_instance_extensions_count; i++) {
        printf("  %s\n", all_instance_extensions[i].extensionName);
    }

    context.enabled_validation_layers = enabled_validation_layers;

    temp_end(temp_arena);

    #if 1
    {
        // surface creation
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = (HWND)handle,
        };
        VK_ASSERT(vkCreateWin32SurfaceKHR(context.instance, &ci, nullptr, &context.vkSurface_));
        #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        VkAndroidSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, .pNext = nullptr, .flags = 0, .window = (ANativeWindow*)window};
        VK_ASSERT(vkCreateAndroidSurfaceKHR(vkInstance_, &ci, nullptr, &context.vkSurface_));
        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        VkXlibSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .flags = 0,
            .dpy = (Display*)display,
            .window = (Window)window,
        };
        VK_ASSERT(vkCreateXlibSurfaceKHR(vkInstance_, &ci, nullptr, &context.vkSurface_));
        #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VkWaylandSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .flags = 0,
            .display = (wl_display*)display,
            .surface = (wl_surface*)window,
        };
        VK_ASSERT(vkCreateWaylandSurfaceKHR(vkInstance_, &ci, nullptr, &context.vkSurface_));
        #elif defined(VK_USE_PLATFORM_MACOS_MVK)
        VkMacOSSurfaceCreateInfoMVK ci = {
            .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
            .flags = 0,
            .pView = window,
        };
        VK_ASSERT(vkCreateMacOSSurfaceMVK(vkInstance_, &ci, nullptr, &context.vkSurface_));
        #else
        #error Implement for other platforms
        #endif
    }
    #endif

    #if 1
     context.rayTracingPipelineProperties = 
    { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
     context.accelerationStructureProperties = 
    { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};

    // provided by Vulkan 1.2
    context.vkPhysicalDeviceDepthStencilResolveProperties = 
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES,
        nullptr,
    };
    context.vkPhysicalDeviceDriverProperties = 
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
        &context.vkPhysicalDeviceDepthStencilResolveProperties,
    };

    context.vkPhysicalDeviceVulkan12Properties = 
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
        &context.vkPhysicalDeviceDriverProperties,
    };
    // provided by Vulkan 1.1
     context.vkPhysicalDeviceProperties2 = 
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        &context.vkPhysicalDeviceVulkan12Properties,
        VkPhysicalDeviceProperties{},
    };
    
     context.vkFeatures13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
     context.vkFeatures12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &context.vkFeatures13};
     context.vkFeatures11 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &context.vkFeatures12};
     context.vkFeatures10 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &context.vkFeatures11};
    #endif
    return context;
}
internal void
vulkan_create_context(Arena *transient, HWND handle, VulkanContext* context)
{
    if(volkInitialize() != VK_SUCCESS)
    {
        abort();
    }
    // Instance creation
    TempArena temp_arena = temp_begin(transient);
    u32 num_layer_props = 0;
    vkEnumerateInstanceLayerProperties(&num_layer_props, 0);
    printf("layer props: %d\n", num_layer_props);

    VkLayerProperties *layer_props = arena_push_size(temp_arena.arena, VkLayerProperties, num_layer_props);
    vkEnumerateInstanceLayerProperties(&num_layer_props, layer_props);

    b32 enabled_validation_layers = false;
    {
        u32 layer_idx = 0;
        while(!enabled_validation_layers && layer_idx < num_layer_props)
        {
            VkLayerProperties *layer_prop = layer_props + layer_idx;
            if(!strcmp(layer_prop->layerName, k_def_validation_layer[0]))
            {
                enabled_validation_layers = true;
            }
            layer_idx++;
        }
    }

    // NOTE here i decided to create a big enough stack array
    VkExtensionProperties all_instance_extensions[100];
    u32 all_instance_extensions_count = 0;
    {
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(0, &all_instance_extensions_count, 0));
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(0, &all_instance_extensions_count, all_instance_extensions));
        printf("all instance Count is %d\n", all_instance_extensions_count);
    }

    // collect instance extensions from all validation layers
    if (enabled_validation_layers) 
    {
        u32 count = 0;
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(k_def_validation_layer[0], &count, 0));
        if (count > 0) 
        {
            printf("Count is %d\n", count);
            size_t sz = all_instance_extensions_count;
            VK_ASSERT(vkEnumerateInstanceExtensionProperties(k_def_validation_layer[0], &count, all_instance_extensions + sz));
            all_instance_extensions_count += count;
        }
    }

    //printf("%d %s %s\n: ", all_instance_extensions_count, all_instance_extensions[0].extensionName, all_instance_extensions[all_instance_extensions_count - 1].extensionName);
    const char **instance_extension_names = arena_push_size(temp_arena.arena, const char *, 50);
    const char **instance_extension_name = instance_extension_names;
    *instance_extension_name++ = VK_KHR_SURFACE_EXTENSION_NAME;
    #if defined(_WIN32)
        *instance_extension_name++ = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        *instance_extension_name++ = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
    #elif defined(__linux__)
    #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        *instance_extension_name++ = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    #else
        *instance_extension_name++ = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
    #endif
    #elif defined(__APPLE__)
        *instance_extension_name++ = VK_EXT_LAYER_SETTINGS_EXTENSION_NAME;
        *instance_extension_name++ = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
    #endif
    #if defined(LVK_WITH_VULKAN_PORTABILITY)
        *instance_extension_name++ = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    #endif

    b32 has_debug_utils = vulkan_has_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, all_instance_extensions, all_instance_extensions_count);

    if (has_debug_utils) 
    {
        *instance_extension_name++ = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    if (enabled_validation_layers) {
        *instance_extension_name++ = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;
    }

    //if (config_.enableHeadlessSurface) {
    //    instanceExtensionNames.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
    //}

    //for (const char* ext : config_.extensionsInstance) {
    //    if (ext) {
    //    instanceExtensionNames.push_back(ext);
    //    }
    //}


    #if !defined(ANDROID)
        // GPU Assisted Validation doesn't work on Android.
        // It implicitly requires vertexPipelineStoresAndAtomics feature that's not supported even on high-end devices.
        VkValidationFeatureEnableEXT validationFeaturesEnabled[] = {
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        };
    #endif // ANDROID

    #if defined(__APPLE__)
        // Shader validation doesn't work in MoltenVK for SPIR-V 1.6 under Vulkan 1.3:
        // "Invalid SPIR-V binary version 1.6 for target environment SPIR-V 1.5 (under Vulkan 1.2 semantics)."
        VkValidationFeatureDisableEXT validationFeaturesDisabled[] = {
            VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT,
            VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT,
        };
    #endif // __APPLE__

    VkValidationFeaturesEXT features = 
    {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext = nullptr,
    #if !defined(ANDROID)
        .enabledValidationFeatureCount = enabled_validation_layers ? array_count(validationFeaturesEnabled) : 0u,
        .pEnabledValidationFeatures = enabled_validation_layers ? validationFeaturesEnabled : nullptr,
    #endif
    #if defined(__APPLE__)
        .disabledValidationFeatureCount = enabled_validation_layers ? array_count(validationFeaturesDisabled) : 0u,
        .pDisabledValidationFeatures = enabled_validation_layers ? validationFeaturesDisabled : nullptr,
    #endif
    };

    #if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
        // https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Configuration_Parameters.md
        int useMetalArgumentBuffers = 1;
        VkBool32 gpuav_descriptor_checks = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8688
        VkBool32 gpuav_indirect_draws_buffers = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8579
        VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/9222
        #define LAYER_SETTINGS_BOOL32(name, var)                                                                                        \
            VkLayerSettingEXT {                                                                                                           \
                .pLayerName = k_def_validation_layer[0], .pSettingName = name, .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT, .valueCount = 1, \
                .pValues = var,                                                                                                             \
            }

        VkLayerSettingEXT settings[] = {
            LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks", &gpuav_descriptor_checks),
            LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers", &gpuav_indirect_draws_buffers),
            LAYER_SETTINGS_BOOL32("gpuav_post_process_descriptor_indexing", &gpuav_post_process_descriptor_indexing),
            {"MoltenVK", "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &useMetalArgumentBuffers},
        };
        #undef LAYER_SETTINGS_BOOL32
        VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
            .pNext = enabled_validation_layers ? &features : nullptr,
            .settingCount = array_count(settings),
            .pSettings = settings,
        };
    #endif // defined(VK_EXT_layer_settings) && VK_EXT_layer_settings


    VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = 0,
    .pApplicationName = "Vulkan",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Vulkan",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateFlags flags = 0;
    #if defined(LVK_WITH_VULKAN_PORTABILITY)
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
    VkInstanceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    #if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
        .pNext = &layerSettingsCreateInfo,
    #else
        .pNext = enabled_validation_layers ? &features : nullptr,
    #endif // defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
        .flags = flags,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = enabled_validation_layers ? 1 : 0u,
        .ppEnabledLayerNames = enabled_validation_layers ? k_def_validation_layer : nullptr,
        .enabledExtensionCount = u32(instance_extension_name - instance_extension_names),
        .ppEnabledExtensionNames = instance_extension_names,
    };
    VK_ASSERT(vkCreateInstance(&ci, nullptr, &context->instance));


    // TODO: see what does it do
    volkLoadInstance(context->instance);
    // debug messenger
    if (has_debug_utils) {
        const VkDebugUtilsMessengerCreateInfoEXT ci = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = &vulkanDebugCallback,
            .pUserData = (void*)context,
        };
        VK_ASSERT(vkCreateDebugUtilsMessengerEXT(context->instance, &ci, nullptr, &context->vkDebugUtilsMessenger_));
    }

    printf("\nVulkan instance extensions:\n");

    for (u32 i = 0; i < all_instance_extensions_count; i++) {
        printf("  %s\n", all_instance_extensions[i].extensionName);
    }

    context->enabled_validation_layers = enabled_validation_layers;

    temp_end(temp_arena);

    #if 1
    {
        // surface creation
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = (HWND)handle,
        };
        VK_ASSERT(vkCreateWin32SurfaceKHR(context->instance, &ci, nullptr, &context->vkSurface_));
        #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        VkAndroidSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, .pNext = nullptr, .flags = 0, .window = (ANativeWindow*)window};
        VK_ASSERT(vkCreateAndroidSurfaceKHR(vkInstance_, &ci, nullptr, &context->vkSurface_));
        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        VkXlibSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .flags = 0,
            .dpy = (Display*)display,
            .window = (Window)window,
        };
        VK_ASSERT(vkCreateXlibSurfaceKHR(vkInstance_, &ci, nullptr, &context->vkSurface_));
        #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VkWaylandSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .flags = 0,
            .display = (wl_display*)display,
            .surface = (wl_surface*)window,
        };
        VK_ASSERT(vkCreateWaylandSurfaceKHR(vkInstance_, &ci, nullptr, &context->vkSurface_));
        #elif defined(VK_USE_PLATFORM_MACOS_MVK)
        VkMacOSSurfaceCreateInfoMVK ci = {
            .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
            .flags = 0,
            .pView = window,
        };
        VK_ASSERT(vkCreateMacOSSurfaceMVK(vkInstance_, &ci, nullptr, &context->vkSurface_));
        #else
        #error Implement for other platforms
        #endif
    }
    #endif

    #if 1
     context->rayTracingPipelineProperties = 
    { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
     context->accelerationStructureProperties = 
    { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};

    // provided by Vulkan 1.2
    context->vkPhysicalDeviceDepthStencilResolveProperties = 
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES,
        nullptr,
    };
    context->vkPhysicalDeviceDriverProperties = 
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
        &context->vkPhysicalDeviceDepthStencilResolveProperties,
    };

    context->vkPhysicalDeviceVulkan12Properties = 
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
        &context->vkPhysicalDeviceDriverProperties,
    };
    // provided by Vulkan 1.1
     context->vkPhysicalDeviceProperties2 = 
    {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        &context->vkPhysicalDeviceVulkan12Properties,
        VkPhysicalDeviceProperties{},
    };
    
     context->vkFeatures13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
     context->vkFeatures12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &context->vkFeatures13};
     context->vkFeatures11 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &context->vkFeatures12};
     context->vkFeatures10 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &context->vkFeatures11};
    #endif
}

internal b32
vulkan_is_host_visible_single_heap_memory(VkPhysicalDevice phys_dev) 
{
    b32 result = false;
    VkPhysicalDeviceMemoryProperties mem_properties;

    vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_properties);

    if (mem_properties.memoryHeapCount == 1) {
        u32 flag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((mem_properties.memoryTypes[i].propertyFlags & flag) == flag) {
                result = true;
                break;
            }
        }
    }

    return result;
}

struct VulkanDeviceExtensionPropertiesResult
{
    u32 count;
    VkExtensionProperties *extensions;
};

internal VulkanDeviceExtensionPropertiesResult
vulkan_get_device_extension_props(Arena *arena, VkPhysicalDevice phys_dev, const char *validation_layer = 0)
{
    VulkanDeviceExtensionPropertiesResult result = {0};
    u32 num_extensions = 0;
    vkEnumerateDeviceExtensionProperties(phys_dev, validation_layer, &num_extensions, 0);
    VkExtensionProperties *all_device_extensions = arena_push_size(arena, VkExtensionProperties, num_extensions);
    vkEnumerateDeviceExtensionProperties(phys_dev, validation_layer, &num_extensions, all_device_extensions);
    // copy it back to arena? yes!

    // todo: ask chat about it!
    //memcpy((void*)(arena_push_size(arena, VkExtensionProperties, num_extensions)), all_device_extensions, num_extensions);

    /* this is one is just a version without the macro
    VkExtensionProperties *dest = arena_push_size(arena, VkExtensionProperties, num_extensions);
    memcpy((void*)dest, all_device_extensions, num_extensions * sizeof(VkExtensionProperties));
    */
    //VkExtensionProperties* dest = (VkExtensionProperties *)arena_push_copy(arena, num_extensions * sizeof(VkExtensionProperties), all_device_extensions);

    // this one was wrong entirely
    //memcpy((void*)temp_arena.pos, all_device_extensions, num_extensions);
    //return (VkExtensionProperties*) temp_arena.pos;
    result.count = num_extensions;
    //result.extensions = dest;
    result.extensions = all_device_extensions;

    /*
        this will work as long as i dont push anything else on memory. If I do then what I wrote here will be overwritten!
        easy as fuck to see!
        result.extensions = all_device_extensions; 
        and then before using this result something like the following will demonstrate my point:

        u32 *array = arena_push_size(temp_arena.arena, u32, 100);
        u32 *elem = array;
        for(u32 i = 0; i < 100; i++)
        {
            *elem++ = 10;
        }

        Conclusion: I have to do the copy if I'm going to push more stuff into the arena before finishing using the memory allocated here!
        if I dont do any subsequent allocations then it doesn't matter at all!
     */

    return result;
}

internal void
vulkan_add_next_physical_device_properties(VulkanContext *context, void* properties) 
{
    #if 1
      if (!properties)
    return;

  std::launder(reinterpret_cast<VkBaseOutStructure*>(properties))->pNext =
      std::launder(reinterpret_cast<VkBaseOutStructure*>(context->vkPhysicalDeviceProperties2.pNext));

  context->vkPhysicalDeviceProperties2.pNext = properties;
  #endif

  #if 0
  // TODO use this one
    if(!properties) return;
    //context->vkPhysicalDeviceProperties2.pNext = properties;
    // Cast 'properties' to VkBaseOutStructure pointer and assign its pNext
    ((VkBaseOutStructure*)properties)->pNext =
        (VkBaseOutStructure*)(vkPhysicalDeviceProperties2_.pNext);

    // Update the context chain to point to our new properties.
    vkPhysicalDeviceProperties2_.pNext = properties;
    #endif
}

u32 findQueueFamilyIndex(VkPhysicalDevice physDev, VkQueueFlags flags) {

  u32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> props(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, props.data());

  auto findDedicatedQueueFamilyIndex = [&props](VkQueueFlags require,
                                                VkQueueFlags avoid) -> u32 {
    for (u32 i = 0; i != props.size(); i++) {
      bool isSuitable = (props[i].queueFlags & require) == require;
      bool isDedicated = (props[i].queueFlags & avoid) == 0;
      if (props[i].queueCount && isSuitable && isDedicated)
        return i;
    }
    return DeviceQueues::INVALID;
  };

  // dedicated queue for compute
  if (flags & VK_QUEUE_COMPUTE_BIT) {
    u32 q = findDedicatedQueueFamilyIndex(flags, VK_QUEUE_GRAPHICS_BIT);
    if (q != DeviceQueues::INVALID)
      return q;
  }

  // dedicated queue for transfer
  if (flags & VK_QUEUE_TRANSFER_BIT) {
    u32 q = findDedicatedQueueFamilyIndex(flags, VK_QUEUE_GRAPHICS_BIT);
    if (q != DeviceQueues::INVALID)
      return q;
  }

  // any suitable
  return findDedicatedQueueFamilyIndex(flags, 0);
}



internal void
vulkan_init_context(Arena *arena, Arena *transient, VulkanContext *context, VulkanDeviceHWDesc *decs)
{
    TempArena temp_arena = temp_begin(transient);
    context->physical_device = (VkPhysicalDevice) decs->guid;
    b32 use_staging = !vulkan_is_host_visible_single_heap_memory(context->physical_device);

    context->currentMaxTextures_ = 16;
    context->currentMaxSamplers_ = 16;
    context->currentMaxAccelStructs_ = 1;

    /* NOTE 
      I'm going to keep doing this. I would need to have a dynamic array probably but
      in practice it wouldn't matter if i allocate these extension in a slack array
    */
    VulkanDeviceExtensionPropertiesResult all_device_extensions = vulkan_get_device_extension_props(temp_arena.arena, context->physical_device, 0);
    if(context->enabled_validation_layers)
    {
        for(const char *layer: k_def_validation_layer)
        {
            VulkanDeviceExtensionPropertiesResult res = vulkan_get_device_extension_props(temp_arena.arena, context->physical_device, layer);
            all_device_extensions.count += res.count;
        }
    }

    if (vulkan_has_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, all_device_extensions.extensions, all_device_extensions.count)) 
    {
        //vulkan_add_next_physical_device_properties(context, (void*)&context->accelerationStructureProperties);
    }

    if (vulkan_has_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, all_device_extensions.extensions, all_device_extensions.count)) 
    {
        //vulkan_add_next_physical_device_properties(context, (void*)&context->rayTracingPipelineProperties);
    }



     std::vector<VkExtensionProperties> allDeviceExtensions;
 getDeviceExtensionProps(context->physical_device, allDeviceExtensions);
 if (context->enabled_validation_layers) {
   for (const char* layer : k_def_validation_layer) {
     getDeviceExtensionProps(context->physical_device, allDeviceExtensions, layer);
   }
 }


#if 0
    PFN_vkGetPhysicalDeviceProperties2 lcdtm;
    lcdtm = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr((VkInstance)context->instance, "vkGetPhysicalDeviceProperties2");
    lcdtm(context->physical_device, &context->vkPhysicalDeviceProperties2);

    PFN_vkGetPhysicalDeviceProperties2KHR getPhysDevProps2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)
    vkGetInstanceProcAddr((VkInstance)context->instance, "vkGetPhysicalDeviceProperties2KHR");
	if(getPhysDevProps2KHR) {
		getPhysDevProps2KHR(context->physical_device, &context->vkPhysicalDeviceProperties2);
	}
#endif

vkGetPhysicalDeviceFeatures2(context->physical_device, &context->vkFeatures10);
vkGetPhysicalDeviceProperties2(context->physical_device, &context->vkPhysicalDeviceProperties2);
// vkGetPhysicalDeviceFeatures2(context->physical_device, &vkFeatures10_);
// vkGetPhysicalDeviceProperties2(context->physical_device, &vkPhysicalDeviceProperties2_);



    u32 api_version = context->vkPhysicalDeviceProperties2.properties.apiVersion;
    printf("Vulkan phsyical device: %s\n", context->vkPhysicalDeviceProperties2.properties.deviceName);
    printf("API version: %i.%i.%i.%i\n", 
        VK_API_VERSION_MAJOR(api_version),
        VK_API_VERSION_MINOR(api_version),
        VK_API_VERSION_PATCH(api_version),
        VK_API_VERSION_VARIANT(api_version));
    
    //printf("Driver info: %s %s\n", vkPhysicalDeviceDriverProperties_.driverName, vkPhysicalDeviceDriverProperties_.driverInfo);
    printf("Driver info: %s %s\n", context->vkPhysicalDeviceDriverProperties.driverName, context->vkPhysicalDeviceDriverProperties.driverInfo);

    #if 1
    printf("Vulkan physical device extensions: %d\n", all_device_extensions.count);
    for(u32 i = 0; i < all_device_extensions.count; i++)
    {
        VkExtensionProperties *device_extension = all_device_extensions.extensions + i;
        printf("%s\n", device_extension->extensionName);

    }
    #endif

    DeviceQueues &queues = context->queues;
    queues.graphicsQueueFamilyIndex = findQueueFamilyIndex(context->physical_device, VK_QUEUE_GRAPHICS_BIT);
    queues.computeQueueFamilyIndex = findQueueFamilyIndex(context->physical_device, VK_QUEUE_COMPUTE_BIT);

    if (queues.graphicsQueueFamilyIndex == DeviceQueues::INVALID) {
        abort();
    }

    if (queues.computeQueueFamilyIndex == DeviceQueues::INVALID) {
        abort();
    }

    f32 queuePriority = 1.0f;

    VkDeviceQueueCreateInfo ciQueue[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queues.graphicsQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queues.computeQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        },
    };
    u32 num_queues = ciQueue[0].queueFamilyIndex == ciQueue[1].queueFamilyIndex ? 1 : 2;


    //const char* deviceExtensionNames[] = 
    std::vector<const char*> deviceExtensionNames = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    #if defined(__APPLE__)
        // All supported Vulkan 1.3 extensions
        // https://github.com/KhronosGroup/MoltenVK/issues/1930
        VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_EXT_4444_FORMATS_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
        VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME,
        VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME,
        VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME,
        VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME,
        VK_EXT_PRIVATE_DATA_EXTENSION_NAME,
        VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME,
        VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME,
        VK_EXT_TEXEL_BUFFER_ALIGNMENT_EXTENSION_NAME,
        VK_EXT_TEXTURE_COMPRESSION_ASTC_HDR_EXTENSION_NAME,
    #endif
    #if defined(LVK_WITH_VULKAN_PORTABILITY)
        "VK_KHR_portability_subset",
    #endif
    };

    VkPhysicalDeviceFeatures2 vkFeatures10_ = context->vkFeatures10;
    VkPhysicalDeviceVulkan11Features vkFeatures11_ = context->vkFeatures11;
    VkPhysicalDeviceVulkan12Features vkFeatures12_ = context->vkFeatures12;
    VkPhysicalDeviceVulkan13Features vkFeatures13_ = context->vkFeatures13;

    VkPhysicalDeviceFeatures deviceFeatures10 = 
    {
      .geometryShader = vkFeatures10_.features.geometryShader, // enable if supported
      .tessellationShader = vkFeatures10_.features.tessellationShader, // enable if supported
      .sampleRateShading = VK_TRUE,
      .multiDrawIndirect = VK_TRUE,
      .drawIndirectFirstInstance = VK_TRUE,
      .depthBiasClamp = VK_TRUE,
      .fillModeNonSolid = vkFeatures10_.features.fillModeNonSolid, // enable if supported
      .samplerAnisotropy = VK_TRUE,
      .textureCompressionBC = vkFeatures10_.features.textureCompressionBC, // enable if supported
      .vertexPipelineStoresAndAtomics = vkFeatures10_.features.vertexPipelineStoresAndAtomics, // enable if supported
      .fragmentStoresAndAtomics = VK_TRUE,
      .shaderImageGatherExtended = VK_TRUE,
      .shaderInt64 = vkFeatures10_.features.shaderInt64, // enable if supported
    };
    VkPhysicalDeviceVulkan11Features deviceFeatures11 = 
    {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
      //.pNext = config_.extensionsDeviceFeatures,
      .pNext = 0,
      .storageBuffer16BitAccess = VK_TRUE,
      .samplerYcbcrConversion = vkFeatures11_.samplerYcbcrConversion, // enable if supported
      .shaderDrawParameters = VK_TRUE,
  };
  VkPhysicalDeviceVulkan12Features deviceFeatures12 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &deviceFeatures11,
      .drawIndirectCount = vkFeatures12_.drawIndirectCount, // enable if supported
      .storageBuffer8BitAccess = vkFeatures12_.storageBuffer8BitAccess, // enable if supported
      .uniformAndStorageBuffer8BitAccess = vkFeatures12_.uniformAndStorageBuffer8BitAccess, // enable if supported
      .shaderFloat16 = vkFeatures12_.shaderFloat16, // enable if supported
      .descriptorIndexing = VK_TRUE,
      .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
      .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
      .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
      .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
      .descriptorBindingPartiallyBound = VK_TRUE,
      .descriptorBindingVariableDescriptorCount = VK_TRUE,
      .runtimeDescriptorArray = VK_TRUE,
      .scalarBlockLayout = VK_TRUE,
      .uniformBufferStandardLayout = VK_TRUE,
      .hostQueryReset = vkFeatures12_.hostQueryReset, // enable if supported
      .timelineSemaphore = VK_TRUE,
      .bufferDeviceAddress = VK_TRUE,
      .vulkanMemoryModel = vkFeatures12_.vulkanMemoryModel, // enable if supported
      .vulkanMemoryModelDeviceScope = vkFeatures12_.vulkanMemoryModelDeviceScope, // enable if supported
  };
  VkPhysicalDeviceVulkan13Features deviceFeatures13 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &deviceFeatures12,
      .subgroupSizeControl = VK_TRUE,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
      .maintenance4 = VK_TRUE,
  };

#ifdef __APPLE__
  VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeature = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
      .pNext = &deviceFeatures13,
      .extendedDynamicState = VK_TRUE,
  };

  VkPhysicalDeviceExtendedDynamicState2FeaturesEXT dynamicState2Feature = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT,
      .pNext = &dynamicStateFeature,
      .extendedDynamicState2 = VK_TRUE,
  };

  VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Feature = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
      .pNext = &dynamicState2Feature,
      .synchronization2 = VK_TRUE,
  };

  void* createInfoNext = &synchronization2Feature;
#else
  void* createInfoNext = &deviceFeatures13;
#endif
  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
      .accelerationStructure = VK_TRUE,
      .accelerationStructureCaptureReplay = VK_FALSE,
      .accelerationStructureIndirectBuild = VK_FALSE,
      .accelerationStructureHostCommands = VK_FALSE,
      .descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE,
  };
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
      .rayTracingPipeline = VK_TRUE,
      .rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE,
      .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE,
      .rayTracingPipelineTraceRaysIndirect = VK_TRUE,
      .rayTraversalPrimitiveCulling = VK_FALSE,
  };
  VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
      .rayQuery = VK_TRUE,
  };
  VkPhysicalDeviceIndexTypeUint8FeaturesEXT indexTypeUint8Features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT,
      .indexTypeUint8 = VK_TRUE,
  };




  #if 0

    auto addOptionalExtension = [&all_device_extensions, &deviceExtensionNames, &createInfoNext](
                                  const char* name, bool& enabled, void* features = nullptr) mutable -> bool {
    if (!vulkan_has_extension(name, all_device_extensions.extensions, all_device_extensions.count))
      return false;
    enabled = true;
    deviceExtensionNames.push_back(name);
    if (features) {
      std::launder(reinterpret_cast<VkBaseOutStructure*>(features))->pNext =
          std::launder(reinterpret_cast<VkBaseOutStructure*>(createInfoNext));
      createInfoNext = features;
    }
    return true;
  };
  auto addOptionalExtensions = [&all_device_extensions, &deviceExtensionNames, &createInfoNext](
                                   const char* name1, const char* name2, bool& enabled, void* features = nullptr) mutable {
    if (!vulkan_has_extension(name1, all_device_extensions.extensions, all_device_extensions.count) || !vulkan_has_extension(name2, all_device_extensions.extensions, all_device_extensions.count))
      return;
    enabled = true;
    deviceExtensionNames.push_back(name1);
    deviceExtensionNames.push_back(name2);
    if (features) {
      std::launder(reinterpret_cast<VkBaseOutStructure*>(features))->pNext =
          std::launder(reinterpret_cast<VkBaseOutStructure*>(createInfoNext));
      createInfoNext = features;
    }
  };
  #endif


  auto addOptionalExtension = [&allDeviceExtensions, &deviceExtensionNames, &createInfoNext](
                                const char* name, bool& enabled, void* features = nullptr) mutable -> bool {
  if (!hasExtension(name, allDeviceExtensions))
    return false;
  enabled = true;
  deviceExtensionNames.push_back(name);
  if (features) {
    std::launder(reinterpret_cast<VkBaseOutStructure*>(features))->pNext =
        std::launder(reinterpret_cast<VkBaseOutStructure*>(createInfoNext));
    createInfoNext = features;
  }
  return true;
};
auto addOptionalExtensions = [&allDeviceExtensions, &deviceExtensionNames, &createInfoNext](
                                 const char* name1, const char* name2, bool& enabled, void* features = nullptr) mutable {
  if (!hasExtension(name1, allDeviceExtensions) || !hasExtension(name2, allDeviceExtensions))
    return;
  enabled = true;
  deviceExtensionNames.push_back(name1);
  deviceExtensionNames.push_back(name2);
  if (features) {
    std::launder(reinterpret_cast<VkBaseOutStructure*>(features))->pNext =
        std::launder(reinterpret_cast<VkBaseOutStructure*>(createInfoNext));
    createInfoNext = features;
  }
};


#if 0
#if defined(LVK_WITH_TRACY)
  addOptionalExtension(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, hasCalibratedTimestamps_, nullptr);
#endif
#endif

  addOptionalExtensions(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                        hasAccelerationStructure_,
                        &accelerationStructureFeatures);
  addOptionalExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME, hasRayQuery_, &rayQueryFeatures);
  addOptionalExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, hasRayTracingPipeline_, &rayTracingFeatures);
#if defined(VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME)
  if (!addOptionalExtension(VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME, has8BitIndices_, &indexTypeUint8Features))
#endif // VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME
  {
    addOptionalExtension(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME, has8BitIndices_, &indexTypeUint8Features);
  }

  // check extensions
   {
   std::string missingExtensions;
   for (const char* ext : deviceExtensionNames) {
     if (!hasExtension(ext, allDeviceExtensions))
       missingExtensions += "\n   " + std::string(ext);
   }
   if (!missingExtensions.empty()) {
     printf("Missing Vulkan device extensions: %s\n", missingExtensions.c_str());
     assert(false);
   }
 }

 #if 0
  {
    std::string missingExtensions;
    for (const char* ext : deviceExtensionNames) {
      if (!vulkan_has_extension(ext, all_device_extensions.extensions, all_device_extensions.count))
        missingExtensions += "\n   " + std::string(ext);
    }
    if (!missingExtensions.empty()) {
      //MINILOG_LOG_PROC(minilog::FatalError, "Missing Vulkan device extensions: %s\n", missingExtensions.c_str());
      assert(false);
      abort();
      //return Result(Result::Code::RuntimeError);
    }
  }
  #endif




  // check features
  {
    std::string missingFeatures;
#define CHECK_VULKAN_FEATURE(reqFeatures, availFeatures, feature, version)     \
  if ((reqFeatures.feature == VK_TRUE) && (availFeatures.feature == VK_FALSE)) \
    missingFeatures.append("\n   " version " ." #feature);
#define CHECK_FEATURE_1_0(feature) CHECK_VULKAN_FEATURE(deviceFeatures10, vkFeatures10_.features, feature, "1.0 ");
    CHECK_FEATURE_1_0(robustBufferAccess);
    CHECK_FEATURE_1_0(fullDrawIndexUint32);
    CHECK_FEATURE_1_0(imageCubeArray);
    CHECK_FEATURE_1_0(independentBlend);
    CHECK_FEATURE_1_0(geometryShader);
    CHECK_FEATURE_1_0(tessellationShader);
    CHECK_FEATURE_1_0(sampleRateShading);
    CHECK_FEATURE_1_0(dualSrcBlend);
    CHECK_FEATURE_1_0(logicOp);
    CHECK_FEATURE_1_0(multiDrawIndirect);
    CHECK_FEATURE_1_0(drawIndirectFirstInstance);
    CHECK_FEATURE_1_0(depthClamp);
    CHECK_FEATURE_1_0(depthBiasClamp);
    CHECK_FEATURE_1_0(fillModeNonSolid);
    CHECK_FEATURE_1_0(depthBounds);
    CHECK_FEATURE_1_0(wideLines);
    CHECK_FEATURE_1_0(largePoints);
    CHECK_FEATURE_1_0(alphaToOne);
    CHECK_FEATURE_1_0(multiViewport);
    CHECK_FEATURE_1_0(samplerAnisotropy);
    CHECK_FEATURE_1_0(textureCompressionETC2);
    CHECK_FEATURE_1_0(textureCompressionASTC_LDR);
    CHECK_FEATURE_1_0(textureCompressionBC);
    CHECK_FEATURE_1_0(occlusionQueryPrecise);
    CHECK_FEATURE_1_0(pipelineStatisticsQuery);
    CHECK_FEATURE_1_0(vertexPipelineStoresAndAtomics);
    CHECK_FEATURE_1_0(fragmentStoresAndAtomics);
    CHECK_FEATURE_1_0(shaderTessellationAndGeometryPointSize);
    CHECK_FEATURE_1_0(shaderImageGatherExtended);
    CHECK_FEATURE_1_0(shaderStorageImageExtendedFormats);
    CHECK_FEATURE_1_0(shaderStorageImageMultisample);
    CHECK_FEATURE_1_0(shaderStorageImageReadWithoutFormat);
    CHECK_FEATURE_1_0(shaderStorageImageWriteWithoutFormat);
    CHECK_FEATURE_1_0(shaderUniformBufferArrayDynamicIndexing);
    CHECK_FEATURE_1_0(shaderSampledImageArrayDynamicIndexing);
    CHECK_FEATURE_1_0(shaderStorageBufferArrayDynamicIndexing);
    CHECK_FEATURE_1_0(shaderStorageImageArrayDynamicIndexing);
    CHECK_FEATURE_1_0(shaderClipDistance);
    CHECK_FEATURE_1_0(shaderCullDistance);
    CHECK_FEATURE_1_0(shaderFloat64);
    CHECK_FEATURE_1_0(shaderInt64);
    CHECK_FEATURE_1_0(shaderInt16);
    CHECK_FEATURE_1_0(shaderResourceResidency);
    CHECK_FEATURE_1_0(shaderResourceMinLod);
    CHECK_FEATURE_1_0(sparseBinding);
    CHECK_FEATURE_1_0(sparseResidencyBuffer);
    CHECK_FEATURE_1_0(sparseResidencyImage2D);
    CHECK_FEATURE_1_0(sparseResidencyImage3D);
    CHECK_FEATURE_1_0(sparseResidency2Samples);
    CHECK_FEATURE_1_0(sparseResidency4Samples);
    CHECK_FEATURE_1_0(sparseResidency8Samples);
    CHECK_FEATURE_1_0(sparseResidency16Samples);
    CHECK_FEATURE_1_0(sparseResidencyAliased);
    CHECK_FEATURE_1_0(variableMultisampleRate);
    CHECK_FEATURE_1_0(inheritedQueries);
#undef CHECK_FEATURE_1_0
#define CHECK_FEATURE_1_1(feature) CHECK_VULKAN_FEATURE(deviceFeatures11, vkFeatures11_, feature, "1.1 ");
    CHECK_FEATURE_1_1(storageBuffer16BitAccess);
    CHECK_FEATURE_1_1(uniformAndStorageBuffer16BitAccess);
    CHECK_FEATURE_1_1(storagePushConstant16);
    CHECK_FEATURE_1_1(storageInputOutput16);
    CHECK_FEATURE_1_1(multiview);
    CHECK_FEATURE_1_1(multiviewGeometryShader);
    CHECK_FEATURE_1_1(multiviewTessellationShader);
    CHECK_FEATURE_1_1(variablePointersStorageBuffer);
    CHECK_FEATURE_1_1(variablePointers);
    CHECK_FEATURE_1_1(protectedMemory);
    CHECK_FEATURE_1_1(samplerYcbcrConversion);
    CHECK_FEATURE_1_1(shaderDrawParameters);
#undef CHECK_FEATURE_1_1
#define CHECK_FEATURE_1_2(feature) CHECK_VULKAN_FEATURE(deviceFeatures12, vkFeatures12_, feature, "1.2 ");
    CHECK_FEATURE_1_2(samplerMirrorClampToEdge);
    CHECK_FEATURE_1_2(drawIndirectCount);
    CHECK_FEATURE_1_2(storageBuffer8BitAccess);
    CHECK_FEATURE_1_2(uniformAndStorageBuffer8BitAccess);
    CHECK_FEATURE_1_2(storagePushConstant8);
    CHECK_FEATURE_1_2(shaderBufferInt64Atomics);
    CHECK_FEATURE_1_2(shaderSharedInt64Atomics);
    CHECK_FEATURE_1_2(shaderFloat16);
    CHECK_FEATURE_1_2(shaderInt8);
    CHECK_FEATURE_1_2(descriptorIndexing);
    CHECK_FEATURE_1_2(shaderInputAttachmentArrayDynamicIndexing);
    CHECK_FEATURE_1_2(shaderUniformTexelBufferArrayDynamicIndexing);
    CHECK_FEATURE_1_2(shaderStorageTexelBufferArrayDynamicIndexing);
    CHECK_FEATURE_1_2(shaderUniformBufferArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderSampledImageArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderStorageBufferArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderStorageImageArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderInputAttachmentArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderUniformTexelBufferArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderStorageTexelBufferArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(descriptorBindingUniformBufferUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingSampledImageUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingStorageImageUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingStorageBufferUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingUniformTexelBufferUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingStorageTexelBufferUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingUpdateUnusedWhilePending);
    CHECK_FEATURE_1_2(descriptorBindingPartiallyBound);
    CHECK_FEATURE_1_2(descriptorBindingVariableDescriptorCount);
    CHECK_FEATURE_1_2(runtimeDescriptorArray);
    CHECK_FEATURE_1_2(samplerFilterMinmax);
    CHECK_FEATURE_1_2(scalarBlockLayout);
    CHECK_FEATURE_1_2(imagelessFramebuffer);
    CHECK_FEATURE_1_2(uniformBufferStandardLayout);
    CHECK_FEATURE_1_2(shaderSubgroupExtendedTypes);
    CHECK_FEATURE_1_2(separateDepthStencilLayouts);
    CHECK_FEATURE_1_2(hostQueryReset);
    CHECK_FEATURE_1_2(timelineSemaphore);
    CHECK_FEATURE_1_2(bufferDeviceAddress);
    CHECK_FEATURE_1_2(bufferDeviceAddressCaptureReplay);
    CHECK_FEATURE_1_2(bufferDeviceAddressMultiDevice);
    CHECK_FEATURE_1_2(vulkanMemoryModel);
    CHECK_FEATURE_1_2(vulkanMemoryModelDeviceScope);
    CHECK_FEATURE_1_2(vulkanMemoryModelAvailabilityVisibilityChains);
    CHECK_FEATURE_1_2(shaderOutputViewportIndex);
    CHECK_FEATURE_1_2(shaderOutputLayer);
    CHECK_FEATURE_1_2(subgroupBroadcastDynamicId);
#undef CHECK_FEATURE_1_2
#define CHECK_FEATURE_1_3(feature) CHECK_VULKAN_FEATURE(deviceFeatures13, vkFeatures13_, feature, "1.3 ");
    CHECK_FEATURE_1_3(robustImageAccess);
    CHECK_FEATURE_1_3(inlineUniformBlock);
    CHECK_FEATURE_1_3(descriptorBindingInlineUniformBlockUpdateAfterBind);
    CHECK_FEATURE_1_3(pipelineCreationCacheControl);
    CHECK_FEATURE_1_3(privateData);
    CHECK_FEATURE_1_3(shaderDemoteToHelperInvocation);
    CHECK_FEATURE_1_3(shaderTerminateInvocation);
    CHECK_FEATURE_1_3(subgroupSizeControl);
    CHECK_FEATURE_1_3(computeFullSubgroups);
    CHECK_FEATURE_1_3(synchronization2);
    CHECK_FEATURE_1_3(textureCompressionASTC_HDR);
    CHECK_FEATURE_1_3(shaderZeroInitializeWorkgroupMemory);
    CHECK_FEATURE_1_3(dynamicRendering);
    CHECK_FEATURE_1_3(shaderIntegerDotProduct);
    CHECK_FEATURE_1_3(maintenance4);
#undef CHECK_FEATURE_1_3
    if (!missingFeatures.empty()) {
        for (const auto& i: missingFeatures)
            std::cout << i << '\n';
        #if 0
      MINILOG_LOG_PROC(
#ifndef __APPLE__
          minilog::FatalError,
#else
          minilog::Warning,
#endif
          "Missing Vulkan features: %s\n",
          missingFeatures.c_str());
      // Do not exit here in case of MoltenVK, some 1.3 features are available via extensions.
#ifndef __APPLE__
      assert(false);
      abort();
#endif

#endif
    }
  }


    VkDeviceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = createInfoNext,
        .queueCreateInfoCount = num_queues,
        .pQueueCreateInfos = ciQueue,
        .enabledExtensionCount = (u32)deviceExtensionNames.size(),
        .ppEnabledExtensionNames = deviceExtensionNames.data(),
        .pEnabledFeatures = &deviceFeatures10,
    };
    VK_ASSERT_RETURN(vkCreateDevice(context->physical_device, &ci, nullptr, &context->device));
    volkLoadDevice(context->device);

    #if defined(__APPLE__)
        vkCmdBeginRendering = vkCmdBeginRenderingKHR;
        vkCmdEndRendering = vkCmdEndRenderingKHR;
        vkCmdSetDepthWriteEnable = vkCmdSetDepthWriteEnableEXT;
        vkCmdSetDepthTestEnable = vkCmdSetDepthTestEnableEXT;
        vkCmdSetDepthCompareOp = vkCmdSetDepthCompareOpEXT;
        vkCmdSetDepthBiasEnable = vkCmdSetDepthBiasEnableEXT;
    #endif

    vkGetDeviceQueue(context->device, queues.graphicsQueueFamilyIndex, 0, &queues.graphicsQueue);
    vkGetDeviceQueue(context->device, queues.computeQueueFamilyIndex, 0, &queues.computeQueue);

    VK_ASSERT(setDebugObjectName(context->device, VK_OBJECT_TYPE_DEVICE, (uint64_t)context->device, "Device: VulkanContext::device"));

    // select a depth-resolve mode
    //TODO whats the use of a lambda?
    //context->depthResolveMode_ = [this]() -> VkResolveModeFlagBits {
    //    const VkResolveModeFlags modes = context->vkPhysicalDeviceDepthStencilResolveProperties.supportedDepthResolveModes;
    //    if (modes & VK_RESOLVE_MODE_AVERAGE_BIT)
    //    return VK_RESOLVE_MODE_AVERAGE_BIT;
    //    // this mode is always available
    //    return VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
    //}();

    {
        VkResolveModeFlags modes = context->vkPhysicalDeviceDepthStencilResolveProperties.supportedDepthResolveModes;
        if (modes & VK_RESOLVE_MODE_AVERAGE_BIT)
        {
            context->depthResolveMode_ = VK_RESOLVE_MODE_AVERAGE_BIT;
        }
        else
        {
            context->depthResolveMode_ = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        }
    }


    // See this if i should do temp and then copy or just arena like it is now
    context->immediate_ = (VulkanImmediateCommands*)arena_push_size(arena, VulkanImmediateCommands, 1);
    vulkan_immediate_commands_create(context->immediate_, context->device, queues.graphicsQueueFamilyIndex, "VulkanContext::immediate_");


    // create Vulkan pipeline cache
    {
        VkPipelineCacheCreateInfo ci = {
            VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            nullptr,
            VkPipelineCacheCreateFlags(0),
            context->pipelineCacheDataSize,
            context->pipelineCacheData,
        };
        vkCreatePipelineCache(context->device, &ci, nullptr, &context->pipelineCache_);
    }

    if (LVK_VULKAN_USE_VMA) {
        context->vma = vulkan_vma_allocator_create(
            context->physical_device, context->device, context->instance, api_version > VK_API_VERSION_1_3 ? VK_API_VERSION_1_3 : api_version);
        assert(context->vma != VK_NULL_HANDLE);
    }

    context->stagingDevice_ = (VulkanStagingDevice*)arena_push_size(arena, VulkanStagingDevice, 1);
    vulkan_staging_device_create(context->stagingDevice_, context);

    // default texture
    {
        u32 pixel = 0xFF000000;
        context->dummyTexture_ = vulkan_create_texture(context, 
                                {
                                    .format = Format_RGBA_UN8,
                                    .dimensions = {1, 1, 1},
                                    .usage = TextureUsageBits_Sampled | TextureUsageBits_Storage,
                                    .data = &pixel,
                                }, "Dummy 1x1 (black)");
                            //.release();
        printf("idx: %d, gen: %d\n", context->dummyTexture_.idx, context->dummyTexture_.gen);
        #if 0
        if (!LVK_VERIFY(result.isOk())) {
        return result;
        }
        #endif
        assert(context->texturesPool_.count == 1);
    }




    // default sampler
    assert(context->samplersPool_.count == 0);
    vulkan_sampler_create(
        context,
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 0.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = static_cast<float>(LVK_MAX_MIP_LEVELS - 1),
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        },
        Format_Invalid,
        "Sampler: default");

    growDescriptorPool(context, context->currentMaxTextures_, context->currentMaxSamplers_, context->currentMaxAccelStructs_);

    querySurfaceCapabilities(context);


    #if 0
    #if defined(LVK_WITH_TRACY_GPU)
    std::vector<VkTimeDomainEXT> timeDomains;

    if (hasCalibratedTimestamps_) {
        u32 numTimeDomains = 0;
        VK_ASSERT(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(context->physical_device, &numTimeDomains, nullptr));
        timeDomains.resize(numTimeDomains);
        VK_ASSERT(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(context->physical_device, &numTimeDomains, timeDomains.data()));
    }

    bool hasHostQuery = vkFeatures12_.hostQueryReset && [&timeDomains]() -> bool {
        for (VkTimeDomainEXT domain : timeDomains)
        if (domain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT || domain == VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT)
            return true;
        return false;
    }();

    if (hasHostQuery) {
        pimpl_->tracyVkCtx_ = TracyVkContextHostCalibrated(
            context->physical_device, context->device, vkResetQueryPool, vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, vkGetCalibratedTimestampsEXT);
    } else {
        VkCommandPoolCreateInfo ciCommandPool = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = queues.graphicsQueueFamilyIndex,
        };
        VK_ASSERT(vkCreateCommandPool(context->device, &ciCommandPool, nullptr, &pimpl_->tracyCommandPool_));
        setDebugObjectName(
            context->device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)pimpl_->tracyCommandPool_, "Command Pool: VulkanContextImpl::tracyCommandPool_");
         VkCommandBufferAllocateInfo aiCommandBuffer = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pimpl_->tracyCommandPool_,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VK_ASSERT(vkAllocateCommandBuffers(context->device, &aiCommandBuffer, &pimpl_->tracyCommandBuffer_));
        if (hasCalibratedTimestamps_) {
        pimpl_->tracyVkCtx_ = TracyVkContextCalibrated(context->physical_device,
                                                        context->device,
                                                        queues.graphicsQueue,
                                                        pimpl_->tracyCommandBuffer_,
                                                        vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
                                                        vkGetCalibratedTimestampsEXT);
        } else {
        pimpl_->tracyVkCtx_ = TracyVkContext(context->physical_device, context->device, queues.graphicsQueue, pimpl_->tracyCommandBuffer_);
        };
    }
    assert(pimpl_->tracyVkCtx_);
    #endif // LVK_WITH_TRACY_GPU
    #endif

    temp_end(temp_arena);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats, ColorSpace colorSpace) {
    assert(!formats.empty());

    auto isNativeSwapChainBGR = [](std::vector<VkSurfaceFormatKHR>& formats) -> bool {
        for (VkSurfaceFormatKHR& fmt : formats) {
        // The preferred format should be the one which is closer to the beginning of the formats
        // container. If BGR is encountered earlier, it should be picked as the format of choice. If RGB
        // happens to be earlier, take it.
        if (fmt.format == VK_FORMAT_R8G8B8A8_UNORM || fmt.format == VK_FORMAT_R8G8B8A8_SRGB ||
            fmt.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32) {
            return false;
        }
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM || fmt.format == VK_FORMAT_B8G8R8A8_SRGB ||
            fmt.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32) {
            return true;
        }
        }
        return false;
    };

    auto colorSpaceToVkSurfaceFormat = [](ColorSpace colorSpace, bool isBGR) -> VkSurfaceFormatKHR {
        switch (colorSpace) {
        case ColorSpace_SRGB_LINEAR:
        // the closest thing to sRGB linear
        return VkSurfaceFormatKHR{isBGR ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_BT709_LINEAR_EXT};
        case ColorSpace_SRGB_NONLINEAR:
        [[fallthrough]];
        default:
        // default to normal sRGB non linear.
        return VkSurfaceFormatKHR{isBGR ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        }
    };

    VkSurfaceFormatKHR preferred = colorSpaceToVkSurfaceFormat(colorSpace, isNativeSwapChainBGR(formats));

    for (VkSurfaceFormatKHR& fmt : formats) {
        if (fmt.format == preferred.format && fmt.colorSpace == preferred.colorSpace) {
        return fmt;
        }
    }

    // if we can't find a matching format and color space, fallback on matching only format
    for (VkSurfaceFormatKHR& fmt : formats) {
        if (fmt.format == preferred.format) {
        return fmt;
        }
    }

    printf("Could not find a native swap chain format that matched our designed swapchain format. Defaulting to first supported format.\n");

    return formats[0];
}



void vulkan_init_swapchain(Arena *arena, Arena *transient, VulkanContext *context, u32 width, u32 height)
{
    context->swapchain_ = (VulkanSwapchain*)arena_push_size(arena, VulkanSwapchain, 1);
    //
    context->swapchain_->ctx_ = context;
    context->swapchain_->device_ = context->device;
    context->swapchain_->graphicsQueue_ = context->queues.graphicsQueue;
    context->swapchain_->width_ = width;
    context->swapchain_->height_ = height;
    VulkanSwapchain *swap = context->swapchain_;
    swap->surfaceFormat_ = {.format = VK_FORMAT_UNDEFINED};

    swap->surfaceFormat_ = chooseSwapSurfaceFormat(context->deviceSurfaceFormats_, ColorSpace_SRGB_LINEAR);
    swap->getNextImage_ = true;

    gui_assert(context->vkSurface_ != VK_NULL_HANDLE, "VulkanContext::Surface is empty!");
    VkBool32 queueFamilySupportsPresentation = VK_FALSE;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(
        context->physical_device, context->queues.graphicsQueueFamilyIndex, context->vkSurface_, &queueFamilySupportsPresentation));
    gui_assert(queueFamilySupportsPresentation == VK_TRUE, "The queue family used with the swapchain does not support presentation");

    VulkanContext *ctx = context;
    auto chooseSwapImageCount = [](VkSurfaceCapabilitiesKHR& caps) -> u32 {
        u32 desired = caps.minImageCount + 1;
        bool exceeded = caps.maxImageCount > 0 && desired > caps.maxImageCount;
        return exceeded ? caps.maxImageCount : desired;
    };

    auto chooseSwapPresentMode = [](std::vector<VkPresentModeKHR>& modes) -> VkPresentModeKHR {
    #if defined(__linux__) || defined(_M_ARM64)
        if (std::find(modes.cbegin(), modes.cend(), VK_PRESENT_MODE_IMMEDIATE_KHR) != modes.cend()) {
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    #endif // __linux__
        if (std::find(modes.cbegin(), modes.cend(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.cend()) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    };

    auto chooseUsageFlags = [](VkPhysicalDevice pd, VkSurfaceKHR surface, VkFormat format) -> VkImageUsageFlags {
        VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VkSurfaceCapabilitiesKHR caps = {};
        VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &caps));

        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(pd, format, &props);

        bool isStorageSupported = (caps.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) > 0;
        bool isTilingOptimalSupported = (props.optimalTilingFeatures & VK_IMAGE_USAGE_STORAGE_BIT) > 0;

        if (isStorageSupported && isTilingOptimalSupported) {
        usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        return usageFlags;
    };

    VkImageUsageFlags usageFlags = chooseUsageFlags(ctx->physical_device, ctx->vkSurface_, context->swapchain_->surfaceFormat_.format);
    bool isCompositeAlphaOpaqueSupported = (ctx->deviceSurfaceCaps_.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) != 0;
    VkSwapchainCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = context->vkSurface_,
        .minImageCount = chooseSwapImageCount(ctx->deviceSurfaceCaps_),
        .imageFormat = context->swapchain_->surfaceFormat_.format,
        .imageColorSpace = context->swapchain_->surfaceFormat_.colorSpace,
        .imageExtent = {.width = width, .height = height},
        .imageArrayLayers = 1,
        .imageUsage = usageFlags,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &context->queues.graphicsQueueFamilyIndex,
    #if defined(ANDROID)
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    #else
        .preTransform = context->deviceSurfaceCaps_.currentTransform,
    #endif
        .compositeAlpha = isCompositeAlphaOpaqueSupported ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        .presentMode = chooseSwapPresentMode(context->devicePresentModes_),
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    VK_ASSERT(vkCreateSwapchainKHR(context->device, &ci, nullptr, &context->swapchain_->swapchain_));

    VkImage swapchainImages[VulkanSwapchain::LVK_MAX_SWAPCHAIN_IMAGES];
    VK_ASSERT(vkGetSwapchainImagesKHR(context->device, context->swapchain_->swapchain_, &context->swapchain_->numSwapchainImages_, nullptr));
    if (context->swapchain_->numSwapchainImages_ > VulkanSwapchain::LVK_MAX_SWAPCHAIN_IMAGES) {
        assert(context->swapchain_->numSwapchainImages_ <= VulkanSwapchain::LVK_MAX_SWAPCHAIN_IMAGES);
        context->swapchain_->numSwapchainImages_ = VulkanSwapchain::LVK_MAX_SWAPCHAIN_IMAGES;
    }
    VK_ASSERT(vkGetSwapchainImagesKHR(context->device, context->swapchain_->swapchain_, &context->swapchain_->numSwapchainImages_, swapchainImages));

    assert(context->swapchain_->numSwapchainImages_ > 0);

    char debugNameImage[256] = {0};
    char debugNameImageView[256] = {0};

    // create images, image views and framebuffers
    for (u32 i = 0; i < context->swapchain_->numSwapchainImages_; i++) {
        context->swapchain_->acquireSemaphore_[i] = vulkan_create_semaphore(context->device, "Semaphore: swapchain-acquire");

        snprintf(debugNameImage, sizeof(debugNameImage) - 1, "Image: swapchain %u", i);
        snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: swapchain %u", i);
        VulkanImage image = {
            .vkImage_ = swapchainImages[i],
            .vkUsageFlags_ = usageFlags,
            .vkExtent_ = VkExtent3D{.width = width, .height = height, .depth = 1},
            .vkType_ = VK_IMAGE_TYPE_2D,
            .vkImageFormat_ = context->swapchain_->surfaceFormat_.format,
            .isSwapchainImage_ = true,
            .isOwningVkImage_ = false,
            .isDepthFormat_ = vulkan_image_is_depth_format(context->swapchain_->surfaceFormat_.format),
            .isStencilFormat_ = vulkan_image_is_stencil_format(context->swapchain_->surfaceFormat_.format),
        };

        VK_ASSERT(setDebugObjectName(context->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)image.vkImage_, debugNameImage));

        image.imageView_ = vulkan_image_view_create(&image, context->device,
                                                VK_IMAGE_VIEW_TYPE_2D,
                                                context->swapchain_->surfaceFormat_.format,
                                                VK_IMAGE_ASPECT_COLOR_BIT,
                                                0,
                                                VK_REMAINING_MIP_LEVELS,
                                                0,
                                                1,
                                                {},
                                                nullptr,
                                                debugNameImageView);

        context->swapchain_->swapchainTextures_[i] = pool_create(&context->texturesPool_, image);
    }

    // TODO see this in detail!!
    /*
    
     vkQueueSubmit2(): pSubmits[0].pSignalSemaphoreInfos[1].semaphore signal value (0x3) in VkQueue 0x1327c25cc110[] must be greater than current timeline semaphore VkSemaphore 0xf9a524000000009e[Semaphore: VulkanContext::timelineSemaphore_] value (0x3). The Vulkan spec states: If the semaphore member of any element of pSignalSemaphoreInfos is a timeline semaphore, the value member of that element must have a value greater than the current value of the semaphore when the semaphore signal operation is executed (https://vulkan.lunarg.com/doc/view/1.3.268.0/windows/1.3-extensions/vkspec.html#VUID-VkSubmitInfo2-semaphore-03882)
    
    */
    context->timelineSemaphore_ = createSemaphoreTimeline(context->device, context->swapchain_->numSwapchainImages_ - 1, "Semaphore: VulkanContext::timelineSemaphore_");
}

// OBS: probably just a handle. Store a HANDLE custom type inside the custom type window that can link to the actual OS window?
internal VulkanContext *
vulkan_create_context_with_swapchain(Arena *arena, Arena *transient, OS_Window window, u32 width, u32 height)
{


    #if 0
    VulkanContext context = {0};
    context = vulkan_create_context_inside(transient, window.handle);
    #else
    VulkanContext* context = (VulkanContext*)arena_push_size(arena, VulkanContext, 1);
    //VulkanContext context = {0};
    vulkan_create_context(transient, window.handle, context);
    #endif
    VulkanQueryDeviceResult queried_devices = vulkan_query_devices(transient, context, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    
    vulkan_init_context(arena, transient, context, &queried_devices.out_devices[0]);
    vulkan_init_swapchain(arena, transient, context, width, height);
    return context;
}

internal CommandBuffer *
vulkan_context_cmd_buffer_acquire(VulkanContext *ctx)
{
    //LVK_PROFILER_FUNCTION();

    gui_assert(!ctx->currentCommandBuffer_.ctx_, "Cannot acquire more than 1 command buffer simultaneously");

    #if defined(_M_ARM64)
    vkDeviceWaitIdle(vkDevice_); // a temporary workaround for Windows on Snapdragon
    #endif

    //ctx->currentCommandBuffer_ = CommandBuffer(this);
    ctx->currentCommandBuffer_ = {.ctx_ = ctx, .wrapper_ = vulkan_immediate_commands_acquire(ctx->immediate_)};

    return &ctx->currentCommandBuffer_;
}

internal void
processDeferredTasks(VulkanContext *ctx)
{
    while(!ctx->deferredTasks_.empty() && vulkan_immediate_commands_is_ready(ctx->immediate_, ctx->deferredTasks_.front().handle_, true))
    {
        ctx->deferredTasks_.front().task_();
        ctx->deferredTasks_.pop_front();
    }
}

// CONTINUE HERE
internal SubmitHandle
vulkan_context_cmd_buffer_submit(VulkanContext *ctx, CommandBuffer *vkCmdBuffer, TextureHandle present)
{
    //LVK_PROFILER_FUNCTION();
    // This is not needed, explicit in the function arg
    //CommandBuffer* vkCmdBuffer = static_cast<CommandBuffer*>(&commandBuffer);

    assert(vkCmdBuffer);
    assert(vkCmdBuffer->ctx_);
    assert(vkCmdBuffer->wrapper_);

    #if defined(LVK_WITH_TRACY_GPU)
        TracyVkCollect(pimpl_->tracyVkCtx_, vkCmdBuffer->wrapper_->cmdBuf_);
    #endif // LVK_WITH_TRACY_GPU

    if (present) {
        //const VulkanImage& tex = *texturesPool_.get(present);
        VulkanImage* tex = pool_get(&ctx->texturesPool_, present);

        assert(tex->isSwapchainImage_);

        /*
        tex.transitionLayout(vkCmdBuffer->wrapper_->cmdBuf_,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
        */
        vulkan_image_transition_layout(tex, vkCmdBuffer->wrapper_->cmdBuf_,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                         VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
    }

    //const bool shouldPresent = hasSwapchain() && present;
    bool shouldPresent = ctx->swapchain_ && present;

    if (shouldPresent) {
        // if we a presenting a swapchain image, signal our timeline semaphore
        //const uint64_t signalValue = ctx->swapchain_->currentFrameIndex_ + ctx->swapchain_->getNumSwapchainImages();
        uint64_t signalValue = ctx->swapchain_->currentFrameIndex_ + ctx->swapchain_->numSwapchainImages_;
        // we wait for this value next time we want to acquire this swapchain image
        ctx->swapchain_->timelineWaitValues_[ctx->swapchain_->currentImageIndex_] = signalValue;
        //ctx->immediate_->signalSemaphore(timelineSemaphore_, signalValue);
        vulkan_immediate_commands_signal_semaphore(ctx->immediate_, ctx->timelineSemaphore_, signalValue);
    }

    //vkCmdBuffer->lastSubmitHandle_ = immediate_->submit(*vkCmdBuffer->wrapper_);
    vkCmdBuffer->lastSubmitHandle_ = vulkan_immediate_commands_submit(ctx->immediate_, vkCmdBuffer->wrapper_);

    if (shouldPresent) {
        //ctx->swapchain_->present(ctx->immediate_->acquireLastSubmitSemaphore());
        VkSemaphore sem = vulkan_immediate_commands_acquire_last_submit_semaphore(ctx->immediate_);
        vulkan_swapchain_present(ctx->swapchain_, sem);
    }

    processDeferredTasks(ctx);

    SubmitHandle handle = vkCmdBuffer->lastSubmitHandle_;

    // reset
    ctx->currentCommandBuffer_ = {};

    return handle;
}

int main() 
{
    u32 window_width = 1280;
    u32 window_height = 720;
    global_w32_window = os_win32_open_window("Vulkan example", window_width, window_height, win32_main_callback, WindowOpenFlags_Centered);
    Arena arena {};
    arena_init(&arena, mb(1));
    Arena transient_arena {};
    arena_init(&transient_arena, mb(10));
    VulkanContext *ctx = vulkan_create_context_with_swapchain(&arena, &transient_arena, global_w32_window, window_width, window_height);
    while(global_w32_window.is_running)
    {
        Win32ProcessPendingMessages();
        CommandBuffer * buf = vulkan_context_cmd_buffer_acquire(ctx);
        TextureHandle curr_swapchain_tex = vulkan_context_get_current_swapchain_texture(ctx);
        vulkan_context_cmd_buffer_submit(ctx, buf, curr_swapchain_tex);    
    }
    vkDestroyInstance(ctx->instance, nullptr);
}
