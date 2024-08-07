#ifndef NODEC_GAME_ENGINE__WINDOW_HPP_
#define NODEC_GAME_ENGINE__WINDOW_HPP_

#include <array>
#include <cassert>
#include <stdexcept>

#include <Windows.h>

#include <nodec/logging/logging.hpp>
#include <nodec_input/keyboard/impl/keyboard_device.hpp>
#include <nodec_input/mouse/impl/mouse_device.hpp>

#include <nodec/formatter.hpp>
#include <nodec/macros.hpp>
#include <nodec/signals/signal.hpp>

#include "graphics/graphics.hpp"

class Window {
public:
    class HrException : public std::runtime_error {
    public:
        static std::string TranslateErrorCode(HRESULT hr) noexcept;

    public:
        HrException(HRESULT hr, const char *file, size_t line) noexcept
            : errorCode(hr), runtime_error(nodec::ErrorFormatter<HrException>(file, line)
                                           << "[Error Code] 0x" << std::hex << std::uppercase << hr << std::dec
                                           << " (" << (unsigned long)hr << ")\n"
                                           << "[Description] " << TranslateErrorCode(hr)){};

        HRESULT ErrorCode() const noexcept {
            return errorCode;
        }

    private:
        const HRESULT errorCode;
    };

private:
    /**
     * @brief Singleton class. Manage registration/cleanup of window class.
     */
    class WindowClass {
    public:
        static const wchar_t *GetName() noexcept;
        static HINSTANCE GetInstance() noexcept;

    private:
        WindowClass();
        ~WindowClass();
        static constexpr const wchar_t *wndClassName = L"Nodec Engine Window";
        static WindowClass wndClass;
        HINSTANCE hInst;

    private:
        NODEC_DISABLE_COPY(WindowClass)
    };

public:
    Window(int width, int height,
           int gfxWidth, int gfxHeight,
           const wchar_t *name,
           nodec_input::keyboard::impl::KeyboardDevice *pKeyboard,
           nodec_input::mouse::impl::MouseDevice *pMouse);

    ~Window();

public:
    static bool ProcessMessages(int &exit_code) noexcept;
    void set_title(const std::string &title);

    Graphics &graphics() {
        return *graphics_;
    }

    nodec::Vector2f position() const noexcept {
        RECT rect;
        GetWindowRect(hWnd, &rect);
        return nodec::Vector2f{(float)rect.left, (float)rect.top};
    }

    nodec::Vector2f screen_position() const noexcept {
        POINT top_left{0, 0};
        ClientToScreen(hWnd, &top_left);
        return nodec::Vector2f{(float)top_left.x, (float)top_left.y};
    }

public:
    using WindowSignal = nodec::signals::Signal<void(Window &)>;

    decltype(auto) WindowDestroyed() {
        return mWindowDestroyed.signal_interface();
    }

private:
    WindowSignal mWindowDestroyed;

private:
    static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    int mWidth;
    int mHeight;
    HWND hWnd;
    std::unique_ptr<Graphics> graphics_;
    nodec_input::keyboard::impl::KeyboardDevice *mpKeyboard;
    nodec_input::mouse::impl::MouseDevice *mpMouse;

private:
    NODEC_DISABLE_COPY(Window)
};

#endif