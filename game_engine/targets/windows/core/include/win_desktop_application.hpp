#ifndef NODEC_GAME_ENGINE__WIN_DESKTOP_APPLICATION_HPP_
#define NODEC_GAME_ENGINE__WIN_DESKTOP_APPLICATION_HPP_

#include <memory>

#include <Windows.h>

#include <nodec/asyncio/event_loop.hpp>
#include <nodec/logging/logging.hpp>
#include <nodec_application/impl/application_impl.hpp>

#include "logging.hpp"
#include "window.hpp"

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

    nodec::asyncio::EventLoop &event_loop() {
        return event_loop_;
    }

protected:
    virtual void setup() = 0;
    
private:
    nodec::asyncio::EventLoop event_loop_;

    int main() {
        // --- Init Logging ---
        init_logging(nodec::logging::Level::Debug);

        nodec::logging::info(__FILE__, __LINE__) << "Hello world. Application start.";

        setup();

        event_loop_.spin();

        nodec::logging::info(__FILE__, __LINE__) << "Application Successfully Ending. See you.";

        return 0;
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

#endif