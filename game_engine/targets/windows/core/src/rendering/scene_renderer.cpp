
#include <rendering/scene_renderer.hpp>

#include <DirectXMath.h>

#include <nodec/iterator.hpp>
#include <nodec/unicode.hpp>
#include <nodec_scene/components/local_to_world.hpp>

#include <Font/FontBackend.hpp>

class MeshDrawCommand : public DrawCommand {
public:
    MeshDrawCommand(const DirectX::XMMATRIX &matrix_m,
                    std::shared_ptr<MeshBackend> mesh,
                    std::shared_ptr<MaterialBackend> material)
        : matrix_m(matrix_m), mesh(mesh), material(material) {}

    void draw(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
              SceneRendererContext &renderer_context, Graphics &gfx) override {
        using namespace DirectX;
        renderer_context.bs_default().bind(&gfx);
        auto matrix_m_inverse = DirectX::XMMatrixInverse(nullptr, matrix_m);
        auto matrix_mvp = matrix_m * matrix_v * matrix_p;

        auto &cb_model_properties = renderer_context.cb_model_properties();

        XMStoreFloat4x4(&cb_model_properties.data().matrix_m, matrix_m);
        XMStoreFloat4x4(&cb_model_properties.data().matrix_m_inverse, matrix_m_inverse);
        XMStoreFloat4x4(&cb_model_properties.data().matrix_mvp, matrix_mvp);

        cb_model_properties.apply();
        material->bind_constant_buffer(&gfx, SceneRenderingConstants::MATERIAL_PROPERTIES_CB_SLOT);
        renderer_context.set_cull_mode(material->cull_mode());

        auto &cb_texture_config = renderer_context.cb_texture_config();
        cb_texture_config.data().tex_has_flag = 0;
        auto tex_slot = renderer_context.bind_texture_entries(material->texture_entries(), cb_texture_config.data().tex_has_flag);
        cb_texture_config.apply();

        mesh->bind(&gfx);
        gfx.DrawIndexed(static_cast<UINT>(mesh->triangles.size()));
    }

    DirectX::XMMATRIX matrix_m;
    std::shared_ptr<MeshBackend> mesh;
    std::shared_ptr<MaterialBackend> material;
};

class ImageDrawCommand : public DrawCommand {
public:
    ImageDrawCommand(const DirectX::XMMATRIX &matrix_m,
                     std::shared_ptr<TextureBackend> image,
                     std::shared_ptr<MaterialBackend> material,
                     const nodec::Vector4f &color)
        : matrix_m(matrix_m), image(image), material(material), color(color) {}

    void draw(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
              SceneRendererContext &renderer_context, Graphics &gfx) override {
        using namespace DirectX;
        renderer_context.bs_alpha_blend().bind(&gfx);

        auto matrix_m_inverse = DirectX::XMMatrixInverse(nullptr, matrix_m);
        auto matrix_mvp = matrix_m * matrix_v * matrix_p;

        auto &cb_model_properties = renderer_context.cb_model_properties();

        XMStoreFloat4x4(&cb_model_properties.data().matrix_m, matrix_m);
        XMStoreFloat4x4(&cb_model_properties.data().matrix_m_inverse, matrix_m_inverse);
        XMStoreFloat4x4(&cb_model_properties.data().matrix_mvp, matrix_mvp);

        cb_model_properties.apply();
        renderer_context.set_cull_mode(material->cull_mode());

        auto backup_image = material->get_texture_entry("image");
        auto backup_color = material->get_vector4_property("color");

        material->set_texture_entry(
            "image", {image, {nodec_rendering::Sampler::FilterMode::Bilinear, nodec_rendering::Sampler::WrapMode::Clamp}});
        material->set_vector4_property("color", color);
        material->bind_constant_buffer(&gfx, SceneRenderingConstants::MATERIAL_PROPERTIES_CB_SLOT);

        auto &cb_texture_config = renderer_context.cb_texture_config();
        cb_texture_config.data().tex_has_flag = 0;
        auto tex_slot = renderer_context.bind_texture_entries(material->texture_entries(), cb_texture_config.data().tex_has_flag);
        cb_texture_config.apply();

        auto &mesh = renderer_context.quad_mesh();
        mesh.bind(&gfx);
        gfx.DrawIndexed(static_cast<UINT>(mesh.triangles.size()));

        if (backup_image) {
            material->set_texture_entry("image", *backup_image);
        }
        if (backup_color) {
            material->set_vector4_property("color", *backup_color);
        }
    }

    DirectX::XMMATRIX matrix_m;
    std::shared_ptr<TextureBackend> image;
    std::shared_ptr<MaterialBackend> material;
    nodec::Vector4f color;
};

class TextDrawCommand : public DrawCommand {
public:
    TextDrawCommand(const DirectX::XMMATRIX &matrix_m,
                    const nodec_rendering::components::TextRenderer &text_renderer)
        : matrix_m(matrix_m), text_renderer(text_renderer) {}

    void draw(const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_p,
              SceneRendererContext &renderer_context, Graphics &gfx) override {
        using namespace DirectX;
        renderer_context.bs_alpha_blend().bind(&gfx);

        auto material = static_cast<MaterialBackend *>(text_renderer.material.get());
        assert(material);

        auto font = static_cast<FontBackend *>(text_renderer.font.get());
        assert(font);

        auto backup_mask = material->get_texture_entry("mask");
        auto backup_color = material->get_vector4_property("color");

        const auto u32_text = nodec::unicode::utf8to32<std::u32string>(text_renderer.text);
        const float pixels_per_unit = static_cast<float>(text_renderer.pixels_per_unit);
        const float pixel_size = text_renderer.pixel_size;

        auto &font_character_database = renderer_context.font_character_database();

        float offset_x = 0.0f;
        float offset_y = 0.0f;

        for (const auto &chCode : u32_text) {
            if (chCode == '\n') {
                offset_y -= pixel_size / pixels_per_unit;
                offset_x = 0.0f;
                continue;
            }

            const auto &character = font_character_database.Get(font->GetFace(), pixel_size, chCode);

            float pos_x = offset_x + character.bearing.x / pixels_per_unit;
            float pos_y = offset_y - (character.size.y - character.bearing.y) / pixels_per_unit;
            float w = character.size.x / pixels_per_unit;
            float h = character.size.y / pixels_per_unit;

            offset_x += (character.advance >> 6) / pixels_per_unit;

            if (!character.pFontTexture) {
                continue;
            }

            material->set_texture_entry("mask",
                                        {character.pFontTexture, {nodec_rendering::Sampler::FilterMode::Bilinear, nodec_rendering::Sampler::WrapMode::Clamp}});
            material->set_vector4_property("color", text_renderer.color);
            material->bind_constant_buffer(&gfx, SceneRenderingConstants::MATERIAL_PROPERTIES_CB_SLOT);

            renderer_context.set_cull_mode(material->cull_mode());

            auto &cb_texture_config = renderer_context.cb_texture_config();
            cb_texture_config.data().tex_has_flag = 0x00;
            auto tex_slot = renderer_context.bind_texture_entries(material->texture_entries(), cb_texture_config.data().tex_has_flag);
            cb_texture_config.apply();

            XMMATRIX matrix_m_ch{matrix_m};
            matrix_m_ch = XMMatrixScaling(w / 2, h / 2, 1.0f) * XMMatrixTranslation(pos_x + w / 2, pos_y + h / 2, 0.0f) * matrix_m_ch;

            // matrixM
            auto matrix_m_inverse = XMMatrixInverse(nullptr, matrix_m_ch);

            // DirectX Math using row-major representation
            // HLSL using column-major representation
            auto matrix_mvp = matrix_m_ch * matrix_v * matrix_p;

            auto &cb_model_properties = renderer_context.cb_model_properties();

            XMStoreFloat4x4(&cb_model_properties.data().matrix_m, matrix_m_ch);
            XMStoreFloat4x4(&cb_model_properties.data().matrix_m_inverse, matrix_m_inverse);
            XMStoreFloat4x4(&cb_model_properties.data().matrix_mvp, matrix_mvp);

            cb_model_properties.apply();

            auto &mesh = renderer_context.screen_quad_mesh();
            mesh.bind(&gfx);
            gfx.DrawIndexed(static_cast<UINT>(mesh.triangles.size()));
        } // End foreach character.
        if (backup_mask) {
            material->set_texture_entry("mask", *backup_mask);
        }

        if (backup_color) {
            material->set_vector4_property("color", *backup_color);
        }
    }

    DirectX::XMMATRIX matrix_m;
    const nodec_rendering::components::TextRenderer &text_renderer;
};

SceneRenderer::SceneRenderer(nodec_scene::Scene &scene,
                             Graphics &gfx,
                             nodec::resource_management::ResourceRegistry &resource_registry)
    : logger_(nodec::logging::get_logger("engine.scene-renderer")),
      scene_(scene),
      gfx_(gfx),
      renderer_context_(logger_, gfx, resource_registry) {
}

void SceneRenderer::push_draw_command(std::shared_ptr<ShaderBackend> shader, bool is_transparent, std::unique_ptr<DrawCommand> command,
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
        auto *transparent_group = static_cast<TransparentDrawGroup *>(group.get());
        // Calculate the depth from the camera.
        auto model_position = matrix_m.r[3];
        auto camera_position = matrix_v_inverse.r[3];
        // auto depth = XMVectorGetX(XMVector3Length(model_position - camera_position));

        auto camera_direction = matrix_v_inverse.r[2]; // Assuming the camera looks along the -Z axis

        // Compute the vector from the camera to the object
        auto camera_to_object = model_position - camera_position;

        // Project this vector onto the camera's view direction
        auto projected_length = XMVectorGetX(XMVector3Dot(camera_to_object, camera_direction)) / XMVectorGetX(XMVector3LengthSq(camera_direction));

        // The projected length is the distance from the camera to the object along the view direction
        auto depth = projected_length;

        transparent_group->draw_commands.emplace(
            depth, std::move(command));
    } else {
        auto *opaque_group = static_cast<OpaqueDrawGroup *>(group.get());
        opaque_group->draw_commands.push_back(std::move(command));
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

    renderer_context_.cb_scene_properties().buffer().bind(SceneRenderingConstants::SCENE_PROPERTIES_CB_SLOT);
    renderer_context_.cb_texture_config().buffer().bind(SceneRenderingConstants::TEXTURE_CONFIG_CB_SLOT);

    setup_scene_lighting(scene);

    // Render the scene per each camera.
    scene.registry().view<const Camera, const LocalToWorld>().each([&](SceneEntity camera_entity, const Camera &camera, const LocalToWorld &camera_local_to_world) {
        ID3D11RenderTargetView *pCameraRenderTargetView = &render_target;

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

            // If some effects, the off-screen buffer is needed.
            if (activePostProcessEffects.size() > 0) {
                auto &buffer = context.geometry_buffer("screen");

                // Set the render target.
                pCameraRenderTargetView = &buffer.render_target_view();
            }
        }

        auto matrix_p = XMMatrixIdentity();

        const auto aspect = static_cast<float>(context.target_width()) / context.target_height();
        switch (camera.projection) {
        case Camera::Projection::Perspective:
            matrix_p = XMMatrixPerspectiveFovLH(
                XMConvertToRadians(camera.fov_angle),
                aspect,
                camera.near_clip_plane, camera.far_clip_plane);
            break;
        case Camera::Projection::Orthographic:
            matrix_p = XMMatrixOrthographicLH(
                camera.ortho_width, camera.ortho_width / aspect,
                camera.near_clip_plane, camera.far_clip_plane);
            break;
        default:
            break;
        }
        const auto matrix_p_inverse = XMMatrixInverse(nullptr, matrix_p);

        XMMATRIX matrix_v_inverse{camera_local_to_world.value.m};

        auto matrix_v = XMMatrixInverse(nullptr, matrix_v_inverse);

        render(scene, matrix_v, matrix_v_inverse, matrix_p, matrix_p_inverse,
               pCameraRenderTargetView, context);

        // --- Post Processing ---
        if (activePostProcessEffects.size() > 0) {
            gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            renderer_context_.bs_default().bind(&gfx_);

            for (std::size_t i = 0; i < activePostProcessEffects.size(); ++i) {
                if (i != activePostProcessEffects.size() - 1) {
                    pCameraRenderTargetView = &context.geometry_buffer("screen_back").render_target_view();
                } else {
                    // if last, render target is frame buffer.
                    pCameraRenderTargetView = &render_target;
                }

                // It is assured that material and shader are exists.
                // It is checked at the beginning of rendering pass of camera.
                auto materialBackend = std::static_pointer_cast<MaterialBackend>(activePostProcessEffects[i]->material);
                auto shaderBackend = std::static_pointer_cast<ShaderBackend>(materialBackend->shader());

                renderer_context_.set_cull_mode(materialBackend->cull_mode());
                materialBackend->bind_constant_buffer(&gfx_, SceneRenderingConstants::MATERIAL_PROPERTIES_CB_SLOT);

                auto &cb_texture_config = renderer_context_.cb_texture_config();
                cb_texture_config.data().tex_has_flag = 0x00;
                const auto slot_offset = renderer_context_.bind_texture_entries(materialBackend->texture_entries(), cb_texture_config.data().tex_has_flag);
                cb_texture_config.apply();

                for (int passNum = 0; passNum < shaderBackend->pass_count(); ++passNum) {
                    if (passNum == shaderBackend->pass_count() - 1) {
                        // If last pass.
                        // The render target must be one final target.
                        D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, context.target_width(), context.target_height());
                        gfx_.context().RSSetViewports(1u, &vp);
                        gfx_.context().OMSetRenderTargets(1, &pCameraRenderTargetView, nullptr);

                    } else {
                        // If halfway pass.
                        // Support the multiple render targets.

                        const auto &targets = shaderBackend->render_targets(passNum);

                        std::vector<ID3D11RenderTargetView *> renderTargets(targets.size());
                        std::vector<D3D11_VIEWPORT> vps(targets.size());
                        for (size_t i = 0; i < targets.size(); ++i) {
                            auto &buffer = context.geometry_buffer(targets[i]);
                            renderTargets[i] = &buffer.render_target_view();
                            gfx_.context().ClearRenderTargetView(renderTargets[i], Vector4f::zero.v);

                            vps[i] = CD3D11_VIEWPORT(0.f, 0.f, static_cast<FLOAT>(buffer.width()), static_cast<FLOAT>(buffer.height()));
                        }

                        gfx_.context().OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), nullptr);
                        gfx_.context().RSSetViewports(static_cast<UINT>(vps.size()), vps.data());
                    }

                    // --- Bind texture resources.
                    // Bind sampler for textures.

                    renderer_context_.sampler_state({Sampler::FilterMode::Bilinear, Sampler::WrapMode::Clamp}).BindPS(&gfx_, slot_offset);
                    const auto &texture_resources = shaderBackend->texture_resources(passNum);
                    for (std::size_t i = 0; i < texture_resources.size(); ++i) {
                        auto &buffer = context.geometry_buffer(texture_resources[i]);
                        auto *view = &buffer.shader_resource_view();
                        gfx_.context().PSSetShaderResources(slot_offset + i, 1u, &view);
                    }
                    shaderBackend->bind(passNum);

                    auto &screen_quad_mesh = renderer_context_.screen_quad_mesh();

                    screen_quad_mesh.bind(&gfx_);
                    gfx_.DrawIndexed(screen_quad_mesh.triangles.size());
                    renderer_context_.unbind_all_shader_resources(slot_offset, static_cast<UINT>(texture_resources.size()));
                } // End foreach pass.
                renderer_context_.unbind_all_shader_resources(slot_offset);
                context.swap_geometry_buffers("screen", "screen_back");
            } // End foreach effect.
        }
    }); // End foreach camera
}

void SceneRenderer::render(nodec_scene::Scene &scene, nodec::Matrix4x4f &view, nodec::Matrix4x4f &projection, ID3D11RenderTargetView *render_target, SceneRenderingContext &context) {
    assert(render_target != nullptr);

    using namespace DirectX;
    using namespace nodec;

    renderer_context_.cb_scene_properties().buffer().bind(SceneRenderingConstants::SCENE_PROPERTIES_CB_SLOT);
    renderer_context_.cb_texture_config().buffer().bind(SceneRenderingConstants::TEXTURE_CONFIG_CB_SLOT);

    setup_scene_lighting(scene);

    XMMATRIX matrix_v{view.m};
    auto matrix_v_inverse = XMMatrixInverse(nullptr, matrix_v);

    XMMATRIX matrix_p{projection.m};
    auto matrix_p_inverse = XMMatrixInverse(nullptr, matrix_p);

    render(scene, matrix_v, matrix_v_inverse, matrix_p, matrix_p_inverse,
           render_target, context);
}

void SceneRenderer::render(nodec_scene::Scene &scene,
                           const DirectX::XMMATRIX &matrix_v, const DirectX::XMMATRIX &matrix_v_inverse,
                           const DirectX::XMMATRIX &matrix_p, const DirectX::XMMATRIX &matrix_p_inverse,
                           ID3D11RenderTargetView *render_target, SceneRenderingContext &context) {
    assert(render_target != nullptr);
    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene::components;
    using namespace nodec_rendering::components;
    using namespace nodec_rendering;
    using namespace DirectX;

    // TODO: Culling.

    // Group the draw-command by the shader.
    {
        scene.registry().view<const MeshRenderer, const LocalToWorld>(type_list<NonVisible>{}).each([&](SceneEntity entity, const MeshRenderer &renderer, const LocalToWorld &local_to_world) {
            if (renderer.meshes.size() != renderer.materials.size()) return;
            for (int i = 0; i < renderer.meshes.size(); ++i) {
                auto &mesh = renderer.meshes[i];
                auto &material = renderer.materials[i];
                if (!mesh || !material) continue;

                auto mesh_backend = std::static_pointer_cast<MeshBackend>(mesh);
                auto material_backend = std::static_pointer_cast<MaterialBackend>(material);
                auto shader_backend = std::static_pointer_cast<ShaderBackend>(material->shader());
                if (!shader_backend) continue;

                auto matrix_m = XMMATRIX(local_to_world.value.m);
                const bool is_transparent = material_backend->is_transparent();

                auto command = std::make_unique<MeshDrawCommand>(
                    matrix_m,
                    mesh_backend,
                    material_backend);

                push_draw_command(shader_backend, is_transparent, std::move(command), matrix_m, matrix_v_inverse);
            } // End foreach mesh
        });
        scene.registry().view<const ImageRenderer, const LocalToWorld>(type_list<NonVisible>{}).each([&](SceneEntity entity, const ImageRenderer &renderer, const LocalToWorld &local_to_world) {
            auto &image = renderer.image;
            auto &material = renderer.material;
            if (!image || !material) return;

            auto image_backend = std::static_pointer_cast<TextureBackend>(image);
            auto material_backend = std::static_pointer_cast<MaterialBackend>(material);
            auto shader_backend = std::static_pointer_cast<ShaderBackend>(material->shader());
            if (!shader_backend) return;

            const bool is_transparent = material_backend->is_transparent();

            auto matrix_m = XMMATRIX(local_to_world.value.m);
            const auto width = image_backend->width() / (renderer.pixels_per_unit + std::numeric_limits<float>::epsilon());
            const auto height = image_backend->height() / (renderer.pixels_per_unit + std::numeric_limits<float>::epsilon());
            matrix_m = XMMatrixScaling(width, height, 1.0f) * matrix_m;

            auto command = std::make_unique<ImageDrawCommand>(
                matrix_m,
                image_backend,
                material_backend,
                renderer.color);

            push_draw_command(shader_backend, is_transparent, std::move(command), matrix_m, matrix_v_inverse);
        });

        scene.registry().view<const TextRenderer, const LocalToWorld>(type_list<NonVisible>{}).each([&](SceneEntity entity, const TextRenderer &renderer, const LocalToWorld &local_to_world) {
            auto &material = renderer.material;
            auto &font = renderer.font;
            if (!material || !font) return;

            auto material_backend = std::static_pointer_cast<MaterialBackend>(material);
            auto shader_backend = std::static_pointer_cast<ShaderBackend>(material->shader());
            if (!shader_backend) return;

            auto matrix_m = XMMATRIX(local_to_world.value.m);

            const bool is_transparent = material_backend->is_transparent();
            auto command = std::make_unique<TextDrawCommand>(
                XMMATRIX(local_to_world.value.m),
                renderer);

            push_draw_command(shader_backend, is_transparent, std::move(command), matrix_m, matrix_v_inverse);
        });
    }

    // Clear render target view with solid color.
    {
        const float color[] = {0.1f, 0.1f, 0.1f, 1.0f};
        gfx_.context().ClearRenderTargetView(render_target, color);
    }

    auto &cb_scene_properties = renderer_context_.cb_scene_properties();

    XMStoreFloat4x4(&cb_scene_properties.data().matrix_p, matrix_p);
    XMStoreFloat4x4(&cb_scene_properties.data().matrix_p_inverse, matrix_p_inverse);

    XMStoreFloat4x4(&cb_scene_properties.data().matrix_v, matrix_v);
    XMStoreFloat4x4(&cb_scene_properties.data().matrix_v_inverse, matrix_v_inverse);

    XMVECTOR scale, rotQuat, trans;
    XMMatrixDecompose(&scale, &rotQuat, &trans, matrix_v_inverse);

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

    // Reset depth buffer.
    gfx_.context().ClearDepthStencilView(&context.depth_stencil_view(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Render skybox.
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

        gfx_.context().OMSetRenderTargets(1, &render_target, nullptr);
        D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, static_cast<FLOAT>(context.target_width()), static_cast<FLOAT>(context.target_height()));
        gfx_.context().RSSetViewports(1u, &vp);
        gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        shader_backend->bind();

        material_backend->bind_constant_buffer(&gfx_, SceneRenderingConstants::MATERIAL_PROPERTIES_CB_SLOT);
        renderer_context_.set_cull_mode(material_backend->cull_mode());

        auto &cb_texture_config = renderer_context_.cb_texture_config();
        cb_texture_config.data().tex_has_flag = 0x00;
        renderer_context_.bind_texture_entries(material_backend->texture_entries(), cb_texture_config.data().tex_has_flag);
        cb_texture_config.apply();

        // set_cull_mode(nodec_rendering::CullMode::Back);

        norm_cube_mesh.bind(&gfx_);
        gfx_.DrawIndexed(norm_cube_mesh.triangles.size());
    }();

    for (auto iter = draw_groups_.begin(); iter != draw_groups_.end();) {
        auto &draw_group = iter->second;
        auto shader = draw_group->shader.lock();
        if (!shader) {
            iter = draw_groups_.erase(iter);
            continue;
        }

        if (shader->pass_count() == 1) {
            // One pass shader.
            gfx_.context().OMSetRenderTargets(1, &render_target, &context.depth_stencil_view());
            D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, context.target_width(), context.target_height());
            gfx_.context().RSSetViewports(1u, &vp);
            gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            shader->bind();

            // now lets draw the mesh.
            renderer_context_.cb_model_properties().buffer().bind(SceneRenderingConstants::MODEL_PROPERTIES_CB_SLOT);
            draw_group->draw_all(matrix_v, matrix_p, renderer_context_, gfx_);
        } else {
            // Multi pass shader.
            const auto passCount = shader->pass_count();
            // first pass
            {
                const auto &targets = shader->render_targets(0);

                std::vector<ID3D11RenderTargetView *> renderTargets(targets.size());
                std::vector<D3D11_VIEWPORT> vps(targets.size());
                for (size_t i = 0; i < targets.size(); ++i) {
                    auto &buffer = context.geometry_buffer(targets[i]);
                    renderTargets[i] = &buffer.render_target_view();
                    gfx_.context().ClearRenderTargetView(renderTargets[i], Vector4f::zero.v);

                    vps[i] = CD3D11_VIEWPORT(0.f, 0.f, buffer.width(), buffer.height());
                }

                gfx_.context().OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), &context.depth_stencil_view());
                gfx_.context().RSSetViewports(vps.size(), vps.data());
                gfx_.context().ClearDepthStencilView(&context.depth_stencil_view(), D3D11_CLEAR_DEPTH, 1.0f, 0);

                gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                shader->bind(0);

                renderer_context_.cb_model_properties().buffer().bind(SceneRenderingConstants::MODEL_PROPERTIES_CB_SLOT);
                draw_group->draw_all(matrix_v, matrix_p, renderer_context_, gfx_);
            }

            // medium pass
            // NOTE: This pass maybe not necessary.
            //  Alternatively, we can use the another shader pass.
            {
            }

            // final pass
            {
                const auto passNum = passCount - 1;

                gfx_.context().OMSetRenderTargets(1, &render_target, nullptr);
                D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, context.target_width(), context.target_height());
                gfx_.context().RSSetViewports(1u, &vp);
                gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                const auto &textureResources = shader->texture_resources(passNum);
                for (size_t i = 0; i < textureResources.size(); ++i) {
                    auto &buffer = context.geometry_buffer(textureResources[i]);
                    auto *view = &buffer.shader_resource_view();
                    gfx_.context().PSSetShaderResources(static_cast<UINT>(i), 1u, &view);
                }

                shader->bind(passNum);

                renderer_context_.sampler_state({Sampler::FilterMode::Bilinear, Sampler::WrapMode::Clamp}).BindPS(&gfx_, 0);

                auto &screen_quad_mesh = renderer_context_.screen_quad_mesh();
                screen_quad_mesh.bind(&gfx_);
                gfx_.DrawIndexed(static_cast<UINT>(screen_quad_mesh.triangles.size()));

                renderer_context_.unbind_all_shader_resources(static_cast<UINT>(textureResources.size()));
            }
        }

        draw_group->clear_draw_commands();

        ++iter;
    }
}
