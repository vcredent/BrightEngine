/* ======================================================================== */
/* vulkan_context.h                                                         */
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
#include <vulkan/vulkan.h>
#include "hardware.h"

class VulkanContext : public HardwareDevice {
public:
    explicit VulkanContext();
    ~VulkanContext() override;

public:
    VkInstance get_instance() { return inst; }
    VkDevice get_device() { return device; }

protected:
    void _window_create(VkSurfaceKHR surface); /* initialize */
    void _clean_up_all(); /* destroy */

private:
    void _create_physical_device(VkSurfaceKHR surface);
    void _initialize_queues(VkPhysicalDevice gpu, VkSurfaceKHR surface);
    void _create_device(VkDevice *p_device);

    typedef struct SwapchainImageResource {
        VkImage image;
        VkImageView image_view;
        VkFramebuffer framebuffer;
        VkCommandBuffer command_buffer;
    } SwapchainImageResource;

    typedef struct Window {
        VkSurfaceKHR surface;
        VkSurfaceCapabilitiesKHR capabilities;
        VkSwapchainKHR swapchain = nullptr;
        VkFormat format;
        VkColorSpaceKHR colorspace;
        uint32_t image_buffer_count;
        VkSurfaceTransformFlagBitsKHR transform;
        uint32_t width;
        uint32_t height;
        VkCompositeAlphaFlagBitsKHR composite_alpha;
        VkPresentModeKHR present_mode;
        SwapchainImageResource *swap_chain_resources = nullptr;
        VkCommandPool command_pool;
    } Window;

    void _initialize_window(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
    void _create_swap_chain(VkDevice device, Window *window);
    void _clean_up_swap_chain(VkDevice device, Window *window);
    void _update_swap_chain(VkDevice device, Window *window);

private:
    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    uint32_t graph_queue_family;
    VkQueue graph_command_queue;
    Window *window = nullptr;
};
