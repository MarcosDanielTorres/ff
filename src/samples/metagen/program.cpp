#include <stdio.h>

struct VulkanImage {};
struct VulkanBuffer {};
struct VkSampler {};

#define POOL(IGNORED, ...) IGNORED
struct Context
{
    POOL(VulkanImage textures_pool, Texture);
    POOL(VkSampler samplers_pool, Sampler);
    POOL(VulkanBuffer buffers_pool, Buffer);
};

int main()
{
    Context context;
}