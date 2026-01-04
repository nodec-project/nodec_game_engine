#include "picking_renderer.hpp"

#include <DirectXMath.h>

#include <nodec/logging/logging.hpp>
#include <nodec_rendering/components/image_renderer.hpp>
#include <nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_rendering/components/non_visible.hpp>
#include <nodec_scene/components/local_to_world.hpp>

#include <rendering/mesh_backend.hpp>
#include <rendering/texture_backend.hpp>

namespace {

// Embedded shader source code for picking
const char* g_picking_vs_source = R"(
cbuffer cbModelProperties : register(b2) {
    matrix matrixMVP;
    matrix matrixM;
    matrix matrixMInverse;
};

struct VSIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float3 tangent : TANGENT0;
};

struct VSOut {
    float4 position : SV_Position;
};

VSOut main(VSIn input) {
    VSOut output;
    output.position = mul(matrixMVP, float4(input.position, 1.0));
    return output;
}
)";

const char* g_picking_ps_source = R"(
cbuffer cbPickingProperties : register(b4) {
    uint entityId;
    uint3 padding;
};

uint main() : SV_Target {
    return entityId;
}
)";

} // namespace

PickingRenderer::PickingRenderer(Graphics& gfx, UINT width, UINT height)
    : gfx_(gfx), width_(width), height_(height) {
    compile_shaders();
    create_resources();
}

void PickingRenderer::compile_shaders() {
    using namespace Microsoft::WRL;

    UINT compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    ComPtr<ID3DBlob> error_blob;

    // Compile vertex shader
    HRESULT hr = D3DCompile(
        g_picking_vs_source, strlen(g_picking_vs_source),
        "picking_vs", nullptr, nullptr,
        "main", "vs_5_0", compile_flags, 0,
        &vs_blob, &error_blob);

    if (FAILED(hr)) {
        if (error_blob) {
            nodec::logging::error(__FILE__, __LINE__)
                << "Picking VS compile error: "
                << static_cast<const char*>(error_blob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile picking vertex shader");
    }

    // Compile pixel shader
    hr = D3DCompile(
        g_picking_ps_source, strlen(g_picking_ps_source),
        "picking_ps", nullptr, nullptr,
        "main", "ps_5_0", compile_flags, 0,
        &ps_blob, &error_blob);

    if (FAILED(hr)) {
        if (error_blob) {
            nodec::logging::error(__FILE__, __LINE__)
                << "Picking PS compile error: "
                << static_cast<const char*>(error_blob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile picking pixel shader");
    }

    // Create vertex shader
    ThrowIfFailedGfx(
        gfx_.device().CreateVertexShader(
            vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
            nullptr, &vertex_shader_),
        &gfx_, __FILE__, __LINE__);

    // Create pixel shader
    ThrowIfFailedGfx(
        gfx_.device().CreatePixelShader(
            ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(),
            nullptr, &pixel_shader_),
        &gfx_, __FILE__, __LINE__);

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    ThrowIfFailedGfx(
        gfx_.device().CreateInputLayout(
            layout, static_cast<UINT>(std::size(layout)),
            vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
            &input_layout_),
        &gfx_, __FILE__, __LINE__);

    // Create constant buffers
    cb_model_properties_ = std::make_unique<ConstantBuffer>(
        gfx_, sizeof(ModelPropertiesCB), &model_properties_data_);

    cb_picking_properties_ = std::make_unique<ConstantBuffer>(
        gfx_, sizeof(PickingPropertiesCB), &picking_properties_data_);
}

void PickingRenderer::create_resources() {
    // Create picking render target (R32_UINT format for entity IDs)
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width_;
    tex_desc.Height = height_;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R32_UINT;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ThrowIfFailedGfx(
        gfx_.device().CreateTexture2D(&tex_desc, nullptr, &picking_texture_),
        &gfx_, __FILE__, __LINE__);

    // Create render target view
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = DXGI_FORMAT_R32_UINT;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    ThrowIfFailedGfx(
        gfx_.device().CreateRenderTargetView(
            picking_texture_.Get(), &rtv_desc, &picking_rtv_),
        &gfx_, __FILE__, __LINE__);

    // Create staging texture for CPU readback (1x1 pixel)
    D3D11_TEXTURE2D_DESC staging_desc = {};
    staging_desc.Width = 1;
    staging_desc.Height = 1;
    staging_desc.MipLevels = 1;
    staging_desc.ArraySize = 1;
    staging_desc.Format = DXGI_FORMAT_R32_UINT;
    staging_desc.SampleDesc.Count = 1;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ThrowIfFailedGfx(
        gfx_.device().CreateTexture2D(&staging_desc, nullptr, &staging_texture_),
        &gfx_, __FILE__, __LINE__);

    // Create depth buffer
    D3D11_TEXTURE2D_DESC depth_desc = {};
    depth_desc.Width = width_;
    depth_desc.Height = height_;
    depth_desc.MipLevels = 1;
    depth_desc.ArraySize = 1;
    depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_desc.SampleDesc.Count = 1;
    depth_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ThrowIfFailedGfx(
        gfx_.device().CreateTexture2D(&depth_desc, nullptr, &depth_texture_),
        &gfx_, __FILE__, __LINE__);

    // Create depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    ThrowIfFailedGfx(
        gfx_.device().CreateDepthStencilView(
            depth_texture_.Get(), &dsv_desc, &depth_dsv_),
        &gfx_, __FILE__, __LINE__);

    // Create rasterizer state (cull back)
    D3D11_RASTERIZER_DESC raster_desc = {};
    raster_desc.FillMode = D3D11_FILL_SOLID;
    raster_desc.CullMode = D3D11_CULL_BACK;
    raster_desc.FrontCounterClockwise = FALSE;
    raster_desc.DepthClipEnable = TRUE;

    ThrowIfFailedGfx(
        gfx_.device().CreateRasterizerState(&raster_desc, &rasterizer_state_),
        &gfx_, __FILE__, __LINE__);

    // Create blend state (blending disabled for R32_UINT format)
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;
    blend_desc.RenderTarget[0].BlendEnable = FALSE;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ThrowIfFailedGfx(
        gfx_.device().CreateBlendState(&blend_desc, &blend_state_),
        &gfx_, __FILE__, __LINE__);

    // Create quad mesh for ImageRenderer (0.5 x 0.5 centered at origin)
    quad_mesh_ = std::make_unique<MeshBackend>();
    quad_mesh_->vertices.resize(4);
    quad_mesh_->triangles.resize(6);

    // Quad vertices: position, normal (facing -Z), UV
    quad_mesh_->vertices[0] = {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
    quad_mesh_->vertices[1] = {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    quad_mesh_->vertices[2] = {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    quad_mesh_->vertices[3] = {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

    // Two triangles
    quad_mesh_->triangles[0] = 0;
    quad_mesh_->triangles[1] = 1;
    quad_mesh_->triangles[2] = 2;

    quad_mesh_->triangles[3] = 0;
    quad_mesh_->triangles[4] = 2;
    quad_mesh_->triangles[5] = 3;

    quad_mesh_->update_device_memory(&gfx_);
}

void PickingRenderer::resize(UINT width, UINT height) {
    if (width == width_ && height == height_) return;
    if (width == 0 || height == 0) return;

    width_ = width;
    height_ = height;

    // Reset resources
    picking_texture_.Reset();
    picking_rtv_.Reset();
    depth_texture_.Reset();
    depth_dsv_.Reset();

    // Recreate resources
    create_resources();
}

void PickingRenderer::render(nodec_scene::Scene& scene,
                              const nodec::Matrix4x4f& view,
                              const nodec::Matrix4x4f& projection) {
    using namespace nodec;
    using namespace nodec_scene::components;
    using namespace nodec_rendering::components;
    using namespace DirectX;

    // Clear picking buffer to 0 (no entity)
    UINT clear_value[4] = {0, 0, 0, 0};
    gfx_.context().ClearRenderTargetView(picking_rtv_.Get(), reinterpret_cast<const float*>(clear_value));
    gfx_.context().ClearDepthStencilView(depth_dsv_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Set render target
    gfx_.context().OMSetRenderTargets(1, picking_rtv_.GetAddressOf(), depth_dsv_.Get());

    // Set blend state (disable blending for R32_UINT format)
    gfx_.context().OMSetBlendState(blend_state_.Get(), nullptr, 0xFFFFFFFF);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    gfx_.context().RSSetViewports(1, &viewport);

    // Set rasterizer state
    gfx_.context().RSSetState(rasterizer_state_.Get());

    // Bind shaders
    gfx_.context().VSSetShader(vertex_shader_.Get(), nullptr, 0);
    gfx_.context().PSSetShader(pixel_shader_.Get(), nullptr, 0);
    gfx_.context().IASetInputLayout(input_layout_.Get());
    gfx_.context().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Bind constant buffers
    cb_model_properties_->bind(2);  // slot b2
    cb_picking_properties_->bind(4); // slot b4

    // Convert view/projection matrices
    XMMATRIX xm_view = XMMATRIX(view.m);
    XMMATRIX xm_proj = XMMATRIX(projection.m);
    XMMATRIX xm_vp = xm_view * xm_proj;

    // Reset ID mapping
    id_to_entity_.clear();

    // Render MeshRenderer entities
    {
        auto mesh_view = scene.registry().view<MeshRenderer, LocalToWorld>(
            nodec::type_list<NonVisible>{});

        for (auto entity : mesh_view) {
            auto& mesh_renderer = scene.registry().get_component<MeshRenderer>(entity);
            auto& local_to_world = scene.registry().get_component<LocalToWorld>(entity);

            for (auto& mesh : mesh_renderer.meshes) {
                if (!mesh) continue;

                auto* mesh_backend = static_cast<MeshBackend*>(mesh.get());
                if (!mesh_backend->vertex_buffer() || !mesh_backend->index_buffer()) continue;

                // Assign entity ID (1-based: index 0 has ID 1)
                id_to_entity_.push_back(entity);
                uint32_t id = static_cast<uint32_t>(id_to_entity_.size());

                // Set model properties
                XMMATRIX xm_m = XMMATRIX(local_to_world.value.m);
                XMMATRIX xm_mvp = xm_m * xm_vp;
                XMMATRIX xm_m_inv = XMMatrixInverse(nullptr, xm_m);

                XMStoreFloat4x4(&model_properties_data_.matrix_mvp, xm_mvp);
                XMStoreFloat4x4(&model_properties_data_.matrix_m, xm_m);
                XMStoreFloat4x4(&model_properties_data_.matrix_m_inverse, xm_m_inv);
                cb_model_properties_->update(&model_properties_data_);

                // Set picking properties
                picking_properties_data_.entity_id = id;
                cb_picking_properties_->update(&picking_properties_data_);

                // Draw mesh
                mesh_backend->bind(&gfx_);
                gfx_.DrawIndexed(static_cast<UINT>(mesh_backend->triangles.size()));
            }
        }
    }

    // Render ImageRenderer entities
    {
        auto image_view = scene.registry().view<ImageRenderer, LocalToWorld>(
            nodec::type_list<NonVisible>{});

        for (auto entity : image_view) {
            auto& img_renderer = scene.registry().get_component<ImageRenderer>(entity);
            auto& local_to_world = scene.registry().get_component<LocalToWorld>(entity);

            if (!img_renderer.image) continue;

            auto* tex_backend = static_cast<TextureBackend*>(img_renderer.image.get());
            float ppu = img_renderer.pixels_per_unit + 1e-6f;
            float img_width = tex_backend->width() / ppu;
            float img_height = tex_backend->height() / ppu;

            // Assign entity ID (1-based: index 0 has ID 1)
            id_to_entity_.push_back(entity);
            uint32_t id = static_cast<uint32_t>(id_to_entity_.size());

            // Calculate model matrix with image scale
            XMMATRIX xm_scale = XMMatrixScaling(img_width, img_height, 1.0f);
            XMMATRIX xm_local = XMMATRIX(local_to_world.value.m);
            XMMATRIX xm_m = xm_scale * xm_local;
            XMMATRIX xm_mvp = xm_m * xm_vp;
            XMMATRIX xm_m_inv = XMMatrixInverse(nullptr, xm_m);

            XMStoreFloat4x4(&model_properties_data_.matrix_mvp, xm_mvp);
            XMStoreFloat4x4(&model_properties_data_.matrix_m, xm_m);
            XMStoreFloat4x4(&model_properties_data_.matrix_m_inverse, xm_m_inv);
            cb_model_properties_->update(&model_properties_data_);

            // Set picking properties
            picking_properties_data_.entity_id = id;
            cb_picking_properties_->update(&picking_properties_data_);

            // Draw quad
            quad_mesh_->bind(&gfx_);
            gfx_.DrawIndexed(static_cast<UINT>(quad_mesh_->triangles.size()));
        }
    }

    // Restore render target (will be set by scene renderer)
}

nodec_scene::SceneEntity PickingRenderer::pick(int x, int y) {
    using namespace nodec;

    // Bounds check
    if (x < 0 || x >= static_cast<int>(width_) ||
        y < 0 || y >= static_cast<int>(height_)) {
        return nodec_scene::SceneEntity{entities::null_entity};
    }

    // Copy single pixel to staging texture
    D3D11_BOX box = {};
    box.left = static_cast<UINT>(x);
    box.top = static_cast<UINT>(y);
    box.front = 0;
    box.right = static_cast<UINT>(x) + 1;
    box.bottom = static_cast<UINT>(y) + 1;
    box.back = 1;

    gfx_.context().CopySubresourceRegion(
        staging_texture_.Get(), 0, 0, 0, 0,
        picking_texture_.Get(), 0, &box);

    // Map and read
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = gfx_.context().Map(staging_texture_.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        return nodec_scene::SceneEntity{entities::null_entity};
    }

    uint32_t entity_id = *static_cast<uint32_t*>(mapped.pData);
    gfx_.context().Unmap(staging_texture_.Get(), 0);

    // Lookup entity (ID is 1-based, so index = entity_id - 1)
    if (entity_id > 0 && entity_id <= id_to_entity_.size()) {
        return id_to_entity_[entity_id - 1];
    }

    return nodec_scene::SceneEntity{entities::null_entity};
}
