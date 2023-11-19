#ifndef NODEC_GAME_EDITOR__SCENE_GIZMO_RENDERER_HPP_
#define NODEC_GAME_EDITOR__SCENE_GIZMO_RENDERER_HPP_

#include <d3d11.h>

#include <nodec/logging/logging.hpp>
#include <nodec_resources/resources.hpp>
#include <nodec_scene/scene.hpp>

#include <Graphics/Graphics.hpp>
#include <Graphics/RasterizerState.hpp>
#include <Rendering/MaterialBackend.hpp>
#include <Rendering/cb_model_properties.hpp>
#include <Rendering/cb_scene_properties.hpp>
#include <Rendering/cb_texture_config.hpp>
#include <Rendering/scene_rendering_context.hpp>

class SceneGizmoRenderer {
    static constexpr UINT SCENE_PROPERTIES_CB_SLOT = 0;
    static constexpr UINT TEXTURE_CONFIG_CB_SLOT = 1;
    static constexpr UINT MODEL_PROPERTIES_CB_SLOT = 2;
    static constexpr UINT MATERIAL_PROPERTIES_CB_SLOT = 3;

public:
    SceneGizmoRenderer(Graphics &gfx, nodec_resources::Resources &resources);

    void render(nodec_scene::Scene &scene,
                const nodec::Matrix4x4f &view, const nodec::Matrix4x4f &projection,
                ID3D11RenderTargetView &render_target, SceneRenderingContext &context);

    void clear_gizmos(nodec_scene::Scene &scene);

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    Graphics &gfx_;
    nodec_resources::Resources &resources_;
    CBSceneProperties cb_scene_properties_;
    CBTextureConfig cb_texture_properties_;
    CBModelProperties cb_model_properties_;
    RasterizerState rs_gizmo_wire_;

    std::shared_ptr<MaterialBackend> gizmo_wire_material_;
};

#endif