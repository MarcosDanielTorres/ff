#pragma once
struct ShaderModuleHandle
{
    u32 idx;
    u32 gen;

};
struct RenderPipelineHandle
{
    u32 idx;
    u32 gen;
};
struct BufferHandle
{
    u32 idx;
    u32 gen;
    explicit operator bool() const {
        return gen != 0;
    }
};
struct SamplerHandle
{
    u32 idx;
    u32 gen;
};
struct TextureHandle
{
    u32 idx;
    u32 gen;
    // allow conditions 'if (handle)'
    explicit operator bool() const {
        return gen != 0;
    }
};