/**
 * @file:   vulkan_device.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016 Jesper Stefansson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "vulkan_render.h"

extern VulkanDevice *g_vulkan;

PFN_vkCreateDebugReportCallbackEXT   CreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT  DestroyDebugReportCallbackEXT;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback_func(VkFlags flags,
                    VkDebugReportObjectTypeEXT object_type,
                    u64 object,
                    usize location,
                    i32 message_code,
                    const char* layer,
                    const char* message,
                    void * user_data)
{
    (void)object;
    (void)location;
    (void)user_data;
    // TODO(jesper): multiple flags can be set at the same time, I don't really
    // have a way to express this in my current logging system so I let the log
    // type be decided by a severity precedence
    LogChannel channel;
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        channel = Log_error;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        channel = Log_warning;
    } else if (flags & (VK_DEBUG_REPORT_DEBUG_BIT_EXT |
                        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
    {
        channel = Log_info;
    } else {
        // NOTE: this would only happen if they extend the report callback
        // flags
        channel = Log_info;
    }

    const char *object_str;
    switch (object_type) {
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        object_str = "VkBuffer";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
        object_str = "VkBufferView";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
        object_str = "VkCommandBuffer";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
        object_str = "VkCommandPool";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT:
        object_str = "VkDebugReport";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
        object_str = "VkDescriptorPool";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
        object_str = "VkDescriptorSet";
        // TODO(jesper): silencing all the descriptor sets for now to make a
        // spurious ridiculous warning shut up. need to filter more fine-grained
        // at some point
        return VK_FALSE;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
        object_str = "VkDescriptorSetLayout";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
        object_str = "VkDevice";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
        object_str = "VkDeviceMemory";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
        object_str = "VkEvent";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
        object_str = "VkFence";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
        object_str = "VkFramebuffer";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        object_str = "VkImage";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
        object_str = "VkImageView";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
        object_str = "VkInstance";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
        object_str = "VkPhysicalDevice";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
        object_str = "VkPipelineCache";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
        object_str = "VkPipeline";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
        object_str = "VkPipelineLayout";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
        object_str = "VkQueryPool";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
        object_str = "VkQueue";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
        object_str = "VkRenderPass";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
        object_str = "VkSampler";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
        object_str = "VkSemaphore";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
        object_str = "VkShaderModule";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
        object_str = "VkSurfaceKHR";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
        object_str = "VkSwapchainKHR";
        break;

    default:
    case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
        object_str = "unknown";
        break;
    }

    DEBUG_LOG(channel, "[Vulkan:%s] [%s:%d] - %s",
              layer, object_str, message_code, message);
    DEBUG_ASSERT(channel != Log_error);
    return VK_FALSE;
}

const char *vendor_string(u32 id)
{
    switch (id) {
    case 0x10DE: return "NVIDIA";
    case 0x1002: return "AMD";
    case 0x8086: return "INTEL";
    default:     return "unknown";
    }
}

u32 find_memory_type(VulkanPhysicalDevice physical_device, u32 filter,
                     VkMemoryPropertyFlags req_flags)
{
    for (u32 i = 0; i < physical_device.memory.memoryTypeCount; i++) {
        auto flags = physical_device.memory.memoryTypes[i].propertyFlags;
        if ((filter & (1 << i)) &&
            (flags & req_flags) == req_flags)
        {
            return i;
        }
    }

    return UINT32_MAX;
}

static const VkFormat g_depth_formats[] = {
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT
};

VkFormat find_depth_format(VulkanPhysicalDevice *device)
{
    VkFormatFeatureFlags flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (i32 i = 0; i < ARRAY_SIZE(g_depth_formats); i++) {
        VkFormat format = g_depth_formats[i];

        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->handle, format, &properties);

        if ((properties.optimalTilingFeatures & flags) == flags) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

bool has_stencil(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkCommandBuffer command_buffer_begin()
{
    // TODO(jesper): don't allocate command buffers on demand; allocate a big
    // pool of them in the device init and keep a freelist if unused ones, or
    // ring buffer, or something
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool        = g_vulkan->command_pool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer buffer;
    VkResult result = vkAllocateCommandBuffers(g_vulkan->handle, &allocate_info, &buffer);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(buffer, &begin_info);
    DEBUG_ASSERT(result == VK_SUCCESS);

    return buffer;
}

void present_semaphore(VkSemaphore semaphore)
{
    array_add(&g_vulkan->present_semaphores, semaphore);
}

void present_frame(u32 image)
{
    VkPresentInfoKHR info = {};
    info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = g_vulkan->present_semaphores.count;
    info.pWaitSemaphores    = g_vulkan->present_semaphores.data;
    info.swapchainCount     = 1;
    info.pSwapchains        = &g_vulkan->swapchain.handle;
    info.pImageIndices      = &image;

    VkResult result = vkQueuePresentKHR(g_vulkan->queue, &info);
    DEBUG_ASSERT(result == VK_SUCCESS);

    g_vulkan->present_semaphores.count = 0;
}

void submit_semaphore_wait(VkSemaphore semaphore, VkPipelineStageFlags stage)
{
    array_add(&g_vulkan->semaphores_submit_wait,        semaphore);
    array_add(&g_vulkan->semaphores_submit_wait_stages, stage);
}

void submit_semaphore_signal(VkSemaphore semaphore)
{
    array_add(&g_vulkan->semaphores_submit_signal, semaphore);
}

void submit_frame()
{
    VkSubmitInfo info = {};
    info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    info.commandBufferCount   = g_vulkan->commands_queued.count;
    info.pCommandBuffers      = g_vulkan->commands_queued.data;

    info.waitSemaphoreCount   = g_vulkan->semaphores_submit_wait.count;
    info.pWaitSemaphores      = g_vulkan->semaphores_submit_wait.data;
    info.pWaitDstStageMask    = g_vulkan->semaphores_submit_wait_stages.data;

    info.signalSemaphoreCount = g_vulkan->semaphores_submit_signal.count;
    info.pSignalSemaphores    = g_vulkan->semaphores_submit_signal.data;


    vkQueueSubmit(g_vulkan->queue, 1, &info, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_vulkan->queue);

    // TODO(jesper): move the command buffers into a free list instead of
    // actually freeing them, to be reset and reused with
    // command_buffer_begin
    vkFreeCommandBuffers(g_vulkan->handle, g_vulkan->command_pool,
                         g_vulkan->commands_queued.count,
                         g_vulkan->commands_queued.data);

    g_vulkan->commands_queued.count          = 0;
    g_vulkan->semaphores_submit_wait.count   = 0;
    g_vulkan->semaphores_submit_signal.count = 0;
}

void renderpass_end(VkCommandBuffer cmd)
{
    vkCmdEndRenderPass(cmd);
}

#define VK_LOAD_FUNC(i, f) (PFN_##f)vkGetInstanceProcAddr(i, #f)
void vulkan_load(VkInstance instance)
{
    CreateDebugReportCallbackEXT  = VK_LOAD_FUNC(instance, vkCreateDebugReportCallbackEXT);
    DestroyDebugReportCallbackEXT = VK_LOAD_FUNC(instance, vkDestroyDebugReportCallbackEXT);
}

void renderpass_begin(VkCommandBuffer cmd, u32 image)
{
    VkClearValue clear_values[2];
    clear_values[0].color        = { {1.0f, 0.0f, 0.0f, 0.0f} };
    clear_values[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass        = g_vulkan->renderpass;
    info.framebuffer       = g_vulkan->framebuffers[image];
    info.renderArea.offset = { 0, 0 };
    info.renderArea.extent = g_vulkan->swapchain.extent;
    info.clearValueCount   = 2;
    info.pClearValues      = clear_values;

    vkCmdBeginRenderPass(cmd, &info, VK_SUBPASS_CONTENTS_INLINE);
}

void command_buffer_end(VkCommandBuffer buffer,
                        bool submit = true)
{
    VkResult result = vkEndCommandBuffer(buffer);
    DEBUG_ASSERT(result == VK_SUCCESS);

    array_add(&g_vulkan->commands_queued, buffer);

    if (submit) {
        submit_frame();
    }
}

void image_transition(VkCommandBuffer command,
                      VkImage image, VkFormat format,
                      VkImageLayout src,
                      VkImageLayout dst)
{
    VkImageAspectFlags aspect_mask = 0;
    switch (dst) {
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;

        if (has_stencil(format)) {
            aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        break;
    default:
        aspect_mask |= VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    }

    VkImageMemoryBarrier barrier = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = src;
    barrier.newLayout                       = dst;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = aspect_mask;
    // TODO(jesper): support mip layers
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    switch (src) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        barrier.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    default:
        // TODO(jesper): unimplemented transfer
        DEBUG_ASSERT(false);
        barrier.srcAccessMask = 0;
        break;
    }

    switch (dst) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    default:
        // TODO(jesper): unimplemented transfer
        DEBUG_ASSERT(false);
        barrier.dstAccessMask = 0;
        break;
    }

    vkCmdPipelineBarrier(command,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
}

void image_transition_immediate(VkImage image, VkFormat format,
                                VkImageLayout src,
                                VkImageLayout dst)
{
    VkCommandBuffer command = command_buffer_begin();
    image_transition(command, image, format, src, dst);
    command_buffer_end(command);
}

VkImage image_create(VkFormat format,
                     u32 width,
                     u32 height,
                     VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkDeviceMemory *memory)
{
    VkImage image;

    VkImageCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType         = VK_IMAGE_TYPE_2D;
    info.format            = format;
    info.extent.width      = width;
    info.extent.height     = height;
    info.extent.depth      = 1;
    info.mipLevels         = 1;
    info.arrayLayers       = 1;
    info.samples           = VK_SAMPLE_COUNT_1_BIT;
    info.tiling            = tiling;
    info.initialLayout     = VK_IMAGE_LAYOUT_PREINITIALIZED;
    info.usage             = usage;
    info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(g_vulkan->handle, &info, nullptr, &image);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan->handle, image, &mem_requirements);

    // TODO(jesper): look into host coherent
    u32 memory_type = find_memory_type(g_vulkan->physical_device,
                                       mem_requirements.memoryTypeBits,
                                       properties);
    DEBUG_ASSERT(memory_type != UINT32_MAX);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize       = mem_requirements.size;
    alloc_info.memoryTypeIndex      = memory_type;

    result = vkAllocateMemory(g_vulkan->handle, &alloc_info, nullptr, memory);
    DEBUG_ASSERT(result == VK_SUCCESS);

    vkBindImageMemory(g_vulkan->handle, image, *memory, 0);

    return image;
}

VulkanSwapchain swapchain_create(VulkanPhysicalDevice *physical_device,
                                 VkSurfaceKHR surface,
                                 Settings *settings)

{
    VkResult result;
    VulkanSwapchain swapchain = {};
    swapchain.surface = surface;

    // figure out the color space for the swapchain
    u32 formats_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->handle,
                                                  swapchain.surface,
                                                  &formats_count,
                                                  nullptr);
    DEBUG_ASSERT(result == VK_SUCCESS);

    auto formats = g_frame->alloc_array<VkSurfaceFormatKHR>(formats_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->handle,
                                                  swapchain.surface,
                                                  &formats_count,
                                                  formats);
    DEBUG_ASSERT(result == VK_SUCCESS);

    // NOTE: if impl. reports only 1 surface format and that is undefined
    // it has no preferred format, so we choose BGRA8_UNORM
    if (formats_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        swapchain.format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        swapchain.format = formats[0].format;
    }

    // TODO: does the above note affect the color space at all?
    VkColorSpaceKHR surface_colorspace = formats[0].colorSpace;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->handle,
                                                       swapchain.surface,
                                                       &surface_capabilities);
    DEBUG_ASSERT(result == VK_SUCCESS);

    // figure out the present mode for the swapchain
    u32 present_modes_count;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->handle,
                                                       swapchain.surface,
                                                       &present_modes_count,
                                                       nullptr);
    DEBUG_ASSERT(result == VK_SUCCESS);

    auto present_modes = g_frame->alloc_array<VkPresentModeKHR>(present_modes_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->handle,
                                                       swapchain.surface,
                                                       &present_modes_count,
                                                       present_modes);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < present_modes_count; ++i) {
        const VkPresentModeKHR &mode = present_modes[i];

        if (settings->video.vsync && mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            surface_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }

        if (!settings->video.vsync && mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            surface_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        }
    }

    swapchain.extent = surface_capabilities.currentExtent;
    if (swapchain.extent.width == (u32) (-1)) {
        // TODO(grouse): clean up usage of window dimensions
        DEBUG_ASSERT(settings->video.resolution.width  >= 0);
        DEBUG_ASSERT(settings->video.resolution.height >= 0);

        swapchain.extent.width  = (u32)settings->video.resolution.width;
        swapchain.extent.height = (u32)settings->video.resolution.height;
    }

    // TODO: determine the number of VkImages to use in the swapchain
    u32 desired_swapchain_images = surface_capabilities.minImageCount + 1;

    if (surface_capabilities.maxImageCount > 0) {
        desired_swapchain_images = MIN(desired_swapchain_images,
                                       surface_capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface               = swapchain.surface;
    create_info.minImageCount         = desired_swapchain_images;
    create_info.imageFormat           = swapchain.format;
    create_info.imageColorSpace       = surface_colorspace;
    create_info.imageExtent           = swapchain.extent;
    create_info.imageArrayLayers      = 1;
    create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices   = &g_vulkan->queue_family_index;
    create_info.preTransform          = surface_capabilities.currentTransform;
    create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode           = surface_present_mode;
    create_info.clipped               = VK_TRUE;
    create_info.oldSwapchain          = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(g_vulkan->handle,
                                  &create_info,
                                  nullptr,
                                  &swapchain.handle);

    DEBUG_ASSERT(result == VK_SUCCESS);

    result = vkGetSwapchainImagesKHR(g_vulkan->handle,
                                     swapchain.handle,
                                     &swapchain.images_count,
                                     nullptr);
    DEBUG_ASSERT(result == VK_SUCCESS);

    swapchain.images = g_frame->alloc_array<VkImage>(swapchain.images_count);

    result = vkGetSwapchainImagesKHR(g_vulkan->handle,
                                     swapchain.handle,
                                     &swapchain.images_count,
                                     swapchain.images);
    DEBUG_ASSERT(result == VK_SUCCESS);

    swapchain.imageviews = g_persistent->alloc_array<VkImageView>(swapchain.images_count);

    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel   = 0;
    subresource_range.levelCount     = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount     = 1;

    VkImageViewCreateInfo imageview_create_info = {};
    imageview_create_info.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageview_create_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    imageview_create_info.format           = swapchain.format;
    imageview_create_info.subresourceRange = subresource_range;

    for (i32 i = 0; i < (i32) swapchain.images_count; ++i)
    {
        imageview_create_info.image = swapchain.images[i];

        result = vkCreateImageView(g_vulkan->handle, &imageview_create_info, nullptr,
                                   &swapchain.imageviews[i]);
        DEBUG_ASSERT(result == VK_SUCCESS);
    }

    swapchain.depth.format = find_depth_format(physical_device);
    DEBUG_ASSERT(swapchain.depth.format != VK_FORMAT_UNDEFINED);

    swapchain.depth.image = image_create(swapchain.depth.format,
                                         swapchain.extent.width,
                                         swapchain.extent.height,
                                         VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                         &swapchain.depth.memory);

    VkImageViewCreateInfo view_info = {};
    view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                           = swapchain.depth.image;
    view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                          = swapchain.depth.format,
    view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;

    result = vkCreateImageView(g_vulkan->handle, &view_info, nullptr,
                               &swapchain.depth.imageview);
    DEBUG_ASSERT(result == VK_SUCCESS);

    image_transition_immediate(swapchain.depth.image, swapchain.depth.format,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    return swapchain;
}

VkSampler create_sampler()
{
    VkSampler sampler;

    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter               = VK_FILTER_LINEAR;
    sampler_info.minFilter               = VK_FILTER_LINEAR;
    sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable           = VK_FALSE;
    sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkResult result = vkCreateSampler(g_vulkan->handle,
                                      &sampler_info,
                                      nullptr,
                                      &sampler);
    DEBUG_ASSERT(result == VK_SUCCESS);

    return sampler;
}

VulkanShader create_shader(ShaderID id)
{
    VulkanShader shader = {};

    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    switch (id) {
    case ShaderID_generic_vert: {
        char *path = platform_resolve_path(GamePath_shaders, "generic.vert.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_VERTEX_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    case ShaderID_generic_frag: {
        char *path = platform_resolve_path(GamePath_shaders, "generic.frag.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    case ShaderID_terrain_vert: {
        char *path = platform_resolve_path(GamePath_shaders, "terrain.vert.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_VERTEX_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    case ShaderID_terrain_frag: {
        char *path = platform_resolve_path(GamePath_shaders, "terrain.frag.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    case ShaderID_mesh_vert: {
        char *path = platform_resolve_path(GamePath_shaders, "mesh.vert.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_VERTEX_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    case ShaderID_mesh_frag: {
        char *path = platform_resolve_path(GamePath_shaders, "mesh.frag.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    case ShaderID_basic2d_vert: {
        char *path = platform_resolve_path(GamePath_shaders, "basic2d.vert.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_VERTEX_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    case ShaderID_basic2d_frag: {
        char *path = platform_resolve_path(GamePath_shaders, "basic2d.frag.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    case ShaderID_font_frag: {
        char *path = platform_resolve_path(GamePath_shaders, "font.frag.spv");

        usize size;
        u32 *source = (u32*)platform_file_read(path, &size);
        DEBUG_ASSERT(source != nullptr);

        shader.name   = "main";
        shader.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;

        info.codeSize = size;
        info.pCode    = source;

        VkResult result = vkCreateShaderModule(g_vulkan->handle, &info,
                                               nullptr, &shader.module);
        DEBUG_ASSERT(result == VK_SUCCESS);

        free(source);
        free(path);
    } break;
    default:
        DEBUG_LOG("unknown shader id: %d", id);
        DEBUG_ASSERT(false);
        break;
    }

    return shader;
}

VulkanPipeline pipeline_create_font()
{
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    VkResult result;
    VulkanPipeline pipeline = {};
    pipeline.id = Pipeline_font;

    pipeline.shaders[ShaderStage_vertex]   = create_shader(ShaderID_basic2d_vert);
    pipeline.shaders[ShaderStage_fragment] = create_shader(ShaderID_font_frag);

    // TODO(jesper): i think it probably makes sense to move this into the
    // material, but unsure
    pipeline.sampler_count = 1;
    pipeline.samplers = g_persistent->alloc_array<VkSampler>(pipeline.sampler_count);
    pipeline.samplers[0] = create_sampler();

    auto layouts = array_create<VkDescriptorSetLayout>(g_stack);
    { // material
        auto binds = array_create<VkDescriptorSetLayoutBinding>(g_stack);
        array_add(&binds, {
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        });

        VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {};
        descriptor_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout_info.bindingCount = (i32)binds.count;
        descriptor_layout_info.pBindings    = binds.data;

        result = vkCreateDescriptorSetLayout(g_vulkan->handle,
                                             &descriptor_layout_info,
                                             nullptr,
                                             &pipeline.descriptor_layout_material);
        DEBUG_ASSERT(result == VK_SUCCESS);
        array_add(&layouts, pipeline.descriptor_layout_material);
    }

    VkPushConstantRange push_constants = {};
    push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constants.offset = 0;
    push_constants.size = sizeof(Matrix4);

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount         = (i32)layouts.count;
    layout_info.pSetLayouts            = layouts.data;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constants;

    result = vkCreatePipelineLayout(g_vulkan->handle,
                                    &layout_info,
                                    nullptr,
                                    &pipeline.layout);
    DEBUG_ASSERT(result == VK_SUCCESS);

    auto vbinds = array_create<VkVertexInputBindingDescription>(g_stack);
    array_add(&vbinds, { 0, sizeof(f32) * 4, VK_VERTEX_INPUT_RATE_VERTEX });

    auto vdescs = array_create<VkVertexInputAttributeDescription>(g_stack);
    array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 });
    array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(f32) * 2 });


    VkPipelineVertexInputStateCreateInfo vii = {};
    vii.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount   = (i32)vbinds.count;
    vii.pVertexBindingDescriptions      = vbinds.data;
    vii.vertexAttributeDescriptionCount = (i32)vdescs.count;
    vii.pVertexAttributeDescriptions    = vdescs.data;

    VkPipelineInputAssemblyStateCreateInfo iai = {};
    iai.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iai.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iai.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (f32) g_vulkan->swapchain.extent.width;
    viewport.height   = (f32) g_vulkan->swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_vulkan->swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports    = &viewport;
    viewport_info.scissorCount  = 1;
    viewport_info.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable        = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode             = VK_POLYGON_MODE_FILL;
    raster.cullMode                = VK_CULL_MODE_NONE;
    raster.depthBiasEnable         = VK_FALSE;
    raster.lineWidth               = 1.0;

    VkPipelineColorBlendAttachmentState cba = {};
    cba.blendEnable         = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp        = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cba.alphaBlendOp        = VK_BLEND_OP_ADD;
    cba.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
                                                 VK_COLOR_COMPONENT_G_BIT |
                                                 VK_COLOR_COMPONENT_B_BIT |
                                                 VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbi = {};
    cbi.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbi.logicOpEnable     = VK_FALSE;
    cbi.logicOp           = VK_LOGIC_OP_CLEAR;
    cbi.attachmentCount   = 1;
    cbi.pAttachments      = &cba;
    cbi.blendConstants[0] = 0.0f;
    cbi.blendConstants[1] = 0.0f;
    cbi.blendConstants[2] = 0.0f;
    cbi.blendConstants[3] = 0.0f;

    VkPipelineMultisampleStateCreateInfo msi = {};
    msi.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msi.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    msi.sampleShadingEnable   = VK_FALSE;
    msi.minSampleShading      = 0;
    msi.pSampleMask           = nullptr;
    msi.alphaToCoverageEnable = VK_FALSE;
    msi.alphaToOneEnable      = VK_FALSE;

    // NOTE(jesper): it seems like it'd be worth creating and caching this
    // inside the VulkanShader objects
    auto stages = array_create<VkPipelineShaderStageCreateInfo>(g_stack);
    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_vertex].stage,
        pipeline.shaders[ShaderStage_vertex].module,
        pipeline.shaders[ShaderStage_vertex].name,
        nullptr
    });

    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_fragment].stage,
        pipeline.shaders[ShaderStage_fragment].module,
        pipeline.shaders[ShaderStage_fragment].name,
        nullptr
    });

    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable       = VK_FALSE;
    ds.depthWriteEnable      = VK_FALSE;
    ds.depthBoundsTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pinfo = {};
    pinfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pinfo.stageCount          = (i32)stages.count;
    pinfo.pStages             = stages.data;
    pinfo.pVertexInputState   = &vii;
    pinfo.pInputAssemblyState = &iai;
    pinfo.pViewportState      = &viewport_info;
    pinfo.pRasterizationState = &raster;
    pinfo.pMultisampleState   = &msi;
    pinfo.pColorBlendState    = &cbi;
    pinfo.pDepthStencilState  = &ds;
    pinfo.layout              = pipeline.layout;
    pinfo.renderPass          = g_vulkan->renderpass;
    pinfo.basePipelineHandle  = VK_NULL_HANDLE;
    pinfo.basePipelineIndex   = -1;

    result = vkCreateGraphicsPipelines(g_vulkan->handle,
                                       VK_NULL_HANDLE,
                                       1,
                                       &pinfo,
                                       nullptr,
                                       &pipeline.handle);
    DEBUG_ASSERT(result == VK_SUCCESS);
    return pipeline;
}

VulkanPipeline pipeline_create_basic2d()
{
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    VkResult result;
    VulkanPipeline pipeline = {};
    pipeline.id = Pipeline_basic2d;

    pipeline.shaders[ShaderStage_vertex]   = create_shader(ShaderID_basic2d_vert);
    pipeline.shaders[ShaderStage_fragment] = create_shader(ShaderID_basic2d_frag);

    // TODO(jesper): i think it probably makes sense to move this into the
    // material, but unsure
    pipeline.sampler_count = 1;
    pipeline.samplers = g_persistent->alloc_array<VkSampler>(pipeline.sampler_count);
    pipeline.samplers[0] = create_sampler();

    auto layouts = array_create<VkDescriptorSetLayout>(g_stack);
    { // material
        auto binds = array_create<VkDescriptorSetLayoutBinding>(g_stack);
        array_add(&binds, {
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        });

        VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {};
        descriptor_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout_info.bindingCount = (i32)binds.count;
        descriptor_layout_info.pBindings    = binds.data;

        result = vkCreateDescriptorSetLayout(g_vulkan->handle,
                                             &descriptor_layout_info,
                                             nullptr,
                                             &pipeline.descriptor_layout_material);
        DEBUG_ASSERT(result == VK_SUCCESS);
        array_add(&layouts, pipeline.descriptor_layout_material);
    }

    VkPushConstantRange push_constants = {};
    push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constants.offset = 0;
    push_constants.size = sizeof(Matrix4);

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount         = (i32)layouts.count;
    layout_info.pSetLayouts            = layouts.data;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constants;

    result = vkCreatePipelineLayout(g_vulkan->handle,
                                    &layout_info,
                                    nullptr,
                                    &pipeline.layout);
    DEBUG_ASSERT(result == VK_SUCCESS);

    auto vbinds = array_create<VkVertexInputBindingDescription>(g_stack);
    array_add(&vbinds, { 0, sizeof(f32) * 4, VK_VERTEX_INPUT_RATE_VERTEX });

    auto vdescs = array_create<VkVertexInputAttributeDescription>(g_stack);
    array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 });
    array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(f32) * 2 });


    VkPipelineVertexInputStateCreateInfo vii = {};
    vii.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount   = (i32)vbinds.count;
    vii.pVertexBindingDescriptions      = vbinds.data;
    vii.vertexAttributeDescriptionCount = (i32)vdescs.count;
    vii.pVertexAttributeDescriptions    = vdescs.data;

    VkPipelineInputAssemblyStateCreateInfo iai = {};
    iai.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iai.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iai.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (f32) g_vulkan->swapchain.extent.width;
    viewport.height   = (f32) g_vulkan->swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_vulkan->swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports    = &viewport;
    viewport_info.scissorCount  = 1;
    viewport_info.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable        = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode             = VK_POLYGON_MODE_FILL;
    raster.cullMode                = VK_CULL_MODE_NONE;
    raster.depthBiasEnable         = VK_FALSE;
    raster.lineWidth               = 1.0;

    VkPipelineColorBlendAttachmentState cba = {};
    cba.blendEnable         = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp        = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cba.alphaBlendOp        = VK_BLEND_OP_ADD;
    cba.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
                                                 VK_COLOR_COMPONENT_G_BIT |
                                                 VK_COLOR_COMPONENT_B_BIT |
                                                 VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbi = {};
    cbi.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbi.logicOpEnable     = VK_FALSE;
    cbi.logicOp           = VK_LOGIC_OP_CLEAR;
    cbi.attachmentCount   = 1;
    cbi.pAttachments      = &cba;
    cbi.blendConstants[0] = 0.0f;
    cbi.blendConstants[1] = 0.0f;
    cbi.blendConstants[2] = 0.0f;
    cbi.blendConstants[3] = 0.0f;

    VkPipelineMultisampleStateCreateInfo msi = {};
    msi.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msi.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    msi.sampleShadingEnable   = VK_FALSE;
    msi.minSampleShading      = 0;
    msi.pSampleMask           = nullptr;
    msi.alphaToCoverageEnable = VK_FALSE;
    msi.alphaToOneEnable      = VK_FALSE;

    // NOTE(jesper): it seems like it'd be worth creating and caching this
    // inside the VulkanShader objects
    auto stages = array_create<VkPipelineShaderStageCreateInfo>(g_stack);
    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_vertex].stage,
        pipeline.shaders[ShaderStage_vertex].module,
        pipeline.shaders[ShaderStage_vertex].name,
        nullptr
    });

    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_fragment].stage,
        pipeline.shaders[ShaderStage_fragment].module,
        pipeline.shaders[ShaderStage_fragment].name,
        nullptr
    });

    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable       = VK_FALSE;
    ds.depthWriteEnable      = VK_FALSE;
    ds.depthBoundsTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pinfo = {};
    pinfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pinfo.stageCount          = (i32)stages.count;
    pinfo.pStages             = stages.data;
    pinfo.pVertexInputState   = &vii;
    pinfo.pInputAssemblyState = &iai;
    pinfo.pViewportState      = &viewport_info;
    pinfo.pRasterizationState = &raster;
    pinfo.pMultisampleState   = &msi;
    pinfo.pColorBlendState    = &cbi;
    pinfo.pDepthStencilState  = &ds;
    pinfo.layout              = pipeline.layout;
    pinfo.renderPass          = g_vulkan->renderpass;
    pinfo.basePipelineHandle  = VK_NULL_HANDLE;
    pinfo.basePipelineIndex   = -1;

    result = vkCreateGraphicsPipelines(g_vulkan->handle,
                                       VK_NULL_HANDLE,
                                       1,
                                       &pinfo,
                                       nullptr,
                                       &pipeline.handle);
    DEBUG_ASSERT(result == VK_SUCCESS);
    return pipeline;
}

VulkanPipeline pipeline_create_generic()
{
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    VkResult result;
    VulkanPipeline pipeline = {};
    pipeline.id = Pipeline_generic;

    pipeline.shaders[ShaderStage_vertex]   = create_shader(ShaderID_generic_vert);
    pipeline.shaders[ShaderStage_fragment] = create_shader(ShaderID_generic_frag);

    pipeline.sampler_count = 1;
    pipeline.samplers = g_persistent->alloc_array<VkSampler>(pipeline.sampler_count);
    pipeline.samplers[0] = create_sampler();

    auto layouts = array_create<VkDescriptorSetLayout>(g_frame);
    { // pipeline
        auto binds = array_create<VkDescriptorSetLayoutBinding>(g_stack);
        array_add(&binds, {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            nullptr
        });

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = (i32)binds.count;
        layout_info.pBindings    = binds.data;

        result = vkCreateDescriptorSetLayout(g_vulkan->handle,
                                             &layout_info,
                                             nullptr,
                                             &pipeline.descriptor_layout_pipeline);
        DEBUG_ASSERT(result == VK_SUCCESS);

        array_add(&layouts, pipeline.descriptor_layout_pipeline);
    }

    { // material
        auto binds = array_create<VkDescriptorSetLayoutBinding>(g_stack);
        array_add(&binds, {
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        });

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = (i32)binds.count;
        layout_info.pBindings    = binds.data;

        result = vkCreateDescriptorSetLayout(g_vulkan->handle,
                                             &layout_info,
                                             nullptr,
                                             &pipeline.descriptor_layout_pipeline);
        DEBUG_ASSERT(result == VK_SUCCESS);

        array_add(&layouts, pipeline.descriptor_layout_material);
    }

    // NOTE(jesper): create a pool size descriptor for each type of
    // descriptor this shader program uses
    auto psizes = array_create<VkDescriptorPoolSize>(g_stack);
    array_add(&psizes, { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = (i32)psizes.count;
    pool_info.pPoolSizes    = psizes.data;
    pool_info.maxSets       = 1;

    result = vkCreateDescriptorPool(g_vulkan->handle,
                                    &pool_info,
                                    nullptr,
                                    &pipeline.descriptor_pool);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkDescriptorSetAllocateInfo dai = {};
    dai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dai.descriptorPool     = pipeline.descriptor_pool;
    dai.descriptorSetCount = 1;
    dai.pSetLayouts        = &pipeline.descriptor_layout_pipeline;

    result = vkAllocateDescriptorSets(g_vulkan->handle,
                                      &dai,
                                      &pipeline.descriptor_set);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkPushConstantRange push_constants = {};
    push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constants.offset = 0;
    push_constants.size = sizeof(Matrix4);

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount         = (i32)layouts.count;
    layout_info.pSetLayouts            = layouts.data;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constants;

    result = vkCreatePipelineLayout(g_vulkan->handle,
                                    &layout_info,
                                    nullptr,
                                    &pipeline.layout);
    DEBUG_ASSERT(result == VK_SUCCESS);

    auto vbinds = array_create<VkVertexInputBindingDescription>(g_stack);
    array_add(&vbinds, { 0, sizeof(f32) * 9, VK_VERTEX_INPUT_RATE_VERTEX });

    auto vdescs = array_create<VkVertexInputAttributeDescription>(g_stack);
    array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
    array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(f32) * 3 });
    array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32_SFLOAT,    sizeof(f32) * 7 });

    VkPipelineVertexInputStateCreateInfo vii = {};
    vii.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount   = (i32)vbinds.count;
    vii.pVertexBindingDescriptions      = vbinds.data;
    vii.vertexAttributeDescriptionCount = (i32)vdescs.count;
    vii.pVertexAttributeDescriptions    = vdescs.data;

    VkPipelineInputAssemblyStateCreateInfo iai = {};
    iai.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iai.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iai.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (f32) g_vulkan->swapchain.extent.width;
    viewport.height   = (f32) g_vulkan->swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_vulkan->swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports    = &viewport;
    viewport_info.scissorCount  = 1;
    viewport_info.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable        = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode             = VK_POLYGON_MODE_FILL;
    raster.cullMode                = VK_CULL_MODE_NONE;
    raster.depthBiasEnable         = VK_FALSE;
    raster.lineWidth               = 1.0;

    VkPipelineColorBlendAttachmentState cba = {};
    cba.blendEnable    = VK_FALSE;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbi = {};
    cbi.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbi.logicOpEnable     = VK_FALSE;
    cbi.logicOp           = VK_LOGIC_OP_CLEAR;
    cbi.attachmentCount   = 1;
    cbi.pAttachments      = &cba;
    cbi.blendConstants[0] = 0.0f;
    cbi.blendConstants[1] = 0.0f;
    cbi.blendConstants[2] = 0.0f;
    cbi.blendConstants[3] = 0.0f;

    VkPipelineMultisampleStateCreateInfo msi = {};
    msi.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msi.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    msi.sampleShadingEnable   = VK_FALSE;
    msi.minSampleShading      = 0;
    msi.pSampleMask           = nullptr;
    msi.alphaToCoverageEnable = VK_FALSE;
    msi.alphaToOneEnable      = VK_FALSE;

    // NOTE(jesper): it seems like it'd be worth creating and caching this
    // inside the VulkanShader objects
    auto stages = array_create<VkPipelineShaderStageCreateInfo>(g_stack);
    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_vertex].stage,
        pipeline.shaders[ShaderStage_vertex].module,
        pipeline.shaders[ShaderStage_vertex].name,
        nullptr
    });

    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_fragment].stage,
        pipeline.shaders[ShaderStage_fragment].module,
        pipeline.shaders[ShaderStage_fragment].name,
        nullptr
    });

    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable       = VK_FALSE;
    ds.depthWriteEnable      = VK_FALSE;
    ds.depthBoundsTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pinfo = {};
    pinfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pinfo.stageCount          = (i32)stages.count;
    pinfo.pStages             = stages.data;
    pinfo.pVertexInputState   = &vii;
    pinfo.pInputAssemblyState = &iai;
    pinfo.pViewportState      = &viewport_info;
    pinfo.pRasterizationState = &raster;
    pinfo.pMultisampleState   = &msi;
    pinfo.pColorBlendState    = &cbi;
    pinfo.pDepthStencilState  = &ds;
    pinfo.layout              = pipeline.layout;
    pinfo.renderPass          = g_vulkan->renderpass;
    pinfo.basePipelineHandle  = VK_NULL_HANDLE;
    pinfo.basePipelineIndex   = -1;

    result = vkCreateGraphicsPipelines(g_vulkan->handle,
                                       VK_NULL_HANDLE,
                                       1,
                                       &pinfo,
                                       nullptr,
                                       &pipeline.handle);
    DEBUG_ASSERT(result == VK_SUCCESS);
    return pipeline;
}

VulkanPipeline pipeline_create_terrain()
{
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    VkResult result;
    VulkanPipeline pipeline = {};
    pipeline.id = Pipeline_terrain;

    pipeline.shaders[ShaderStage_vertex]   = create_shader(ShaderID_terrain_vert);
    pipeline.shaders[ShaderStage_fragment] = create_shader(ShaderID_terrain_frag);

    auto layouts = array_create<VkDescriptorSetLayout>(g_frame);
    { // pipeline
        auto binds = array_create<VkDescriptorSetLayoutBinding>(g_stack);
        array_add(&binds, {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            nullptr
        });

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = (u32)binds.count;
        layout_info.pBindings    = binds.data;

        result = vkCreateDescriptorSetLayout(g_vulkan->handle,
                                             &layout_info,
                                             nullptr,
                                             &pipeline.descriptor_layout_pipeline);
        DEBUG_ASSERT(result == VK_SUCCESS);

        array_add(&layouts, pipeline.descriptor_layout_pipeline);
    }

    // NOTE(jesper): create a pool size descriptor for each type of
    // descriptor this shader program uses
    auto psizes = array_create<VkDescriptorPoolSize>(g_stack);
    array_add(&psizes, { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = (u32)psizes.count;
    pool_info.pPoolSizes    = psizes.data;
    pool_info.maxSets       = 1;

    result = vkCreateDescriptorPool(g_vulkan->handle,
                                    &pool_info,
                                    nullptr,
                                    &pipeline.descriptor_pool);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkDescriptorSetAllocateInfo dai = {};
    dai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dai.descriptorPool     = pipeline.descriptor_pool;
    dai.descriptorSetCount = 1;
    dai.pSetLayouts        = &pipeline.descriptor_layout_pipeline;

    result = vkAllocateDescriptorSets(g_vulkan->handle,
                                      &dai,
                                      &pipeline.descriptor_set);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkPushConstantRange push_constants = {};
    push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constants.offset = 0;
    push_constants.size = sizeof(Matrix4);

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount         = (u32)layouts.count;
    layout_info.pSetLayouts            = layouts.data;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constants;

    result = vkCreatePipelineLayout(g_vulkan->handle,
                                    &layout_info,
                                    nullptr,
                                    &pipeline.layout);
    DEBUG_ASSERT(result == VK_SUCCESS);

    auto vbinds = array_create<VkVertexInputBindingDescription>(g_stack);
    array_add(&vbinds, { 0, sizeof(f32) * 6, VK_VERTEX_INPUT_RATE_VERTEX });

    auto vdescs = array_create<VkVertexInputAttributeDescription>(g_stack);
    array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
    array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(f32) * 3 });

    VkPipelineVertexInputStateCreateInfo vii = {};
    vii.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount   = (u32)vbinds.count;
    vii.pVertexBindingDescriptions      = vbinds.data;
    vii.vertexAttributeDescriptionCount = (u32)vdescs.count;
    vii.pVertexAttributeDescriptions    = vdescs.data;

    VkPipelineInputAssemblyStateCreateInfo iai = {};
    iai.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iai.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iai.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (f32) g_vulkan->swapchain.extent.width;
    viewport.height   = (f32) g_vulkan->swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_vulkan->swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports    = &viewport;
    viewport_info.scissorCount  = 1;
    viewport_info.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable        = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode             = VK_POLYGON_MODE_FILL;
    raster.cullMode                = VK_CULL_MODE_NONE;
    raster.depthBiasEnable         = VK_FALSE;
    raster.lineWidth               = 1.0;

    VkPipelineColorBlendAttachmentState cba = {};
    cba.blendEnable    = VK_FALSE;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbi = {};
    cbi.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbi.logicOpEnable     = VK_FALSE;
    cbi.logicOp           = VK_LOGIC_OP_CLEAR;
    cbi.attachmentCount   = 1;
    cbi.pAttachments      = &cba;
    cbi.blendConstants[0] = 0.0f;
    cbi.blendConstants[1] = 0.0f;
    cbi.blendConstants[2] = 0.0f;
    cbi.blendConstants[3] = 0.0f;

    VkPipelineMultisampleStateCreateInfo msi = {};
    msi.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msi.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    msi.sampleShadingEnable   = VK_FALSE;
    msi.minSampleShading      = 0;
    msi.pSampleMask           = nullptr;
    msi.alphaToCoverageEnable = VK_FALSE;
    msi.alphaToOneEnable      = VK_FALSE;

    // NOTE(jesper): it seems like it'd be worth creating and caching this
    // inside the VulkanShader objects
    auto stages = array_create<VkPipelineShaderStageCreateInfo>(g_stack);
    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_vertex].stage,
        pipeline.shaders[ShaderStage_vertex].module,
        pipeline.shaders[ShaderStage_vertex].name,
        nullptr
    });

    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_fragment].stage,
        pipeline.shaders[ShaderStage_fragment].module,
        pipeline.shaders[ShaderStage_fragment].name,
        nullptr
    });

    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable       = VK_TRUE;
    ds.depthWriteEnable      = VK_TRUE;
    ds.depthCompareOp        = VK_COMPARE_OP_LESS;
    ds.depthBoundsTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pinfo = {};
    pinfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pinfo.stageCount          = (u32)stages.count;
    pinfo.pStages             = stages.data;
    pinfo.pVertexInputState   = &vii;
    pinfo.pInputAssemblyState = &iai;
    pinfo.pViewportState      = &viewport_info;
    pinfo.pRasterizationState = &raster;
    pinfo.pMultisampleState   = &msi;
    pinfo.pColorBlendState    = &cbi;
    pinfo.pDepthStencilState  = &ds;
    pinfo.layout              = pipeline.layout;
    pinfo.renderPass          = g_vulkan->renderpass;
    pinfo.basePipelineHandle  = VK_NULL_HANDLE;
    pinfo.basePipelineIndex   = -1;

    result = vkCreateGraphicsPipelines(g_vulkan->handle,
                                       VK_NULL_HANDLE,
                                       1,
                                       &pinfo,
                                       nullptr,
                                       &pipeline.handle);
    DEBUG_ASSERT(result == VK_SUCCESS);
    return pipeline;
}

VulkanPipeline pipeline_create_mesh()
{
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    VkResult result;
    VulkanPipeline pipeline = {};
    pipeline.id = Pipeline_mesh;

    pipeline.shaders[ShaderStage_vertex]   = create_shader(ShaderID_mesh_vert);
    pipeline.shaders[ShaderStage_fragment] = create_shader(ShaderID_mesh_frag);

    // TODO(jesper): i think it probably makes sense to move this into the
    // material, but unsure
    pipeline.sampler_count = 1;
    pipeline.samplers = g_persistent->alloc_array<VkSampler>(pipeline.sampler_count);
    pipeline.samplers[0] = create_sampler();


    auto layouts = array_create<VkDescriptorSetLayout>(g_frame);
    { // pipeline
        auto binds = array_create<VkDescriptorSetLayoutBinding>(g_stack);
        array_add(&binds, {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            nullptr
        });

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = (u32)binds.count;
        layout_info.pBindings    = binds.data;

        result = vkCreateDescriptorSetLayout(g_vulkan->handle,
                                             &layout_info,
                                             nullptr,
                                             &pipeline.descriptor_layout_pipeline);
        DEBUG_ASSERT(result == VK_SUCCESS);

        array_add(&layouts, pipeline.descriptor_layout_pipeline);
    }

    { // material
        auto binds = array_create<VkDescriptorSetLayoutBinding>(g_stack);
        array_add(&binds, {
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        });

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = (u32)binds.count;
        layout_info.pBindings    = binds.data;

        result = vkCreateDescriptorSetLayout(g_vulkan->handle,
                                             &layout_info,
                                             nullptr,
                                             &pipeline.descriptor_layout_material);
        DEBUG_ASSERT(result == VK_SUCCESS);

        array_add(&layouts, pipeline.descriptor_layout_material);
    }

    // NOTE(jesper): create a pool size descriptor for each type of
    // descriptor this shader program uses
    auto pool_sizes = array_create<VkDescriptorPoolSize>(g_frame);
    array_add(&pool_sizes, { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1 });
    //array_add(&pool_sizes, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 });

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = (u32)pool_sizes.count;
    pool_info.pPoolSizes    = pool_sizes.data;
    pool_info.maxSets       = 1;

    result = vkCreateDescriptorPool(g_vulkan->handle,
                                    &pool_info,
                                    nullptr,
                                    &pipeline.descriptor_pool);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkDescriptorSetAllocateInfo dai = {};
    dai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dai.descriptorPool     = pipeline.descriptor_pool;
    dai.descriptorSetCount = 1;
    dai.pSetLayouts        = &layouts[0];

    result = vkAllocateDescriptorSets(g_vulkan->handle,
                                      &dai,
                                      &pipeline.descriptor_set);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkPushConstantRange push_constants = {};
    push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constants.offset = 0;
    push_constants.size = sizeof(Matrix4);

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount         = (u32)layouts.count;
    layout_info.pSetLayouts            = layouts.data;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constants;

    result = vkCreatePipelineLayout(g_vulkan->handle,
                                    &layout_info,
                                    nullptr,
                                    &pipeline.layout);
    DEBUG_ASSERT(result == VK_SUCCESS);

    auto vbinds = array_create<VkVertexInputBindingDescription>(g_stack);
    array_add(&vbinds, { 0, sizeof(f32) * 8, VK_VERTEX_INPUT_RATE_VERTEX });

    auto vdescs = array_create<VkVertexInputAttributeDescription>(g_stack);
    array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
    array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(f32) * 3 });
    array_add(&vdescs, { 2, 0, VK_FORMAT_R32G32_SFLOAT,    sizeof(f32) * 6 });

    VkPipelineVertexInputStateCreateInfo vii = {};
    vii.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount   = (u32)vbinds.count;
    vii.pVertexBindingDescriptions      = vbinds.data;
    vii.vertexAttributeDescriptionCount = (u32)vdescs.count;
    vii.pVertexAttributeDescriptions    = vdescs.data;

    VkPipelineInputAssemblyStateCreateInfo iai = {};
    iai.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iai.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iai.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (f32) g_vulkan->swapchain.extent.width;
    viewport.height   = (f32) g_vulkan->swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_vulkan->swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports    = &viewport;
    viewport_info.scissorCount  = 1;
    viewport_info.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable        = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode             = VK_POLYGON_MODE_FILL;
    raster.cullMode                = VK_CULL_MODE_NONE;
    raster.depthBiasEnable         = VK_FALSE;
    raster.lineWidth               = 1.0;

    VkPipelineColorBlendAttachmentState cba = {};
    cba.blendEnable    = VK_FALSE;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbi = {};
    cbi.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbi.logicOpEnable     = VK_FALSE;
    cbi.logicOp           = VK_LOGIC_OP_CLEAR;
    cbi.attachmentCount   = 1;
    cbi.pAttachments      = &cba;
    cbi.blendConstants[0] = 0.0f;
    cbi.blendConstants[1] = 0.0f;
    cbi.blendConstants[2] = 0.0f;
    cbi.blendConstants[3] = 0.0f;

    VkPipelineMultisampleStateCreateInfo msi = {};
    msi.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msi.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    msi.sampleShadingEnable   = VK_FALSE;
    msi.minSampleShading      = 0;
    msi.pSampleMask           = nullptr;
    msi.alphaToCoverageEnable = VK_FALSE;
    msi.alphaToOneEnable      = VK_FALSE;

    // NOTE(jesper): it seems like it'd be worth creating and caching this
    // inside the VulkanShader objects
    auto stages = array_create<VkPipelineShaderStageCreateInfo>(g_stack);
    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_vertex].stage,
        pipeline.shaders[ShaderStage_vertex].module,
        pipeline.shaders[ShaderStage_vertex].name,
        nullptr
    });

    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_fragment].stage,
        pipeline.shaders[ShaderStage_fragment].module,
        pipeline.shaders[ShaderStage_fragment].name,
        nullptr
    });

    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable       = VK_TRUE;
    ds.depthWriteEnable      = VK_TRUE;
    ds.depthCompareOp        = VK_COMPARE_OP_LESS;
    ds.depthBoundsTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pinfo = {};
    pinfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pinfo.stageCount          = (u32)stages.count;
    pinfo.pStages             = stages.data;
    pinfo.pVertexInputState   = &vii;
    pinfo.pInputAssemblyState = &iai;
    pinfo.pViewportState      = &viewport_info;
    pinfo.pRasterizationState = &raster;
    pinfo.pMultisampleState   = &msi;
    pinfo.pColorBlendState    = &cbi;
    pinfo.pDepthStencilState  = &ds;
    pinfo.layout              = pipeline.layout;
    pinfo.renderPass          = g_vulkan->renderpass;
    pinfo.basePipelineHandle  = VK_NULL_HANDLE;
    pinfo.basePipelineIndex   = -1;

    result = vkCreateGraphicsPipelines(g_vulkan->handle,
                                       VK_NULL_HANDLE,
                                       1,
                                       &pinfo,
                                       nullptr,
                                       &pipeline.handle);
    DEBUG_ASSERT(result == VK_SUCCESS);
    return pipeline;
}

void image_copy(u32 width, u32 height,
                VkImage src, VkImage dst)
{
    VkCommandBuffer command = command_buffer_begin();

    // TODO(jesper): support mip layers
    VkImageSubresourceLayers subresource = {};
    subresource.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.baseArrayLayer           = 0;
    subresource.mipLevel                 = 0;
    subresource.layerCount               = 1;

    VkImageCopy region = {};
    // TODO(jesper): support copying images from/to different subresources?
    region.srcSubresource = subresource;
    region.dstSubresource = subresource;
    // TODO(jesper): support subregion copy
    region.srcOffset = { 0, 0, 0 };
    region.dstOffset = { 0, 0, 0 };
    region.extent.width = width;
    region.extent.height = height;
    region.extent.depth = 1;

    vkCmdCopyImage(command,
                   src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &region);

    command_buffer_end(command);
}



VulkanTexture texture_create(u32 width, u32 height,
                             VkFormat format, void *pixels,
                             VkComponentMapping components)
{
    VkResult result;

    VulkanTexture texture = {};
    texture.format = format;
    texture.width  = width;
    texture.height = height;

    VkDeviceMemory staging_memory;
    VkImage staging_image = image_create(format, width, height,
                                         VK_IMAGE_TILING_LINEAR,
                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         &staging_memory);


    VkImageSubresource subresource = {};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel   = 0;
    subresource.arrayLayer = 0;

    VkSubresourceLayout staging_image_layout;
    vkGetImageSubresourceLayout(g_vulkan->handle, staging_image,
                                &subresource, &staging_image_layout);

    void *data;
    // TODO(jesper): hardcoded 1 byte per channel and 4 channels, lookup based
    // on the receieved VkFormat

    i32 num_channels;
    i32 bytes_per_channel;
    switch (format) {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_UINT:
        num_channels      = 1;
        bytes_per_channel = 1;
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
        num_channels      = 4;
        bytes_per_channel = 1;
        break;
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16_SINT:
        num_channels      = 1;
        bytes_per_channel = 2;
        break;
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
        num_channels      = 1;
        bytes_per_channel = 4;
        break;
    case VK_FORMAT_R32G32_SFLOAT:
        num_channels      = 2;
        bytes_per_channel = 4;
        break;
    case VK_FORMAT_R16G16_SFLOAT:
        num_channels      = 2;
        bytes_per_channel = 2;
        break;
    default:
        DEBUG_ASSERT(false);
        num_channels      = 1;
        bytes_per_channel = 2;
        break;
    }

    VkDeviceSize size = width * height * num_channels * bytes_per_channel;
    vkMapMemory(g_vulkan->handle, staging_memory, 0, size, 0, &data);

    u32 row_pitch = width * num_channels * bytes_per_channel;
    if (staging_image_layout.rowPitch == row_pitch) {
        memcpy(data, pixels, (usize)size);
    } else {
        u8 *bytes = (u8*)data;
        u8 *pixel_bytes = (u8*)pixels;
        for (i32 y = 0; y < (i32)height; y++) {
            memcpy(&bytes[y * num_channels * staging_image_layout.rowPitch],
                   &pixel_bytes[y * width * num_channels * bytes_per_channel],
                   width * num_channels * bytes_per_channel);
        }
    }

    vkUnmapMemory(g_vulkan->handle, staging_memory);

    texture.image = image_create(format, width, height,
                                 VK_IMAGE_TILING_OPTIMAL,
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 &texture.memory);

    image_transition_immediate(staging_image, format,
                               VK_IMAGE_LAYOUT_PREINITIALIZED,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    image_transition_immediate(texture.image, format,
                               VK_IMAGE_LAYOUT_PREINITIALIZED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    image_copy(width, height, staging_image, texture.image);
    image_transition_immediate(texture.image, format,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageViewCreateInfo view_info = {};
    view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                           = texture.image;
    view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                          = texture.format;
    view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;
    view_info.components = components;

    result = vkCreateImageView(g_vulkan->handle, &view_info, nullptr,
                               &texture.image_view);
    DEBUG_ASSERT(result == VK_SUCCESS);

    vkFreeMemory(g_vulkan->handle, staging_memory, nullptr);
    vkDestroyImage(g_vulkan->handle, staging_image, nullptr);

    return texture;
}

void vkdebug_create()
{
    VkDebugReportCallbackCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

    create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_DEBUG_BIT_EXT;

    create_info.pfnCallback = &debug_callback_func;

    VkResult result = CreateDebugReportCallbackEXT(g_vulkan->instance,
                                                   &create_info,
                                                   nullptr,
                                                   &g_vulkan->debug_callback);
    DEBUG_ASSERT(result == VK_SUCCESS);
}

void vkdebug_destroy()
{
    DestroyDebugReportCallbackEXT(g_vulkan->instance, g_vulkan->debug_callback, nullptr);
    g_vulkan->debug_callback = nullptr;
}

void device_create(PlatformState *platform, Settings *settings)
{
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    g_vulkan = g_persistent->ialloc<VulkanDevice>();

    VkResult result;
    /**************************************************************************
     * Create VkInstance
     *************************************************************************/
    {
        // NOTE(jesper): currently we don't assert about missing required
        // extensions or layers, and we don't store any internal state about
        // which ones we've enabled.
        u32 supported_layers_count = 0;
        result = vkEnumerateInstanceLayerProperties(&supported_layers_count,
                                                    nullptr);
        DEBUG_ASSERT(result == VK_SUCCESS);

        auto supported_layers = g_frame->alloc_array<VkLayerProperties>(supported_layers_count);

        result = vkEnumerateInstanceLayerProperties(&supported_layers_count,
                                                    supported_layers);
        DEBUG_ASSERT(result == VK_SUCCESS);

        for (u32 i = 0; i < supported_layers_count; i++) {
            DEBUG_LOG("VkLayerProperties[%u]", i);
            DEBUG_LOG("  layerName            : %s",
                      supported_layers[i].layerName);
            DEBUG_LOG("  specVersion          : %u.%u.%u",
                      VK_VERSION_MAJOR(supported_layers[i].specVersion),
                      VK_VERSION_MINOR(supported_layers[i].specVersion),
                      VK_VERSION_PATCH(supported_layers[i].specVersion));
            DEBUG_LOG("  implementationVersion: %u",
                      supported_layers[i].implementationVersion);
            DEBUG_LOG("  description          : %s",
                      supported_layers[i].description);
        }

        u32 supported_extensions_count = 0;
        result = vkEnumerateInstanceExtensionProperties(nullptr,
                                                        &supported_extensions_count,
                                                        nullptr);
        DEBUG_ASSERT(result == VK_SUCCESS);

        auto supported_extensions = g_frame->alloc_array<VkExtensionProperties>(supported_extensions_count);
        result = vkEnumerateInstanceExtensionProperties(nullptr,
                                                        &supported_extensions_count,
                                                        supported_extensions);
        DEBUG_ASSERT(result == VK_SUCCESS);

        for (u32 i = 0; i < supported_extensions_count; i++) {
            DEBUG_LOG("vkExtensionProperties[%u]", i);
            DEBUG_LOG("  extensionName: %s", supported_extensions[i].extensionName);
            DEBUG_LOG("  specVersion  : %u", supported_extensions[i].specVersion);
        }


        // NOTE(jesper): we might want to store these in the device for future
        // usage/debug information
        i32 enabled_layers_count = 0;
        auto enabled_layers = g_frame->alloc_array<char*>(supported_layers_count);

        i32 enabled_extensions_count = 0;
        auto enabled_extensions = g_frame->alloc_array<char*>(supported_extensions_count);

        for (i32 i = 0; i < (i32)supported_layers_count; ++i) {
            VkLayerProperties &layer = supported_layers[i];

            if (platform_vulkan_enable_instance_layer(layer) ||
                strcmp(layer.layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
            {
                enabled_layers[enabled_layers_count++] = layer.layerName;
            }
        }

        for (i32 i = 0; i < (i32)supported_extensions_count; ++i) {
            VkExtensionProperties &extension = supported_extensions[i];

            if (platform_vulkan_enable_instance_extension(extension) ||
                strcmp(extension.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0 ||
                strcmp(extension.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
            {
                enabled_extensions[enabled_extensions_count++] = extension.extensionName;
            }
        }

        // Create the VkInstance
        VkApplicationInfo app_info = {};
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName   = "leary";
        app_info.applicationVersion = 1;
        app_info.pEngineName        = "leary";
        app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 37);

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledLayerCount = (u32) enabled_layers_count;
        create_info.ppEnabledLayerNames = enabled_layers;
        create_info.enabledExtensionCount = (u32) enabled_extensions_count;
        create_info.ppEnabledExtensionNames = enabled_extensions;

        result = vkCreateInstance(&create_info, nullptr, &g_vulkan->instance);
        DEBUG_ASSERT(result == VK_SUCCESS);
    }

    /**************************************************************************
     * Create debug callbacks
     *************************************************************************/
    {
        vulkan_load(g_vulkan->instance);
        vkdebug_create();
    }

    /**************************************************************************
     * Create and choose VkPhysicalDevice
     *************************************************************************/
    {
        u32 count = 0;
        result = vkEnumeratePhysicalDevices(g_vulkan->instance, &count, nullptr);
        DEBUG_ASSERT(result == VK_SUCCESS);

        auto physical_devices = g_frame->alloc_array<VkPhysicalDevice>(count);
        result = vkEnumeratePhysicalDevices(g_vulkan->instance,
                                            &count, physical_devices);
        DEBUG_ASSERT(result == VK_SUCCESS);

        bool found_device = false;
        for (u32 i = 0; i < count; i++) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

            DEBUG_LOG("VkPhysicalDeviceProperties[%u]", i);
            DEBUG_LOG("  apiVersion    : %d.%d.%d",
                      VK_VERSION_MAJOR(properties.apiVersion),
                      VK_VERSION_MINOR(properties.apiVersion),
                      VK_VERSION_PATCH(properties.apiVersion));
            // NOTE(jesper): only confirmed to be accurate, by experimentation,
            // on nvidia
            DEBUG_LOG("  driverVersion : %u.%u",
                      (properties.driverVersion >> 22),
                      (properties.driverVersion >> 14) & 0xFF);
            DEBUG_LOG("  vendorID      : 0x%X %s",
                      properties.vendorID,
                      vendor_string(properties.vendorID));
            DEBUG_LOG("  deviceID      : 0x%X", properties.deviceID);
            switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                DEBUG_LOG("  deviceType: Integrated GPU");
                if (!found_device) {
                    g_vulkan->physical_device.handle     = physical_devices[i];
                    g_vulkan->physical_device.properties = properties;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                DEBUG_LOG("  deviceType    : Discrete GPU");
                g_vulkan->physical_device.handle     = physical_devices[i];
                g_vulkan->physical_device.properties = properties;
                found_device = true;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                DEBUG_LOG("  deviceType    : Virtual GPU");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                DEBUG_LOG("  deviceType    : CPU");
                break;
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                DEBUG_LOG("  deviceTyswapchain_acquirepe    : Unknown");
                break;
            }
            DEBUG_LOG("  deviceName    : %s", properties.deviceName);
        }

        vkGetPhysicalDeviceMemoryProperties(g_vulkan->physical_device.handle,
                                            &g_vulkan->physical_device.memory);

        vkGetPhysicalDeviceFeatures(g_vulkan->physical_device.handle,
                                    &g_vulkan->physical_device.features);
    }

    // NOTE(jesper): this is so annoying. The surface belongs with the swapchain
    // (imo), but to create the swapchain we need the device, and to create the
    // device we need the surface
    VkSurfaceKHR surface;
    result = platform_vulkan_create_surface(g_vulkan->instance, &surface, platform);
    DEBUG_ASSERT(result == VK_SUCCESS);


    /**************************************************************************
     * Create VkDevice and get its queue
     *************************************************************************/
    {
        u32 queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(g_vulkan->physical_device.handle,
                                                 &queue_family_count,
                                                 nullptr);

        auto queue_families = g_frame->alloc_array<VkQueueFamilyProperties>(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(g_vulkan->physical_device.handle,
                                                 &queue_family_count,
                                                 queue_families);

        g_vulkan->queue_family_index = 0;
        for (u32 i = 0; i < queue_family_count; ++i) {
            const VkQueueFamilyProperties &property = queue_families[i];

            // figure out if the queue family supports present
            VkBool32 supports_present = VK_FALSE;
            result = vkGetPhysicalDeviceSurfaceSupportKHR(g_vulkan->physical_device.handle,
                                                          g_vulkan->queue_family_index,
                                                          surface,
                                                          &supports_present);
            DEBUG_ASSERT(result == VK_SUCCESS);


            DEBUG_LOG("VkQueueFamilyProperties[%u]", i);
            DEBUG_LOG("  queueCount                 : %u",
                      property.queueCount);
            DEBUG_LOG("  timestampValidBits         : %u",
                      property.timestampValidBits);
            DEBUG_LOG("  minImageTransferGranualrity: (%u, %u, %u)",
                      property.minImageTransferGranularity.depth,
                      property.minImageTransferGranularity.height,
                      property.minImageTransferGranularity.depth);
            DEBUG_LOG("  supportsPresent            : %d",
                      (i32)supports_present);

            // if it doesn't we keep on searching
            if (supports_present == VK_FALSE)
                continue;

            // we're just interested in getting a graphics queue going for
            // now, so choose the first one
            // TODO: COMPUTE: find a compute queue, potentially asynchronous
            // (separate from graphics queue)
            // TODO: get a separate queue for transfer if one exist to do
            // buffer copy commands on while graphics/compute queue is doing
            // its own thing
            if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                g_vulkan->queue_family_index = i;
                break;
            }
        }

        // TODO: when we have more than one queue we'll need to figure out how
        // to handle this, for now just set highest queue priroity for the 1
        // queue we create
        f32 priority = 1.0f;

        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = g_vulkan->queue_family_index;
        queue_info.queueCount       = 1;
        queue_info.pQueuePriorities = &priority;

        // TODO: look into other extensions
        const char *device_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.queueCreateInfoCount    = 1;
        device_info.pQueueCreateInfos       = &queue_info;
        device_info.enabledExtensionCount   = 1;
        device_info.ppEnabledExtensionNames = device_extensions;
        device_info.pEnabledFeatures        = &g_vulkan->physical_device.features;

        result = vkCreateDevice(g_vulkan->physical_device.handle,
                                &device_info,
                                nullptr,
                                &g_vulkan->handle);
        DEBUG_ASSERT(result == VK_SUCCESS);

        // NOTE: does it matter which queue we choose?
        u32 queue_index = 0;
        vkGetDeviceQueue(g_vulkan->handle,
                         g_vulkan->queue_family_index,
                         queue_index,
                         &g_vulkan->queue);
    }

    /**************************************************************************
     * Create VkCommandPool
     *************************************************************************/
    {
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        create_info.queueFamilyIndex = g_vulkan->queue_family_index;

        result = vkCreateCommandPool(g_vulkan->handle,
                                     &create_info,
                                     nullptr,
                                     &g_vulkan->command_pool);
        DEBUG_ASSERT(result == VK_SUCCESS);

        g_vulkan->commands_queued               = array_create<VkCommandBuffer>(g_heap);

        g_vulkan->semaphores_submit_wait        = array_create<VkSemaphore>(g_heap);
        g_vulkan->semaphores_submit_wait_stages = array_create<VkPipelineStageFlags>(g_heap);

        g_vulkan->semaphores_submit_signal      = array_create<VkSemaphore>(g_heap);

        g_vulkan->present_semaphores = array_create<VkSemaphore>(g_heap);
    }


    /**************************************************************************
     * Create VkSwapchainKHR
     *************************************************************************/
    // NOTE(jesper): the swapchain is stuck in the VulkanDevice creation right
    // now because we're still hardcoding the depth buffer creation, among other
    // things, in the VulkanDevice, which requires the created swapchain.
    // Really I think it mostly/only need the extent, but same difference
    g_vulkan->swapchain = swapchain_create(&g_vulkan->physical_device, surface, settings);

    /**************************************************************************
     * Create vkRenderPass
     *************************************************************************/
    {
        auto descs = array_create<VkAttachmentDescription>(g_stack);
        array_add(&descs, {
             0,
             g_vulkan->swapchain.format,
             VK_SAMPLE_COUNT_1_BIT,
             VK_ATTACHMENT_LOAD_OP_CLEAR,
             VK_ATTACHMENT_STORE_OP_STORE,
             VK_ATTACHMENT_LOAD_OP_DONT_CARE,
             VK_ATTACHMENT_STORE_OP_DONT_CARE,
             VK_IMAGE_LAYOUT_UNDEFINED,
             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        });

        array_add(&descs, {
            0,
            g_vulkan->swapchain.depth.format,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        });

        auto color = array_create<VkAttachmentReference>(g_stack);
        array_add(&color, { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

        VkAttachmentReference depth = {
            1,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount    = (u32)color.count;
        subpass_description.pColorAttachments       = color.data;
        subpass_description.pDepthStencilAttachment = &depth;

        VkRenderPassCreateInfo create_info = {};
        create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = (u32)descs.count;
        create_info.pAttachments    = descs.data;
        create_info.subpassCount    = 1;
        create_info.pSubpasses      = &subpass_description;
        create_info.dependencyCount = 0;
        create_info.pDependencies   = nullptr;

        result = vkCreateRenderPass(g_vulkan->handle,
                                    &create_info,
                                    nullptr,
                                    &g_vulkan->renderpass);
        DEBUG_ASSERT(result == VK_SUCCESS);
    }

    /**************************************************************************
     * Create Framebuffers
     *************************************************************************/
    {
        auto buffer = g_persistent->alloc_array<VkFramebuffer>(g_vulkan->swapchain.images_count);
        g_vulkan->framebuffers = array_create_static<VkFramebuffer>(buffer, g_vulkan->swapchain.images_count);

        VkFramebufferCreateInfo create_info = {};
        create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass      = g_vulkan->renderpass;
        create_info.width           = g_vulkan->swapchain.extent.width;
        create_info.height          = g_vulkan->swapchain.extent.height;
        create_info.layers          = 1;

        auto attachments = g_stack->alloc_array<VkImageView>(2);
        attachments[1] = g_vulkan->swapchain.depth.imageview;

        for (i32 i = 0; i < (i32)g_vulkan->swapchain.images_count; ++i)
        {
            attachments[0] = g_vulkan->swapchain.imageviews[i];

            create_info.attachmentCount = 2;
            create_info.pAttachments    = attachments;

            VkFramebuffer framebuffer;

            result = vkCreateFramebuffer(g_vulkan->handle,
                                         &create_info,
                                         nullptr,
                                         &framebuffer);
            DEBUG_ASSERT(result == VK_SUCCESS);

            array_add(&g_vulkan->framebuffers, framebuffer);
        }
    }

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    result = vkCreateSemaphore(g_vulkan->handle,
                               &semaphore_info,
                               nullptr,
                               &g_vulkan->swapchain.available);
    DEBUG_ASSERT(result == VK_SUCCESS);

    result = vkCreateSemaphore(g_vulkan->handle,
                               &semaphore_info,
                               nullptr,
                               &g_vulkan->render_completed);
    DEBUG_ASSERT(result == VK_SUCCESS);
}

void pipeline_destroy(VulkanPipeline pipeline)
{
    // TODO(jesper): find a better way to clean these up; not every pipeline
    // will have every shader stage and we'll probably want to keep shader
    // stages in a map of some sort of in the device to reuse
    vkDestroyShaderModule(g_vulkan->handle,
                          pipeline.shaders[ShaderStage_vertex].module,
                          nullptr);
    vkDestroyShaderModule(g_vulkan->handle,
                          pipeline.shaders[ShaderStage_fragment].module,
                          nullptr);

    for (i32 i = 0; i < pipeline.sampler_count; i++) {
        vkDestroySampler(g_vulkan->handle, pipeline.samplers[i], nullptr);
    }

    vkDestroyDescriptorPool(g_vulkan->handle, pipeline.descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(g_vulkan->handle, pipeline.descriptor_layout_pipeline, nullptr);
    vkDestroyDescriptorSetLayout(g_vulkan->handle, pipeline.descriptor_layout_material, nullptr);
    vkDestroyPipelineLayout(g_vulkan->handle, pipeline.layout, nullptr);
    vkDestroyPipeline(g_vulkan->handle, pipeline.handle, nullptr);
}

void swapchain_destroy(VulkanSwapchain swapchain)
{
    for (i32 i = 0; i < (i32)g_vulkan->swapchain.images_count; i++) {
        vkDestroyImageView(g_vulkan->handle, swapchain.imageviews[i], nullptr);
    }

    vkDestroyImageView(g_vulkan->handle, swapchain.depth.imageview, nullptr);
    vkDestroyImage(g_vulkan->handle, swapchain.depth.image, nullptr);
    vkFreeMemory(g_vulkan->handle, swapchain.depth.memory, nullptr);

    vkDestroySwapchainKHR(g_vulkan->handle, swapchain.handle, nullptr);
    vkDestroySurfaceKHR(g_vulkan->instance, swapchain.surface, nullptr);
}

void texture_destroy(VulkanTexture texture)
{
    vkDestroyImageView(g_vulkan->handle, texture.image_view, nullptr);
    vkDestroyImage(g_vulkan->handle, texture.image, nullptr);
    vkFreeMemory(g_vulkan->handle, texture.memory, nullptr);
}

void buffer_destroy(VulkanBuffer buffer)
{
    vkFreeMemory(g_vulkan->handle, buffer.memory, nullptr);
    vkDestroyBuffer(g_vulkan->handle, buffer.handle, nullptr);
}

void buffer_destroy_ubo(VulkanUniformBuffer ubo)
{
    buffer_destroy(ubo.staging);
    buffer_destroy(ubo.buffer);
}

void vulkan_destroy()
{
    for (i32 i = 0; i < g_vulkan->framebuffers.count; ++i) {
        vkDestroyFramebuffer(g_vulkan->handle, g_vulkan->framebuffers[i], nullptr);
    }
    array_destroy(&g_vulkan->framebuffers);

    vkDestroyRenderPass(g_vulkan->handle, g_vulkan->renderpass, nullptr);


    vkDestroyCommandPool(g_vulkan->handle, g_vulkan->command_pool, nullptr);


    vkDestroySemaphore(g_vulkan->handle, g_vulkan->swapchain.available, nullptr);
    vkDestroySemaphore(g_vulkan->handle, g_vulkan->render_completed, nullptr);


    // TODO(jesper): move out of here when the swapchain<->device dependency is
    // fixed
    swapchain_destroy(g_vulkan->swapchain);


    vkDestroyDevice(g_vulkan->handle,     nullptr);

    vkdebug_destroy();

    vkDestroyInstance(g_vulkan->instance, nullptr);
}

VulkanBuffer create_buffer(usize size,
                           VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags memory_flags)
{
    VulkanBuffer buffer = {};
    buffer.size = size;

    VkBufferCreateInfo create_info = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size                  = size;
    create_info.usage                 = usage;
    create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = nullptr;

    VkResult result = vkCreateBuffer(g_vulkan->handle,
                                     &create_info,
                                     nullptr,
                                     &buffer.handle);
    DEBUG_ASSERT(result == VK_SUCCESS);

    // TODO: allocate buffers from large memory pool in VulkanDevice
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(g_vulkan->handle, buffer.handle, &memory_requirements);

    u32 index = find_memory_type(g_vulkan->physical_device,
                                 memory_requirements.memoryTypeBits,
                                 memory_flags);
    DEBUG_ASSERT(index != UINT32_MAX);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize  = memory_requirements.size;
    allocate_info.memoryTypeIndex = index;

    result = vkAllocateMemory(g_vulkan->handle,
                              &allocate_info,
                              nullptr,
                              &buffer.memory);
    DEBUG_ASSERT(result == VK_SUCCESS);

    result = vkBindBufferMemory(g_vulkan->handle, buffer.handle, buffer.memory, 0);
    DEBUG_ASSERT(result == VK_SUCCESS);
    return buffer;
}

void buffer_copy(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBuffer command = command_buffer_begin();

    VkBufferCopy region = {};
    region.srcOffset    = 0;
    region.dstOffset    = 0;
    region.size         = size;

    vkCmdCopyBuffer(command, src, dst, 1, &region);

    command_buffer_end(command, true);
}

VulkanBuffer create_vbo(void *data, usize size)
{
    VulkanBuffer vbo = create_buffer(size,
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    void *mapped;
    vkMapMemory(g_vulkan->handle, vbo.memory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(g_vulkan->handle, vbo.memory);

    return vbo;
}

VulkanBuffer create_vbo(usize size)
{
    VulkanBuffer vbo = create_buffer(size,
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    return vbo;
}

VulkanBuffer create_ibo(u32 *indices, usize size)
{
    VulkanBuffer staging = create_buffer(size,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vkMapMemory(g_vulkan->handle, staging.memory, 0, size, 0, &data);
    memcpy(data, indices, size);
    vkUnmapMemory(g_vulkan->handle, staging.memory);

    VulkanBuffer ib = create_buffer(size,
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffer_copy(staging.handle, ib.handle, size);

    buffer_destroy(staging);

    return ib;
}

VulkanUniformBuffer create_ubo(usize size)
{
    VulkanUniformBuffer ubo;
    ubo.staging = create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    ubo.buffer = create_buffer(size,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    return ubo;
}


void buffer_data(VulkanUniformBuffer ubo, void *data, usize offset, usize size)
{
    void *mapped;
    vkMapMemory(g_vulkan->handle,
                ubo.staging.memory,
                offset, size,
                0,
                &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(g_vulkan->handle, ubo.staging.memory);

    buffer_copy(ubo.staging.handle, ubo.buffer.handle, size);
}

u32 swapchain_acquire()
{
    VkResult result;
    u32 image_index;
    result = vkAcquireNextImageKHR(g_vulkan->handle,
                                   g_vulkan->swapchain.handle,
                                   UINT64_MAX,
                                   g_vulkan->swapchain.available,
                                   VK_NULL_HANDLE,
                                   &image_index);
    DEBUG_ASSERT(result == VK_SUCCESS);
    return image_index;
}

Material create_material(VulkanPipeline *pipeline, MaterialID id)
{
    Material mat = {};
    mat.id       = id;
    mat.pipeline = pipeline;

    auto pool_sizes = array_create<VkDescriptorPoolSize>(g_frame);

    switch (id) {
    case Material_phong: {
        array_add(&pool_sizes, VkDescriptorPoolSize{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1
        });
    } break;
    case Material_basic2d: {
        array_add(&pool_sizes, VkDescriptorPoolSize{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1
        });
    } break;
    default:
        DEBUG_LOG("unknown material");
        DEBUG_ASSERT(false);
        break;
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = (u32)pool_sizes.count;
    pool_info.pPoolSizes    = pool_sizes.data;
    pool_info.maxSets       = 1;

    VkResult result = vkCreateDescriptorPool(g_vulkan->handle,
                                             &pool_info,
                                             nullptr,
                                             &mat.descriptor_pool);
    DEBUG_ASSERT(result == VK_SUCCESS);

    VkDescriptorSetAllocateInfo dai = {};
    dai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dai.descriptorPool     = mat.descriptor_pool;
    dai.descriptorSetCount = 1;
    dai.pSetLayouts        = &pipeline->descriptor_layout_material;

    result = vkAllocateDescriptorSets(g_vulkan->handle,
                                      &dai,
                                      &mat.descriptor_set);
    DEBUG_ASSERT(result == VK_SUCCESS);

    return mat;
}

void destroy_material(Material material)
{
    vkDestroyDescriptorPool(g_vulkan->handle, material.descriptor_pool, nullptr);
}

void set_texture(Material *material, ResourceSlot slot, VulkanTexture *texture)
{
    // TODO(jesper): use this to figure out which sampler and dstBinding to set
    (void)slot;

    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView   = texture->image_view;
    // TODO(jesper): figure this one out based on ResourceSlot
    // NOTE(jesper): there might be some merit to sticking the sampler inside
    // the material, as this'd allow us to use different image samplers with
    // different materials? would make sense I think
    image_info.sampler     = material->pipeline->samplers[0];

    VkWriteDescriptorSet writes = {};
    writes.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes.dstSet          = material->descriptor_set;

    // TODO(jesper): the dstBinding depends on ResourceSlot and MaterialID
    switch (slot) {
    case ResourceSlot_diffuse:
        writes.dstBinding      = 0;
        break;
    default:
        DEBUG_LOG("unknown resource slot");
        DEBUG_ASSERT(false);
        break;
    }

    writes.dstArrayElement = 0;
    writes.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes.descriptorCount = 1;
    writes.pImageInfo      = &image_info;

    vkUpdateDescriptorSets(g_vulkan->handle, 1, &writes, 0, nullptr);
}

void set_ubo(VulkanPipeline *pipeline,
             ResourceSlot slot,
             VulkanUniformBuffer *ubo)
{
    // TODO(jesper): use this to figure out the dstBinding to use
    (void)slot;

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = ubo->buffer.handle;
    // TODO(jesper): support ubo offsets
    buffer_info.offset = 0;
    buffer_info.range  = ubo->buffer.size;

    VkWriteDescriptorSet writes = {};
    writes.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes.dstSet          = pipeline->descriptor_set;
    // TODO(jesper): set this based on ResourceSlot
    writes.dstBinding      = 0;
    writes.dstArrayElement = 0;
    writes.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes.descriptorCount = 1;
    writes.pBufferInfo     = &buffer_info;

    vkUpdateDescriptorSets(g_vulkan->handle, 1, &writes, 0, nullptr);
}

PushConstants push_constants_create(PipelineID pipeline)
{
    PushConstants c = {};

    switch (pipeline) {
    case Pipeline_font: {
        c.offset = 0;
        c.size   = sizeof(Matrix4);
        c.data   = g_heap->alloc(sizeof(Matrix4));
    } break;
    case Pipeline_basic2d: {
        c.offset = 0;
        c.size   = sizeof(Matrix4);
        c.data   = g_heap->alloc(sizeof(Matrix4));
    } break;
    default:
        // TODO(jesper): error handling
        DEBUG_ASSERT(false);
        break;
    }

    return c;
}

template<typename T>
void set_push_constant(PushConstants *c, T t)
{
    // TODO(jesper): handle multiple push constants
    memcpy(c->data, &t, sizeof(T));
}