#ifndef NODEC_GAME_ENGINE__GRAPHICS__GRAPHICS_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__GRAPHICS_HPP_

#include "DxgiInfoLogger.hpp"

#include <nodec/formatter.hpp>
#include <nodec/logging/logging.hpp>
#include <nodec/macros.hpp>

// Prevent to define min/max macro in windows api.
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <wrl.h>

#include <stdexcept>

class Graphics {
public:
    Graphics(HWND hWnd, int width, int height);
    ~Graphics();

    void begin_frame() noexcept;
    void end_frame();

    void DrawIndexed(UINT count);

    ID3D11Device &device() noexcept {
        return *device_.Get();
    }
    ID3D11DeviceContext &context() noexcept {
        return *context_.Get();
    }

    DxgiInfoLogger &info_logger() noexcept {
        return mInfoLogger;
    };

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
    UINT width_;
    UINT height_;

    DxgiInfoLogger mInfoLogger;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mpSwap;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_;
    // Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mpDSV;
    std::shared_ptr<nodec::logging::Logger> logger_;

private:
    NODEC_DISABLE_COPY(Graphics)
};

inline void ThrowIfFailedGfx(const std::string &type, HRESULT hr, Graphics *pGfx, const char *file, size_t line) {
    using namespace nodec;
    if (FAILED(hr)) {
        const auto logs = pGfx->info_logger().Dump();

        throw std::runtime_error(
            ErrorFormatter<std::runtime_error>(file, line)
            << "GraphicsError::" << type << " [Error Code] 0x" << std::hex << std::uppercase << hr << std::dec
            << " (" << (unsigned long)hr << ")\n"
            << "Last Dxgi Debug Logs:\n"
            << logs);
    }
}

inline void ThrowIfFailedGfx(HRESULT hr, Graphics *pGfx, const char *file, size_t line) {
    ThrowIfFailedGfx("", hr, pGfx, file, line);
}

#endif