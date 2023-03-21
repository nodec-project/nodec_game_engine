#pragma once

#include <nodec/resource_management/resource_registry.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>

class SceneSerializationModuleBackend : public nodec_scene_serialization::SceneSerialization {
public:
    SceneSerializationModuleBackend(nodec::resource_management::ResourceRegistry *resource_registry);
};