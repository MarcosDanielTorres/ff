#include <stdio.h>
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "os/os_core.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "os/os_core.cpp"
#include <string>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

const char* k_def_validation_layer[] = {"VK_LAYER_KHRONOS_validation"};

#define VK_ASSERT(func)                                            \
  {                                                                \
    const VkResult vk_assert_result = func;                        \
    if (vk_assert_result != VK_SUCCESS) {                          \
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
    u64 guid = 0;
    VkPhysicalDeviceType type;
};

struct VulkanQueryDeviceResult
{
    u32 num_compatible_devices;
    VulkanDeviceHWDesc out_devices[8];
};

struct VulkanContext
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    b32 enabled_validation_layers;
};

internal VulkanQueryDeviceResult
vulkan_query_devices(Arena *arena, VulkanContext context, VkPhysicalDeviceType desired_type)
{
    VulkanQueryDeviceResult result = {0};
    u32 device_count = 0;
    TempArena temp_arena = temp_begin(arena);
    vkEnumeratePhysicalDevices(context.instance, &device_count, 0);
    VkPhysicalDevice *phys_devices = arena_push_size(temp_arena.arena, VkPhysicalDevice, device_count);
    vkEnumeratePhysicalDevices(context.instance, &device_count, phys_devices);

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
vulkan_create_context(Arena *arena)
{
    VulkanContext context = {0};
    {
        // Instance creation
        TempArena temp_arena = temp_begin(arena);
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
        /*
        {
            for(u32 layer = 0; layer < num_layer_props; layer++)
            {
                for(u32 def_val_layer = 0; def_val_layer < array_count(k_default_validation_layers); def_val_layer++)
                {
                    if(!strcmp(layer_props[layer].layerName, k_default_validation_layers[def_val_layer]))
                    {
                        enabled_validation_layers = true;
                    }
                }
            }
        }
        */

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
        //volkLoadInstance(vkInstance_);

        printf("\nVulkan instance extensions:\n");

        for (u32 i = 0; i < all_instance_extensions_count; i++) {
            printf("  %s\n", all_instance_extensions[i].extensionName);
        }

        context.enabled_validation_layers = enabled_validation_layers;

        temp_end(temp_arena);
    }

    #if 0
    {
        // surface creation
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        const VkWin32SurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = (HWND)window,
        };
        VK_ASSERT(vkCreateWin32SurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
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
    return context;
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

internal VkExtensionProperties *
vulkan_get_device_extension_props(Arena *arena, VkPhysicalDevice phys_dev, const char *validation_layer = 0)
{
    TempArena temp_arena = temp_begin(arena);
    u32 num_extensions = 0;
    vkEnumerateDeviceExtensionProperties(phys_dev, validation_layer, &num_extensions, 0);
    VkExtensionProperties *all_device_extensions = arena_push_size(temp_arena.arena, VkExtensionProperties, num_extensions);
    vkEnumerateDeviceExtensionProperties(phys_dev, validation_layer, &num_extensions, all_device_extensions);
    // copy it back to arena?
    memcpy((void*)temp_arena.pos, all_device_extensions, num_extensions);
    temp_end(temp_arena);
    return (VkExtensionProperties*) temp_arena.pos;
}

void vulkan_init_context(Arena *arena, VulkanContext *context, VulkanDeviceHWDesc *decs)
{
    TempArena temp_arena = temp_begin(arena);
    context->physical_device = (VkPhysicalDevice) decs->guid;
    b32 use_staging = !vulkan_is_host_visible_single_heap_memory(context->physical_device);


    u32 all_device_extensions_count = 0;
    VkExtensionProperties *all_device_extensions = vulkan_get_device_extension_props(temp_arena.arena, context->physical_device, 0);
    {
        //getDeviceExtensionProps(context->phsyical_device, all_device_extensions);
        //if(context->enabled_validation_layers)
        //{
        //    for (const char* layer : kDefaultValidationLayers) {
        //        getDeviceExtensionProps(vkPhysicalDevice_, all_device_extensions, layer);
        //    }
        //}
        printf("all device extensions count is %d\n", all_device_extensions_count);
    }


    //if (vulkan_has_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, allDeviceExtensions)) {
    //    addNextPhysicalDeviceProperties(&accelerationStructureProperties_);
    //}
    //if (vulkan_has_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, allDeviceExtensions)) {
    //    addNextPhysicalDeviceProperties(&rayTracingPipelineProperties_);
    //}

    //vkGetPhysicalDeviceFeatures2(vkPhysicalDevice_, &vkFeatures10_);
    //vkGetPhysicalDeviceProperties2(vkPhysicalDevice_, &vkPhysicalDeviceProperties2_);

    temp_end(temp_arena);

}

void vulkan_init_swapchain(VulkanContext *context, u32 width, u32 height)
{

}

// OBS: probably just a handle. Store a HANDLE custom type inside the custom type window that can link to the actual OS window?
internal VulkanContext
vulkan_create_context_with_swapchain(Arena *arena, OS_Window window, u32 width, u32 height)
{
    VulkanContext context = vulkan_create_context(arena);
    VulkanQueryDeviceResult queried_devices = vulkan_query_devices(arena, context, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    
    vulkan_init_context(arena, &context, &queried_devices.out_devices[0]);

    return context;
}

int main() 
{
    u32 window_width = 1280;
    u32 window_height = 720;
    global_w32_window = os_win32_open_window("Vulkan example", window_width, window_height, win32_main_callback, WindowOpenFlags_Centered);
    Arena arena {};
    arena_init(&arena, mb(3));
    VulkanContext context = vulkan_create_context_with_swapchain(&arena, global_w32_window, window_width, window_height);
    while(global_w32_window.is_running)
    {
        
    }
}