#include <scene_serialization/scene_serialization_backend.hpp>

#include <nodec_animation/serialization/components/animator.hpp>
#include <nodec_physics/serialization/components/physics_shape.hpp>
#include <nodec_physics/serialization/components/rigid_body.hpp>
#include <nodec_physics/serialization/components/static_rigid_body.hpp>
#include <nodec_physics/serialization/components/trigger_body.hpp>
#include <nodec_rendering/serialization/components/camera.hpp>
#include <nodec_rendering/serialization/components/directional_light.hpp>
#include <nodec_rendering/serialization/components/image_renderer.hpp>
#include <nodec_rendering/serialization/components/mesh_renderer.hpp>
#include <nodec_rendering/serialization/components/non_visible.hpp>
#include <nodec_rendering/serialization/components/point_light.hpp>
#include <nodec_rendering/serialization/components/post_processing.hpp>
#include <nodec_rendering/serialization/components/scene_lighting.hpp>
#include <nodec_rendering/serialization/components/text_renderer.hpp>
#include <nodec_scene/serialization/components/local_transform.hpp>
#include <nodec_scene/serialization/components/name.hpp>
#include <nodec_scene_audio/serialization/components/audio_source.hpp>
#include <nodec_scene_serialization/components/non_serialized.hpp>
#include <nodec_scene_serialization/components/prefab.hpp>

SceneSerializationBackend::SceneSerializationBackend(nodec::resource_management::ResourceRegistry *resource_registry,
                                                     nodec_scene_serialization::SceneSerialization &serialization) {
    {
        using namespace nodec_scene::components;
        serialization.register_component<Name, SerializableName>();
        serialization.register_component<LocalTransform, SerializableLocalTransform>();
    }

    {
        using namespace nodec_rendering::components;
        serialization.register_component<MeshRenderer, SerializableMeshRenderer>();
        serialization.register_component<ImageRenderer, SerializableImageRenderer>();
        serialization.register_component<TextRenderer, SerializableTextRenderer>();
        serialization.register_component<PostProcessing, SerializablePostProcessing>();
        serialization.register_component<Camera, SerializableCamera>();
        serialization.register_component<DirectionalLight, SerializableDirectionalLight>();
        serialization.register_component<PointLight, SerializablePointLight>();
        serialization.register_component<SceneLighting, SerializableSceneLighting>();
        serialization.register_component<NonVisible, SerializableNonVisible>();
    }
    {
        using namespace nodec_scene_audio::components;
        serialization.register_component<AudioSource, SerializableAudioSource>();
    }

    {
        using namespace nodec_physics::components;
        serialization.register_component<PhysicsShape, SerializablePhysicsShape>();
        serialization.register_component<RigidBody, SerializableRigidBody>();
        serialization.register_component<StaticRigidBody, SerializableStaticRigidBody>();
        serialization.register_component<TriggerBody, SerializableTriggerBody>();
    }
    {
        using namespace nodec_scene_serialization::components;
        serialization.register_component<Prefab>();
    }

    {
        using namespace nodec_animation::components;
        serialization.register_component<Animator, SerializableAnimator>();
        serialization.register_component<AnimatorStart, SerializableAnimatorStart>();
        serialization.register_component<AnimatorStop, SerializableAnimatorStop>();
    }
}
