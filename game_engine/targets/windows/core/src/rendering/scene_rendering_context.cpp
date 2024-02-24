#include <rendering/scene_rendering_context.hpp>

SceneRenderingContext::SceneRenderingContext(
    std::uint32_t target_width,
    std::uint32_t target_height, Graphics &gfx)
    : gfx_(gfx), target_width_(target_width), target_height_(target_height) {
    {
        // Generate the depth stencil buffer texture.
        Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_stencil_texture;
        D3D11_TEXTURE2D_DESC depth_stencil_buffer_desc{};
        depth_stencil_buffer_desc.Width = target_width;
        depth_stencil_buffer_desc.Height = target_height;
        depth_stencil_buffer_desc.MipLevels = 1;
        depth_stencil_buffer_desc.ArraySize = 1;
        depth_stencil_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_buffer_desc.SampleDesc.Count = 1;
        depth_stencil_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_stencil_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        ThrowIfFailedGfx(
            gfx.device().CreateTexture2D(&depth_stencil_buffer_desc, nullptr, &depth_stencil_texture),
            &gfx, __FILE__, __LINE__);

        D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
        depth_stencil_view_desc.Format = depth_stencil_buffer_desc.Format;
        depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        ThrowIfFailedGfx(
            gfx.device().CreateDepthStencilView(depth_stencil_texture.Get(), &depth_stencil_view_desc, &depth_stencil_view_),
            &gfx, __FILE__, __LINE__);
    }
}