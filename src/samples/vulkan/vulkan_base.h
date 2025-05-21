#pragma once

const char* k_def_validation_layer[] = {"VK_LAYER_KHRONOS_validation"};
enum { LVK_MAX_COLOR_ATTACHMENTS = 8 };
enum { LVK_MAX_MIP_LEVELS = 16 };

// These bindings should match GLSL declarations injected into shaders in VulkanContext::createShaderModule().
enum Bindings {
  kBinding_Textures = 0,
  kBinding_Samplers = 1,
  kBinding_StorageImages = 2,
  kBinding_YUVImages = 3,
  kBinding_AccelerationStructures = 4,
  kBinding_NumBindings = 5,
};

enum Topology : u8 {
    Topology_Point,
    Topology_Line,
    Topology_LineStrip,
    Topology_Triangle,
    Topology_TriangleStrip,
    Topology_Patch,
};

enum CullMode : u8 { CullMode_None, CullMode_Front, CullMode_Back };
enum WindingMode : u8 { WindingMode_CCW, WindingMode_CW };

enum CompareOp : u8 
{
    CompareOp_Never = 0,
    CompareOp_Less,
    CompareOp_Equal,
    CompareOp_LessEqual,
    CompareOp_Greater,
    CompareOp_NotEqual,
    CompareOp_GreaterEqual,
    CompareOp_AlwaysPass
};

struct DepthState
{
    CompareOp compareOp = CompareOp_AlwaysPass;
    b32 isDepthWriteEnabled = false;
};

enum PolygonMode : u8 
{
    PolygonMode_Fill = 0,
    PolygonMode_Line = 1,
};

struct SpecializationConstantEntry 
{
    u32 constantId = 0;
    u32 offset = 0; // offset within ShaderSpecializationConstantDesc::data
    size_t size = 0;
};

struct SpecializationConstantDesc 
{
    enum { LVK_SPECIALIZATION_CONSTANTS_MAX = 16 };
    SpecializationConstantEntry entries[LVK_SPECIALIZATION_CONSTANTS_MAX] = {};
    const void* data = nullptr;
    size_t dataSize = 0;
    u32 getNumSpecializationConstants() const {
        u32 n = 0;
        while (n < LVK_SPECIALIZATION_CONSTANTS_MAX && entries[n].size) {
        n++;
        }
        return n;
    }
};

enum class VertexFormat 
{
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

enum Format : u8 
{
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

enum LoadOp : u8 {
    LoadOp_Invalid = 0,
    LoadOp_DontCare,
    LoadOp_Load,
    LoadOp_Clear,
    LoadOp_None,
};

enum StoreOp : u8 
{
    StoreOp_DontCare = 0,
    StoreOp_Store,
    StoreOp_MsaaResolve,
    StoreOp_None,
};

enum ShaderStage : u8 
{
    Stage_Vert,
    Stage_Tesc,
    Stage_Tese,
    Stage_Geom,
    Stage_Frag,
    Stage_Comp,
    Stage_Task,
    Stage_Mesh,
    // ray tracing
    Stage_RayGen,
    Stage_AnyHit,
    Stage_ClosestHit,
    Stage_Miss,
    Stage_Intersection,
    Stage_Callable,
};





enum BlendOp : u8 
{
    BlendOp_Add = 0,
    BlendOp_Subtract,
    BlendOp_ReverseSubtract,
    BlendOp_Min,
    BlendOp_Max
};

enum BlendFactor : u8 
{
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

struct ScissorRect 
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
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

struct Viewport {
  float x = 0.0f;
  float y = 0.0f;
  float width = 1.0f;
  float height = 1.0f;
  float minDepth = 0.0f;
  float maxDepth = 1.0f;
};



enum StencilOp : u8 
{
    StencilOp_Keep = 0,
    StencilOp_Zero,
    StencilOp_Replace,
    StencilOp_IncrementClamp,
    StencilOp_DecrementClamp,
    StencilOp_Invert,
    StencilOp_IncrementWrap,
    StencilOp_DecrementWrap
};

struct Dependencies {
    enum { LVK_MAX_SUBMIT_DEPENDENCIES = 4 };
    TextureHandle textures[LVK_MAX_SUBMIT_DEPENDENCIES] = {};
    BufferHandle buffers[LVK_MAX_SUBMIT_DEPENDENCIES] = {};
};


struct StencilState 
{
    StencilOp stencilFailureOp = StencilOp_Keep;
    StencilOp depthFailureOp = StencilOp_Keep;
    StencilOp depthStencilPassOp = StencilOp_Keep;
    CompareOp stencilCompareOp = CompareOp_AlwaysPass;
    u32 readMask = (u32)~0;
    u32 writeMask = (u32)~0;
};

struct ColorAttachment 
{
    Format format = Format_Invalid;
    b32 blendEnabled = false;
    BlendOp rgbBlendOp = BlendOp::BlendOp_Add;
    BlendOp alphaBlendOp = BlendOp::BlendOp_Add;
    BlendFactor srcRGBBlendFactor = BlendFactor_One;
    BlendFactor srcAlphaBlendFactor = BlendFactor_One;
    BlendFactor dstRGBBlendFactor = BlendFactor_Zero;
    BlendFactor dstAlphaBlendFactor = BlendFactor_Zero;
};

struct ShaderModuleDesc 
{
    ShaderStage stage = Stage_Frag;
    const char* data = nullptr;
    size_t dataSize = 0; // if `dataSize` is non-zero, interpret `data` as binary shader data
    const char* debugName = "";

    ShaderModuleDesc(const char* source, ShaderStage stage, const char* debugName) : stage(stage), data(source), debugName(debugName) {}
    ShaderModuleDesc(const void* data, size_t dataLength, ShaderStage stage, const char* debugName) :
        stage(stage), data(static_cast<const char*>(data)), dataSize(dataLength), debugName(debugName) {
        Assert(dataSize);
    }
};

struct Framebuffer  
{
    struct AttachmentDesc 
    {
        TextureHandle texture;
        TextureHandle resolveTexture;
    };

    AttachmentDesc color[LVK_MAX_COLOR_ATTACHMENTS] = {};
    AttachmentDesc depthStencil;

    const char* debugName = "";

    u32 getNumColorAttachments()  
    {
        u32 n = 0;
        while (n < LVK_MAX_COLOR_ATTACHMENTS && color[n].texture) 
        {
        n++;
        }
        return n;
    }
};

struct VertexInput 
{
    enum { LVK_VERTEX_ATTRIBUTES_MAX = 16 };
    enum { LVK_VERTEX_BUFFER_MAX = 16 };
    struct VertexAttribute  
    {
        u32 location = 0; // a buffer which contains this attribute stream
        u32 binding = 0;
        VertexFormat format = VertexFormat::Invalid; // per-element format
        uintptr_t offset = 0; // an offset where the first element of this attribute stream starts
    } attributes[LVK_VERTEX_ATTRIBUTES_MAX];

    struct VertexInputBinding  {
        u32 stride = 0;
    } inputBindings[LVK_VERTEX_BUFFER_MAX];

    u32 getNumAttributes()
    {
        u32 n = 0;
        while (n < LVK_VERTEX_ATTRIBUTES_MAX && attributes[n].format != VertexFormat::Invalid)
        {
            n++;
        }
        return n;
    }

    u32 getNumInputBindings()
    {
        u32 n = 0;
        while (n < LVK_VERTEX_BUFFER_MAX && inputBindings[n].stride)
        {
            n++;
        }
        return n;
    }

    u32 getVertexSize();

    b32 operator==(const VertexInput& other)  
    {
        return memcmp(this, &other, sizeof(VertexInput)) == 0;
    }
};

struct ShaderModuleState  
{
    VkShaderModule sm = VK_NULL_HANDLE;
    u32 pushConstantsSize = 0;
};

struct RenderPipelineDesc  
{
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

    u32 samplesCount = 1u;
    u32 patchControlPoints = 0;
    float minSampleShading = 0.0f;

    const char* debugName = "";

    u32 getNumColorAttachments()
    {
        u32 n = 0;
        while (n < LVK_MAX_COLOR_ATTACHMENTS && color[n].format != Format_Invalid)
        {
            n++;
        }
        return n;
    }
};

struct RenderPass final {
  struct AttachmentDesc final {
    LoadOp loadOp = LoadOp_Invalid;
    StoreOp storeOp = StoreOp_Store;
    uint8_t layer = 0;
    uint8_t level = 0;
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float clearDepth = 1.0f;
    uint32_t clearStencil = 0;
  };

  AttachmentDesc color[LVK_MAX_COLOR_ATTACHMENTS] = {};
  AttachmentDesc depth = {.loadOp = LoadOp_DontCare, .storeOp = StoreOp_DontCare};
  AttachmentDesc stencil = {.loadOp = LoadOp_Invalid, .storeOp = StoreOp_DontCare};

  uint32_t getNumColorAttachments() const {
    uint32_t n = 0;
    while (n < LVK_MAX_COLOR_ATTACHMENTS && color[n].loadOp != LoadOp_Invalid) {
      n++;
    }
    return n;
  }
};

internal VkResult
setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, const char* name)
{
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

struct RenderPipelineState  
{
  RenderPipelineDesc desc_;

  u32 numBindings_ = 0;
  u32 numAttributes_ = 0;
  VkVertexInputBindingDescription vkBindings_[VertexInput::LVK_VERTEX_BUFFER_MAX] = {};
  VkVertexInputAttributeDescription vkAttributes_[VertexInput::LVK_VERTEX_ATTRIBUTES_MAX] = {};

  // non-owning, the last seen VkDescriptorSetLayout from VulkanContext::vkDSL_ (if the context has a new layout, invalidate all VkPipeline objects)
  VkDescriptorSetLayout lastVkDescriptorSetLayout_ = VK_NULL_HANDLE;

  VkShaderStageFlags shaderStageFlags_ = 0;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;

  void* specConstantDataStorage_ = nullptr;
};

struct VulkanImage
{
     // clang-format off
    [[nodiscard]] inline bool isSampledImage() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_SAMPLED_BIT) > 0; }
    [[nodiscard]] inline bool isStorageImage() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT) > 0; }
    [[nodiscard]] inline bool isColorAttachment() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) > 0; }
    [[nodiscard]] inline bool isDepthAttachment() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) > 0; }
    [[nodiscard]] inline bool isAttachment() const { return (vkUsageFlags_ & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) > 0; }
    // clang-format on

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
    u32 numLevels_ = 1u;
    u32 numLayers_ = 1u;
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
    [[nodiscard]] inline u8* getMappedPtr() const { return static_cast<u8*>(mappedPtr_); }
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