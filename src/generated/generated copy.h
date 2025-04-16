#pragma once
b32 handle_is_valid(ShaderModuleHandle handle)
{
    b32 result = false;
    if (handle.gen != 0)
    {
        result = true;
    }
    return result;
}

b32 handle_is_empty(ShaderModuleHandle handle)
{
    b32 result = false;
    if (handle.gen == 0)
    {
        result = true;
    }
    return result;
}

struct Pool_ShaderModule
{
    struct Pool_ShaderModule_Entry
    {
        u32 gen = 1;
        ShaderModuleState data;
    };
    u32 count = 1;
    Pool_ShaderModule_Entry entries[15];
};

ShaderModuleHandle pool_create(Pool_ShaderModule *pool, ShaderModuleState data)
{
    ShaderModuleHandle handle = {0};
    assert(pool->count < array_count(pool->entries));
    handle.idx = pool->count++;
    handle.gen = 1;
    Pool_ShaderModule::Pool_ShaderModule_Entry entry = {.gen = 1, .data = data};
    pool->entries[handle.idx] = entry;
    return handle;
}

ShaderModuleState *pool_get(Pool_ShaderModule *pool, ShaderModuleHandle handle)
{
    ShaderModuleState *result = 0;
    if(handle_is_valid(handle));
    {
        result = &pool->entries[handle.idx].data;
    }
    return result;
}

b32 handle_is_valid(RenderPipelineHandle handle)
{
    b32 result = false;
    if (handle.gen != 0)
    {
        result = true;
    }
    return result;
}

b32 handle_is_empty(RenderPipelineHandle handle)
{
    b32 result = false;
    if (handle.gen == 0)
    {
        result = true;
    }
    return result;
}

struct Pool_RenderPipeline
{
    struct Pool_RenderPipeline_Entry
    {
        u32 gen = 1;
        RenderPipelineState data;
    };
    u32 count = 1;
    Pool_RenderPipeline_Entry entries[15];
};

RenderPipelineHandle pool_create(Pool_RenderPipeline *pool, RenderPipelineState data)
{
    RenderPipelineHandle handle = {0};
    assert(pool->count < array_count(pool->entries));
    handle.idx = pool->count++;
    handle.gen = 1;
    Pool_RenderPipeline::Pool_RenderPipeline_Entry entry = {.gen = 1, .data = data};
    pool->entries[handle.idx] = entry;
    return handle;
}

RenderPipelineState *pool_get(Pool_RenderPipeline *pool, RenderPipelineHandle handle)
{
    RenderPipelineState *result = 0;
    if(handle_is_valid(handle));
    {
        result = &pool->entries[handle.idx].data;
    }
    return result;
}

b32 handle_is_valid(BufferHandle handle)
{
    b32 result = false;
    if (handle.gen != 0)
    {
        result = true;
    }
    return result;
}

b32 handle_is_empty(BufferHandle handle)
{
    b32 result = false;
    if (handle.gen == 0)
    {
        result = true;
    }
    return result;
}

struct Pool_Buffer
{
    struct Pool_Buffer_Entry
    {
        u32 gen = 1;
        VulkanBuffer data;
    };
    u32 count = 1;
    Pool_Buffer_Entry entries[15];
};

BufferHandle pool_create(Pool_Buffer *pool, VulkanBuffer data)
{
    BufferHandle handle = {0};
    assert(pool->count < array_count(pool->entries));
    handle.idx = pool->count++;
    handle.gen = 1;
    Pool_Buffer::Pool_Buffer_Entry entry = {.gen = 1, .data = data};
    pool->entries[handle.idx] = entry;
    return handle;
}

VulkanBuffer *pool_get(Pool_Buffer *pool, BufferHandle handle)
{
    VulkanBuffer *result = 0;
    if(handle_is_valid(handle));
    {
        result = &pool->entries[handle.idx].data;
    }
    return result;
}

b32 handle_is_valid(SamplerHandle handle)
{
    b32 result = false;
    if (handle.gen != 0)
    {
        result = true;
    }
    return result;
}

b32 handle_is_empty(SamplerHandle handle)
{
    b32 result = false;
    if (handle.gen == 0)
    {
        result = true;
    }
    return result;
}

struct Pool_Sampler
{
    struct Pool_Sampler_Entry
    {
        u32 gen = 1;
        VkSampler data;
    };
    u32 count = 1;
    Pool_Sampler_Entry entries[15];
};

SamplerHandle pool_create(Pool_Sampler *pool, VkSampler data)
{
    SamplerHandle handle = {0};
    assert(pool->count < array_count(pool->entries));
    handle.idx = pool->count++;
    handle.gen = 1;
    Pool_Sampler::Pool_Sampler_Entry entry = {.gen = 1, .data = data};
    pool->entries[handle.idx] = entry;
    return handle;
}

VkSampler *pool_get(Pool_Sampler *pool, SamplerHandle handle)
{
    VkSampler *result = 0;
    if(handle_is_valid(handle));
    {
        result = &pool->entries[handle.idx].data;
    }
    return result;
}

b32 handle_is_valid(TextureHandle handle)
{
    b32 result = false;
    if (handle.gen != 0)
    {
        result = true;
    }
    return result;
}

b32 handle_is_empty(TextureHandle handle)
{
    b32 result = false;
    if (handle.gen == 0)
    {
        result = true;
    }
    return result;
}

struct Pool_Texture
{
    struct Pool_Texture_Entry
    {
        u32 gen = 1;
        VulkanImage data;
    };
    u32 count = 1;
    Pool_Texture_Entry entries[15];
};

TextureHandle pool_create(Pool_Texture *pool, VulkanImage data)
{
    TextureHandle handle = {0};
    assert(pool->count < array_count(pool->entries));
    handle.idx = pool->count++;
    handle.gen = 1;
    Pool_Texture::Pool_Texture_Entry entry = {.gen = 1, .data = data};
    pool->entries[handle.idx] = entry;
    return handle;
}

VulkanImage *pool_get(Pool_Texture *pool, TextureHandle handle)
{
    VulkanImage *result = 0;
    if(handle_is_valid(handle));
    {
        result = &pool->entries[handle.idx].data;
    }
    return result;
}
