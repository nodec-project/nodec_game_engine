#include <rendering/scene_renderer_context.hpp>

#include <d3dcompiler.h>

namespace {

// Simple passthrough shader for copying texture to render target
constexpr const char *g_copy_vs_code = R"(
struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT main(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD0) {
    VS_OUTPUT output;
    output.pos = float4(position, 1.0);
    output.uv = uv;
    return output;
}
)";

constexpr const char *g_copy_ps_code = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);

float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_Target {
    return tex.Sample(samp, uv);
}
)";

} // namespace

SceneRendererContext::SceneRendererContext(std::shared_ptr<nodec::logging::Logger> logger, Graphics &gfx,
                                           nodec::resource_management::ResourceRegistry &resource_registry)
    : logger_(logger), gfx_(gfx),
      font_character_database_(&gfx),
      cb_scene_properties_(gfx),
      cb_model_properties_(gfx),
      cb_texture_config_(gfx), rs_cull_none_(gfx, D3D11_CULL_NONE),
      rs_cull_front_(gfx, D3D11_CULL_FRONT),
      rs_cull_back_(gfx, D3D11_CULL_BACK),
      bs_default_(BlendState::CreateDefaultBlend(gfx)),
      bs_alpha_blend_(BlendState::CreateAlphaBlend(gfx)) {
    using namespace nodec_rendering::resources;
    using namespace nodec::resource_management;
    using namespace nodec;

    // Get quad mesh from resource registry.
    {
        // 0.5 x 0.5 quad.
        auto quad_mesh = resource_registry.get_resource_direct<Mesh>("org.nodec.game-engine/meshes/quad.mesh");

        if (!quad_mesh) {
            logger_->fatal(__FILE__, __LINE__) << "Cannot load the essential resource 'quad.mesh'.\n"
                                                  "Make sure the 'org.nodec.game-engine' resource-package is installed.";
            throw std::runtime_error("Cannot load the essential resource 'quad.mesh'.");
        }

        quad_mesh_ = std::static_pointer_cast<MeshBackend>(quad_mesh);
    }

    {
        auto norm_cube_mesh = resource_registry.get_resource_direct<Mesh>("org.nodec.game-engine/meshes/norm-cube.mesh");
        if (!norm_cube_mesh) {
            logger_->fatal(__FILE__, __LINE__) << "Cannot load the essential resource 'norm-cube.mesh'.\n"
                                                  "Make sure the 'org.nodec.game-engine' resource-package is installed.";
            throw std::runtime_error("Cannot load the essential resource 'norm-cube.mesh'.");
        }
        norm_cube_mesh_ = std::static_pointer_cast<MeshBackend>(norm_cube_mesh);
    }

    // Make screen quad mesh in NDC space which is not depend on target view size.
    {
        screen_quad_mesh_.reset(new MeshBackend());

        screen_quad_mesh_->vertices.resize(4);
        screen_quad_mesh_->triangles.resize(6);

        screen_quad_mesh_->vertices[0] = {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}};
        screen_quad_mesh_->vertices[1] = {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}};
        screen_quad_mesh_->vertices[2] = {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}};
        screen_quad_mesh_->vertices[3] = {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}};

        screen_quad_mesh_->triangles[0] = 0;
        screen_quad_mesh_->triangles[1] = 1;
        screen_quad_mesh_->triangles[2] = 2;

        screen_quad_mesh_->triangles[3] = 0;
        screen_quad_mesh_->triangles[4] = 2;
        screen_quad_mesh_->triangles[5] = 3;
        screen_quad_mesh_->update_device_memory(&gfx_);
    }

    // Compile and create copy shader for compose_to_render_target
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vs_blob;
        Microsoft::WRL::ComPtr<ID3DBlob> ps_blob;
        Microsoft::WRL::ComPtr<ID3DBlob> error_blob;

        HRESULT hr = D3DCompile(g_copy_vs_code, strlen(g_copy_vs_code), nullptr, nullptr, nullptr,
                                "main", "vs_5_0", 0, 0, &vs_blob, &error_blob);
        if (FAILED(hr)) {
            const char *error_msg = error_blob ? static_cast<const char *>(error_blob->GetBufferPointer()) : "Unknown error";
            logger_->error(__FILE__, __LINE__) << "Failed to compile copy vertex shader: " << error_msg;
            throw std::runtime_error("Failed to compile copy vertex shader");
        }

        hr = D3DCompile(g_copy_ps_code, strlen(g_copy_ps_code), nullptr, nullptr, nullptr,
                        "main", "ps_5_0", 0, 0, &ps_blob, &error_blob);
        if (FAILED(hr)) {
            const char *error_msg = error_blob ? static_cast<const char *>(error_blob->GetBufferPointer()) : "Unknown error";
            logger_->error(__FILE__, __LINE__) << "Failed to compile copy pixel shader: " << error_msg;
            throw std::runtime_error("Failed to compile copy pixel shader");
        }

        gfx_.device().CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &copy_vs_);
        gfx_.device().CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &copy_ps_);

        // Create input layout matching screen_quad_mesh vertex format
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        gfx_.device().CreateInputLayout(layout, 3, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &copy_input_layout_);
    }
}

void SceneRendererContext::bind_copy_shader() {
    gfx_.context().IASetInputLayout(copy_input_layout_.Get());
    gfx_.context().VSSetShader(copy_vs_.Get(), nullptr, 0);
    gfx_.context().PSSetShader(copy_ps_.Get(), nullptr, 0);
}