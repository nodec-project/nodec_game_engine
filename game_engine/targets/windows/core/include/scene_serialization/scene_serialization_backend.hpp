#ifndef NODEC_GAME_ENGINE__SCENE_SERIALIZATION_SCENE_SERIALIZATION_BACKEND_HPP_
#define NODEC_GAME_ENGINE__SCENE_SERIALIZATION_SCENE_SERIALIZATION_BACKEND_HPP_

#include <nodec/resource_management/resource_registry.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>

class SceneSerializationBackend final {
public:
    SceneSerializationBackend(nodec::resource_management::ResourceRegistry *resource_registry, nodec_scene_serialization::SceneSerialization &serialization);
};

#endif