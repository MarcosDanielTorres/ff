#include <stdio.h>
#include <vector>
#include <iostream>
#include <string>
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

#define LVK_VULKAN_USE_VMA 1


#define VOLK_IMPLEMENTATION
#define VK_NO_PROTOTYPES 1
#include <Volk/volk.h>
#include <vulkan/vulkan_win32.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vma/vk_mem_alloc.h>

const char* k_def_validation_layer[] = {"VK_LAYER_KHRONOS_validation"};


enum TextureUsageBits : uint8_t {
  TextureUsageBits_Sampled = 1 << 0,
  TextureUsageBits_Storage = 1 << 1,
  TextureUsageBits_Attachment = 1 << 2,
};

enum Format : uint8_t {
  Format_Invalid = 0,

  Format_R_UN8,
  Format_R_UI16,
  Format_R_UI32,
  Format_R_UN16,
  Format_R_F16,
  Format_R_F32,

  Format_RG_UN8,
  Format_RG_UI16,
  Format_RG_UI32,
  Format_RG_UN16,
  Format_RG_F16,
  Format_RG_F32,

  Format_RGBA_UN8,
  Format_RGBA_UI32,
  Format_RGBA_F16,
  Format_RGBA_F32,
  Format_RGBA_SRGB8,

  Format_BGRA_UN8,
  Format_BGRA_SRGB8,

  Format_ETC2_RGB8,
  Format_ETC2_SRGB8,
  Format_BC7_RGBA,

  Format_Z_UN16,
  Format_Z_UN24,
  Format_Z_F32,
  Format_Z_UN24_S_UI8,
  Format_Z_F32_S_UI8,

  Format_YUV_NV12,
  Format_YUV_420p,
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

bool getNumImagePlanes(Format format) {
  return properties[format].numPlanes;
}




VkResult setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, const char* name) {
  if (!name || !*name || !vkSetDebugUtilsObjectNameEXT) {
    return VK_SUCCESS;
  }
  const VkDebugUtilsObjectNameInfoEXT ni = {
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



bool hasExtension(const char* ext, const std::vector<VkExtensionProperties>& props) {
  for (const VkExtensionProperties& p : props) {
    if (strcmp(ext, p.extensionName) == 0)
      return true;
  }
  return false;
}

void getDeviceExtensionProps(VkPhysicalDevice dev, std::vector<VkExtensionProperties>& props, const char* validationLayer = nullptr) {
  uint32_t numExtensions = 0;
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


struct DeviceQueues
{
    const static uint32_t INVALID = 0xFFFFFFFF;
    uint32_t graphicsQueueFamilyIndex = INVALID;
    uint32_t computeQueueFamilyIndex = INVALID;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
};

struct TextureHandle 
{

};

struct VulkanContext
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;


    std::unique_ptr<lvk::VulkanImmediateCommands> immediate_;
    std::unique_ptr<lvk::VulkanStagingDevice> stagingDevice_;


    // Originalmente en una config
    b32 enabled_validation_layers;
    size_t pipelineCacheDataSize;
    const void* pipelineCacheData;
    VkPipelineCache pipelineCache_;

    // a texture/sampler was created since the last descriptor set update
    mutable bool awaitingNewImmutableSamplers_ = false;

    // Originalmente en una config

    DeviceQueues queues;
    TextureHandle dummyTexture_;


    // TODO borrar algunos de estos porque no todo es necesario para esta demo
    VkResolveModeFlagBits depthResolveMode_ = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
    std::vector<VkFormat> deviceDepthFormats_;
    std::vector<VkSurfaceFormatKHR> deviceSurfaceFormats_;
    VkSurfaceCapabilitiesKHR deviceSurfaceCaps_;
    std::vector<VkPresentModeKHR> devicePresentModes_;
    // TODO borrar algunos de estos porque no todo es necesario para esta demo

    u32 currentMaxTextures_ = 16;
    u32 currentMaxSamplers_ = 16;
    u32 currentMaxAccelStructs_ = 1;
    VkDescriptorSetLayout vkDSL_ = VK_NULL_HANDLE;
    VkDescriptorPool vkDPool_ = VK_NULL_HANDLE;
    VkDescriptorSet vkDSet_ = VK_NULL_HANDLE;


    lvk::Pool<lvk::Sampler, VkSampler> samplersPool_;
    lvk::Pool<lvk::Texture, lvk::VulkanImage> texturesPool_;


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

void deferredTask(VulkanContext* context, std::packaged_task<void()>&& task, SubmitHandle handle) const {
  if (handle.empty()) {
    handle = immediate_->getNextSubmitHandle();
  }
  pimpl_->deferredTasks_.emplace_back(std::move(task), handle);
}


growDescriptorPool(VulkanContext *context, u32 maxTextures, u32 maxSamplers, u32 maxAccelStructs) {
  context->currentMaxTextures_ = maxTextures;
  context->currentMaxSamplers_ = maxSamplers;
  context->currentMaxAccelStructs_ = maxAccelStructs;

#if LVK_VULKAN_PRINT_COMMANDS
  LLOGL("growDescriptorPool(%u, %u)\n", maxTextures, maxSamplers);
#endif // LVK_VULKAN_PRINT_COMMANDS

  gui_assert(maxTextures <= context->vkPhysicalDeviceVulkan12Properties.maxDescriptorSetUpdateAfterBindSampledImages,
     "Max Textures exceeded: %u (max %u)", maxTextures, context->vkPhysicalDeviceVulkan12Properties.maxDescriptorSetUpdateAfterBindSampledImages);

#if 0
  if (!LVK_VERIFY(maxSamplers <= context->vkPhysicalDeviceVulkan12Properties.maxDescriptorSetUpdateAfterBindSamplers)) {
    LLOGW("Max Samplers exceeded %u (max %u)", maxSamplers, context->vkPhysicalDeviceVulkan12Properties.maxDescriptorSetUpdateAfterBindSamplers);
  }
#endif

  if (vkDSL_ != VK_NULL_HANDLE) {
    deferredTask(std::packaged_task<void()>([device = context->device, dsl = vkDSL_]() { vkDestroyDescriptorSetLayout(device, dsl, nullptr); }));
  }
  if (vkDPool_ != VK_NULL_HANDLE) {
    deferredTask(std::packaged_task<void()>([device = context->device, dp = vkDPool_]() { vkDestroyDescriptorPool(device, dp, nullptr); }));
  }

  bool hasYUVImages = false;

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

  std::vector<VkSampler> immutableSamplers;
  const VkSampler* immutableSamplersData = nullptr;

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
  const uint32_t flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
                         VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
  VkDescriptorBindingFlags bindingFlags[kBinding_NumBindings];
  for (int i = 0; i < kBinding_NumBindings; ++i) {
    bindingFlags[i] = flags;
  }
  const VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlagsCI = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
      .bindingCount = uint32_t(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1),
      .pBindingFlags = bindingFlags,
  };
  const VkDescriptorSetLayoutCreateInfo dslci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = &setLayoutBindingFlagsCI,
      .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
      .bindingCount = uint32_t(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1),
      .pBindings = bindings,
  };
  VK_ASSERT(vkCreateDescriptorSetLayout(vkDevice_, &dslci, nullptr, &vkDSL_));
  VK_ASSERT(setDebugObjectName(
      vkDevice_, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)vkDSL_, "Descriptor Set Layout: VulkanContext::vkDSL_"));

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
        .poolSizeCount = uint32_t(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1),
        .pPoolSizes = poolSizes,
    };
    VK_ASSERT_RETURN(vkCreateDescriptorPool(vkDevice_, &ci, nullptr, &vkDPool_));
    const VkDescriptorSetAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vkDPool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &vkDSL_,
    };
    VK_ASSERT_RETURN(vkAllocateDescriptorSets(vkDevice_, &ai, &vkDSet_));
  }

  awaitingNewImmutableSamplers_ = false;

  return Result();
}

SamplerHandle createSampler(VulkanContext *context, const VkSamplerCreateInfo& ci,
                                                     Format yuvFormat,
                                                     const char* debugName) {
  //LVK_PROFILER_FUNCTION_COLOR(LVK_PROFILER_COLOR_CREATE);

  VkSamplerCreateInfo cinfo = ci;

  if (yuvFormat != Format_Invalid) {
    cinfo.pNext = getOrCreateYcbcrConversionInfo(yuvFormat);
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

  SamplerHandle handle = context->samplersPool_.create(VkSampler(sampler));

  context->awaitingCreation_ = true;

  return handle;
}




void querySurfaceCapabilities(VulkanContext *context) {
  // enumerate only the formats we are using
  const VkFormat depthFormats[] = {
      VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM};
  for (const VkFormat& depthFormat : depthFormats) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(context->phsyical_device, depthFormat, &formatProps);

    if (formatProps.optimalTilingFeatures) {
      context->deviceDepthFormats_.push_back(depthFormat);
    }
  }

  if (context->surface == VK_NULL_HANDLE) {
    return;
  }

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device, context->surface, &context->deviceSurfaceCaps_);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->surface, &formatCount, nullptr);

  if (formatCount) {
    deviceSurfaceFormats_.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->surface, &formatCount, context->deviceSurfaceFormats_.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->surface, &presentModeCount, nullptr);

  if (presentModeCount) {
    context->devicePresentModes_.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->surface, &presentModeCount, context->devicePresentModes_.data());
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
        const int useMetalArgumentBuffers = 1;
        const VkBool32 gpuav_descriptor_checks = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8688
        const VkBool32 gpuav_indirect_draws_buffers = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8579
        const VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/9222
        #define LAYER_SETTINGS_BOOL32(name, var)                                                                                        \
            VkLayerSettingEXT {                                                                                                           \
                .pLayerName = k_def_validation_layer[0], .pSettingName = name, .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT, .valueCount = 1, \
                .pValues = var,                                                                                                             \
            }

        const VkLayerSettingEXT settings[] = {
            LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks", &gpuav_descriptor_checks),
            LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers", &gpuav_indirect_draws_buffers),
            LAYER_SETTINGS_BOOL32("gpuav_post_process_descriptor_indexing", &gpuav_post_process_descriptor_indexing),
            {"MoltenVK", "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &useMetalArgumentBuffers},
        };
        #undef LAYER_SETTINGS_BOOL32
        const VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {
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

    printf("\nVulkan instance extensions:\n");

    for (u32 i = 0; i < all_instance_extensions_count; i++) {
        printf("  %s\n", all_instance_extensions[i].extensionName);
    }

    context.enabled_validation_layers = enabled_validation_layers;

    temp_end(temp_arena);

    #if 0
    {
        // surface creation
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        const VkWin32SurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = (HWND)handle,
        };
        VK_ASSERT(vkCreateWin32SurfaceKHR(context.instance, &ci, nullptr, &vkSurface_));
        #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        const VkAndroidSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, .pNext = nullptr, .flags = 0, .window = (ANativeWindow*)window};
        VK_ASSERT(vkCreateAndroidSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        const VkXlibSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .flags = 0,
            .dpy = (Display*)display,
            .window = (Window)window,
        };
        VK_ASSERT(vkCreateXlibSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
        #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        const VkWaylandSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .flags = 0,
            .display = (wl_display*)display,
            .surface = (wl_surface*)window,
        };
        VK_ASSERT(vkCreateWaylandSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
        #elif defined(VK_USE_PLATFORM_MACOS_MVK)
        const VkMacOSSurfaceCreateInfoMVK ci = {
            .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
            .flags = 0,
            .pView = window,
        };
        VK_ASSERT(vkCreateMacOSSurfaceMVK(vkInstance_, &ci, nullptr, &vkSurface_));
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
        const int useMetalArgumentBuffers = 1;
        const VkBool32 gpuav_descriptor_checks = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8688
        const VkBool32 gpuav_indirect_draws_buffers = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8579
        const VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/9222
        #define LAYER_SETTINGS_BOOL32(name, var)                                                                                        \
            VkLayerSettingEXT {                                                                                                           \
                .pLayerName = k_def_validation_layer[0], .pSettingName = name, .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT, .valueCount = 1, \
                .pValues = var,                                                                                                             \
            }

        const VkLayerSettingEXT settings[] = {
            LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks", &gpuav_descriptor_checks),
            LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers", &gpuav_indirect_draws_buffers),
            LAYER_SETTINGS_BOOL32("gpuav_post_process_descriptor_indexing", &gpuav_post_process_descriptor_indexing),
            {"MoltenVK", "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &useMetalArgumentBuffers},
        };
        #undef LAYER_SETTINGS_BOOL32
        const VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {
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

    printf("\nVulkan instance extensions:\n");

    for (u32 i = 0; i < all_instance_extensions_count; i++) {
        printf("  %s\n", all_instance_extensions[i].extensionName);
    }

    context->enabled_validation_layers = enabled_validation_layers;

    temp_end(temp_arena);

    #if 0
    {
        // surface creation
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        const VkWin32SurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = (HWND)handle,
        };
        VK_ASSERT(vkCreateWin32SurfaceKHR(context.instance, &ci, nullptr, &vkSurface_));
        #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        const VkAndroidSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, .pNext = nullptr, .flags = 0, .window = (ANativeWindow*)window};
        VK_ASSERT(vkCreateAndroidSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        const VkXlibSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .flags = 0,
            .dpy = (Display*)display,
            .window = (Window)window,
        };
        VK_ASSERT(vkCreateXlibSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
        #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        const VkWaylandSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .flags = 0,
            .display = (wl_display*)display,
            .surface = (wl_surface*)window,
        };
        VK_ASSERT(vkCreateWaylandSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
        #elif defined(VK_USE_PLATFORM_MACOS_MVK)
        const VkMacOSSurfaceCreateInfoMVK ci = {
            .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
            .flags = 0,
            .pView = window,
        };
        VK_ASSERT(vkCreateMacOSSurfaceMVK(vkInstance_, &ci, nullptr, &vkSurface_));
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
    if(!properties) return;
    //context->vkPhysicalDeviceProperties2.pNext = properties;
    // Cast 'properties' to VkBaseOutStructure pointer and assign its pNext
    ((VkBaseOutStructure*)properties)->pNext =
        (VkBaseOutStructure*)(vkPhysicalDeviceProperties2_.pNext);

    // Update the context chain to point to our new properties.
    vkPhysicalDeviceProperties2_.pNext = properties;
    #endif
}

uint32_t findQueueFamilyIndex(VkPhysicalDevice physDev, VkQueueFlags flags) {

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> props(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, props.data());

  auto findDedicatedQueueFamilyIndex = [&props](VkQueueFlags require,
                                                VkQueueFlags avoid) -> uint32_t {
    for (uint32_t i = 0; i != props.size(); i++) {
      const bool isSuitable = (props[i].queueFlags & require) == require;
      const bool isDedicated = (props[i].queueFlags & avoid) == 0;
      if (props[i].queueCount && isSuitable && isDedicated)
        return i;
    }
    return DeviceQueues::INVALID;
  };

  // dedicated queue for compute
  if (flags & VK_QUEUE_COMPUTE_BIT) {
    const uint32_t q = findDedicatedQueueFamilyIndex(flags, VK_QUEUE_GRAPHICS_BIT);
    if (q != DeviceQueues::INVALID)
      return q;
  }

  // dedicated queue for transfer
  if (flags & VK_QUEUE_TRANSFER_BIT) {
    const uint32_t q = findDedicatedQueueFamilyIndex(flags, VK_QUEUE_GRAPHICS_BIT);
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

    #if 0
    printf("Vulkan phsyical device extensions: %d\n", all_device_extensions.count);
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


#if defined(LVK_WITH_TRACY)
  addOptionalExtension(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, hasCalibratedTimestamps_, nullptr);
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
        .enabledExtensionCount = (uint32_t)deviceExtensionNames.size(),
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
        const VkResolveModeFlags modes = context->vkPhysicalDeviceDepthStencilResolveProperties.supportedDepthResolveModes;
        if (modes & VK_RESOLVE_MODE_AVERAGE_BIT)
        {
            context->depthResolveMode_ = VK_RESOLVE_MODE_AVERAGE_BIT;
        }
        else
        {
            context->depthResolveMode_ = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        }
    }

    context->immediate_ =
        std::make_unique<VulkanImmediateCommands>(context->device, queues.graphicsQueueFamilyIndex, "VulkanContext::immediate_");

    // create Vulkan pipeline cache
    {
        const VkPipelineCacheCreateInfo ci = {
            VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            nullptr,
            VkPipelineCacheCreateFlags(0),
            context->pipelineCacheDataSize,
            context->pipelineCacheData,
        };
        vkCreatePipelineCache(context->device, &ci, nullptr, &context->pipelineCache_);
    }

    if (LVK_VULKAN_USE_VMA) {
        pimpl_->vma_ = createVmaAllocator(
            context->physical_device, context->device, context->instance, api_version > VK_API_VERSION_1_3 ? VK_API_VERSION_1_3 : api_version);
        assert(pimpl_->vma_ != VK_NULL_HANDLE);
    }

    stagingDevice_ = std::make_unique<VulkanStagingDevice>(*this);

    // default texture
    {
        const uint32_t pixel = 0xFF000000;
        Result result;
        context->dummyTexture_ = createTexture(
                                {
                                    .format = Format_RGBA_UN8,
                                    .dimensions = {1, 1, 1},
                                    .usage = TextureUsageBits_Sampled | TextureUsageBits_Storage,
                                    .data = &pixel,
                                },
                                "Dummy 1x1 (black)",
                                &result)
                            .release();
        #if 0
        if (!LVK_VERIFY(result.isOk())) {
        return result;
        }
        #endif
        assert(texturesPool_.numObjects() == 1);
    }

    // default sampler
    assert(samplersPool_.numObjects() == 0);
    createSampler(
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
        nullptr,
        Format_Invalid,
        "Sampler: default");

    growDescriptorPool(context, context->currentMaxTextures_, context->currentMaxSamplers_, context->currentMaxAccelStructs_);

    querySurfaceCapabilities(context);

    #if defined(LVK_WITH_TRACY_GPU)
    std::vector<VkTimeDomainEXT> timeDomains;

    if (hasCalibratedTimestamps_) {
        uint32_t numTimeDomains = 0;
        VK_ASSERT(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(context->physical_device, &numTimeDomains, nullptr));
        timeDomains.resize(numTimeDomains);
        VK_ASSERT(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(context->physical_device, &numTimeDomains, timeDomains.data()));
    }

    const bool hasHostQuery = vkFeatures12_.hostQueryReset && [&timeDomains]() -> bool {
        for (VkTimeDomainEXT domain : timeDomains)
        if (domain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT || domain == VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT)
            return true;
        return false;
    }();

    if (hasHostQuery) {
        pimpl_->tracyVkCtx_ = TracyVkContextHostCalibrated(
            context->physical_device, context->device, vkResetQueryPool, vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, vkGetCalibratedTimestampsEXT);
    } else {
        const VkCommandPoolCreateInfo ciCommandPool = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = queues.graphicsQueueFamilyIndex,
        };
        VK_ASSERT(vkCreateCommandPool(context->device, &ciCommandPool, nullptr, &pimpl_->tracyCommandPool_));
        setDebugObjectName(
            context->device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)pimpl_->tracyCommandPool_, "Command Pool: VulkanContextImpl::tracyCommandPool_");
        const VkCommandBufferAllocateInfo aiCommandBuffer = {
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


    temp_end(temp_arena);
}

void vulkan_init_swapchain(VulkanContext *context, u32 width, u32 height)
{

}

// OBS: probably just a handle. Store a HANDLE custom type inside the custom type window that can link to the actual OS window?
internal void
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
    vulkan_create_context_with_swapchain(&arena, &transient_arena, global_w32_window, window_width, window_height);

    while(global_w32_window.is_running)
    {
        
    }
}