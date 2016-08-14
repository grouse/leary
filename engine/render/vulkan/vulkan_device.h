#ifndef LEARY_VULKAN_DEVICE_H
#define LEARY_VULKAN_DEVICE_H

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan.h>

class GameWindow;
class VulkanDevice;

struct VulkanBuffer {
	size_t               size;

	VkBuffer             vk_buffer;
	VkDeviceMemory       vk_memory;

	VulkanDevice*        device;

	void create(VulkanDevice* device, VkBufferUsageFlags usage, size_t size, uint8_t* data);
	void destroy();
};

class VulkanDevice {
public:
	void create(const GameWindow& window);
	void destroy();

	void present();

	uint32_t         m_width;
	uint32_t         m_height;

	VkInstance       m_instance;

	// Device and its queue(s)
	VkDevice         m_device;
	VkQueue          m_queue;
	uint32_t         m_queueFamilyIndex;

	// Physical device
	VkPhysicalDevice                 m_physicalDevice;
	VkPhysicalDeviceMemoryProperties memory_properties;


	// Swapchain
	VkSurfaceKHR     m_surface;
	VkFormat         m_surfaceFormat;
	VkSwapchainKHR   m_swapchain;

	uint32_t         m_swapchainImagesCount;
	VkImage         *m_swapchainImages;
	VkImageView     *m_swapchainImageViews;

	// Command pool and buffers
	VkCommandPool    m_commandPool;

	VkCommandBuffer  m_commandBuffers[2];
	VkCommandBuffer  m_commandBufferInit;
	VkCommandBuffer  m_commandBufferPresent;

	// Depth Buffer
	VkImage          m_depthImage;
	VkImageView      m_depthImageView;
	VkDeviceMemory   m_depthMemory;

	// Render pass
	VkRenderPass     m_renderPass;

	// Framebuffer
	VkFramebuffer   *m_framebuffers;
	uint32_t         m_framebuffersCount;

	// Vertex buffer
	VulkanBuffer     vertex_buffer;

	// Pipeline
	VkPipeline       m_pipeline;
	VkPipelineLayout m_pipelineLayout;

	VkShaderModule   m_vertexShader;
	VkShaderModule   m_fragmentShader;


};

#endif // LEARY_VULKAN_DEVICE_H
