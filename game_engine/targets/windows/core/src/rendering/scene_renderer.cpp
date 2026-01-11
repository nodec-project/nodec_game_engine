#include <rendering/scene_renderer.hpp>

#include <DirectXMath.h>

#include <nodec/iterator.hpp>
#include <nodec/unicode.hpp>
#include <nodec_scene/components/local_to_world.hpp>

#include <Font/FontBackend.hpp>
#include <rendering/image_renderer_activity.hpp>
#include <rendering/mesh_renderer_activity.hpp>
#include <rendering/text_renderer_activity.hpp>

#include <map>
#include <utility>

namespace {

// Camera info for sorting and grouping
struct CameraInfo {
    nodec_scene::SceneEntity entity;
    const nodec_rendering::components::Camera *camera;
    const nodec_scene::components::LocalToWorld *local_to_world;
};

// =============================================================================
// ECS Layer-based Fold Expression Template
// Process entities with RenderLayer<N> for each layer matching culling_mask
// Uses C++17 fold expressions instead of recursive templates.
// Note: Entities without any RenderLayer component are NOT rendered.
// =============================================================================

// Process a single layer for given Activity/Component types
template<std::uint32_t N, typename Activity, typename Component, typename ProcessFn>
void process_renderer_layer(
    nodec_scene::SceneRegistry &registry,
    std::uint32_t culling_mask,
    ProcessFn &process_fn) {
    using namespace nodec_rendering::components;
    using namespace nodec_scene::components;
    using nodec_scene::SceneEntity;

    if (culling_mask & (1u << N)) {
        registry.view<Activity, const Component, const LocalToWorld, const RenderLayer<N>>(
                    nodec::type_list<NonVisible>{})
            .each([&](SceneEntity entity, Activity &activity,
                      const Component &component, const LocalToWorld &local_to_world,
                      const RenderLayer<N> &) {
                process_fn(entity, activity, component, local_to_world);
            });
    }
}

// Expand all layers using fold expression (processes from layer 31 down to 0)
template<typename Activity, typename Component, typename ProcessFn, std::size_t... Ns>
void process_renderers_expand(
    nodec_scene::SceneRegistry &registry,
    std::uint32_t culling_mask,
    ProcessFn &process_fn,
    std::index_sequence<Ns...>) {
    (process_renderer_layer<31 - Ns, Activity, Component>(registry, culling_mask, process_fn), ...);
}

// Main entry point - process all matching layers for given Activity/Component types
template<typename Activity, typename Component, typename ProcessFn>
void process_renderers_by_layer(
    nodec_scene::SceneRegistry &registry,
    std::uint32_t culling_mask,
    ProcessFn &&process_fn) {
    process_renderers_expand<Activity, Component>(
        registry, culling_mask, process_fn,
        std::make_index_sequence<32>{});
}

} // anonymous namespace

SceneRenderer::SceneRenderer(nodec_scene::Scene &scene,
                             Graphics &gfx,
                             nodec::resource_management::ResourceRegistry &resource_registry)
    : logger_(nodec::logging::get_logger("engine.scene-renderer")),
      scene_(scene),
      gfx_(gfx),
      renderer_context_(logger_, gfx, resource_registry)
// occlusion_system_(gfx) // オクルージョンシステムの初期化
{
}

void SceneRenderer::push_draw_command(std::shared_ptr<ShaderBackend> shader, bool is_transparent,
                                      const std::shared_ptr<MaterialBackend> &material_backend,
                                      DrawCommand *command,
                                      const DirectX::XMMATRIX &matrix_m, const DirectX::XMMATRIX &matrix_v_inverse) {
    using namespace DirectX;
    auto key = DrawGroupPriorityKey(shader, is_transparent);
    auto &group = draw_groups_[key];
    if (!group) {
        if (is_transparent) {
            group = std::make_unique<TransparentDrawGroup>();
        } else {
            group = std::make_unique<OpaqueDrawGroup>();
        }
        group->shader = shader;
    }
    if (is_transparent) {
        using namespace DirectX;
        auto *transparent_group = static_cast<TransparentDrawGroup *>(group.get());
        // Calculate the depth from the camera.
        auto model_position = matrix_m.r[3];
        auto camera_position = matrix_v_inverse.r[3];
        // auto depth = XMVectorGetX(XMVector3Length(model_position - camera_position));

        auto camera_direction = matrix_v_inverse.r[2]; // Assuming the camera looks along the -Z axis

        // Compute the vector from the camera to the object
        XMVECTOR camera_to_object = XMVectorSubtract(model_position, camera_position);

        // Project this vector onto the camera's view direction
        auto projected_length = XMVectorGetX(XMVector3Dot(camera_to_object, camera_direction)) / XMVectorGetX(XMVector3LengthSq(camera_direction));

        // The projected length is the distance from the camera to the object along the view direction
        auto depth = projected_length;

        transparent_group->draw_commands.emplace(depth, command);
    } else {
        auto *opaque_group = static_cast<OpaqueDrawGroup *>(group.get());
        opaque_group->append_draw_command(material_backend, command);

        // auto material_id = reinterpret_cast<std::intptr_t>(material_backend.get());
        // opaque_group->draw_commands.emplace(material_id, std::move(command));
    }
}
void SceneRenderer::setup_scene_lighting(nodec_scene::Scene &scene) {
    using namespace nodec_rendering::components;
    using namespace nodec_scene;
    using namespace nodec_scene::components;
    using namespace nodec;

    auto &cb_scene_properties = renderer_context_.cb_scene_properties();

    cb_scene_properties.data().lights.directional.enabled = 0x00;
    scene.registry().view<const nodec_rendering::components::DirectionalLight, const LocalToWorld>().each(
        [&](SceneEntity entt, const nodec_rendering::components::DirectionalLight &light, const LocalToWorld &local_to_world) {
            auto &directional = cb_scene_properties.data().lights.directional;
            directional.enabled = 0x01;
            directional.color = light.color;
            directional.intensity = light.intensity;

            auto direction = local_to_world.value * Vector4f{0.0f, 0.0f, 1.0f, 0.0f};
            directional.direction.set(direction.x, direction.y, direction.z);
        });

    scene.registry().view<const nodec_rendering::components::SceneLighting>().each(
        [&](SceneEntity entt, const nodec_rendering::components::SceneLighting &lighting) {
            cb_scene_properties.data().lights.ambient_color = lighting.ambient_color;
        });
}

void SceneRenderer::render(nodec_scene::Scene &scene,
                           ID3D11RenderTargetView &render_target, SceneRenderingContext &context) {
    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene::components;
    using namespace nodec_rendering;
    using namespace nodec_rendering::components;
    using namespace nodec_rendering::resources;
    using namespace DirectX;

    auto &scene_registry = scene.registry();

    renderer_context_.begin_render();

    setup_scene_lighting(scene);

    // --- Camera Grouping and Stacking ---
    // Group cameras by camera_group, then sort by priority within each group
    std::map<std::string, std::vector<CameraInfo>> camera_groups;

    scene.registry().view<const Camera, const LocalToWorld>().each(
        [&](SceneEntity camera_entity, const Camera &camera, const LocalToWorld &local_to_world) {
            camera_groups[camera.camera_group].push_back({camera_entity, &camera, &local_to_world});
        });

    // Process each camera group
    for (auto &[group_name, cameras] : camera_groups) {
        // Sort cameras by priority (lower values render first)
        std::sort(cameras.begin(), cameras.end(),
                  [](const CameraInfo &a, const CameraInfo &b) {
                      return a.camera->priority < b.camera->priority;
                  });

        // Separate Base and Overlay cameras
        std::vector<CameraInfo> base_cameras;
        std::vector<CameraInfo> overlay_cameras;

        for (const auto &cam_info : cameras) {
            if (cam_info.camera->render_type == Camera::RenderType::Base) {
                base_cameras.push_back(cam_info);
            } else {
                overlay_cameras.push_back(cam_info);
            }
        }

        // Edge case: No Base camera in group
        if (base_cameras.empty()) {
            logger_->warn(__FILE__, __LINE__) << "No Base camera in group: " << group_name << ". Skipping group.";
            continue;
        }

        // Edge case: Multiple Base cameras (use first one, warn)
        if (base_cameras.size() > 1) {
            logger_->warn(__FILE__, __LINE__) << "Multiple Base cameras in group: " << group_name
                                              << ". Using first one (priority: " << base_cameras.front().camera->priority << ").";
        }

        // Get the Base camera and ensure it has a rendering context
        const CameraInfo &base_cam_info = base_cameras.front();
        SceneEntity base_camera_entity = base_cam_info.entity;

        auto base_activity_result = scene_registry.emplace_component<CameraActivity>(base_camera_entity);
        auto &base_activity = base_activity_result.first;
        if (base_activity_result.second) {
            base_activity.state = std::make_unique<CameraState>();
        }

        // Create owned_rendering_context for BaseCamera if not exists
        if (!base_activity.owned_rendering_context) {
            base_activity.owned_rendering_context = std::make_unique<SceneRenderingContext>(
                context.target_width(), context.target_height(), gfx_);
        }
        base_activity.rendering_context = base_activity.owned_rendering_context.get();

        SceneRenderingContext &camera_context = *base_activity.rendering_context;

        // Lambda to render a single camera (Base or Overlay)
        auto render_camera = [&](const CameraInfo &cam_info, bool is_base_camera) {
            SceneEntity camera_entity = cam_info.entity;
            const Camera &camera = *cam_info.camera;
            const LocalToWorld &camera_local_to_world = *cam_info.local_to_world;

            // Use target_buffer as the primary render target
            ID3D11RenderTargetView *camera_render_target_view = &camera_context.target_buffer().render_target_view();

            // --- Get active post process effects. ---
            std::vector<const PostProcessing::Effect *> activePostProcessEffects;
            {
                const auto *postProcessing = scene.registry().try_get_component<const PostProcessing>(camera_entity);

                if (postProcessing) {
                    for (const auto &effect : postProcessing->effects) {
                        if (effect.enabled && effect.material && effect.material->shader()) {
                            activePostProcessEffects.push_back(&effect);
                        }
                    }
                }

                // If some effects, render scene to target_buffer_back (input for PostProcess)
                if (activePostProcessEffects.size() > 0) {
                    camera_render_target_view = &camera_context.target_buffer_back().render_target_view();
                }
            }

            // Get or create CameraActivity for this camera
            auto camera_activity_result = scene_registry.emplace_component<CameraActivity>(camera_entity);
            auto &camera_activity = camera_activity_result.first;
            if (camera_activity_result.second) {
                camera_activity.state = std::make_unique<CameraState>();
            }

            // Set rendering context (BaseCamera owns, OverlayCamera shares)
            if (!is_base_camera) {
                camera_activity.rendering_context = base_activity.rendering_context;
            }

            const auto aspect = static_cast<float>(camera_context.target_width()) / camera_context.target_height();
            camera_activity.state->update_projection(camera, aspect);
            camera_activity.state->update_transform(camera_local_to_world.value);

            // Write back computed matrices to Camera component for use by other systems (e.g., UI raycasting)
            {
                auto &camera_mut = scene_registry.get_component<Camera>(camera_entity);
                camera_mut.projection_matrix = camera_activity.state->get_projection_matrix();
                camera_mut.world2camera_matrix = camera_activity.state->get_view_matrix();
            }

            // Clear buffers based on camera type
            if (is_base_camera) {
                // BaseCamera: clear all (target_buffer, geometry buffers, depth stencil)
                camera_context.clear_all(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
            } else {
                // OverlayCamera: only clear geometry buffers (preserve target_buffer)
                camera_context.clear_geometry_buffers();

                // Clear depth buffer if clear_depth is true
                if (camera.clear_depth) {
                    camera_context.clear_depth_stencil();
                }
            }

            // Render scene (skybox only for Base camera)
            render_internal(scene, *camera_activity.state, camera.culling_mask,
                            camera_render_target_view, camera_context, is_base_camera);

            // --- Post Processing ---
            // Ping-pong pattern using target_buffer / target_buffer_back:
            // - Scene output is in target_buffer_back
            // - Each effect reads from target_buffer_back (via "screen" alias), writes to target_buffer
            // - After each effect (except last), swap buffers so next effect can read previous output
            if (activePostProcessEffects.size() > 0) {
                gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                renderer_context_.bs_default().bind();

                for (std::size_t i = 0; i < activePostProcessEffects.size(); ++i) {
                    // Always write to target_buffer
                    camera_render_target_view = &camera_context.target_buffer().render_target_view();

                    auto material_backend = std::static_pointer_cast<MaterialBackend>(activePostProcessEffects[i]->material);
                    auto shader_backend = std::static_pointer_cast<ShaderBackend>(material_backend->shader());

                    renderer_context_.bind_material(material_backend.get());
                    const UINT slot_offset = material_backend->texture_entries().size();

                    for (int passNum = 0; passNum < shader_backend->pass_count(); ++passNum) {
                        if (passNum == shader_backend->pass_count() - 1) {
                            // Last pass: render to target_buffer
                            D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f,
                                                                static_cast<FLOAT>(camera_context.target_width()),
                                                                static_cast<FLOAT>(camera_context.target_height()));
                            gfx_.context().RSSetViewports(1u, &vp);
                            gfx_.context().OMSetRenderTargets(1, &camera_render_target_view, nullptr);

                        } else {
                            // Intermediate pass: use multiple render targets (geometry buffers)
                            const auto &targets = shader_backend->render_targets(passNum);

                            std::vector<ID3D11RenderTargetView *> renderTargets(targets.size());
                            std::vector<D3D11_VIEWPORT> vps(targets.size());
                            for (size_t j = 0; j < targets.size(); ++j) {
                                auto &buffer = camera_context.geometry_buffer(targets[j]);
                                renderTargets[j] = &buffer.render_target_view();
                                vps[j] = CD3D11_VIEWPORT(0.f, 0.f,
                                                         static_cast<FLOAT>(buffer.width()),
                                                         static_cast<FLOAT>(buffer.height()));
                            }

                            gfx_.context().OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), nullptr);
                            gfx_.context().RSSetViewports(static_cast<UINT>(vps.size()), vps.data());
                        }

                        renderer_context_.sampler_state({Sampler::FilterMode::Bilinear, Sampler::WrapMode::Clamp}).BindPS(&gfx_, slot_offset);

                        // Bind shader resources - use shader_resource_view() to support $target_back and "screen" alias
                        const auto &texture_resources = shader_backend->texture_resources(passNum);
                        for (std::size_t j = 0; j < texture_resources.size(); ++j) {
                            auto *view = camera_context.shader_resource_view(texture_resources[j]);
                            if (!view) {
                                // Fallback to geometry_buffer for other named buffers
                                auto &buffer = camera_context.geometry_buffer(texture_resources[j]);
                                view = &buffer.shader_resource_view();
                            }
                            gfx_.context().PSSetShaderResources(slot_offset + j, 1u, &view);
                        }
                        shader_backend->bind(passNum);

                        auto &screen_quad_mesh = renderer_context_.screen_quad_mesh();

                        screen_quad_mesh.bind(&gfx_);
                        gfx_.DrawIndexed(screen_quad_mesh.triangles.size());
                        renderer_context_.unbind_all_shader_resources(slot_offset, static_cast<UINT>(texture_resources.size()));
                    } // End foreach pass.
                    renderer_context_.unbind_all_shader_resources(slot_offset);

                    // Swap buffers for next effect (not after last effect)
                    if (i != activePostProcessEffects.size() - 1) {
                        camera_context.swap_target_buffers();
                    }
                } // End foreach effect.
            }
        };

        // Render Base camera (first one only)
        render_camera(base_cam_info, true);

        // Render Overlay cameras
        for (const auto &cam_info : overlay_cameras) {
            render_camera(cam_info, false);
        }

        // Final composition: copy target_buffer to render_target
        compose_to_render_target(camera_context, render_target);

    } // End foreach camera group
}

void SceneRenderer::render(nodec_scene::Scene &scene, const CameraState &camera_state, ID3D11RenderTargetView *render_target, SceneRenderingContext &context) {
    assert(render_target != nullptr);

    using namespace DirectX;
    using namespace nodec;

    renderer_context_.begin_render();

    setup_scene_lighting(scene);

    // Clear all buffers (this API is for direct rendering without camera stack)
    context.clear_all(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

    // Clear the external render target as well
    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    gfx_.context().ClearRenderTargetView(render_target, clear_color);

    // Use default culling mask (all layers) when rendering without Camera component
    // render_skybox = true for standalone rendering
    render_internal(scene, camera_state, 0xFFFFFFFF,
                    render_target, context, true);
}

float calculate_screen_pixel_size(const DirectX::XMMATRIX &world_view_projection, const nodec::gfx::BoundingBox &bounds,
                                  float screen_width, float screen_height) {
    using namespace DirectX;
    using namespace nodec;

    auto max_point = (bounds.max)();
    auto min_point = (bounds.min)();

    // バウンディングボックスの8頂点を取得
    XMVECTOR corners[8];
    corners[0] = XMVectorSet(min_point.x, min_point.y, min_point.z, 1.0f);
    corners[1] = XMVectorSet(max_point.x, min_point.y, min_point.z, 1.0f);
    corners[2] = XMVectorSet(min_point.x, max_point.y, min_point.z, 1.0f);
    corners[3] = XMVectorSet(max_point.x, max_point.y, min_point.z, 1.0f);
    corners[4] = XMVectorSet(min_point.x, min_point.y, max_point.z, 1.0f);
    corners[5] = XMVectorSet(max_point.x, min_point.y, max_point.z, 1.0f);
    corners[6] = XMVectorSet(min_point.x, max_point.y, max_point.z, 1.0f);
    corners[7] = XMVectorSet(max_point.x, max_point.y, max_point.z, 1.0f);

    // スクリーン空間に変換して最小/最大座標を求める
    float min_x = FLT_MAX, min_y = FLT_MAX;
    float max_x = -FLT_MAX, max_y = -FLT_MAX;

    for (int i = 0; i < 8; ++i) {
        // ワールド座標からNDC空間へ変換
        XMVECTOR projected = XMVector4Transform(corners[i], world_view_projection);

        // 透視除算（w除算）を行いNDC座標に変換
        XMVECTOR ndc = projected;
        float w = XMVectorGetW(projected);

        // If w is close to 0 (points on camera's view plane or numerically unstable points), skip processing
        if ((w > 0.f ? w : -w) < 0.0001f) continue;

        ndc = XMVectorScale(ndc, 1.0f / w);

        // NDC空間の座標を取得
        float x = XMVectorGetX(ndc);
        float y = XMVectorGetY(ndc);

        // 最小/最大座標を更新
        min_x = (std::min)(min_x, x);
        min_y = (std::min)(min_y, y);
        max_x = (std::max)(max_x, x);
        max_y = (std::max)(max_y, y);
    }

    // バウンディングボックスがスクリーン外の場合
    if (min_x > max_x || min_y > max_y) {
        return 0.0f;
    }

    // NDC空間からスクリーンピクセルサイズに変換

    float screen_width_pixels = (max_x - min_x) * 0.5f * screen_width;
    float screen_height_pixels = (max_y - min_y) * 0.5f * screen_height;

    // 幅と高さの大きい方を返す
    return (std::max)(screen_width_pixels, screen_height_pixels);
}

void SceneRenderer::render_internal(nodec_scene::Scene &scene,
                                    const CameraState &camera_state,
                                    std::uint32_t culling_mask,
                                    ID3D11RenderTargetView *render_target, SceneRenderingContext &context,
                                    bool render_skybox) {
    assert(render_target != nullptr);
    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene::components;
    using namespace nodec_rendering::components;
    using namespace nodec_rendering;
    using namespace DirectX;

    auto &scene_registry = scene.registry();

    // オクルージョンテストを開始
    // auto view_proj = camera_state.matrix_v() * camera_state.matrix_p();
    // occlusion_system_.begin_occlusion_test(view_proj);

    DirectX::XMMATRIX matrix_vp = camera_state.matrix_v() * camera_state.matrix_p();

    // Group the draw-command by the shader.
    // Using ECS layer-based view iteration for efficient culling

    // --- MeshRenderer ---
    {
        // Ensure MeshRendererActivity exists for all MeshRenderer entities
        scene_registry.view<MeshRenderer>(type_list<MeshRendererActivity>{})
            .each([&](SceneEntity entity, MeshRenderer &) {
                scene_registry.emplace_component<MeshRendererActivity>(entity);
            });
        // Remove orphaned activities
        {
            auto view = scene_registry.view<MeshRendererActivity>(type_list<MeshRenderer>{});
            scene_registry.remove_components<MeshRendererActivity>(view.begin(), view.end());
        }

        // ECS-optimized: Iterate only entities matching culling_mask layers
        process_renderers_by_layer<MeshRendererActivity, MeshRenderer>(
            scene_registry, culling_mask,
            [&](SceneEntity entity, MeshRendererActivity &activity,
                const MeshRenderer &renderer, const LocalToWorld &local_to_world) {
                auto commands = activity.get_commands_if_needed(camera_state, entity, renderer, local_to_world);
                for (auto *command : commands) {
                    auto is_transparent = command->material_->is_transparent();
                    push_draw_command(
                        std::static_pointer_cast<ShaderBackend>(command->material_->shader()),
                        is_transparent,
                        command->material_,
                        command,
                        command->matrix_m_,
                        camera_state.matrix_v_inverse());
                }
            });
    }

    // --- ImageRenderer ---
    {
        scene_registry.view<ImageRenderer>(type_list<ImageRendererActivity>{})
            .each([&](SceneEntity entity, ImageRenderer &) {
                scene_registry.emplace_component<ImageRendererActivity>(entity);
            });
        {
            auto view = scene_registry.view<ImageRendererActivity>(type_list<ImageRenderer>{});
            scene_registry.remove_components<ImageRendererActivity>(view.begin(), view.end());
        }

        // ECS-optimized: Iterate only entities matching culling_mask layers
        process_renderers_by_layer<ImageRendererActivity, ImageRenderer>(
            scene_registry, culling_mask,
            [&](SceneEntity entity, ImageRendererActivity &activity,
                const ImageRenderer &renderer, const LocalToWorld &local_to_world) {
                auto command = activity.get_commands_if_needed(camera_state, entity, renderer, local_to_world);
                if (!command) return;

                const bool is_transparent = command->material_->is_transparent();
                push_draw_command(
                    std::static_pointer_cast<ShaderBackend>(command->material_->shader()),
                    is_transparent,
                    command->material_,
                    command,
                    command->matrix_m_,
                    camera_state.matrix_v_inverse());
            });
    }

    // --- TextRenderer ---
    {
        scene_registry.view<TextRenderer>(type_list<TextRendererActivity>{})
            .each([&](SceneEntity entity, TextRenderer &) {
                scene_registry.emplace_component<TextRendererActivity>(entity);
            });
        {
            auto view = scene_registry.view<TextRendererActivity>(type_list<TextRenderer>{});
            scene_registry.remove_components<TextRendererActivity>(view.begin(), view.end());
        }

        // ECS-optimized: Iterate only entities matching culling_mask layers
        process_renderers_by_layer<TextRendererActivity, TextRenderer>(
            scene_registry, culling_mask,
            [&](SceneEntity entity, TextRendererActivity &activity,
                const TextRenderer &renderer, const LocalToWorld &local_to_world) {
                auto command = activity.get_command_if_needed(camera_state, entity, renderer, local_to_world);
                if (!command) return;

                const bool is_transparent = command->material_->is_transparent();
                push_draw_command(command->shader_,
                                  is_transparent,
                                  command->material_,
                                  std::move(command),
                                  command->matrix_m_,
                                  camera_state.matrix_v_inverse());
            });
    }

    // フレーム終了時の処理
    // occlusion_system_.end_frame();

    // Note: Geometry buffer clearing is now handled by the caller (clear_all or clear_geometry_buffers)

    auto &cb_scene_properties = renderer_context_.cb_scene_properties();

    XMStoreFloat4x4(&cb_scene_properties.data().matrix_p, camera_state.matrix_p());
    XMStoreFloat4x4(&cb_scene_properties.data().matrix_p_inverse, camera_state.matrix_p_inverse());

    XMStoreFloat4x4(&cb_scene_properties.data().matrix_v, camera_state.matrix_v());
    XMStoreFloat4x4(&cb_scene_properties.data().matrix_v_inverse, camera_state.matrix_v_inverse());

    XMVECTOR scale, rotQuat, trans;
    XMMatrixDecompose(&scale, &rotQuat, &trans, camera_state.matrix_v_inverse());

    cb_scene_properties.data().camera_position.set(
        XMVectorGetByIndex(trans, 0),
        XMVectorGetByIndex(trans, 1),
        XMVectorGetByIndex(trans, 2),
        XMVectorGetByIndex(trans, 3));

    // Update active point lights.
    {
        auto &lights = cb_scene_properties.data().lights;
        lights.num_of_point_lights = 0;
        auto view = scene.registry().view<const LocalToWorld, const nodec_rendering::components::PointLight>();
        for (const auto &entt : view) {
            // TODO: Light culling.
            const auto index = lights.num_of_point_lights;
            if (index >= nodec::size(lights.point_lights)) break;

            const auto &local_to_world = view.get<const LocalToWorld>(entt);
            const auto &light = view.get<const nodec_rendering::components::PointLight>(entt);
            const auto worldPosition = local_to_world.value * Vector4f(0, 0, 0, 1.0f);

            lights.point_lights[index].position.set(worldPosition.x, worldPosition.y, worldPosition.z);
            lights.point_lights[index].color.set(light.color.x, light.color.y, light.color.z);
            lights.point_lights[index].intensity = light.intensity;
            lights.point_lights[index].range = light.range;

            ++lights.num_of_point_lights;
        }
    }

    cb_scene_properties.apply();

    // Note: Depth buffer clearing is now handled by the caller based on camera.clear_depth

    // Render skybox (only for Base camera)
    if (render_skybox) {
        [&]() {
            auto &norm_cube_mesh = renderer_context_.norm_cube_mesh();

            auto view = scene.registry().view<nodec_rendering::components::SceneLighting>();
            if (view.begin() == view.end()) return;

            auto entt = *view.begin();
            const auto &lighting = view.get<nodec_rendering::components::SceneLighting>(entt);

            auto material_backend = static_cast<MaterialBackend *>(lighting.skybox.get());
            if (!material_backend) return;

            auto shader_backend = static_cast<ShaderBackend *>(material_backend->shader().get());
            if (!shader_backend) return;

            renderer_context_.bs_default().bind();

            gfx_.context().OMSetRenderTargets(1, &render_target, nullptr);
            D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, static_cast<FLOAT>(context.target_width()), static_cast<FLOAT>(context.target_height()));
            gfx_.context().RSSetViewports(1u, &vp);
            gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            shader_backend->bind();

            renderer_context_.bind_material(material_backend);

            norm_cube_mesh.bind(&gfx_);
            gfx_.DrawIndexed(norm_cube_mesh.triangles.size());

            // // --- Render environment map.
            // {
            //     XMStoreFloat4x4(&cb_scene_properties.data().matrix_p, XMMatrixPerspectiveFovLH(XM_PI, 1.0f, 0.1f, 100.0f));
            //     // XMStoreFloat4x4(&cb_scene_properties.data().matrix_p, XMMatrixIdentity());
            //     XMStoreFloat4x4(&cb_scene_properties.data().matrix_p_inverse, XMMatrixIdentity());
            //     XMStoreFloat4x4(&cb_scene_properties.data().matrix_v, XMMatrixIdentity());
            //     XMStoreFloat4x4(&cb_scene_properties.data().matrix_v_inverse, XMMatrixIdentity());
            //     cb_scene_properties.apply();

            //     auto &environment = context.geometry_buffer("environment");
            //     auto *target = &environment.render_target_view();
            //     gfx_.context().OMSetRenderTargets(1, &target, nullptr);

            //     gfx_.DrawIndexed(norm_cube_mesh.triangles.size());

            //     XMStoreFloat4x4(&cb_scene_properties.data().matrix_p, matrix_p);
            //     XMStoreFloat4x4(&cb_scene_properties.data().matrix_p_inverse, matrix_p_inverse);
            //     XMStoreFloat4x4(&cb_scene_properties.data().matrix_v, matrix_v);
            //     XMStoreFloat4x4(&cb_scene_properties.data().matrix_v_inverse, matrix_v_inverse);

            //     cb_scene_properties.apply();
            // }
        }();
    } // End if (render_skybox)

    renderer_context_.cb_model_properties().buffer().bind(SceneRenderingConstants::MODEL_PROPERTIES_CB_SLOT);

    for (auto iter = draw_groups_.begin(); iter != draw_groups_.end();) {
        auto &draw_group = iter->second;
        auto shader = draw_group->shader.lock();
        if (!shader) {
            iter = draw_groups_.erase(iter);
            continue;
        }

        gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (int pass_num = 0; pass_num < shader->pass_count(); ++pass_num) {
            // --- Set render target and view port --- //
            auto &render_targets_name = shader->render_targets(pass_num);

            std::vector<ID3D11RenderTargetView *> render_targets;
            std::vector<D3D11_VIEWPORT> vps;

            if (render_targets_name.size() == 0) {
                render_targets.resize(1);
                vps.resize(1);
                render_targets[0] = render_target;
                vps[0] = CD3D11_VIEWPORT(0.f, 0.f, static_cast<FLOAT>(context.target_width()), static_cast<FLOAT>(context.target_height()));
            } else {
                render_targets.resize(render_targets_name.size());
                vps.resize(render_targets_name.size());
                for (int i = 0; i < render_targets_name.size(); ++i) {
                    auto &name = render_targets_name[i];
                    if (name == "$target") {
                        render_targets[i] = render_target;
                        vps[i] = CD3D11_VIEWPORT(0.f, 0.f, static_cast<FLOAT>(context.target_width()), static_cast<FLOAT>(context.target_height()));
                        continue;
                    }
                    auto &buffer = context.geometry_buffer(name);
                    render_targets[i] = &buffer.render_target_view();
                    // gfx_.context().ClearRenderTargetView(render_targets[i], Vector4f::zero.v);
                    vps[i] = CD3D11_VIEWPORT(0.f, 0.f, static_cast<FLOAT>(buffer.width()), static_cast<FLOAT>(buffer.height()));
                }
            }

            ID3D11DepthStencilView *dsv = pass_num == 0 ? &context.depth_stencil_view() : nullptr;

            gfx_.context().OMSetRenderTargets(static_cast<UINT>(render_targets.size()), render_targets.data(), dsv);
            gfx_.context().RSSetViewports(static_cast<UINT>(vps.size()), vps.data());

            // END Set render target and view port --- //

            const auto &texture_resources_name = shader->texture_resources(pass_num);
            for (int i = 0; i < texture_resources_name.size(); ++i) {
                auto &buffer = context.geometry_buffer(texture_resources_name[i]);
                auto *view = &buffer.shader_resource_view();
                gfx_.context().PSSetShaderResources(i, 1u, &view);
            }

            shader->bind(pass_num);

            if (pass_num == 0) {
                draw_group->draw_all(camera_state.matrix_v(), camera_state.matrix_p(), renderer_context_, gfx_);
            } else {
                auto &screen_quad_mesh = renderer_context_.screen_quad_mesh();
                screen_quad_mesh.bind(&gfx_);
                gfx_.DrawIndexed(static_cast<UINT>(screen_quad_mesh.triangles.size()));
            }
            renderer_context_.unbind_all_shader_resources(static_cast<UINT>(texture_resources_name.size()));
        }

        draw_group->clear_draw_commands();

        ++iter;
    }
}

void SceneRenderer::compose_to_render_target(SceneRenderingContext &context, ID3D11RenderTargetView &render_target) {
    using namespace nodec;
    using namespace nodec_rendering;
    using namespace DirectX;

    // Set render target to the final output
    ID3D11RenderTargetView *rtv = &render_target;
    gfx_.context().OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f,
                                        static_cast<FLOAT>(context.target_width()),
                                        static_cast<FLOAT>(context.target_height()));
    gfx_.context().RSSetViewports(1u, &vp);

    // Bind the target_buffer as a shader resource
    auto *srv = &context.target_buffer().shader_resource_view();
    gfx_.context().PSSetShaderResources(0, 1, &srv);

    // Use copy shader to copy target_buffer to render_target
    renderer_context_.bind_copy_shader();

    // Set sampler
    renderer_context_.sampler_state({Sampler::FilterMode::Bilinear, Sampler::WrapMode::Clamp}).BindPS(&gfx_, 0);

    // Set default blend state
    renderer_context_.bs_default().bind();

    // Set topology and render fullscreen quad
    gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    auto &screen_quad_mesh = renderer_context_.screen_quad_mesh();
    screen_quad_mesh.bind(&gfx_);
    gfx_.DrawIndexed(static_cast<UINT>(screen_quad_mesh.triangles.size()));

    // Unbind SRV to avoid resource hazard
    ID3D11ShaderResourceView *null_srv = nullptr;
    gfx_.context().PSSetShaderResources(0, 1, &null_srv);
}
