#include "graphics/DxgiInfoLogger.hpp"
#include "graphics/graphics.hpp"
#include <Exceptions.hpp>

#include <nodec/formatter.hpp>

#include <dxgidebug.h>

#include <sstream>
#include <stdexcept>

#pragma comment(lib, "dxguid.lib")

DxgiInfoLogger::DxgiInfoLogger()
    : logger_(nodec::logging::get_logger("engine.graphics.dxgi-info-logger")) {
    using namespace Exceptions;

    // define function signature of DXGIGetDebugInterface
    typedef HRESULT(WINAPI * DXGIGetDebugInterface)(REFIID, void **);

    // load the dll that contains the function DXGIGetDebugInterface
    const auto hModDxgiDebug = LoadLibraryEx(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (hModDxgiDebug == nullptr) {
        throw std::runtime_error(nodec::ErrorFormatter<std::runtime_error>(__FILE__, __LINE__)
                                 << "Failed to load the dll 'dxgidebug.dll'");
    }

    // get address of DXGIGetDebugInterface in dll
    const auto DxgiGetDebugInterface = reinterpret_cast<DXGIGetDebugInterface>(
        reinterpret_cast<void *>(GetProcAddress(hModDxgiDebug, "DXGIGetDebugInterface")));
    if (DxgiGetDebugInterface == nullptr) {
        throw std::runtime_error(nodec::ErrorFormatter<std::runtime_error>(__FILE__, __LINE__)
                                 << "Failed to get address of DXGIGetDebugInterface in dll 'dxgidebug.dll'");
    }

    ThrowIfFailed(DxgiGetDebugInterface(__uuidof(IDXGIInfoQueue), &mpDxgiInfoQueue), __FILE__, __LINE__);

    logger_->info(__FILE__, __LINE__) << "Successfully initialized.";
}
//
// void DxgiInfoLogger::SetLatest() noexcept {
//    // set the index (next) so that the next all to
//    // Dump() will only get errors generated after this call.
//    next = mpDxgiInfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);
//}

// bool DxgiInfoLogger::DumpIfAny(nodec::logging::Level logLevel) {
//     std::ostringstream oss;
//
//     oss << "[DxgiInfoLogger] >>> Dump the messsages.\n"
//         << "=== Messages ===\n";
//     if (!GetMessages(oss)) {
//         return false;
//     }
//     oss << "END Messages ===\n";
//
//     nodec::logging::log(logLevel, oss.str(), __FILE__, __LINE__);
//
//     return true;
// }

std::string DxgiInfoLogger::Dump() noexcept {
    std::ostringstream oss;
    GetMessages(oss);
    return oss.str();
}

// void DxgiInfoLogger::Dump(nodec::logging::Level logLevel) {
//     std::ostringstream oss;
//
//     oss << "[DxgiInfoLogger] >>> Dump the messsages.\n"
//         << "=== Messages ===\n";
//     GetMessages(oss);
//     oss << "END Messages ===\n";
//
//     nodec::logging::log(logLevel, oss.str(), __FILE__, __LINE__);
// }

bool DxgiInfoLogger::GetMessages(std::ostringstream &outMessagesStream) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);

    const auto end = mpDxgiInfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);

    if (mNext == end) {
        return false;
    }

    for (; mNext < end; ++mNext) {
        HRESULT hr;
        SIZE_T messageLength{};

        // get the size of message in bytes
        if (FAILED(hr = mpDxgiInfoQueue->GetMessage(DXGI_DEBUG_ALL, mNext, nullptr, &messageLength))) {
            outMessagesStream << "[ERROR] cannnot get the size of message '" << mNext << "': hr=0x"
                              << std::hex << std::uppercase << hr << std::dec << "\n";
            continue;
        }
        // allocate memory for message
        auto bytes = std::make_unique<byte[]>(messageLength);
        auto pMessage = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE *>(bytes.get());
        // get the message and log its description
        if (FAILED(hr = mpDxgiInfoQueue->GetMessage(DXGI_DEBUG_ALL, mNext, pMessage, &messageLength))) {
            outMessagesStream << "[ERROR] cannnot get the size of message '" << mNext << "': hr=0x"
                              << std::hex << std::uppercase << hr << std::dec << "\n";
            continue;
        }
        outMessagesStream << pMessage->pDescription << "\n";
    }
    return true;
}