#pragma once

#include <Font/FontCharacterDatabase.hpp>
#include <Graphics/BlendState.hpp>
#include <Graphics/ConstantBuffer.hpp>
#include <Graphics/GeometryBuffer.hpp>
#include <Graphics/Graphics.hpp>
#include <Graphics/RasterizerState.hpp>
#include <Graphics/SamplerState.hpp>
#include <Rendering/MaterialBackend.hpp>
#include <Rendering/MeshBackend.hpp>
#include <Rendering/ShaderBackend.hpp>
#include <Rendering/TextureBackend.hpp>

#include <nodec_rendering/components/camera.hpp>
#include <nodec_rendering/components/directional_light.hpp>
#include <nodec_rendering/components/image_renderer.hpp>
#include <nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_rendering/components/non_visible.hpp>
#include <nodec_rendering/components/point_light.hpp>
#include <nodec_rendering/components/post_processing.hpp>
#include <nodec_rendering/components/scene_lighting.hpp>
#include <nodec_rendering/components/text_renderer.hpp>
#include <nodec_rendering/sampler.hpp>
#include <nodec_scene/components/transform.hpp>
#include <nodec_scene/scene.hpp>

#include <nodec/resource_management/resource_registry.hpp>
#include <nodec/vector4.hpp>

#include <DirectXMath.h>

#include <algorithm>
#include <cassert>
#include <unordered_set>

class SceneRenderingContext {
public:
    SceneRenderingContext(std::uint32_t target_width, std::uint32_t target_height, Graphics *gfx)
        : gfx_(gfx), target_width_(target_width), target_height_(target_height) {
        assert(gfx_);

        {
            // Generate the depth stencil buffer texture.
            Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilTexture;
            D3D11_TEXTURE2D_DESC depthStencilBufferDesc{};
            depthStencilBufferDesc.Width = target_width;
            depthStencilBufferDesc.Height = target_height;
            depthStencilBufferDesc.MipLevels = 1;
            depthStencilBufferDesc.ArraySize = 1;
            depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            depthStencilBufferDesc.SampleDesc.Count = 1;
            depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
            depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            ThrowIfFailedGfx(
                gfx->device().CreateTexture2D(&depthStencilBufferDesc, nullptr, &depthStencilTexture),
                gfx, __FILE__, __LINE__);

            D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
            depthStencilViewDesc.Format = depthStencilBufferDesc.Format;
            depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

            ThrowIfFailedGfx(
                gfx->device().CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilViewDesc, &depth_stencil_view_),
                gfx, __FILE__, __LINE__);
        }
    }

    GeometryBuffer &geometry_buffer(const std::string &name) {
        return geometry_buffer(name, target_width_, target_height_);
    }

    GeometryBuffer &geometry_buffer(const std::string &name, std::uint32_t width, std::uint32_t height) {
        auto &buffer = geometry_buffers_[name];
        if (!buffer) {
            buffer.reset(new GeometryBuffer(gfx_, width, height));
        }
        return *buffer;
    }

    void swap_geometry_buffers(const std::string &lhs, const std::string &rhs) {
        std::swap(geometry_buffers_[lhs], geometry_buffers_[rhs]);
    }

    std::uint32_t target_width() const noexcept {
        return target_width_;
    }

    std::uint32_t target_height() const noexcept {
        return target_height_;
    }

    ID3D11DepthStencilView &depth_stencil_view() {
        return *depth_stencil_view_.Get();
    }

private:
    std::uint32_t target_width_;
    std::uint32_t target_height_;
    Graphics *gfx_;
    std::unordered_map<std::string, std::unique_ptr<GeometryBuffer>> geometry_buffers_;

    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view_;
};

class SceneRenderer {
    using TextureEntry = nodec_rendering::resources::Material::TextureEntry;

    static constexpr UINT SCENE_PROPERTIES_CB_SLOT = 0;
    static constexpr UINT TEXTURE_CONFIG_CB_SLOT = 1;
    static constexpr UINT MODEL_PROPERTIES_CB_SLOT = 2;
    static constexpr UINT MATERIAL_PROPERTIES_CB_SLOT = 3;

public:
    struct DirectionalLight {
        nodec::Vector3f direction;
        float intensity;
        nodec::Vector4f color;
        uint32_t enabled{0x00};
        uint32_t padding[3];
    };

    struct PointLight {
        nodec::Vector3f position;
        float range;
        nodec::Vector3f color;
        float intensity;
    };

    static constexpr size_t MAX_NUM_OF_POINT_LIGHTS = 1024;

    struct SceneLighting {
        nodec::Vector4f ambientColor;
        uint32_t numOfPointLights;
        uint32_t padding[3];
        DirectionalLight directional;
        PointLight pointLights[MAX_NUM_OF_POINT_LIGHTS];
    };

    struct SceneProperties {
        nodec::Vector4f cameraPos;
        DirectX::XMFLOAT4X4 matrixP;
        DirectX::XMFLOAT4X4 matrixPInverse;
        DirectX::XMFLOAT4X4 matrixV;
        DirectX::XMFLOAT4X4 matrixVInverse;
        SceneLighting lights;
    };

    struct ModelProperties {
        DirectX::XMFLOAT4X4 matrixMVP;
        DirectX::XMFLOAT4X4 matrixM;
        DirectX::XMFLOAT4X4 matrixMInverse;
    };

    struct TextureConfig {
        uint32_t texHasFlag;
        uint32_t padding[3];
    };

public:
    SceneRenderer(Graphics *gfx, nodec::resource_management::ResourceRegistry &resourceRegistry);

    // void Render(nodec_scene::Scene &scene, ID3D11RenderTargetView *pTarget, UINT width, UINT height);
    void Render(nodec_scene::Scene &scene, ID3D11RenderTargetView &render_target, SceneRenderingContext &context);

    void Render(nodec_scene::Scene &scene, nodec::Matrix4x4f &view, nodec::Matrix4x4f &projection,
                ID3D11RenderTargetView *pTarget, SceneRenderingContext &context);

private:
    void SetupSceneLighting(nodec_scene::Scene &scene) {
        using namespace nodec_rendering::components;
        using namespace nodec_scene::components;
        using namespace nodec;

        mSceneProperties.lights.directional.enabled = 0x00;
        scene.registry().view<const nodec_rendering::components::DirectionalLight, const Transform>().each(
            [&](auto entt, const nodec_rendering::components::DirectionalLight &light, const Transform &trfm) {
                auto &directional = mSceneProperties.lights.directional;
                directional.enabled = 0x01;
                directional.color = light.color;
                directional.intensity = light.intensity;

                auto direction = trfm.local2world * Vector4f{0.0f, 0.0f, 1.0f, 0.0f};
                directional.direction.set(direction.x, direction.y, direction.z);
            });

        scene.registry().view<const nodec_rendering::components::SceneLighting>().each([&](auto entt, const nodec_rendering::components::SceneLighting &lighting) {
            mSceneProperties.lights.ambientColor = lighting.ambient_color;
        });
    }

    void Render(nodec_scene::Scene &scene,
                const DirectX::XMMATRIX &matrixV, const DirectX::XMMATRIX &matrixVInverse,
                const DirectX::XMMATRIX &matrixP, const DirectX::XMMATRIX &matrixPInverse,
                ID3D11RenderTargetView *pTarget, SceneRenderingContext &context);

    void RenderModel(nodec_scene::Scene &scene, ShaderBackend *activeShader,
                     const DirectX::XMMATRIX &matrixV, const DirectX::XMMATRIX &matrixP);

    void SetCullMode(const nodec_rendering::CullMode &mode) {
        using namespace nodec_rendering;
        switch (mode) {
        default:
        case CullMode::Back:
            mRSCullBack.Bind(gfx_);
            break;
        case CullMode::Front:
            // TODO: impl
        case CullMode::Off:
            mRSCullNone.Bind(gfx_);
            break;
        }
    }

    /**
     * @brief
     *
     * @param textureEntries
     * @param texHasFlag
     * @return UINT next slot number.
     */
    UINT bind_texture_entries(const std::vector<TextureEntry> &textureEntries, uint32_t &texHasFlag);

    void unbind_all_shader_resources(UINT count) {
        assert(count <= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

        static ID3D11ShaderResourceView *const nulls[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{nullptr};

        gfx_->context().VSSetShaderResources(0, count, nulls);
        gfx_->context().PSSetShaderResources(0, count, nulls);
    }

    SamplerState &GetSamplerState(const nodec_rendering::Sampler &sampler) {
        auto &state = mSamplerStates[sampler];
        if (state) return *state;

        state = SamplerState::Create(gfx_, sampler);
        return *state;
    }

private:
    // slot 0
    SceneProperties mSceneProperties;
    ConstantBuffer mScenePropertiesCB;

    // slot 1
    TextureConfig mTextureConfig;
    ConstantBuffer mTextureConfigCB;

    // slot 2
    ModelProperties mModelProperties;
    ConstantBuffer mModelPropertiesCB;

    std::unordered_map<nodec_rendering::Sampler, nodec::optional<SamplerState>> mSamplerStates;

    RasterizerState mRSCullNone;
    RasterizerState mRSCullBack;

    BlendState mBSDefault;
    BlendState mBSAlphaBlend;

    Graphics *gfx_;

    std::shared_ptr<MeshBackend> mQuadMesh;
    std::unique_ptr<MeshBackend> mScreenQuadMesh;

    FontCharacterDatabase mFontCharacterDatabase;
};