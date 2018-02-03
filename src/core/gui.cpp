/**
 * file:    gui.cpp
 * created: 2018-01-31
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

struct DebugInfo
{
    const char* file;
    u32         line;
};

struct GuiRenderItem
{
    Vector2                position;
    VulkanBuffer           vbo;
    VkDeviceSize           vbo_offset;
    i32                    vertex_count;

    PipelineID             pipeline_id;
    Array<VkDescriptorSet> descriptors;
    PushConstants          constants;

#if LEARY_DEBUG
    DebugInfo              debug_info;
#endif
};


VulkanBuffer g_gui_vbo;
usize g_gui_vbo_offset = 0;
void* g_gui_vbo_map = nullptr;

Array<GuiRenderItem> g_gui_render_queue;

void init_gui()
{
    init_array(&g_gui_render_queue, g_frame);

    g_gui_vbo = create_vbo(1024*1024);
}

void destroy_gui()
{
    destroy_buffer(g_gui_vbo);
}

void gui_frame_start()
{
    g_gui_vbo_offset = 0;
    g_font.offset = 0;

    ASSERT(g_font.buffer == nullptr);
    ASSERT(g_gui_vbo_map == nullptr);

    VkResult result = vkMapMemory(
        g_vulkan->handle,
        g_font.vbo.memory,
        0, VK_WHOLE_SIZE, 0,
        &g_font.buffer);

    ASSERT(result == VK_SUCCESS);

    result = vkMapMemory(
        g_vulkan->handle,
        g_gui_vbo.memory,
        0, VK_WHOLE_SIZE, 0,
        &g_gui_vbo_map);

    ASSERT(result == VK_SUCCESS);
}

void gui_render(VkCommandBuffer command)
{
    if (g_font.buffer != nullptr) {
        vkUnmapMemory(g_vulkan->handle, g_font.vbo.memory);
        g_font.buffer = nullptr;
    }

    if (g_gui_vbo_map != nullptr) {
        vkUnmapMemory(g_vulkan->handle, g_gui_vbo.memory);
        g_gui_vbo_map = nullptr;
    }

    for (i32 i = 0; i < g_gui_render_queue.count; i++) {
        auto &item = g_gui_render_queue[i];

        ASSERT(item.pipeline_id < Pipeline_count);
        VulkanPipeline &pipeline = g_vulkan->pipelines[item.pipeline_id];

        vkCmdBindPipeline(
            command,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.handle);

        if (item.descriptors.count > 0) {
            vkCmdBindDescriptorSets(
                command,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout,
                0,
                (u32)item.descriptors.count,
                item.descriptors.data,
                0, nullptr);
        }

        vkCmdPushConstants(
            command,
            pipeline.layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            item.constants.offset,
            (u32)item.constants.size,
            item.constants.data);

        vkCmdBindVertexBuffers(command, 0, 1, &item.vbo.handle, &item.vbo_offset);
        vkCmdDraw(command, item.vertex_count, 1, 0, 0);
    }
    g_gui_render_queue.count = 0;
}

void gui_textbox(StringView text, Vector2 *pos)
{
    i32 vertex_count = 0;

    usize vertices_size = sizeof(f32) * 24 * text.size;
    auto vertices = (f32*)g_frame->alloc(vertices_size);

    f32 bx = pos->x;

    i32 vi = 0;

    for (i32 i = 0; i < text.size; i++) {
        char c = text[i];

        if (c == '\n') {
            pos->y += 20.0f;
            pos->x  = bx;
            vertices_size -= sizeof(f32)*4*6;
            continue;
        }

        vertex_count += 6;

        stbtt_aligned_quad q = {};
        stbtt_GetBakedQuad(g_font.atlas, 1024, 1024, c, &pos->x, &pos->y, &q, 1);

        Vector2 tl = camera_from_screen(Vector2{q.x0, q.y0 + 15.0f});
        Vector2 tr = camera_from_screen(Vector2{q.x1, q.y0 + 15.0f});
        Vector2 br = camera_from_screen(Vector2{q.x1, q.y1 + 15.0f});
        Vector2 bl = camera_from_screen(Vector2{q.x0, q.y1 + 15.0f});

        vertices[vi++] = tl.x;
        vertices[vi++] = tl.y;
        vertices[vi++] = q.s0;
        vertices[vi++] = q.t0;

        vertices[vi++] = tr.x;
        vertices[vi++] = tr.y;
        vertices[vi++] = q.s1;
        vertices[vi++] = q.t0;

        vertices[vi++] = br.x;
        vertices[vi++] = br.y;
        vertices[vi++] = q.s1;
        vertices[vi++] = q.t1;

        vertices[vi++] = br.x;
        vertices[vi++] = br.y;
        vertices[vi++] = q.s1;
        vertices[vi++] = q.t1;

        vertices[vi++] = bl.x;
        vertices[vi++] = bl.y;
        vertices[vi++] = q.s0;
        vertices[vi++] = q.t1;

        vertices[vi++] = tl.x;
        vertices[vi++] = tl.y;
        vertices[vi++] = q.s0;
        vertices[vi++] = q.t0;
    }

    GuiRenderItem item = {};
    item.pipeline_id = Pipeline_font;

#if LEARY_DEBUG
    item.debug_info.file = __FILE__;
    item.debug_info.line = __LINE__;
#endif // LEARY_DEBUG

    init_array(&item.descriptors, g_frame);
    array_add(&item.descriptors, g_game->materials.font.descriptor_set);

    item.vbo          = g_font.vbo;
    item.vbo_offset   = g_font.offset;
    item.vertex_count = vertex_count;

    Matrix4 t = Matrix4::identity();
    item.constants.offset = 0;
    item.constants.size   = sizeof t;
    item.constants.data   = g_frame->alloc( item.constants.size );
    memcpy(item.constants.data, &t, sizeof t);

    ASSERT(g_font.offset + vertices_size < g_font.vbo.size);
    memcpy((void*)((uptr)g_font.buffer + g_font.offset), vertices, vertices_size);
    g_font.offset += vertices_size;

    array_add(&g_gui_render_queue, item);
}

void gui_frame(Vector2 position, f32 width, f32 height)
{
    struct Vertex {
        Vector2 position;
        Vector4 color;
    };

    ASSERT(g_gui_vbo_offset < g_gui_vbo.size);

    GuiRenderItem item = {};
    item.pipeline_id = Pipeline_gui_basic;

#if LEARY_DEBUG
    item.debug_info.file = __FILE__;
    item.debug_info.line = __LINE__;
#endif // LEARY_DEBUG


    Vertex tl, tr, br, bl;
    tl.position = camera_from_screen(Vector2{ position.x,         position.y });
    tr.position = camera_from_screen(Vector2{ position.x + width, position.y });
    br.position = camera_from_screen(Vector2{ position.x + width, position.y + height });
    bl.position = camera_from_screen(Vector2{ position.x,         position.y + height });

    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    tl.color = tr.color = br.color = bl.color = color;

    Array<Vertex> vertices;
    init_array(&vertices, g_frame, 6);
    array_add(&vertices, tl);
    array_add(&vertices, tr);
    array_add(&vertices, br);

    array_add(&vertices, br);
    array_add(&vertices, bl);
    array_add(&vertices, tl);

    item.vbo          = g_gui_vbo;
    item.vbo_offset   = g_gui_vbo_offset;
    item.vertex_count = vertices.count;

    Matrix4 t = Matrix4::identity();
    item.constants.offset = 0;
    item.constants.size   = sizeof t;
    item.constants.data   = g_frame->alloc( item.constants.size );
    memcpy(item.constants.data, &t, sizeof t);

    ASSERT(g_gui_vbo_offset + vertices.count * sizeof vertices[0] < g_gui_vbo.size);

    memcpy((void*)((uptr)g_gui_vbo_map + g_gui_vbo_offset),
           vertices.data,
           vertices.count * sizeof vertices[0]);

    g_gui_vbo_offset += vertices.count * sizeof vertices[0];
    array_add(&g_gui_render_queue, item);
}
