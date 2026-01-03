#include <rendering/scene_rendering_context.hpp>

SceneRenderingContext::SceneRenderingContext(
    std::uint32_t target_width,
    std::uint32_t target_height, Graphics &gfx)
    : gfx_(gfx), target_width_(target_width), target_height_(target_height) {
    // Create target buffers for camera color output (ping-pong for PostProcessing)
    target_buffer_ = std::make_unique<GeometryBuffer>(&gfx_, target_width, target_height);
    target_buffer_back_ = std::make_unique<GeometryBuffer>(&gfx_, target_width, target_height);

    // Register target buffer SRV for shader access
    shader_resource_views_["$target"] = &target_buffer_->shader_resource_view();
    shader_resource_views_["$target_back"] = &target_buffer_back_->shader_resource_view();

    // Register legacy "screen" alias for PostProcessing compatibility
    // "screen" always points to target_buffer_back (the input for PostProcess effects)
    shader_resource_views_["screen"] = &target_buffer_back_->shader_resource_view();

    {
        // Generate the depth stencil buffer texture.
        D3D11_TEXTURE2D_DESC depth_stencil_buffer_desc{};
        depth_stencil_buffer_desc.Width = target_width;
        depth_stencil_buffer_desc.Height = target_height;
        depth_stencil_buffer_desc.MipLevels = 1;
        depth_stencil_buffer_desc.ArraySize = 1;
        depth_stencil_buffer_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
        depth_stencil_buffer_desc.SampleDesc.Count = 1;
        depth_stencil_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_stencil_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        ThrowIfFailedGfx(
            gfx.device().CreateTexture2D(&depth_stencil_buffer_desc, nullptr, &depth_stencil_texture_),
            &gfx, __FILE__, __LINE__);

        D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
        depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        ThrowIfFailedGfx(
            gfx.device().CreateDepthStencilView(depth_stencil_texture_.Get(), &depth_stencil_view_desc, &depth_stencil_view_),
            &gfx, __FILE__, __LINE__);

        D3D11_SHADER_RESOURCE_VIEW_DESC depth_stencil_srv_desc{};
        depth_stencil_srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        depth_stencil_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        depth_stencil_srv_desc.Texture2D.MipLevels = 1;
        ThrowIfFailedGfx(
            gfx.device().CreateShaderResourceView(depth_stencil_texture_.Get(), &depth_stencil_srv_desc, &depth_stencil_srv_),
            &gfx, __FILE__, __LINE__);

        shader_resource_views_["$depth"] = depth_stencil_srv_.Get();
    }
}

GeometryBuffer &SceneRenderingContext::target_buffer() {
    return *target_buffer_;
}

GeometryBuffer &SceneRenderingContext::target_buffer_back() {
    return *target_buffer_back_;
}

void SceneRenderingContext::swap_target_buffers() {
    std::swap(target_buffer_, target_buffer_back_);

    // Update all SRV pointers to reflect new buffer ownership
    shader_resource_views_["$target"] = &target_buffer_->shader_resource_view();
    shader_resource_views_["$target_back"] = &target_buffer_back_->shader_resource_view();

    // Update legacy "screen" alias - always points to target_buffer_back (input)
    shader_resource_views_["screen"] = &target_buffer_back_->shader_resource_view();
}

void SceneRenderingContext::clear_geometry_buffers() {
    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (auto &[name, buffer] : geometry_buffers_) {
        gfx_.context().ClearRenderTargetView(&buffer->render_target_view(), clear_color);
    }
}

void SceneRenderingContext::clear_depth_stencil() {
    gfx_.context().ClearDepthStencilView(
        depth_stencil_view_.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f, 0);
}

void SceneRenderingContext::clear_all(const nodec::Vector4f &clear_color) {
    // Clear target buffer
    const float color[4] = {clear_color.x, clear_color.y, clear_color.z, clear_color.w};
    gfx_.context().ClearRenderTargetView(&target_buffer_->render_target_view(), color);

    // Clear geometry buffers
    clear_geometry_buffers();

    // Clear depth stencil
    clear_depth_stencil();
}