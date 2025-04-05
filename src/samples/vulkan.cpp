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
struct VulkanContext
{
    VkDevice instance;
    VkDevice device;
};

// OBS: probably just a handle. Store a HANDLE custom type inside the custom type window that can link to the actual OS window?
VulkanContext vulkan_create_context_with_swapchain(Arena *arena, OS_Window window, u32 width, u32 height)
{
    VulkanContext context = {0};

    // Instance creation
    const char* k_default_validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
    u32 num_layer_props = 0;
    vkEnumerateInstanceLayerProperties(&num_layer_props, 0);
    printf("layer props: %d\n", num_layer_props);

    VkLayerProperties *layer_props = arena_push_size(arena, VkLayerProperties, num_layer_props);
    vkEnumerateInstanceLayerProperties(&num_layer_props, layer_props);

    b32 enabled_validation_layers = false;
    const char *k_def_validation_layer = "VK_LAYER_KHRONOS_validation";
    {
        u32 layer_idx = 0;
        while(!enabled_validation_layers && layer_idx < num_layer_props)
        {
            VkLayerProperties *layer_prop = layer_props + layer_idx;
            if(!strcmp(layer_prop->layerName, k_def_validation_layer))
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

    VkExtensionProperties all_instance_extensions[100];
    u32 all_instance_extensions_count = 0;
    {
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &all_instance_extensions_count, nullptr));
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &all_instance_extensions_count, all_instance_extensions));
        printf("all instance Count is %d\n", all_instance_extensions_count);
    }

    // collect instance extensions from all validation layers
    if (enabled_validation_layers) 
    {
        u32 count = 0;
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(k_def_validation_layer, &count, nullptr));
        if (count > 0) 
        {
            printf("Count is %d\n", count);
            const size_t sz = all_instance_extensions_count;
            VK_ASSERT(vkEnumerateInstanceExtensionProperties(k_def_validation_layer, &count, all_instance_extensions + sz));
            all_instance_extensions_count += count;
        }
    }

    //printf("%d %s %s\n: ", all_instance_extensions_count, all_instance_extensions[0].extensionName, all_instance_extensions[all_instance_extensions_count - 1].extensionName);


    const char* instance_extension_names[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        #if defined(_WIN32)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
        #elif defined(__linux__)
        #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
            VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
        #else
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        #endif
        #elif defined(__APPLE__)
            VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
            VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
        #endif
        #if defined(LVK_WITH_VULKAN_PORTABILITY)
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
    };

    if(enabled_validation_layers)
    {
        printf("Fine!\n");
    }




    const VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "LVK/Vulkan",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "LVK/Vulkan",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
    };




    return context;
}

int main() {
    u32 window_width = 1280;
    u32 window_height = 720;
    global_w32_window = os_win32_open_window("Vulkan example", window_width, window_height, win32_main_callback, WindowOpenFlags_Centered);
    Arena arena {};
    arena_init(&arena, mb(3));
    vulkan_create_context_with_swapchain(&arena, global_w32_window, window_width, window_height);
    while(global_w32_window.is_running)
    {
        
    }
}