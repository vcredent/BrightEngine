/* ======================================================================== */
/* vulkan_context.cpp                                                       */
/* ======================================================================== */
/*                        This file is part of:                             */
/*                           PORTABLE ENGINE                                */
/* ======================================================================== */
/*                                                                          */
/* Copyright (C) 2022 Vcredent All rights reserved.                         */
/*                                                                          */
/* Licensed under the Apache License, Version 2.0 (the "License");          */
/* you may not use this file except in compliance with the License.         */
/*                                                                          */
/* You may obtain a copy of the License at                                  */
/*     http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                          */
/* Unless required by applicable law or agreed to in writing, software      */
/* distributed under the License is distributed on an "AS IS" BASIS,        */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, e1ither express or implied */
/* See the License for the specific language governing permissions and      */
/* limitations under the License.                                           */
/*                                                                          */
/* ======================================================================== */
#include "vulkan_context.h"
#include <algorithm>

VkAllocationCallbacks *allocation_callbacks = nullptr;

static VkSurfaceFormatKHR pick_surface_format(const VkSurfaceFormatKHR *surface_formats, uint32_t count)
{
    VkFormat format;
    for (uint32_t i = 0; i < count; i++) {
        format = surface_formats[i].format;
        if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_B8G8R8A8_UNORM ||
                format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
                format == VK_FORMAT_R16G16B16A16_SFLOAT)
            return surface_formats[i];
    }

    printf("Can't find our preferred formats... Falling back to first exposed format. Rendering may be incorrect.\n");

    assert(count >= 1);
    return surface_formats[0];
}

VulkanContext::VulkanContext()
{
    VkResult U_ASSERT_ONLY err;
    void *nextptr = nullptr;

    VkApplicationInfo application_info = {
        /* sType */ VK_STRUCTURE_TYPE_APPLICATION_INFO,
        /* pNext */ nextptr,
        /* pApplicationName */ "PortableX",
        /* applicationVersion */ VK_MAKE_VERSION(1, 0, 0),
        /* pEngineName */ "PortableEngine",
        /* engineVersion */ VK_MAKE_VERSION(1, 0, 0),
        /* apiVersion */ VK_API_VERSION_1_3
    };

    const char * extensions[] = {
            "VK_KHR_surface",
            "VK_KHR_win32_surface"
    };

    const char * layers[] = {
            "VK_LAYER_KHRONOS_validation" /* required install Vulkan SDK */
    };

    VkInstanceCreateInfo instance_create_info = {
        /* sType */ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        /* pNext */ nextptr,
        /* flags */ 0,
        /* pApplicationInfo */ &application_info,
        /* enabledLayerCount */ ARRAY_SIZE(layers),
        /* ppEnabledLayerNames */ layers,
        /* enabledExtensionCount */ ARRAY_SIZE(extensions),
        /* ppEnabledExtensionNames */ extensions
    };

    err = vkCreateInstance(&instance_create_info, allocation_callbacks, &inst);
    assert(!err);
}

VulkanContext::~VulkanContext()
{
    _clean_up_all();
}

void VulkanContext::_window_create(VkSurfaceKHR surface)
{
    _create_physical_device(surface);
    _create_device(&device);
    _initialize_window(gpu, surface);
    _create_swap_chain(device, window);
}

void VulkanContext::_clean_up_all()
{
    // clean handle about display window.
    _clean_up_swap_chain(device, window);
    vkDestroySurfaceKHR(inst, window->surface, allocation_callbacks);
    free(window);

    vkDestroyDevice(device, allocation_callbacks);
    vkDestroyInstance(inst, allocation_callbacks);
}

void VulkanContext::_create_physical_device(VkSurfaceKHR surface)
{
    VkResult U_ASSERT_ONLY err;

    /* select high performance GPU in this machine. */
    uint32_t gpu_count;
    uint32_t gpu_number = 0;
    err = vkEnumeratePhysicalDevices(inst, &gpu_count, nullptr);
    assert(!err);

    if (gpu_count <= 0)
        EXIT_FAIL("-engine error: no available device (GPU), gpu count is: %d\n", gpu_count);

    VkPhysicalDevice *physical_devices = (VkPhysicalDevice *) imalloc(sizeof(VkPhysicalDevice) * gpu_count);
    err = vkEnumeratePhysicalDevices(inst, &gpu_count, physical_devices);
    assert(!err);

    VkPhysicalDeviceProperties physical_device_properties;
    for (int i = 0; i < gpu_count; i++) {
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_properties);
        if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            gpu_number = i;
            break;
        }
    }

    gpu = physical_devices[gpu_number];
    free(physical_devices);

    _initialize_queues(gpu, surface);
}

void VulkanContext::_initialize_queues(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    VkResult U_ASSERT_ONLY err;
    /* find graphics and present queue family. */
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, nullptr);
    VkQueueFamilyProperties *queue_family_properties = (VkQueueFamilyProperties *) imalloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_family_properties);

    // search for a graphics and present queue in the array of queue
    // families, try to find one that supports both.
    for (uint32_t i = 0; i < queue_family_count; i++) {
        VkBool32 is_present;
        err = vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &is_present);
        assert(!err);
        if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (is_present == VK_TRUE)) {
            graph_queue_family = i;
            break;
        }
    }

    free(queue_family_properties);
}

void VulkanContext::_create_device(VkDevice *p_device)
{
    void *nextptr = nullptr;
    float priorities = 1.0f;
    VkResult U_ASSERT_ONLY err;

    VkDeviceQueueCreateInfo queue_create_info = {
        /* sType */ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        /* pNext */ nextptr,
        /* flags */ 0,
        /* queueFamilyIndex */ graph_queue_family,
        /* queueCount */ 1,
        /* pQueuePriorities */ &priorities
    };

    const char *extensions[] = {
        "VK_KHR_swapchain",
    };

    /* create device. */
    VkDeviceCreateInfo device_create_info = {
        /* sType */ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        /* pNext */ nextptr,
        /* flags */ 0,
        /* queueCreateInfoCount */ 1,
        /* pQueueCreateInfos */ &queue_create_info,
        /* enabledLayerCount */ 0,
        /* ppEnabledLayerNames */ nullptr,
        /* enabledExtensionCount */ ARRAY_SIZE(extensions),
        /* ppEnabledExtensionNames */ extensions,
        /* pEnabledFeatures */ nullptr,
    };

    err = vkCreateDevice(gpu, &device_create_info, allocation_callbacks, p_device);
    assert(!err);
    vkGetDeviceQueue(device, graph_queue_family, 0, &graph_command_queue);
}

void VulkanContext::_initialize_window(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    void *nextptr = nullptr;
    VkResult U_ASSERT_ONLY err;

    if (window)
        return;

    /* imalloc display window struct and set surface */
    window = (Window *) imalloc(sizeof(Window));
    memset(window, 0, sizeof(Window));
    window->surface = surface;

    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &window->capabilities);
    assert(!err);

    window->width = window->capabilities.currentExtent.width;
    window->height = window->capabilities.currentExtent.height;

    /* try to find surface format */
    uint32_t surface_format_count;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surface_format_count, nullptr);
    assert(!err);

    if (surface_format_count <= 0)
        EXIT_FAIL("-engine error: vulkan surface image format error, no available surface format, format count: %d\n", surface_format_count);

    VkSurfaceFormatKHR *surface_image_formats = (VkSurfaceFormatKHR *) imalloc(sizeof(VkSurfaceFormatKHR) * surface_format_count);
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surface_format_count, surface_image_formats);
    assert(!err);

    VkSurfaceFormatKHR final_surface_format = pick_surface_format(surface_image_formats, surface_format_count);
    window->format = final_surface_format.format;
    window->colorspace = final_surface_format.colorSpace;
    free(surface_image_formats);

    // we hope the image buffer count eq 3 buffer count
    window->image_buffer_count =
            std::clamp<uint32_t>(3, window->capabilities.minImageCount, window->capabilities.maxImageCount);

    // surface pre transform
    window->transform = (window->capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ?
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : window->capabilities.currentTransform;

    // composite alpha
    window->composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // present mode
    window->present_mode = VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanContext::_create_swap_chain(VkDevice device, Window *window)
{
    void *nextptr = nullptr;
    VkResult U_ASSERT_ONLY err;

    VkCommandPoolCreateInfo command_pool_create_info = {
            /* sType */ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            /* pNext */ nextptr,
            /* flags */ 0,
            /* queueFamilyIndex */ graph_queue_family
    };

    err = vkCreateCommandPool(device, &command_pool_create_info, allocation_callbacks, &window->command_pool);
    assert(!err);

    VkSwapchainCreateInfoKHR swapchain_create_info = {
            /* sType */ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            /* pNext */ nextptr,
            /* flags */ 0,
            /* surface */ window->surface,
            /* minImageCount */ window->image_buffer_count,
            /* imageFormat */ window->format,
            /* imageColorSpace */ window->colorspace,
            /* imageExtent */
                {
                    .width = window->width,
                    .height = window->height,
                },
            /* imageArrayLayers */ 1,
            /* imageUsage */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            /* imageSharingMode */ VK_SHARING_MODE_EXCLUSIVE,
            /* queueFamilyIndexCount */ 1,
            /* pQueueFamilyIndices */ &graph_queue_family,
            /* preTransform */ window->transform,
            /* compositeAlpha */ window->composite_alpha,
            /* presentMode */ window->present_mode,
            /* clipped */ VK_TRUE,
            /* oldSwapchain */ window->swapchain,
    };

    err = vkCreateSwapchainKHR(device, &swapchain_create_info, allocation_callbacks, &window->swapchain);
    assert(!err);

    // create swap chain image resource by image buffer count,
    // create handles:
    //   - VkImage
    //   - VkImageView
    //   - VkFramebuffer
    //   - VkCommandBuffer
    window->swap_chain_resources =
            (SwapchainImageResource *) imalloc(sizeof(SwapchainImageResource) * window->image_buffer_count);

    VkImageCreateInfo image_create_info = {
        /* sType */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext */ nextptr,
        /* flags */ 0,
        /* imageType */ VK_IMAGE_TYPE_2D,
        /* format */ window->format,
        /* extent */
            {
                .width = window->width,
                .height = window->height,
                .depth = 1
            },
        /* mipLevels */ 1,
        /* arrayLayers */ 1,
        /* samples */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling */ VK_IMAGE_TILING_LINEAR,
        /* usage */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* sharingMode */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount */ 1,
        /* pQueueFamilyIndices */ &graph_queue_family,
        /* initialLayout */ VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImageViewCreateInfo image_view_create_info = {
        /* sType */ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        /* pNext */ nextptr,
        /* flags */ 0,
        /* image */ nullptr,
        /* viewType */ VK_IMAGE_VIEW_TYPE_2D,
        /* format */ window->format,
        /* components */
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        /* subresourceRange */
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkFramebufferCreateInfo framebuffer_create_info = {
         /* sType */ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         /* pNext */ nextptr,
         /* flags */ 0,
         /* renderPass */ nullptr,
         /* attachmentCount */ 0,
         /* pAttachments */ nullptr,
         /* width */ window->width,
         /* height */ window->height,
         /* layers */ 0
    };

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        /* sType */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        /* pNext */ nextptr,
        /* commandPool */ window->command_pool,
        /* level */ VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        /* commandBufferCount */ 1,
    };

    for (uint32_t i = 0; i < window->image_buffer_count; i++) {
        // create image buffer
        err = vkCreateImage(device, &image_create_info, allocation_callbacks, &(window->swap_chain_resources[i].image));
        assert(!err);

        // use current create image buffer object to create image view
        image_view_create_info.image = window->swap_chain_resources[i].image;
        err = vkCreateImageView(device, &image_view_create_info, allocation_callbacks, &(window->swap_chain_resources[i].image_view));
        assert(!err);

        // create framebuffer
        // err = vkCreateFramebuffer(device, &framebuffer_create_info, allocation_callbacks, &(window->swap_chain_resources[i].framebuffer));
        // assert(!err);

        // allocate command buffer
        err = vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &(window->swap_chain_resources[i].command_buffer));
        assert(!err);
    }
}

void VulkanContext::_clean_up_swap_chain(VkDevice device, Window *window)
{
    vkDestroySwapchainKHR(device, window->swapchain, allocation_callbacks);
    window->swapchain = nullptr;
    vkDestroyCommandPool(device, window->command_pool, allocation_callbacks);

    // clean up swap chain resource
    for (uint32_t i = 0; i < window->image_buffer_count; i++) {
        vkDestroyImage(device, window->swap_chain_resources[i].image, allocation_callbacks);
        vkDestroyImageView(device, window->swap_chain_resources[i].image_view, allocation_callbacks);
        // vkDestroyFramebuffer(device, window->swap_chain_resources[i].framebuffer, allocation_callbacks);
    }
}

void VulkanContext::_update_swap_chain(VkDevice device, Window *window)
{
    if (window->swapchain)
        _clean_up_swap_chain(device, window);
    _create_swap_chain(device, window);
}