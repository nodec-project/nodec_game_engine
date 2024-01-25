#ifndef NODEC_GAME_ENGINE__INPUT__MOUSE_DEVICE_SYSTEM_HPP_
#define NODEC_GAME_ENGINE__INPUT__MOUSE_DEVICE_SYSTEM_HPP_

#include <nodec_input/input_devices.hpp>
#include <nodec_input/mouse/impl/mouse_device.hpp>

#include "mouse_device_backend.hpp"

class MouseDeviceSystem {
    using MouseDevice = nodec_input::mouse::impl::MouseDevice;
    using Mouse = nodec_input::mouse::Mouse;
    using RegistryOperations = nodec_input::InputDevices::RegistryOperations<Mouse>;
    using DeviceHandle = nodec_input::InputDevices::DeviceHandle<Mouse>;

public:
    using device_type = Mouse;

    MouseDeviceSystem(RegistryOperations registry_ops)
        : registry_ops_{registry_ops} {
        mouse_device_.second = std::make_shared<MouseDeviceBackend>();
        mouse_device_.first = registry_ops_.add_device(mouse_device_.second);
    }

    MouseDevice &device() {
        return *mouse_device_.second;
    }

private:
    std::pair<DeviceHandle, std::shared_ptr<MouseDevice>> mouse_device_;
    RegistryOperations registry_ops_;
};
#endif