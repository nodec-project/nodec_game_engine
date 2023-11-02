#include "scene_gizmo_renderer.hpp"

#include <DirectXMath.h>

#include <nodec_scene/components/local_to_world.hpp>

#include <Rendering/MeshBackend.hpp>

#include "components/gizmo_wire.hpp"

SceneGizmoRenderer::SceneGizmoRenderer(Graphics &gfx, nodec_resources::Resources &resources)
    : gfx_(gfx), logger_(nodec::logging::get_logger("editor.scene-gizmo-renderer")), resources_(resources),
      cb_scene_properties_(gfx),
      cb_texture_properties_(gfx),
      cb_model_properties_(gfx),
      rs_gizmo_wire_(gfx, D3D11_CULL_NONE, D3D11_FILL_WIREFRAME) {
    using namespace nodec;
    using namespace nodec_rendering::resources;

    gizmo_wire_material_ = std::static_pointer_cast<MaterialBackend>(
        resources_.registry().get_resource_direct<Material>(
            "org.nodec.game-editor/materials/gizmo-wire.material"));
    if (!gizmo_wire_material_) {
        logger_->warn(__FILE__, __LINE__) << "Material load failed.";
    }
}

void SceneGizmoRenderer::render(nodec_scene::Scene &scene,
                                const nodec::Matrix4x4f &view, const nodec::Matrix4x4f &projection,
                                ID3D11RenderTargetView &render_target,
                                SceneRenderingContext &context) {
    if (!gizmo_wire_material_) return;
    auto *gizmo_wire_shader = static_cast<ShaderBackend *>(gizmo_wire_material_->shader().get());
    if (!gizmo_wire_shader) return;

    using namespace ::components;
    using namespace nodec_scene;
    using namespace nodec_scene::components;
    using namespace DirectX;

    auto *render_target_ptr = &render_target;
    auto &device_context = gfx_.context();
    auto &scene_registry = scene.registry();

    cb_scene_properties_.buffer().bind(SCENE_PROPERTIES_CB_SLOT);
    cb_texture_properties_.buffer().bind(TEXTURE_CONFIG_CB_SLOT);
    cb_model_properties_.buffer().bind(MODEL_PROPERTIES_CB_SLOT);

    XMMATRIX matrix_p{projection.m};
    XMMATRIX matrix_v{view.m};
    auto matrix_p_inv = XMMatrixInverse(nullptr, matrix_p);
    auto matrix_v_inv = XMMatrixInverse(nullptr, matrix_v);

    // --- update cb_scene_properties ---
    {
        XMStoreFloat4x4(&cb_scene_properties_.data().matrix_p, matrix_p);
        XMStoreFloat4x4(&cb_scene_properties_.data().matrix_p_inverse, matrix_p_inv);

        XMStoreFloat4x4(&cb_scene_properties_.data().matrix_v, matrix_v);
        XMStoreFloat4x4(&cb_scene_properties_.data().matrix_v_inverse, matrix_v_inv);

        {
            XMVECTOR scale, rotation, trans;
            XMMatrixDecompose(&scale, &rotation, &trans, matrix_v_inv);

            cb_scene_properties_.data().camera_position.set(
                XMVectorGetByIndex(trans, 0),
                XMVectorGetByIndex(trans, 1),
                XMVectorGetByIndex(trans, 2),
                XMVectorGetByIndex(trans, 3));
        }
        cb_scene_properties_.apply();
    }

    gizmo_wire_shader->bind();

    device_context.OMSetRenderTargets(1, &render_target_ptr, nullptr);
    D3D11_VIEWPORT vp = CD3D11_VIEWPORT(0.f, 0.f, context.target_width(), context.target_height());
    device_context.RSSetViewports(1u, &vp);

    device_context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    rs_gizmo_wire_.bind();

    scene_registry.view<GizmoWire, LocalToWorld>().each(
        [&](const SceneEntity &entity, const GizmoWire &wire, const LocalToWorld &local_to_world) {
            if (!wire.mesh) return;

            // --- update cb_model_properties ---
            {
                XMMATRIX matrix_m{local_to_world.value.m};
                auto matrix_m_inv = XMMatrixInverse(nullptr, matrix_m);

                auto matrix_mvp = matrix_m * matrix_v * matrix_p;

                XMStoreFloat4x4(&cb_model_properties_.data().matrix_m, matrix_m);
                XMStoreFloat4x4(&cb_model_properties_.data().matrix_m_inverse, matrix_m_inv);
                XMStoreFloat4x4(&cb_model_properties_.data().matrix_mvp, matrix_mvp);

                cb_model_properties_.apply();
            }

            auto *mesh_backend = static_cast<MeshBackend *>(wire.mesh.get());
            mesh_backend->bind(&gfx_);
            device_context.DrawIndexed(mesh_backend->triangles.size(), 0, 0);
        });
}

void SceneGizmoRenderer::clear_gizmos(nodec_scene::Scene &scene) {
    {
        auto view = scene.registry().view<components::GizmoWire>();
        scene.registry().destroy_entities(view.begin(), view.end());
    }
}