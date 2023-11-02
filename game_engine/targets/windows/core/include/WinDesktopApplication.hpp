#pragma once

#include <memory>

#include <Windows.h>

#include <nodec/logging/logging.hpp>
#include <nodec_application/impl/application_impl.hpp>

#include "Window.hpp"
#include "logging.hpp"

class WinDesktopApplication : public nodec_application::impl::ApplicationImpl {
public:
    WinDesktopApplication() {}
    virtual ~WinDesktopApplication() {}

    int run() {
        try {
            return main();
        } catch (...) {
            return on_error_exit();
        }
    }

protected:
    virtual void setup() = 0;
    virtual void loop() = 0;

private:
    int main() {
        // --- Init Logging ---
        init_logging(nodec::logging::Level::Debug);

        //
        //#ifndef NDEBUG
        //    InitLogging(nodec::logging::Level::Debug);
        //#else
        //    InitLogging(nodec::logging::Level::Info);
        //#endif
        // end Init Logging ---

        nodec::logging::info(__FILE__, __LINE__) << "Hello world. Application start.";

        setup();

        int exitCode;
        while (true) {
            if (!Window::ProcessMessages(exitCode)) {
                break;
            }

            loop();
        }

        nodec::logging::info(__FILE__, __LINE__) << "Application Successfully Ending. See you.";

        return exitCode;
    }

    int on_error_exit() {
        try {
            throw;
        } catch (const std::exception &e) {
            nodec::logging::fatal(e.what(), __FILE__, __LINE__);
        } catch (...) {
            nodec::logging::fatal("Unknown Error Exception Occurs.", __FILE__, __LINE__);
        }

        MessageBox(nullptr,
                   L"Unhandled Exception has been caught in main loop. \nFor more detail, Please check the 'output.log'.",
                   L"Fatal Error.", MB_OK | MB_ICONEXCLAMATION);

        nodec::logging::warn(__FILE__, __LINE__)
            << "Unexpected Program Ending.";

        return -1;
    }
};