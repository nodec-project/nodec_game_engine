#ifndef NODEC_GAME_ENGINE__GRAPHICS__GRAPHICS_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__GRAPHICS_HPP_

// #include "DxgiInfoLogger.hpp"

#include <nodec/formatter.hpp>
#include <nodec/logging/logging.hpp>
#include <nodec/macros.hpp>

// Prevent to define min/max macro in windows api.
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <wrl.h>

#include <stdexcept>

// Forward declaration
class GraphicsDevice;

class Graphics {
public:
    // Original constructor (creates its own device, manages ImGui)
    Graphics(HWND hWnd, int width, int height);

    // New constructor (uses external shared device, no ImGui management)
    // When using shared device, ImGui should be managed externally (e.g., by EditorWindow)
    Graphics(HWND hWnd, int width, int height, GraphicsDevice& shared_device, bool manage_imgui = false);

    ~Graphics();

    void begin_frame() noexcept;
    void end_frame();

    // Check if this Graphics instance manages ImGui
    bool manages_imgui() const noexcept { return manages_imgui_; }

    void DrawIndexed(UINT count);

    ID3D11Device &device() noexcept {
        return *device_.Get();
    }
    ID3D11DeviceContext &context() noexcept {
        return *context_.Get();
    }

    // DxgiInfoLogger &info_logger() noexcept {
    //     return mInfoLogger;
    // };

    ID3D11RenderTargetView &render_target_view() noexcept {
        return *render_target_view_.Get();
    }

    UINT width() const noexcept {
        return width_;
    };
    UINT height() const noexcept {
        return height_;
    };

private:
    void init_common(int width, int height);

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    UINT width_;
    UINT height_;

    // Device ownership flag
    bool owns_device_{true};

    // ImGui management flag
    bool manages_imgui_{true};

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> render_target_texture_;

private:
    NODEC_DISABLE_COPY(Graphics)
};

inline void ThrowIfFailedGfx(const std::string &type, HRESULT hr, Graphics *pGfx, const char *file, size_t line) {
    using namespace nodec;
    if (FAILED(hr)) {
        // const auto logs = pGfx->info_logger().Dump();

        throw std::runtime_error(
            ErrorFormatter<std::runtime_error>(file, line)
            << "GraphicsError::" << type << " [Error Code] 0x" << std::hex << std::uppercase << hr << std::dec
            << " (" << (unsigned long)hr << ")"
            // << "Last Dxgi Debug Logs:\n"
            // << logs
        );
    }
}

inline void ThrowIfFailedGfx(HRESULT hr, Graphics *pGfx, const char *file, size_t line) {
    ThrowIfFailedGfx("", hr, pGfx, file, line);
}

#endif