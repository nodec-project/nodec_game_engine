#include <SceneSerialization/SceneSerializationBackend.hpp>

#include <nodec_scene/scene_registry.hpp>
#include <nodec_scene_serialization/components/non_serialized.hpp>
#include <nodec_scene_serialization/components/prefab.hpp>
#include <nodec_serialization/nodec_physics/components/physics_shape.hpp>
#include <nodec_serialization/nodec_physics/components/rigid_body.hpp>
#include <nodec_serialization/nodec_rendering/components/camera.hpp>
#include <nodec_serialization/nodec_rendering/components/directional_light.hpp>
#include <nodec_serialization/nodec_rendering/components/image_renderer.hpp>
#include <nodec_serialization/nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_serialization/nodec_rendering/components/non_visible.hpp>
#include <nodec_serialization/nodec_rendering/components/point_light.hpp>
#include <nodec_serialization/nodec_rendering/components/post_processing.hpp>
#include <nodec_serialization/nodec_rendering/components/scene_lighting.hpp>
#include <nodec_serialization/nodec_rendering/components/text_renderer.hpp>
#include <nodec_serialization/nodec_scene/components/name.hpp>
#include <nodec_serialization/nodec_scene/components/transform.hpp>
#include <nodec_serialization/nodec_scene_audio/components/audio_source.hpp>

SceneSerializationBackend::SceneSerializationBackend(nodec::resource_management::ResourceRegistry *resource_registry,
                                                     nodec_scene_serialization::SceneSerialization &serialization) {
    using namespace nodec_rendering::components;
    using namespace nodec_rendering::resources;
    using namespace nodec_scene::components;
    using namespace nodec_scene_audio::components;
    using namespace nodec_physics::components;
    using namespace nodec_scene;
    using namespace nodec_scene_serialization::components;

    serialization.register_component<Name, SerializableName>();
    serialization.register_component<Transform, SerializableTransform>();

    serialization.register_component<MeshRenderer, SerializableMeshRenderer>();
    serialization.register_component<ImageRenderer, SerializableImageRenderer>();
    serialization.register_component<TextRenderer, SerializableTextRenderer>();
    serialization.register_component<PostProcessing, SerializablePostProcessing>();
    serialization.register_component<Camera, SerializableCamera>();
    serialization.register_component<DirectionalLight, SerializableDirectionalLight>();
    serialization.register_component<PointLight, SerializablePointLight>();
    serialization.register_component<SceneLighting, SerializableSceneLighting>();
    serialization.register_component<NonVisible, SerializableNonVisible>();

    serialization.register_component<AudioSource, SerializableAudioSource>();

    serialization.register_component<RigidBody, SerializableRigidBody>();
    serialization.register_component<PhysicsShape, SerializablePhysicsShape>();

    serialization.register_component<Prefab>();
}
