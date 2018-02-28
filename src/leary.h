/**
 * file:    leary.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#ifndef LEARY_H
#define LEARY_H

#include "build_config.h"

#ifndef INTROSPECT
#define INTROSPECT
#endif

#include <utility>

#include "external/stb/stb_truetype.h"

#include "platform/platform_debug.h"
#include "leary_macros.h"

#include "core/profiling.h"
#include "core/allocator.h"
#include "core/maths.h"
#include "core/gfx_vulkan.h"

enum DebugOverlayItemType {
    Debug_allocator_stack,
    Debug_allocator_free_list,
    Debug_render_item,
    Debug_profile_timers,
    Debug_allocators
};

struct DebugRenderItem {
    Vector2                position;
    PipelineID             pipeline;
    Array<VkDescriptorSet> descriptors  = {};
    VulkanBuffer           vbo;
    i32                    vertex_count = 0;
    PushConstants          constants;
};

struct DebugOverlayItem {
    const char               *title;
    Vector2 tl, br;
    Array<DebugOverlayItem*> children;
    bool                     collapsed;
    DebugOverlayItemType     type;
    union {
        void            *data;
        DebugRenderItem ritem;
    } u;
};

struct DebugOverlay {
    struct {
        VulkanBuffer vbo;
        i32          vertex_count;
        Vector3      position;
    } texture;

    Array<DebugOverlayItem> items;
    Array<DebugRenderItem>  render_queue;
};

struct Entity {
    i32        id;
    i32        index;
    Vector3    position;
    Vector3    scale;
    Quaternion rotation = Quaternion::make( Vector3{ 0.0f, 1.0f, 0.0f });
};

struct IndexRenderObject {
    i32            entity_id;
    PipelineID     pipeline;
    VulkanBuffer   vbo;
    VulkanBuffer   ibo;
    i32            index_count;
    //Matrix4        transform;
    Material       *material;
};

// TODO(jesper): stick these in the pipeline so that we reduce the number of
// pipeline binds
struct RenderObject {
    PipelineID     pipeline;
    VulkanBuffer   vbo;
    i32            vertex_count;
    Matrix4        transform;
    Material       *material;
};

struct Physics {
    Array<Vector3> velocities;
    Array<i32>     entities;
};

struct Camera {
    Matrix4             view;
    Matrix4             projection;
    VulkanUniformBuffer ubo;

    Vector3             position;
    f32 yaw   = 0.0f;
    f32 pitch = 0.0f;
    Quaternion rotation = Quaternion::make({ 1.0f, 0.0f, 0.0f }, 2.2f * PI);
};

struct GameState {
    struct {
        Material terrain;
        Material font;
        Material heightmap;
        Material phong;
        Material player;
    } materials;

    DebugOverlay overlay;

    Camera fp_camera = {};

    Array<RenderObject> render_objects;
    Array<IndexRenderObject> index_render_objects;

    Vector3 velocity = {};

    i32 *key_state;
};

extern GameState    *g_game;

INTROSPECT struct Resolution
{
    i32 width  = 1280;
    i32 height = 720;
};

INTROSPECT struct VideoSettings
{
    Resolution resolution;

    i16 fullscreen = 0;
    i16 vsync      = 1;
};

INTROSPECT struct Settings
{
    VideoSettings video;
};

Entity entities_add(EntityData data);
i32 physics_add(Entity entity);

#endif /* LEARY_H */

