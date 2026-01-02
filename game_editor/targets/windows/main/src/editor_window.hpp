#ifndef NODEC_GAME_EDITOR__EDITOR_WINDOW_HPP_
#define NODEC_GAME_EDITOR__EDITOR_WINDOW_HPP_

#include <graphics/graphics_device.hpp>

#include <nodec/logging/logging.hpp>
#include <nodec/macros.hpp>
#include <nodec/signals/signal.hpp>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <wrl.h>

#include <memory>
#include <string>

/**
 * @brief Editor-dedicated Win32 window with ImGui rendering support.
 *
 * This window is separate from the Game window and handles all ImGui-based
 * editor UI rendering using a shared D3D11 device.
 */
class EditorWindow {
public:
    EditorWindow(GraphicsDevice& graphics_device, int width, int height, const wchar_t* title);
    ~EditorWindow();

    // ImGui frame management
    void begin_frame() noexcept;
    void end_frame();

    // Window properties
    HWND hwnd() const noexcept { return hwnd_; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }

    // Signals
    using WindowSignal = nodec::signals::Signal<void(EditorWindow&)>;
    decltype(auto) destroyed() { return destroyed_signal_.signal_interface(); }

    // For rendering game texture in ImGui
    ID3D11Device* device() noexcept { return graphics_device_.device(); }
    ID3D11DeviceContext* context() noexcept { return graphics_device_.context(); }

private:
    // Win32 window class (singleton)
    class WindowClass {
    public:
        static const wchar_t* GetName() noexcept;
        static HINSTANCE GetInstance() noexcept;
    private:
        WindowClass();
        ~WindowClass();
        static constexpr const wchar_t* wndClassName = L"Nodec Editor Window";
        static WindowClass wndClass;
        HINSTANCE hInst;
        NODEC_DISABLE_COPY(WindowClass)
    };

    // Win32 message handlers
    static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

    void create_swap_chain();
    void create_render_target();
    void cleanup_render_target();

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    GraphicsDevice& graphics_device_;

    HWND hwnd_;
    int width_;
    int height_;

    // DirectX resources (SwapChain owned by this window)
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer_;

    // MSAA support
    Microsoft::WRL::ComPtr<ID3D11Texture2D> msaa_texture_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> msaa_render_target_view_;

    WindowSignal destroyed_signal_;

    NODEC_DISABLE_COPY(EditorWindow)
};

#endif
