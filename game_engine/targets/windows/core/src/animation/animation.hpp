#ifndef NODEC_GAME_ENGINE__ANIMATION__ANIMATION_HPP_
#define NODEC_GAME_ENGINE__ANIMATION__ANIMATION_HPP_

#include <nodec_animation/animation_component_support.hpp>

#include <nodec_rendering/serialization/components/image_renderer.hpp>
#include <nodec_scene/serialization/components/local_transform.hpp>

inline void setup_animation_component_registry(nodec_animation::ComponentRegistry &registry) {
    {
        using namespace nodec_scene::components;
        registry.register_component<LocalTransform>();
    }
    {
        using namespace nodec_rendering::components;
        registry.register_component<ImageRenderer>();
    }
}

#endif