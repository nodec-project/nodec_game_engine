#include <graphics/graphics.hpp>
#include <graphics/graphics_device.hpp>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3dcompiler.h>

#include <sstream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

namespace wrl = Microsoft::WRL;

// About usage of D3D11
//  * https://docs.microsoft.com/en-us/windows/win32/direct3d11/dx-graphics-overviews

Graphics::Graphics(HWND hWnd, int width, int height)
    : width_(width), height_(height), logger_(nodec::logging::get_logger("engine.graphics")) {
    // ThrowIfFailedGfx(
    //     D3D11CreateDevice(
    //         nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION,
    //         &device_, nullptr, &context_),
    //     this, __FILE__, __LINE__);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0; // the refresh rate 0(denominator)/0(numerator)
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2; // フリップモデルではBufferCountを2以上に設定
    sd.OutputWindow = hWnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップモデルのスワップエフェクトに変更
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT swapCreateFlags = 0u;

#ifdef _DEBUG
    swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // create device and front/back buffers, and swap chain and rendering context.
    ThrowIfFailedGfx(D3D11CreateDeviceAndSwapChain(
                         nullptr,                  // video adapter: nullptr=default adapter
                         D3D_DRIVER_TYPE_HARDWARE, // driver type
                         nullptr,                  // Needed if D3D_DRIVER_TYPE_SOFTWARE is enabled
                         swapCreateFlags,          // flag
                         nullptr,
                         0,
                         D3D11_SDK_VERSION,
                         &sd,
                         &swap_chain_,
                         &device_,
                         nullptr,
                         &context_),
                     this, __FILE__, __LINE__);

    // Create MSAA render target
    wrl::ComPtr<ID3D11Texture2D> msaa_texture;
    D3D11_TEXTURE2D_DESC msaa_tex_desc = {};
    msaa_tex_desc.Width = width;
    msaa_tex_desc.Height = height;
    msaa_tex_desc.MipLevels = 1;
    msaa_tex_desc.ArraySize = 1;
    msaa_tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    msaa_tex_desc.SampleDesc.Count = 4; // 4xMSAA
    msaa_tex_desc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
    msaa_tex_desc.Usage = D3D11_USAGE_DEFAULT;
    msaa_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    ThrowIfFailedGfx(
        device_->CreateTexture2D(&msaa_tex_desc, nullptr, &msaa_texture),
        this, __FILE__, __LINE__);
    render_target_texture_ = msaa_texture;

    wrl::ComPtr<ID3D11RenderTargetView> msaa_render_target_view;
    ThrowIfFailedGfx(
        device_->CreateRenderTargetView(msaa_texture.Get(), nullptr, &msaa_render_target_view),
        this, __FILE__, __LINE__);
    render_target_view_ = msaa_render_target_view;

    // wrl::ComPtr<ID3D11Texture2D> msaa_depth_stencil;
    // D3D11_TEXTURE2D_DESC msaa_depth_desc = {};
    // msaa_depth_desc.Width = width;
    // msaa_depth_desc.Height = height;
    // msaa_depth_desc.MipLevels = 1;
    // msaa_depth_desc.ArraySize = 1;
    // msaa_depth_desc.Format = DXGI_FORMAT_D32_FLOAT;
    // msaa_depth_desc.SampleDesc.Count = 4;
    // msaa_depth_desc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
    // msaa_depth_desc.Usage = D3D11_USAGE_DEFAULT;
    // msaa_depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    // ThrowIfFailedGfx(
    //     device_->CreateTexture2D(&msaa_depth_desc, nullptr, &msaa_depth_stencil),
    //     this, __FILE__, __LINE__);

    // gain access to texture subresource in swap chain (back buffer)
    wrl::ComPtr<ID3D11Texture2D> back_buffer;
    ThrowIfFailedGfx(swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer), this, __FILE__, __LINE__);
    back_buffer_ = back_buffer;

    // ThrowIfFailedGfx(device_->CreateRenderTargetView(back_buffer.Get(), nullptr, &render_target_view_), this, __FILE__, __LINE__);

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    wrl::ComPtr<ID3D11DepthStencilState> pDSState;
    ThrowIfFailedGfx(device_->CreateDepthStencilState(&dsDesc, &pDSState), this, __FILE__, __LINE__);

    // bind depth state
    context_->OMSetDepthStencilState(pDSState.Get(), 1u);

    // // create depth stencil texture
    // wrl::ComPtr<ID3D11Texture2D> pDepthStencil;
    // D3D11_TEXTURE2D_DESC descDepth = {};
    // descDepth.Width = width;
    // descDepth.Height = height;
    // descDepth.MipLevels = 1u;
    // descDepth.ArraySize = 1u;
    // descDepth.Format = DXGI_FORMAT_D32_FLOAT;
    // descDepth.SampleDesc.Count = 1u;
    // descDepth.SampleDesc.Quality = 0u;
    // descDepth.Usage = D3D11_USAGE_DEFAULT;
    // descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    // ThrowIfFailedGfx(device_->CreateTexture2D(&descDepth, nullptr, &pDepthStencil), this, __FILE__, __LINE__);

    // configure viewport
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    context_->RSSetViewports(1u, &vp);

    // init imgui d3d impl
    ImGui_ImplDX11_Init(device_.Get(), context_.Get());

    logger_->info(__FILE__, __LINE__) << "Successfully initialized.";
}

// Constructor using shared device
Graphics::Graphics(HWND hWnd, int width, int height, GraphicsDevice& shared_device, bool manage_imgui)
    : width_(width), height_(height), logger_(nodec::logging::get_logger("engine.graphics")),
      owns_device_(false), manages_imgui_(manage_imgui) {

    // Use shared device and context (don't take ownership)
    device_ = shared_device.device();
    context_ = shared_device.context();

    // Create SwapChain for this window using the shared device
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = hWnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailedGfx(
        shared_device.dxgi_factory()->CreateSwapChain(device_.Get(), &sd, &swap_chain_),
        this, __FILE__, __LINE__);

    // Create MSAA render target
    wrl::ComPtr<ID3D11Texture2D> msaa_texture;
    D3D11_TEXTURE2D_DESC msaa_tex_desc = {};
    msaa_tex_desc.Width = width;
    msaa_tex_desc.Height = height;
    msaa_tex_desc.MipLevels = 1;
    msaa_tex_desc.ArraySize = 1;
    msaa_tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    msaa_tex_desc.SampleDesc.Count = 4;
    msaa_tex_desc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
    msaa_tex_desc.Usage = D3D11_USAGE_DEFAULT;
    msaa_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    ThrowIfFailedGfx(
        device_->CreateTexture2D(&msaa_tex_desc, nullptr, &msaa_texture),
        this, __FILE__, __LINE__);
    render_target_texture_ = msaa_texture;

    wrl::ComPtr<ID3D11RenderTargetView> msaa_render_target_view;
    ThrowIfFailedGfx(
        device_->CreateRenderTargetView(msaa_texture.Get(), nullptr, &msaa_render_target_view),
        this, __FILE__, __LINE__);
    render_target_view_ = msaa_render_target_view;

    // Get back buffer
    wrl::ComPtr<ID3D11Texture2D> back_buffer;
    ThrowIfFailedGfx(swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer), this, __FILE__, __LINE__);
    back_buffer_ = back_buffer;

    // Setup depth stencil state
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    wrl::ComPtr<ID3D11DepthStencilState> pDSState;
    ThrowIfFailedGfx(device_->CreateDepthStencilState(&dsDesc, &pDSState), this, __FILE__, __LINE__);
    context_->OMSetDepthStencilState(pDSState.Get(), 1u);

    // Configure viewport
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    context_->RSSetViewports(1u, &vp);

    // init imgui d3d impl only if this instance manages ImGui
    if (manages_imgui_) {
        ImGui_ImplDX11_Init(device_.Get(), context_.Get());
    }

    logger_->info(__FILE__, __LINE__) << "Successfully initialized with shared device. manages_imgui=" << manages_imgui_;
}

Graphics::~Graphics() {
    if (manages_imgui_) {
        ImGui_ImplDX11_Shutdown();
    }

    logger_->info(__FILE__, __LINE__) << "End Graphics.";
}

void Graphics::begin_frame() noexcept {
    if (manages_imgui_) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }
}

void Graphics::end_frame() {
    if (manages_imgui_) {
        // ImGuiのレンダリングをMSAAレンダーターゲットに対して行う
        ImGui::Render();

        // ImGuiをMSAAレンダーターゲットに描画
        // レンダーターゲットを明示的に設定
        ID3D11RenderTargetView* rtv = render_target_view_.Get();
        context_->OMSetRenderTargets(1, &rtv, nullptr);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    // Always resolve MSAA render target to back buffer (for both ImGui and non-ImGui modes)
    context_->ResolveSubresource(
        back_buffer_.Get(), 0u,
        render_target_texture_.Get(), 0u,
        DXGI_FORMAT_B8G8R8A8_UNORM);

    // Always present the swap chain
    HRESULT hr;
    if (FAILED(hr = swap_chain_->Present(1u, 0u))) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED) {
            ThrowIfFailedGfx("DeviceRemoved", hr, this, __FILE__, __LINE__);
        } else {
            ThrowIfFailedGfx(hr, this, __FILE__, __LINE__);
        }
    }
}

void Graphics::DrawIndexed(UINT count) {
    context_->DrawIndexed(count, 0u, 0u);

    // NOTE: The following code is too heavy to run for each model.
    // const auto logs = mInfoLogger.Dump();
    // if (!logs.empty()) {
    //    nodec::logging::WarnStream(__FILE__, __LINE__)
    //        << "[Graphics::DrawIndexed] >>> DXGI debug having messages:\n"
    //        << logs;
    //}
}
