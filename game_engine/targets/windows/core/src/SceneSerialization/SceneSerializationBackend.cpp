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

    serialization.register_component<MeshRenderer, SerializableMeshRenderer>(
        [=](const MeshRenderer &component) {
            auto serializable = std::make_unique<SerializableMeshRenderer>();

            for (auto &mesh : component.meshes) {
                auto name = resource_registry->lookup_name(mesh).first;
                serializable->meshes.push_back(name);
            }

            for (auto &material : component.materials) {
                auto name = resource_registry->lookup_name(material).first;
                serializable->materials.push_back(name);
            }

            return serializable;
        },
        [=](const SerializableMeshRenderer serializable) {
            MeshRenderer component;

            for (auto &name : serializable.meshes) {
                auto mesh = resource_registry->get_resource<Mesh>(name).get();
                component.meshes.push_back(mesh);
            }

            for (auto &name : serializable.materials) {
                auto material = resource_registry->get_resource<Material>(name).get();
                component.materials.push_back(material);
            }
            return component;
        });

    serialization.register_component<ImageRenderer, SerializableImageRenderer>(
        [=](const ImageRenderer &renderer) {
            auto serializable = std::make_unique<SerializableImageRenderer>();
            serializable->image = renderer.image;
            serializable->material = renderer.material;
            serializable->pixels_per_unit = renderer.pixels_per_unit;
            return serializable;
        },
        [=](const SerializableImageRenderer &serializable) {
            ImageRenderer renderer;
            renderer.image = serializable.image;
            renderer.material = serializable.material;
            renderer.pixels_per_unit = serializable.pixels_per_unit;
            return renderer;
        });

    serialization.register_component<TextRenderer, SerializableTextRenderer>(
        [=](const TextRenderer &renderer) {
            auto serializable = std::make_unique<SerializableTextRenderer>();
            serializable->font = renderer.font;
            serializable->material = renderer.material;
            serializable->text = renderer.text;
            serializable->pixel_size = renderer.pixel_size;
            serializable->pixels_per_unit = renderer.pixels_per_unit;
            serializable->color = renderer.color;
            return serializable;
        },
        [=](const SerializableTextRenderer &serializable) {
            TextRenderer renderer;
            renderer.font = serializable.font;
            renderer.material = serializable.material;
            renderer.text = serializable.text;
            renderer.pixel_size = serializable.pixel_size;
            renderer.pixels_per_unit = serializable.pixels_per_unit;
            renderer.color = serializable.color;
            return renderer;
        });

    serialization.register_component<PostProcessing, SerializablePostProcessing>(
        [=](const PostProcessing &processing) {
            auto serializable = std::make_unique<SerializablePostProcessing>();

            for (const auto &effect : processing.effects) {
                const auto name = resource_registry->lookup_name<Material>(effect.material).first;
                serializable->effects.push_back({effect.enabled, name});
            }
            return serializable;
        },
        [=](const SerializablePostProcessing &serializable) {
            PostProcessing processing;
            for (const auto &effect : serializable.effects) {
                processing.effects.push_back({effect.enabled, resource_registry->get_resource<Material>(effect.material).get()});
            }
            return processing;
        });

    serialization.register_component<Name, SerializableName>();
    serialization.register_component<Transform, SerializableTransform>();

    serialization.register_component<Camera, SerializableCamera>(
        [](const Camera &camera) {
            auto serializable_camera = std::make_unique<SerializableCamera>();
            serializable_camera->far_clip_plane = camera.far_clip_plane;
            serializable_camera->near_clip_plane = camera.near_clip_plane;
            serializable_camera->fov_angle = camera.fov_angle;
            serializable_camera->projection = camera.projection;
            serializable_camera->ortho_width = camera.ortho_width;
            return serializable_camera;
        },
        [](const SerializableCamera &serializable_camera) {
            Camera camera;
            camera.far_clip_plane = serializable_camera.far_clip_plane;
            camera.near_clip_plane = serializable_camera.near_clip_plane;
            camera.fov_angle = serializable_camera.fov_angle;
            camera.projection = serializable_camera.projection;
            camera.ortho_width = serializable_camera.ortho_width;
            return camera;
        });

    serialization.register_component<DirectionalLight, SerializableDirectionalLight>(
        [](const DirectionalLight &light) {
            auto serializable_light = std::make_unique<SerializableDirectionalLight>();
            serializable_light->color = light.color;
            serializable_light->intensity = light.intensity;
            return serializable_light;
        },
        [](const SerializableDirectionalLight &serializable_light) {
            DirectionalLight light;
            light.color = serializable_light.color;
            light.intensity = serializable_light.intensity;
            return light;
        });

    serialization.register_component<PointLight, SerializablePointLight>(
        [](const PointLight &light) {
            auto serializable = std::make_unique<SerializablePointLight>();
            serializable->color = light.color;
            serializable->intensity = light.intensity;
            serializable->range = light.range;
            return serializable;
        },
        [](const SerializablePointLight &serializable) {
            PointLight light;
            light.color = serializable.color;
            light.intensity = serializable.intensity;
            light.range = serializable.range;
            return light;
        });

    serialization.register_component<SceneLighting, SerializableSceneLighting>(
        [](const SceneLighting &lighting) {
            auto serializable = std::make_unique<SerializableSceneLighting>();
            serializable->ambient_color = lighting.ambient_color;
            serializable->skybox = lighting.skybox;
            return serializable;
        },
        [](const SerializableSceneLighting &serializable) {
            SceneLighting lighting;
            lighting.ambient_color = serializable.ambient_color;
            lighting.skybox = serializable.skybox;
            return lighting;
        });

    serialization.register_component<AudioSource, SerializableAudioSource>(
        [](const AudioSource &source) {
            auto serializable = std::make_unique<SerializableAudioSource>();
            serializable->clip = source.clip;
            serializable->is_playing = source.is_playing;
            serializable->loop = source.loop;
            serializable->position = source.position;
            serializable->volume = source.volume;
            return serializable;
        },
        [](const SerializableAudioSource &serializable) {
            AudioSource source;
            source.clip = serializable.clip;
            source.is_playing = serializable.is_playing;
            source.loop = serializable.loop;
            source.position = serializable.position;
            source.volume = serializable.volume;
            return source;
        });

    serialization.register_component<NonVisible, SerializableNonVisible>(
        [](const NonVisible &non_visible) {
            auto serializable = std::make_unique<SerializableNonVisible>();
            serializable->self = non_visible.self;
            return serializable;
        },
        [](const SerializableNonVisible &serializable) {
            NonVisible non_visible;
            non_visible.self = serializable.self;
            return non_visible;
        });

    serialization.register_component<RigidBody, SerializableRigidBody>(
        [](const RigidBody &body) {
            auto serializable = std::make_unique<SerializableRigidBody>();
            serializable->mass = body.mass;
            return serializable;
        },
        [](const SerializableRigidBody &serializable) {
            RigidBody body;
            body.mass = serializable.mass;
            return body;
        });

    serialization.register_component<PhysicsShape, SerializablePhysicsShape>(
        [](const PhysicsShape &shape) {
            auto serializable = std::make_unique<SerializablePhysicsShape>();
            serializable->shape_type = shape.shape_type;
            serializable->size = shape.size;
            serializable->radius = shape.radius;
            return serializable;
        },
        [](const SerializablePhysicsShape &serializable) {
            PhysicsShape shape;
            shape.shape_type = serializable.shape_type;
            shape.size = serializable.size;
            shape.radius = serializable.radius;
            return shape;
        });

    serialization.register_component<Prefab>();
}
