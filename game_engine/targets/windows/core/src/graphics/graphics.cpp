#include <graphics/graphics.hpp>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3dcompiler.h>

#include <sstream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

namespace wrl = Microsoft::WRL;
// namespace dx = DirectX;

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
    // // for checking results of d3d functions.
    // mInfoLogger.SetLatest();

    // Create swap chain.

    // wrl::ComPtr<IDXGIDevice> dxgiDevice = nullptr;
    // ThrowIfFailedGfx(device_.As(&dxgiDevice), this, __FILE__, __LINE__);

    // wrl::ComPtr<IDXGIAdapter> dxgiAdapter = nullptr;
    // ThrowIfFailedGfx(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), &dxgiAdapter), this, __FILE__, __LINE__);

    // wrl::ComPtr<IDXGIFactory> dxgiFactory = nullptr;
    // ThrowIfFailedGfx(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), &dxgiFactory), this, __FILE__, __LINE__);

    // wrl::ComPtr<IDXGISwapChain> swapChain = nullptr;
    // ThrowIfFailedGfx(dxgiFactory->CreateSwapChain(
    //                      device_.Get(), // Direct3Dデバイス
    //                      &sd,           // DXGI_SWAP_CHAIN_DESCへのポインタ
    //                      &swapChain),   // 作成されたスワップチェーンへのポインタ
    //                  this, __FILE__, __LINE__);

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
                         &mpSwap,
                         &device_,
                         nullptr,
                         &context_),
                     this, __FILE__, __LINE__);

    // UINT msaa_quality;
    // device_->CheckMultisampleQualityLevels(DXGI_FORMAT_B8G8R8A8_UNORM, 4, &msaa_quality); // 4xMSAAを想定

    // if (msaa_quality < 0) {
    //     logger_->error(__FILE__, __LINE__) << "4x MSAA not supported.";
    //     throw std::runtime_error("4x MSAA not supported.");
    // } //

    // gain access to texture subresource in swap chain (back buffer)
    wrl::ComPtr<ID3D11Texture2D> pBackBuffer;

    ThrowIfFailedGfx(mpSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer), this, __FILE__, __LINE__);
    ThrowIfFailedGfx(device_->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &render_target_view_), this, __FILE__, __LINE__);

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    wrl::ComPtr<ID3D11DepthStencilState> pDSState;
    ThrowIfFailedGfx(device_->CreateDepthStencilState(&dsDesc, &pDSState), this, __FILE__, __LINE__);

    // bind depth state
    context_->OMSetDepthStencilState(pDSState.Get(), 1u);

    // create depth stensil texture
    wrl::ComPtr<ID3D11Texture2D> pDepthStencil;
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1u;
    descDepth.ArraySize = 1u;
    descDepth.Format = DXGI_FORMAT_D32_FLOAT;
    descDepth.SampleDesc.Count = 1u;
    descDepth.SampleDesc.Quality = 0u;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ThrowIfFailedGfx(device_->CreateTexture2D(&descDepth, nullptr, &pDepthStencil), this, __FILE__, __LINE__);

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

    // logger_->info(__FILE__, __LINE__)
    //     << "DXGI Debug Logs:\n"
    //     << mInfoLogger.Dump();

    logger_->info(__FILE__, __LINE__) << "Successfully initialized.";
}

Graphics::~Graphics() {
    ImGui_ImplDX11_Shutdown();

    // logger_->info(__FILE__, __LINE__)
    //     << "DXGI Debug Logs:\n"
    //     << mInfoLogger.Dump();
    logger_->info(__FILE__, __LINE__) << "End Graphics.";
}

void Graphics::begin_frame() noexcept {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Graphics::end_frame() {
    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    HRESULT hr;
    // mInfoLogger.SetLatest();
    if (FAILED(hr = mpSwap->Present(1u, 0u))) {
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
