#include "editor_window.hpp"

#include <nodec/formatter.hpp>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

void ThrowIfFailed(HRESULT hr, const char* file, size_t line, const char* message = "") {
    if (FAILED(hr)) {
        throw std::runtime_error(
            nodec::ErrorFormatter<std::runtime_error>(file, line)
            << "EditorWindow Error: " << message
            << " [Error Code] 0x" << std::hex << std::uppercase << hr << std::dec
            << " (" << (unsigned long)hr << ")"
        );
    }
}

} // namespace

// --- WindowClass singleton ---
EditorWindow::WindowClass EditorWindow::WindowClass::wndClass;

EditorWindow::WindowClass::WindowClass()
    : hInst(GetModuleHandle(nullptr)) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = HandleMsgSetup;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetInstance();
    wc.hIcon = nullptr;
    wc.hCursor = nullptr;
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = GetName();
    wc.hIconSm = nullptr;

    RegisterClassEx(&wc);
}

EditorWindow::WindowClass::~WindowClass() {
    UnregisterClass(wndClassName, GetInstance());
}

const wchar_t* EditorWindow::WindowClass::GetName() noexcept {
    return wndClassName;
}

HINSTANCE EditorWindow::WindowClass::GetInstance() noexcept {
    return wndClass.hInst;
}

// --- EditorWindow ---
EditorWindow::EditorWindow(GraphicsDevice& graphics_device, int width, int height, const wchar_t* title)
    : logger_(nodec::logging::get_logger("editor.window"))
    , graphics_device_(graphics_device)
    , width_(width)
    , height_(height) {

    // Calculate window rect for desired client size
    RECT wr = {};
    wr.left = 100;
    wr.right = width + wr.left;
    wr.top = 100;
    wr.bottom = height + wr.top;
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    // Create Win32 window
    hwnd_ = CreateWindow(
        WindowClass::GetName(),
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr,
        nullptr,
        WindowClass::GetInstance(),
        this
    );

    if (!hwnd_) {
        throw std::runtime_error("Failed to create EditorWindow");
    }

    // Create SwapChain and render target
    create_swap_chain();
    create_render_target();

    // Initialize ImGui for this window
    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(graphics_device_.device(), graphics_device_.context());

    // Show the window
    ShowWindow(hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(hwnd_);

    logger_->info(__FILE__, __LINE__) << "EditorWindow created: " << width << "x" << height;
}

EditorWindow::~EditorWindow() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    cleanup_render_target();

    if (hwnd_) {
        DestroyWindow(hwnd_);
    }

    logger_->info(__FILE__, __LINE__) << "EditorWindow destroyed.";
}

void EditorWindow::create_swap_chain() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = width_;
    sd.BufferDesc.Height = height_;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = hwnd_;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(
        graphics_device_.dxgi_factory()->CreateSwapChain(
            graphics_device_.device(),
            &sd,
            &swap_chain_
        ),
        __FILE__, __LINE__, "Failed to create SwapChain for EditorWindow"
    );
}

void EditorWindow::create_render_target() {
    // Get back buffer
    ThrowIfFailed(
        swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer_),
        __FILE__, __LINE__, "Failed to get back buffer"
    );

    // Create MSAA render target texture
    D3D11_TEXTURE2D_DESC msaa_desc = {};
    msaa_desc.Width = width_;
    msaa_desc.Height = height_;
    msaa_desc.MipLevels = 1;
    msaa_desc.ArraySize = 1;
    msaa_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    msaa_desc.SampleDesc.Count = 4;  // 4x MSAA
    msaa_desc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
    msaa_desc.Usage = D3D11_USAGE_DEFAULT;
    msaa_desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ThrowIfFailed(
        graphics_device_.device()->CreateTexture2D(&msaa_desc, nullptr, &msaa_texture_),
        __FILE__, __LINE__, "Failed to create MSAA texture"
    );

    ThrowIfFailed(
        graphics_device_.device()->CreateRenderTargetView(msaa_texture_.Get(), nullptr, &msaa_render_target_view_),
        __FILE__, __LINE__, "Failed to create MSAA render target view"
    );

    // Also create non-MSAA render target for back buffer
    ThrowIfFailed(
        graphics_device_.device()->CreateRenderTargetView(back_buffer_.Get(), nullptr, &render_target_view_),
        __FILE__, __LINE__, "Failed to create render target view"
    );
}

void EditorWindow::cleanup_render_target() {
    render_target_view_.Reset();
    msaa_render_target_view_.Reset();
    msaa_texture_.Reset();
    back_buffer_.Reset();
}

void EditorWindow::begin_frame() noexcept {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void EditorWindow::end_frame() {
    ImGui::Render();

    // Set MSAA render target and clear
    ID3D11RenderTargetView* rtv = msaa_render_target_view_.Get();
    graphics_device_.context()->OMSetRenderTargets(1, &rtv, nullptr);

    const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    graphics_device_.context()->ClearRenderTargetView(msaa_render_target_view_.Get(), clear_color);

    // Render ImGui
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Resolve MSAA to back buffer
    graphics_device_.context()->ResolveSubresource(
        back_buffer_.Get(), 0,
        msaa_texture_.Get(), 0,
        DXGI_FORMAT_B8G8R8A8_UNORM
    );

    // Present
    HRESULT hr = swap_chain_->Present(1, 0);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED) {
            ThrowIfFailed(hr, __FILE__, __LINE__, "Device removed");
        } else {
            ThrowIfFailed(hr, __FILE__, __LINE__, "Present failed");
        }
    }
}

// --- Message handlers ---
LRESULT CALLBACK EditorWindow::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    if (msg == WM_NCCREATE) {
        const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        EditorWindow* const pWnd = static_cast<EditorWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&EditorWindow::HandleMsgThunk));
        return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK EditorWindow::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    EditorWindow* const pWnd = reinterpret_cast<EditorWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT EditorWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    // Let ImGui handle messages first
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg) {
    case WM_CLOSE:
        destroyed_signal_(*this);
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (swap_chain_ && wParam != SIZE_MINIMIZED) {
            cleanup_render_target();
            swap_chain_->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            width_ = LOWORD(lParam);
            height_ = HIWORD(lParam);
            create_render_target();
        }
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
