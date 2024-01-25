#ifndef NODEC_GAME_ENGINE__INPUT_MOUSE_DEVICE_BACKEND_HPP_
#define NODEC_GAME_ENGINE__INPUT_MOUSE_DEVICE_BACKEND_HPP_

#include <nodec_input/mouse/impl/mouse_device.hpp>

class MouseDeviceBackend : public nodec_input::mouse::impl::MouseDevice {
public:
    void warp_cursor_position(const nodec::Vector2f &position) override;
    void set_cursor_visible(bool visible) override;
};

#endif