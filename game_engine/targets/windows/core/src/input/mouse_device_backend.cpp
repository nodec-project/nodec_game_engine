#include <input/mouse_device_backend.hpp>

#include <Windows.h>

void MouseDeviceBackend::warp_cursor_position(const nodec::Vector2f &position) {
    POINT point;
    point.x = static_cast<LONG>(position.x);
    point.y = static_cast<LONG>(position.y);
    ::SetCursorPos(point.x, point.y);
}

void MouseDeviceBackend::set_cursor_visible(bool visible) {
    if (is_cursor_visible_ == visible) {
        return;
    }
    ::ShowCursor(visible);
    is_cursor_visible_ = visible;
}