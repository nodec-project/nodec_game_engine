#include <graphics/graphics_device.hpp>

#include <nodec/formatter.hpp>

#include <objbase.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace {

void ThrowIfFailed(HRESULT hr, const char* file, size_t line, const char* message = "") {
    if (FAILED(hr)) {
        throw std::runtime_error(
            nodec::ErrorFormatter<std::runtime_error>(file, line)
            << "GraphicsDevice Error: " << message
            << " [Error Code] 0x" << std::hex << std::uppercase << hr << std::dec
            << " (" << (unsigned long)hr << ")"
        );
    }
}

} // namespace

GraphicsDevice::GraphicsDevice()
    : logger_(nodec::logging::get_logger("engine.graphics_device")) {

    // Initialize COM for XAudio2 and other COM-based services
    // COINIT_MULTITHREADED is required for XAudio2's callback threading model
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        // RPC_E_CHANGED_MODE means COM was already initialized with different concurrency model
        // which is acceptable - we just need COM to be initialized
        ThrowIfFailed(hr, __FILE__, __LINE__, "Failed to initialize COM");
    }
    com_initialized_ = (hr != RPC_E_CHANGED_MODE);

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL featureLevel;

    // Create D3D11 Device without SwapChain
    ThrowIfFailed(
        D3D11CreateDevice(
            nullptr,                    // Default adapter
            D3D_DRIVER_TYPE_HARDWARE,   // Hardware driver
            nullptr,                    // No software rasterizer
            createDeviceFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &device_,
            &featureLevel,
            &context_
        ),
        __FILE__, __LINE__, "Failed to create D3D11 device"
    );

    // Get DXGI Factory for creating SwapChains later
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    ThrowIfFailed(
        device_->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice),
        __FILE__, __LINE__, "Failed to get DXGI device"
    );

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    ThrowIfFailed(
        dxgiDevice->GetAdapter(&dxgiAdapter),
        __FILE__, __LINE__, "Failed to get DXGI adapter"
    );

    ThrowIfFailed(
        dxgiAdapter->GetParent(__uuidof(IDXGIFactory), &dxgi_factory_),
        __FILE__, __LINE__, "Failed to get DXGI factory"
    );

    logger_->info(__FILE__, __LINE__) << "GraphicsDevice created successfully. Feature Level: 0x"
                                       << std::hex << featureLevel;
}

GraphicsDevice::~GraphicsDevice() {
    logger_->info(__FILE__, __LINE__) << "GraphicsDevice destroyed.";

    // Uninitialize COM if we initialized it
    if (com_initialized_) {
        CoUninitialize();
    }
}
