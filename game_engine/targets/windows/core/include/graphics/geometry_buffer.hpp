#ifndef NODEC_GAME_ENGINE__GRAPHICS__GEOMETRY_BUFFER_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__GEOMETRY_BUFFER_HPP_

#include "graphics.hpp"

class GeometryBuffer {
public:
    GeometryBuffer(Graphics *gfx, UINT width, UINT height)
        : width_{width}, height_{height} {
        // Generate the render target textures.
        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        ThrowIfFailedGfx(
            gfx->device().CreateTexture2D(&textureDesc, nullptr, &texture_),
            gfx, __FILE__, __LINE__);

        // Generate the render target views.
        D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
        renderTargetViewDesc.Format = textureDesc.Format;
        renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        ThrowIfFailedGfx(
            gfx->device().CreateRenderTargetView(texture_.Get(), &renderTargetViewDesc, &render_target_view_),
            gfx, __FILE__, __LINE__);

        // Generate the shader resource views.
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
        shaderResourceViewDesc.Format = textureDesc.Format;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        ThrowIfFailedGfx(
            gfx->device().CreateShaderResourceView(texture_.Get(), &shaderResourceViewDesc, &shader_resource_view_),
            gfx, __FILE__, __LINE__);
    }

    ID3D11Texture2D& texture() noexcept {
        return *texture_.Get();
    }

    ID3D11RenderTargetView &render_target_view() noexcept {
        return *render_target_view_.Get();
    }

    ID3D11ShaderResourceView &shader_resource_view() noexcept {
        return *shader_resource_view_.Get();
    }

    UINT width() const noexcept {
        return width_;
    }

    UINT height() const noexcept {
        return height_;
    }

private:
    UINT width_;
    UINT height_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_;
};

#endif