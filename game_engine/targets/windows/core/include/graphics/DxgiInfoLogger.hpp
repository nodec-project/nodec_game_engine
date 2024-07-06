#pragma once

#include <mutex>

#include <nodec/logging/logging.hpp>
#include <nodec/macros.hpp>

#include <dxgidebug.h>
#include <wrl.h>

class DxgiInfoLogger {
public:
    DxgiInfoLogger();
    ~DxgiInfoLogger() = default;

    // void SetLatest() noexcept;
    // void Dump(nodec::logging::Level logLevel);
    // bool DumpIfAny(nodec::logging::Level logLevel);

    std::string Dump() noexcept;

private:
    bool GetMessages(std::ostringstream &outMessagesStream);

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    Microsoft::WRL::ComPtr<IDXGIInfoQueue> mpDxgiInfoQueue;
    unsigned long long mNext = 0;

private:
    NODEC_DISABLE_COPY(DxgiInfoLogger)
};