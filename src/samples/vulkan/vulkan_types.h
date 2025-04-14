#pragma once
#include "generated/handles.h"

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

struct Framebuffer final {
  struct AttachmentDesc {
    TextureHandle texture;
    TextureHandle resolveTexture;
  };

  AttachmentDesc color[LVK_MAX_COLOR_ATTACHMENTS] = {};
  AttachmentDesc depthStencil;

  const char* debugName = "";

  uint32_t getNumColorAttachments() const {
    uint32_t n = 0;
    while (n < LVK_MAX_COLOR_ATTACHMENTS && color[n].texture) {
      n++;
    }
    return n;
  }
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

struct ShaderModuleState final {
  VkShaderModule sm = VK_NULL_HANDLE;
  uint32_t pushConstantsSize = 0;
};


enum Topology : uint8_t {
  Topology_Point,
  Topology_Line,
  Topology_LineStrip,
  Topology_Triangle,
  Topology_TriangleStrip,
  Topology_Patch,
};

enum CullMode : uint8_t { CullMode_None, CullMode_Front, CullMode_Back };
enum WindingMode : uint8_t { WindingMode_CCW, WindingMode_CW };
enum PolygonMode : uint8_t {
  PolygonMode_Fill = 0,
  PolygonMode_Line = 1,
};

struct SpecializationConstantEntry {
  uint32_t constantId = 0;
  uint32_t offset = 0; // offset within ShaderSpecializationConstantDesc::data
  size_t size = 0;
};

struct SpecializationConstantDesc {
  enum { LVK_SPECIALIZATION_CONSTANTS_MAX = 16 };
  SpecializationConstantEntry entries[LVK_SPECIALIZATION_CONSTANTS_MAX] = {};
  const void* data = nullptr;
  size_t dataSize = 0;
  uint32_t getNumSpecializationConstants() const {
    uint32_t n = 0;
    while (n < LVK_SPECIALIZATION_CONSTANTS_MAX && entries[n].size) {
      n++;
    }
    return n;
  }
};

enum class VertexFormat {
  Invalid = 0,

  Float1,
  Float2,
  Float3,
  Float4,

  Byte1,
  Byte2,
  Byte3,
  Byte4,

  UByte1,
  UByte2,
  UByte3,
  UByte4,

  Short1,
  Short2,
  Short3,
  Short4,

  UShort1,
  UShort2,
  UShort3,
  UShort4,

  Byte2Norm,
  Byte4Norm,

  UByte2Norm,
  UByte4Norm,

  Short2Norm,
  Short4Norm,

  UShort2Norm,
  UShort4Norm,

  Int1,
  Int2,
  Int3,
  Int4,

  UInt1,
  UInt2,
  UInt3,
  UInt4,

  HalfFloat1,
  HalfFloat2,
  HalfFloat3,
  HalfFloat4,

  Int_2_10_10_10_REV,
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

enum BlendOp : uint8_t {
  BlendOp_Add = 0,
  BlendOp_Subtract,
  BlendOp_ReverseSubtract,
  BlendOp_Min,
  BlendOp_Max
};

enum BlendFactor : uint8_t {
  BlendFactor_Zero = 0,
  BlendFactor_One,
  BlendFactor_SrcColor,
  BlendFactor_OneMinusSrcColor,
  BlendFactor_SrcAlpha,
  BlendFactor_OneMinusSrcAlpha,
  BlendFactor_DstColor,
  BlendFactor_OneMinusDstColor,
  BlendFactor_DstAlpha,
  BlendFactor_OneMinusDstAlpha,
  BlendFactor_SrcAlphaSaturated,
  BlendFactor_BlendColor,
  BlendFactor_OneMinusBlendColor,
  BlendFactor_BlendAlpha,
  BlendFactor_OneMinusBlendAlpha,
  BlendFactor_Src1Color,
  BlendFactor_OneMinusSrc1Color,
  BlendFactor_Src1Alpha,
  BlendFactor_OneMinusSrc1Alpha
};



struct VertexInput final {
  enum { LVK_VERTEX_ATTRIBUTES_MAX = 16 };
  enum { LVK_VERTEX_BUFFER_MAX = 16 };
  struct VertexAttribute final {
    uint32_t location = 0; // a buffer which contains this attribute stream
    uint32_t binding = 0;
    VertexFormat format = VertexFormat::Invalid; // per-element format
    uintptr_t offset = 0; // an offset where the first element of this attribute stream starts
  } attributes[LVK_VERTEX_ATTRIBUTES_MAX];
  struct VertexInputBinding final {
    uint32_t stride = 0;
  } inputBindings[LVK_VERTEX_BUFFER_MAX];

  uint32_t getNumAttributes() const {
    uint32_t n = 0;
    while (n < LVK_VERTEX_ATTRIBUTES_MAX && attributes[n].format != VertexFormat::Invalid) {
      n++;
    }
    return n;
  }
  uint32_t getNumInputBindings() const {
    uint32_t n = 0;
    while (n < LVK_VERTEX_BUFFER_MAX && inputBindings[n].stride) {
      n++;
    }
    return n;
  }
  uint32_t getVertexSize() const;

  bool operator==(const VertexInput& other) const {
    return memcmp(this, &other, sizeof(VertexInput)) == 0;
  }
};

enum CompareOp : uint8_t {
  CompareOp_Never = 0,
  CompareOp_Less,
  CompareOp_Equal,
  CompareOp_LessEqual,
  CompareOp_Greater,
  CompareOp_NotEqual,
  CompareOp_GreaterEqual,
  CompareOp_AlwaysPass
};

enum StencilOp : uint8_t {
  StencilOp_Keep = 0,
  StencilOp_Zero,
  StencilOp_Replace,
  StencilOp_IncrementClamp,
  StencilOp_DecrementClamp,
  StencilOp_Invert,
  StencilOp_IncrementWrap,
  StencilOp_DecrementWrap
};



struct StencilState {
  StencilOp stencilFailureOp = StencilOp_Keep;
  StencilOp depthFailureOp = StencilOp_Keep;
  StencilOp depthStencilPassOp = StencilOp_Keep;
  CompareOp stencilCompareOp = CompareOp_AlwaysPass;
  uint32_t readMask = (uint32_t)~0;
  uint32_t writeMask = (uint32_t)~0;
};



struct ColorAttachment {
  Format format = Format_Invalid;
  bool blendEnabled = false;
  BlendOp rgbBlendOp = BlendOp::BlendOp_Add;
  BlendOp alphaBlendOp = BlendOp::BlendOp_Add;
  BlendFactor srcRGBBlendFactor = BlendFactor_One;
  BlendFactor srcAlphaBlendFactor = BlendFactor_One;
  BlendFactor dstRGBBlendFactor = BlendFactor_Zero;
  BlendFactor dstAlphaBlendFactor = BlendFactor_Zero;
};

struct ShaderModuleHandle;
struct RenderPipelineDesc final {
  Topology topology = Topology_Triangle;

  VertexInput vertexInput;

  ShaderModuleHandle smVert;
  ShaderModuleHandle smTesc;
  ShaderModuleHandle smTese;
  ShaderModuleHandle smGeom;
  ShaderModuleHandle smTask;
  ShaderModuleHandle smMesh;
  ShaderModuleHandle smFrag;

  SpecializationConstantDesc specInfo = {};

  const char* entryPointVert = "main";
  const char* entryPointTesc = "main";
  const char* entryPointTese = "main";
  const char* entryPointGeom = "main";
  const char* entryPointTask = "main";
  const char* entryPointMesh = "main";
  const char* entryPointFrag = "main";

  ColorAttachment color[LVK_MAX_COLOR_ATTACHMENTS] = {};
  Format depthFormat = Format_Invalid;
  Format stencilFormat = Format_Invalid;

  CullMode cullMode = CullMode_None;
  WindingMode frontFaceWinding = WindingMode_CCW;
  PolygonMode polygonMode = PolygonMode_Fill;

  StencilState backFaceStencil = {};
  StencilState frontFaceStencil = {};

  uint32_t samplesCount = 1u;
  uint32_t patchControlPoints = 0;
  float minSampleShading = 0.0f;

  const char* debugName = "";

  uint32_t getNumColorAttachments() const {
    uint32_t n = 0;
    while (n < LVK_MAX_COLOR_ATTACHMENTS && color[n].format != Format_Invalid) {
      n++;
    }
    return n;
  }
};

struct RenderPipelineState final {
  RenderPipelineDesc desc_;

  uint32_t numBindings_ = 0;
  uint32_t numAttributes_ = 0;
  VkVertexInputBindingDescription vkBindings_[VertexInput::LVK_VERTEX_BUFFER_MAX] = {};
  VkVertexInputAttributeDescription vkAttributes_[VertexInput::LVK_VERTEX_ATTRIBUTES_MAX] = {};

  // non-owning, the last seen VkDescriptorSetLayout from VulkanContext::vkDSL_ (if the context has a new layout, invalidate all VkPipeline objects)
  VkDescriptorSetLayout lastVkDescriptorSetLayout_ = VK_NULL_HANDLE;

  VkShaderStageFlags shaderStageFlags_ = 0;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;

  void* specConstantDataStorage_ = nullptr;
};