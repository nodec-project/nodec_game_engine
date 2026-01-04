#ifndef NODEC_GAME_EDITOR__EDITOR_WINDOWS__PICKING_RENDERER_HPP_
#define NODEC_GAME_EDITOR__EDITOR_WINDOWS__PICKING_RENDERER_HPP_

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <vector>

#include <nodec/matrix4x4.hpp>
#include <nodec_scene/scene.hpp>
#include <nodec_scene/scene_entity.hpp>

#include <graphics/graphics.hpp>
#include <graphics/ConstantBuffer.hpp>
#include <rendering/mesh_backend.hpp>

class PickingRenderer {
public:
    PickingRenderer(Graphics& gfx, UINT width, UINT height);

    void resize(UINT width, UINT height);

    // Render all entities to picking buffer
    void render(nodec_scene::Scene& scene,
                const nodec::Matrix4x4f& view,
                const nodec::Matrix4x4f& projection);

    // Read entity ID at pixel position (returns null_entity if no hit)
    nodec_scene::SceneEntity pick(int x, int y);

private:
    void create_resources();
    void compile_shaders();

private:
    Graphics& gfx_;
    UINT width_;
    UINT height_;

    // Picking buffer (R32_UINT)
    Microsoft::WRL::ComPtr<ID3D11Texture2D> picking_texture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> picking_rtv_;

    // Staging buffer for CPU readback (1x1 pixel)
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_texture_;

    // Depth buffer
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_texture_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_dsv_;

    // Shaders
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout_;

    // Rasterizer state
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_state_;

    // Blend state (blending disabled for R32_UINT format)
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state_;

    // Constant buffer structures
    struct ModelPropertiesCB {
        DirectX::XMFLOAT4X4 matrix_mvp;
        DirectX::XMFLOAT4X4 matrix_m;
        DirectX::XMFLOAT4X4 matrix_m_inverse;
    };

    struct PickingPropertiesCB {
        uint32_t entity_id;
        uint32_t padding[3];
    };

    std::unique_ptr<ConstantBuffer> cb_model_properties_;
    std::unique_ptr<ConstantBuffer> cb_picking_properties_;

    ModelPropertiesCB model_properties_data_;
    PickingPropertiesCB picking_properties_data_;

    // Entity ID mapping (rebuilt each frame)
    // ID is 1-based index: entity at index i has ID = i + 1
    // ID 0 = no entity (background)
    std::vector<nodec_scene::SceneEntity> id_to_entity_;

    // Quad mesh for ImageRenderer picking (0.5 x 0.5 centered at origin)
    std::unique_ptr<MeshBackend> quad_mesh_;
};

#endif
