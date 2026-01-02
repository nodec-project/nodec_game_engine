#ifndef NODEC_GAME_ENGINE__GRAPHICS__GRAPHICS_DEVICE_HPP_
#define NODEC_GAME_ENGINE__GRAPHICS__GRAPHICS_DEVICE_HPP_

#include <nodec/logging/logging.hpp>
#include <nodec/macros.hpp>

#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <wrl.h>

#include <stdexcept>

/**
 * @brief Shared D3D11 Device and Context manager.
 *
 * This class owns the D3D11 device and context that can be shared
 * between multiple windows (GameWindow, EditorWindow).
 */
class GraphicsDevice {
public:
    GraphicsDevice();
    ~GraphicsDevice();

    ID3D11Device* device() noexcept {
        return device_.Get();
    }

    ID3D11DeviceContext* context() noexcept {
        return context_.Get();
    }

    IDXGIFactory* dxgi_factory() noexcept {
        return dxgi_factory_.Get();
    }

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGIFactory> dxgi_factory_;
    bool com_initialized_{false};  // Track if we initialized COM

private:
    NODEC_DISABLE_COPY(GraphicsDevice)
};

#endif
