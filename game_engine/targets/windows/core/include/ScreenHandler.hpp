#pragma once

#include "Window.hpp"

#include <nodec_screen/impl/screen_module.hpp>

#include <nodec/logging/logging.hpp>
#include <nodec/macros.hpp>
#include <nodec/signals/signal.hpp>

#include <cassert>

class ScreenHandler {
    using ScreenModule = nodec_screen::impl::ScreenModule;

public:
    ScreenHandler(ScreenModule *pScreenModule)
        : mpScreenModule{pScreenModule} {
    }

    void Setup(Window *pWindow) {
        mpWindow = pWindow;
    }

public:
    void BindHandlersOnBoot() {
        using namespace nodec;
        mResolutionChangedConnection = mpScreenModule->resolution_changed().connect(
            [&](ScreenModule &screen, const Vector2i &resolution) {
                screen.internal_resolution = resolution;
            });

        mSizeChangedConnection = mpScreenModule->size_changed().connect(
            [&](ScreenModule &screen, const Vector2i &size) {
                screen.internal_size = size;
            });

        mTitleChangedConnection = mpScreenModule->title_changed().connect(
            [&](ScreenModule &screen, const std::string &title) {
                screen.internal_title = title;
            });
    }

    void BindHandlersOnRuntime() {
        assert(mpWindow);

        mResolutionChangedConnection.disconnect();
        mSizeChangedConnection.disconnect();

        mTitleChangedConnection = mpScreenModule->title_changed().connect(
            [&](ScreenModule &screen, const std::string &title) {
                try {
                    mpWindow->SetTitle(title);
                    screen.internal_title = title;
                } catch (const std::exception &e) {
                    nodec::logging::error(__FILE__, __LINE__)
                        << "[ScreenHandlers] >>> Exception has been occurred while Window::SetTitle().\n"
                        << "detail: " << e.what();
                } catch (...) {
                    nodec::logging::error(__FILE__, __LINE__)
                        << "[ScreenHandlers] >>> Unknown Exception has been occurred while Window::SetTitle().\n"
                        << "detail: Unavailable.";
                }
            });
    }

private:
    ScreenModule *mpScreenModule;

    Window *mpWindow{nullptr};

    nodec::signals::Connection mResolutionChangedConnection;
    nodec::signals::Connection mSizeChangedConnection;
    nodec::signals::Connection mTitleChangedConnection;

private:
    NODEC_DISABLE_COPY(ScreenHandler)
};