#include <stdio.h>

struct VulkanImage {};
struct VulkanBuffer {};
struct VkSampler {};
struct RenderPipelineState{};
struct ShaderModuleState{};

#define POOL(IGNORED, ...) IGNORED
struct Context
{
    POOL(VulkanImage textures_pool, Texture);
    POOL(VkSampler samplers_pool, Sampler);
    POOL(VulkanBuffer buffers_pool, Buffer);
    POOL(RenderPipelineState render_pipelines_pool, RenderPipeline);
    POOL(ShaderModuleState shader_modules_pool, ShaderModule);
};

int main()
{
    Context context;
}