#include <scene_serialization/scene_serialization_backend.hpp>

#include <nodec_animation/serialization/components/animator.hpp>
#include <nodec_physics/serialization/components/collision_filter.hpp>
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
#include <nodec_rendering/serialization/components/render_layer.hpp>
#include <nodec_rendering/serialization/components/scene_lighting.hpp>
#include <nodec_rendering/serialization/components/text_renderer.hpp>
#include <nodec_scene/serialization/components/local_transform.hpp>
#include <nodec_scene/serialization/components/name.hpp>
#include <nodec_scene_audio/serialization/components/audio_listener.hpp>
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

        // Register all 32 render layers
        serialization.register_component<RenderLayer<0>, SerializableRenderLayer<0>>();
        serialization.register_component<RenderLayer<1>, SerializableRenderLayer<1>>();
        serialization.register_component<RenderLayer<2>, SerializableRenderLayer<2>>();
        serialization.register_component<RenderLayer<3>, SerializableRenderLayer<3>>();
        serialization.register_component<RenderLayer<4>, SerializableRenderLayer<4>>();
        serialization.register_component<RenderLayer<5>, SerializableRenderLayer<5>>();
        serialization.register_component<RenderLayer<6>, SerializableRenderLayer<6>>();
        serialization.register_component<RenderLayer<7>, SerializableRenderLayer<7>>();
        serialization.register_component<RenderLayer<8>, SerializableRenderLayer<8>>();
        serialization.register_component<RenderLayer<9>, SerializableRenderLayer<9>>();
        serialization.register_component<RenderLayer<10>, SerializableRenderLayer<10>>();
        serialization.register_component<RenderLayer<11>, SerializableRenderLayer<11>>();
        serialization.register_component<RenderLayer<12>, SerializableRenderLayer<12>>();
        serialization.register_component<RenderLayer<13>, SerializableRenderLayer<13>>();
        serialization.register_component<RenderLayer<14>, SerializableRenderLayer<14>>();
        serialization.register_component<RenderLayer<15>, SerializableRenderLayer<15>>();
        serialization.register_component<RenderLayer<16>, SerializableRenderLayer<16>>();
        serialization.register_component<RenderLayer<17>, SerializableRenderLayer<17>>();
        serialization.register_component<RenderLayer<18>, SerializableRenderLayer<18>>();
        serialization.register_component<RenderLayer<19>, SerializableRenderLayer<19>>();
        serialization.register_component<RenderLayer<20>, SerializableRenderLayer<20>>();
        serialization.register_component<RenderLayer<21>, SerializableRenderLayer<21>>();
        serialization.register_component<RenderLayer<22>, SerializableRenderLayer<22>>();
        serialization.register_component<RenderLayer<23>, SerializableRenderLayer<23>>();
        serialization.register_component<RenderLayer<24>, SerializableRenderLayer<24>>();
        serialization.register_component<RenderLayer<25>, SerializableRenderLayer<25>>();
        serialization.register_component<RenderLayer<26>, SerializableRenderLayer<26>>();
        serialization.register_component<RenderLayer<27>, SerializableRenderLayer<27>>();
        serialization.register_component<RenderLayer<28>, SerializableRenderLayer<28>>();
        serialization.register_component<RenderLayer<29>, SerializableRenderLayer<29>>();
        serialization.register_component<RenderLayer<30>, SerializableRenderLayer<30>>();
        serialization.register_component<RenderLayer<31>, SerializableRenderLayer<31>>();
    }
    {
        using namespace nodec_scene_audio::components;
        serialization.register_component<AudioSource, SerializableAudioSource>();
        serialization.register_component<AudioListener, SerializableAudioListener>();
        serialization.register_component<AudioPlay, SerializableAudioPlay>();
        serialization.register_component<AudioStop, SerializableAudioStop>();
    }

    {
        using namespace nodec_physics::components;
        serialization.register_component<PhysicsShape, SerializablePhysicsShape>();
        serialization.register_component<RigidBody, SerializableRigidBody>();
        serialization.register_component<StaticRigidBody, SerializableStaticRigidBody>();
        serialization.register_component<TriggerBody, SerializableTriggerBody>();
        serialization.register_component<CollisionFilter, SerializableCollisionFilter>();
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
