/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */


#include "render/vulkan_device.cpp"

#include "core/settings.cpp"
#include "core/tokenizer.cpp"
#include "core/serialize.cpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb/stb_rect_pack.h"
#include "external/stb/stb_truetype.h"

enum InputAction {
	InputAction_move_vertical_start,
	InputAction_move_horizontal_start,
	InputAction_move_vertical_end,
	InputAction_move_horizontal_end,

	InputAction_move_player_vertical_start,
	InputAction_move_player_horizontal_start,
	InputAction_move_player_vertical_end,
	InputAction_move_player_horizontal_end
};

struct Camera {
	Matrix4 view;
	Matrix4 projection;
};

struct RenderedText {
	VulkanBuffer buffer;
	i32          vertex_count;
};

struct RenderObject {
	VulkanBuffer vertices;
	VulkanBuffer indices;
	i32          vertex_count;
};

struct GameState {
	VulkanDevice        vulkan;
	VulkanPipeline      pipeline;

	VulkanTexture       texture;

	i32                 num_objects;
	RenderObject        object;
	Matrix4            *positions;

	VkCommandBuffer     *command_buffers;

	Camera              camera;
	VulkanUniformBuffer camera_ubo;

	Matrix4            ui_camera;

	Vector3            velocity = {};
	Vector3            player_velocity = {};

	VulkanPipeline      font_pipeline;
	VulkanTexture       font_texture;

	stbtt_bakedchar     baked_font[256];

	char                *text_buffer;
	RenderedText        text_vertices;

	i32 *key_state;
};

void render_font(GameState *game, RenderedText *text,
                 const char *str, float x, float y)
{
	i32 offset = 0;

	usize text_length = strlen(str);
	if (text_length == 0) return;

	usize vertices_size = sizeof(f32)*30*text_length;
	f32 *vertices = (f32*)malloc(vertices_size);

	text->vertex_count = (i32)(text_length * 6);

	Matrix4 camera = translate(game->ui_camera, {x, y, 0.0f});

	float tmp_x = 0.0f, tmp_y = 0.0f;

	while (*str) {
		char c = *str++;
		if (c == '\n') {
			tmp_y += 20.0f;
			tmp_x  = 0.0f;
			continue;
		}

		stbtt_aligned_quad q = {};
		stbtt_GetBakedQuad(game->baked_font, 1024, 1024, c, &tmp_x, &tmp_y, &q, 1);

		Vector3 tl = camera * Vector3{q.x0, q.y0 + 15.0f, 0.0f};
		Vector3 tr = camera * Vector3{q.x1, q.y0 + 15.0f, 0.0f};
		Vector3 br = camera * Vector3{q.x1, q.y1 + 15.0f, 0.0f};
		Vector3 bl = camera * Vector3{q.x0, q.y1 + 15.0f, 0.0f};

		vertices[offset++] = tl.x;
		vertices[offset++] = tl.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t0;

		vertices[offset++] = tr.x;
		vertices[offset++] = tr.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t0;

		vertices[offset++] = br.x;
		vertices[offset++] = br.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t1;

		vertices[offset++] = br.x;
		vertices[offset++] = br.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t1;

		vertices[offset++] = bl.x;
		vertices[offset++] = bl.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t1;

		vertices[offset++] = tl.x;
		vertices[offset++] = tl.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t0;
	}

	void *mapped;
	vkMapMemory(game->vulkan.handle, text->buffer.memory,
	            0, VK_WHOLE_SIZE,
	            0, &mapped);

	memcpy(mapped, vertices, vertices_size);
	vkUnmapMemory(game->vulkan.handle, text->buffer.memory);
}


void game_load_settings(Settings *settings)
{
	VAR_UNUSED(settings);
	char *settings_path = platform_resolve_path(GamePath_preferences,
	                                            "settings.conf");
	SERIALIZE_LOAD_CONF(settings_path, Settings, settings);
	free(settings_path);
}

void game_init(Settings *settings, PlatformState *platform, GameState *game)
{
	// TODO(jesper): use one large buffer for the entire thing
	g_profile_timers = {};
	g_profile_timers.max_index = NUM_PROFILE_TIMERS;
	g_profile_timers.names = (const char**)malloc(sizeof(char*) * NUM_PROFILE_TIMERS);
	g_profile_timers.cycles = (u64*)malloc(sizeof(u64) * NUM_PROFILE_TIMERS);
	g_profile_timers.cycles_last = (u64*)malloc(sizeof(u64) * NUM_PROFILE_TIMERS);
	g_profile_timers.open = (bool*)malloc(sizeof(bool) * NUM_PROFILE_TIMERS);

	g_profile_timers_prev = {};
	g_profile_timers_prev.max_index = NUM_PROFILE_TIMERS;
	g_profile_timers_prev.names = (const char**)malloc(sizeof(char*) * NUM_PROFILE_TIMERS);
	g_profile_timers_prev.cycles = (u64*)malloc(sizeof(u64) * NUM_PROFILE_TIMERS);
	g_profile_timers_prev.cycles_last = (u64*)malloc(sizeof(u64) * NUM_PROFILE_TIMERS);
	g_profile_timers_prev.open = (bool*)malloc(sizeof(bool) * NUM_PROFILE_TIMERS);

	game->text_buffer = (char*)malloc(1024 * 1024);

	VAR_UNUSED(platform);
	game->vulkan = create_device(settings, platform);

	Vector4 *pixels = new Vector4[32*32];
	pixels[0]    = { 1.0f, 0.0f, 0.0f, 1.0f };
	pixels[31]   = { 0.0f, 1.0f, 0.0f, 1.0f };
	pixels[1023] = { 0.0f, 0.0f, 1.0f, 1.0f };

	VkComponentMapping components = {};
	game->texture = create_texture(&game->vulkan,
	                               32, 32, VK_FORMAT_R32G32B32A32_SFLOAT,
	                               pixels, components);
	delete[] pixels;

	//game->camera.view = look_at({0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
	game->camera.view = Matrix4::identity();

#if 0
	f32 left   = - (f32)settings->video.resolution.width / 2.0f;
	f32 right  =   (f32)settings->video.resolution.width / 2.0f;
	f32 bottom = - (f32)settings->video.resolution.height / 2.0f;
	f32 top    =   (f32)settings->video.resolution.height / 2.0f;
	game->camera.projection = Matrix4::orthographic(left, right, top, bottom, 0.0f, 1.0f);
#else
	f32 width = (f32)settings->video.resolution.width;
	f32 height = (f32)settings->video.resolution.height;

	f32 aspect = width / height;
	f32 vfov   = radians(45.0f);
	game->camera.projection = Matrix4::perspective(vfov, aspect, 0.1f, 10.0f);
#endif

	game->camera_ubo = create_uniform_buffer(&game->vulkan, sizeof(Camera));
	update_uniform_data(&game->vulkan, game->camera_ubo,
	                    &game->camera, 0, sizeof(Camera));


	game->num_objects = 5;
	game->positions  = (Matrix4*) malloc(5 * sizeof(Matrix4));

	Matrix4 position = rotate_z(Matrix4::identity(), radians(30.0f));
	game->positions[0] = translate(position, {0.0f, 0.0f, -4.0f});

	VkResult result;

	game->command_buffers = (VkCommandBuffer*) malloc(5 * sizeof(VkCommandBuffer));
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool        = game->vulkan.command_pool;
	allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = game->num_objects;

	result = vkAllocateCommandBuffers(game->vulkan.handle,
	                                  &allocate_info,
	                                  game->command_buffers);
	DEBUG_ASSERT(result == VK_SUCCESS);

	f32 vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	};
	u16 indices[] = {
		0, 1, 2, 2, 3, 0
	};

	game->object.vertices = create_vertex_buffer(&game->vulkan,
	                                             sizeof(vertices),
	                                             vertices);
	game->object.indices = create_index_buffer(&game->vulkan,
	                                           indices, sizeof(indices));
	game->object.vertex_count = sizeof(indices) / sizeof(indices[0]);

	game->pipeline = create_pipeline(&game->vulkan);
	update_descriptor_sets(&game->vulkan,
	                       game->pipeline,
	                       game->texture,
	                       game->camera_ubo);


	{
		Matrix4 ui_camera = Matrix4::identity();
		f32 width  = (f32)settings->video.resolution.width;
		f32 height = (f32)settings->video.resolution.height;
		ui_camera[0].x = 2.0f / width;
		ui_camera[1].y = 2.0f / height;
		ui_camera[2].z = 1.0f;
		game->ui_camera = ui_camera;

		game->font_pipeline = create_font_pipeline(&game->vulkan);

		usize font_size;
		char *font_path = platform_resolve_path(GamePath_data,
		                                        "fonts/Roboto-Regular.ttf");
		u8 *font_data = (u8*)platform_file_read(font_path, &font_size);

		u8 *bitmap = new u8[1024*1024];
		stbtt_BakeFontBitmap(font_data, 0, 20.0, bitmap, 1024, 1024, 0,
		                     256, game->baked_font);

		components.a = VK_COMPONENT_SWIZZLE_R;
		game->font_texture = create_texture(&game->vulkan, 1024, 1024,
		                                    VK_FORMAT_R8_UNORM, bitmap,
		                                    components);

		update_descriptor_sets(&game->vulkan,
		                       game->font_pipeline,
		                       game->font_texture);

		game->text_vertices.vertex_count = 0;
		game->text_vertices.buffer = create_vertex_buffer(&game->vulkan, 1024*1024);
	}

	game->key_state = (i32*)malloc(sizeof(i32) * 0xFF);
	for (i32 i = 0; i < 0xFF; i++) {
		game->key_state[i] = InputType_key_release;
	}
}

void game_quit(GameState *game, Settings *settings)
{
	VAR_UNUSED(settings);
	vkQueueWaitIdle(game->vulkan.queue);

	destroy(&game->vulkan, game->text_vertices.buffer);
	destroy(&game->vulkan, game->font_texture);
	destroy(&game->vulkan, game->font_pipeline);

	destroy(&game->vulkan, game->object.vertices);
	destroy(&game->vulkan, game->object.indices);
	destroy(&game->vulkan, game->camera_ubo);
	destroy(&game->vulkan, game->texture);
	destroy(&game->vulkan, game->pipeline);
	destroy(&game->vulkan);


	char *settings_path = platform_resolve_path(GamePath_preferences,
	                                            "settings.conf");
	SERIALIZE_SAVE_CONF(settings_path, Settings, settings);
	free(settings_path);

	platform_quit();
}

void game_input(GameState *game, Settings *settings, InputEvent event)
{
	switch (event.type) {
	case InputType_key_press: {
		game->key_state[event.key.vkey] = InputType_key_press;

		switch (event.key.vkey) {
		case VirtualKey_escape:
			game_quit(game, settings);
			break;
		case VirtualKey_W:
			game->velocity.z = -1.0f;
			break;
		case VirtualKey_S:
			game->velocity.z = 1.0f;
			break;
		case VirtualKey_A:
			game->velocity.x = -1.0f;
			break;
		case VirtualKey_D:
			game->velocity.x = 1.0f;
			break;
		default:
			DEBUG_LOG("unhandled key press: %d", event.key.vkey);
			break;
		}
	} break;
	case InputType_key_release: {
		game->key_state[event.key.vkey] = InputType_key_release;

		switch (event.key.vkey) {
		case VirtualKey_W:
			game->velocity.z = 0.0f;
			if (game->key_state[VirtualKey_S] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_S;
				e.key.repeated = false;

				game_input(game, settings, e);
			}
			break;
		case VirtualKey_S:
			game->velocity.z = 0.0f;
			if (game->key_state[VirtualKey_W] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_W;
				e.key.repeated = false;

				game_input(game, settings, e);
			}
			break;
		case VirtualKey_A:
			game->velocity.x = 0.0f;
			if (game->key_state[VirtualKey_D] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_D;
				e.key.repeated = false;

				game_input(game, settings, e);
			}
			break;
		case VirtualKey_D:
			game->velocity.x = 0.0f;
			if (game->key_state[VirtualKey_A] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_A;
				e.key.repeated = false;

				game_input(game, settings, e);
			}
			break;
		default:
			//DEBUG_LOG("unhandled key release: %d", event.key.vkey);
			break;
		}
	} break;
	default:
		DEBUG_LOG("unhandled input type: %d", event.type);
		break;
	}
}

void game_profile_collate(GameState* game, f32 dt)
{
	PROFILE_FUNCTION();

	for (i32 i = 0; i < g_profile_timers_prev.index - 1; i++) {
		for (i32 j = i+1; j < g_profile_timers_prev.index; j++) {
			if (g_profile_timers_prev.cycles[j] > g_profile_timers_prev.cycles[i]) {
				const char *name_tmp = g_profile_timers_prev.names[j];
				u64 cycles_tmp = g_profile_timers_prev.cycles[j];
				u64 cycles_last_tmp = g_profile_timers_prev.cycles_last[j];

				g_profile_timers_prev.names[j] = g_profile_timers_prev.names[i];
				g_profile_timers_prev.cycles[j] = g_profile_timers_prev.cycles[i];
				g_profile_timers_prev.cycles_last[j] = g_profile_timers_prev.cycles_last[i];

				g_profile_timers_prev.names[i] = name_tmp;
				g_profile_timers_prev.cycles[i] = cycles_tmp;
				g_profile_timers_prev.cycles_last[i] = cycles_last_tmp;
			}
		}
	}

	isize buffer_size = 1024*1024;
	char *buffer = game->text_buffer;
	buffer[0] = '\0';

	f32 dt_ms = dt * 1000.0f;
	i32 bytes = snprintf(buffer, buffer_size, "frametime: %f ms, %f fps\n",
	                     dt_ms, 1000.0f / dt_ms);
	buffer += bytes;
	buffer_size -= bytes;


	for (i32 i = 0; i < g_profile_timers_prev.index; i++) {
		bytes = snprintf(buffer, buffer_size, "%s: %" PRIu64 " cy (%" PRIu64 " cy)\n",
		                 g_profile_timers_prev.names[i],
		                 g_profile_timers_prev.cycles[i],
		                 g_profile_timers_prev.cycles_last[i]);
		buffer += bytes;
		buffer_size -= bytes;
	}

	render_font(game, &game->text_vertices, game->text_buffer, -1.0f, -1.0f);
}

void game_update(GameState* game, f32 dt)
{
	PROFILE_FUNCTION();

	game_profile_collate(game, dt);

	game->positions[0] = translate(game->positions[0], dt * game->velocity);
	game->positions[0] = rotate_x(game->positions[0], dt * 1.0f);
	game->positions[0] = rotate_y(game->positions[0], dt * 1.0f);
	game->positions[0] = rotate_z(game->positions[0], dt * 1.0f);
}

void game_render(GameState *game)
{
	PROFILE_FUNCTION();

	u32 image_index = acquire_swapchain_image(&game->vulkan);

	std::array<VkClearValue, 1> clear_values = {};
	clear_values[0].color        = { {1.0f, 0.0f, 0.0f, 0.0f} };

	VkRenderPassBeginInfo render_info = {};
	render_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_info.renderPass        = game->vulkan.renderpass;
	render_info.framebuffer       = game->vulkan.framebuffers[image_index];
	render_info.renderArea.offset = { 0, 0 };
	render_info.renderArea.extent = game->vulkan.swapchain.extent;
	render_info.clearValueCount   = (u32)clear_values.size();
	render_info.pClearValues      = clear_values.data();

	VkDeviceSize offsets[] = { 0 };
	VkResult result;

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkCommandBuffer command = game->command_buffers[0];
	result = vkBeginCommandBuffer(command, &begin_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	vkCmdBeginRenderPass(command, &render_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  game->pipeline.handle);


	vkCmdBindDescriptorSets(command,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        game->pipeline.layout,
	                        0,
	                        1, &game->pipeline.descriptor_set,
	                        0, nullptr);

	vkCmdBindVertexBuffers(command, 0, 1, &game->object.vertices.handle, offsets);
	vkCmdBindIndexBuffer(command, game->object.indices.handle,
	                     0, VK_INDEX_TYPE_UINT16);

#if 0
	for (i32 i = 0; i < game->num_objects; i++) {
		vkCmdPushConstants(command,
		                   game->pipeline.layout,
		                   VK_SHADER_STAGE_VERTEX_BIT,
		                   0, sizeof(Matrix4),
		                   &game->positions[i]);
		vkCmdDrawIndexed(command, game->object.vertex_count, 1, 0, 0, 0);
	}
#else
	vkCmdPushConstants(command, game->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT,
	                   0, sizeof(Matrix4), &game->positions[0]);
	vkCmdDrawIndexed(command, game->object.vertex_count, 1, 0, 0, 0);
#endif


	vkCmdBindPipeline(command,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  game->font_pipeline.handle);


	vkCmdBindDescriptorSets(command,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        game->font_pipeline.layout,
	                        0,
	                        1, &game->font_pipeline.descriptor_set,
	                        0, nullptr);



	if (game->text_vertices.vertex_count > 0) {
		vkCmdBindVertexBuffers(command, 0, 1,
		                       &game->text_vertices.buffer.handle, offsets);
		vkCmdDraw(command, game->text_vertices.vertex_count, 1, 0, 0);
	}


	vkCmdEndRenderPass(command);

	result = vkEndCommandBuffer(command);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkPipelineStageFlags wait_stages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSemaphore signal_semaphores[] = {
		game->vulkan.render_completed
	};

	VkSubmitInfo present_submit_info = {};
	present_submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	present_submit_info.waitSemaphoreCount   = 1;
	present_submit_info.pWaitSemaphores      = &game->vulkan.swapchain.available;
	present_submit_info.pWaitDstStageMask    = wait_stages;
	present_submit_info.commandBufferCount   = 1;
	present_submit_info.pCommandBuffers      = game->command_buffers;
	present_submit_info.signalSemaphoreCount = 1;
	present_submit_info.pSignalSemaphores    = signal_semaphores;

	result = vkQueueSubmit(game->vulkan.queue, 1, &present_submit_info, VK_NULL_HANDLE);
	DEBUG_ASSERT(result == VK_SUCCESS);


	VkSemaphore wait_semaphores[] = {
		game->vulkan.render_completed
	};

	VkSwapchainKHR swapchains[] = {
		game->vulkan.swapchain.handle
	};
	VkPresentInfoKHR present_info = {};
	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = wait_semaphores;
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = swapchains;
	present_info.pImageIndices      = &image_index;

	result = vkQueuePresentKHR(game->vulkan.queue, &present_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	PROFILE_START(vulkan_swap);
	result = vkQueueWaitIdle(game->vulkan.queue);
	DEBUG_ASSERT(result == VK_SUCCESS);
	PROFILE_END(vulkan_swap);
}

void game_update_and_render(GameState *game, f32 dt)
{
	game_update(game, dt);
	game_render(game);

	ProfileTimers tmp      = g_profile_timers_prev;
	g_profile_timers_prev  = g_profile_timers;
	g_profile_timers       = tmp;

	for (i32 i = 0; i < g_profile_timers.index; i++) {
		g_profile_timers.cycles[i] = 0;
	}
}
