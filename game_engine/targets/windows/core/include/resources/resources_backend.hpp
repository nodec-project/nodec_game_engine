#ifndef NODEC_GAME_ENGINE__RESOURCES__RESOURCES_BACKEND_HPP_
#define NODEC_GAME_ENGINE__RESOURCES__RESOURCES_BACKEND_HPP_

#include <nodec_animation/resources/animation_clip.hpp>
#include <nodec_resources/impl/resources_impl.hpp>
#include <nodec_scene_audio/resources/audio_clip.hpp>

#include "../scene_audio/audio_clip_backend.hpp"
#include "resource_loader.hpp"

class ResourcesBackend : public nodec_resources::impl::ResourcesImpl {
public:
    void setup_on_boot() {
        resource_path_changed_connection_ = resource_path_changed().connect(
            [](ResourcesImpl &resources, const std::string &path) {
                resources.internal_resource_path = path;
            });
    }

    void setup_on_runtime(Graphics &graphics, FontLibrary &font_library, nodec_scene_serialization::SceneSerialization &scene_serialization) {
        using namespace nodec;
        using namespace nodec_rendering::resources;
        using namespace nodec_scene_serialization;
        using namespace nodec_scene_audio::resources;

        resource_path_changed_connection_.disconnect();

        resource_loader_.reset(new ResourceLoader(graphics, registry(), font_library, scene_serialization));

        registry().register_resource_loader<Mesh>(
            [=](auto &name) {
                return resource_loader_->load_direct<Mesh, MeshBackend>(Formatter() << resource_path() << "/" << name);
            },
            [=](auto &name, auto notifyer) {
                return resource_loader_->load_async<Mesh, MeshBackend>(name, Formatter() << resource_path() << "/" << name, notifyer);
            });

        registry().register_resource_loader<Shader>(
            [=](auto &name) {
                return resource_loader_->load_direct<Shader, ShaderBackend>(Formatter() << resource_path() << "/" << name);
            },
            [=](auto &name, auto notifyer) {
                return resource_loader_->load_async<Shader, ShaderBackend>(name, Formatter() << resource_path() << "/" << name, notifyer);
            });

        registry().register_resource_loader<Texture>(
            [=](auto &name) {
                return resource_loader_->load_direct<Texture, TextureBackend>(Formatter() << resource_path() << "/" << name);
            },
            [=](auto &name, auto notifyer) {
                return resource_loader_->load_async<Texture, TextureBackend>(name, Formatter() << resource_path() << "/" << name, notifyer);
            });

        registry().register_resource_loader<Material>(
            [=](auto &name) {
                return resource_loader_->load_direct<Material, MaterialBackend>(Formatter() << resource_path() << "/" << name);
            },
            [=](auto &name, auto notifyer) {
                return resource_loader_->load_async<Material, MaterialBackend>(name, Formatter() << resource_path() << "/" << name, notifyer);
            });

        registry().register_resource_loader<SerializableEntity>(
            [=](auto &name) {
                return resource_loader_->load_direct<SerializableEntity, SerializableEntity>(Formatter() << resource_path() << "/" << name);
            },
            [=](auto &name, auto notifyer) {
                return resource_loader_->load_async<SerializableEntity, SerializableEntity>(name, Formatter() << resource_path() << "/" << name, notifyer);
            });

        registry().register_resource_loader<AudioClip>(
            [=](auto &name) {
                return resource_loader_->load_direct<AudioClip, AudioClipBackend>(Formatter() << resource_path() << "/" << name);
            },
            [=](auto &name, auto notifyer) {
                return resource_loader_->load_async<AudioClip, AudioClipBackend>(name, Formatter() << resource_path() << "/" << name, notifyer);
            });

        registry().register_resource_loader<Font>(
            [=](auto &name) {
                return resource_loader_->load_direct<Font, FontBackend>(Formatter() << resource_path() << "/" << name);
            },
            [=](auto &name, auto notifyer) {
                return resource_loader_->load_async<Font, FontBackend>(name, Formatter() << resource_path() << "/" << name, notifyer);
            });

        {
            using namespace nodec_animation::resources;
            registry().register_resource_loader<AnimationClip>(
                [=](auto &name) {
                    return resource_loader_->load_direct<AnimationClip, AnimationClip>(Formatter() << resource_path() << "/" << name);
                },
                [=](auto &name, auto notifyer) {
                    return resource_loader_->load_async<AnimationClip, AnimationClip>(name, Formatter() << resource_path() << "/" << name, notifyer);
                });
        }
    }

private:
    nodec::signals::Connection resource_path_changed_connection_;
    std::unique_ptr<ResourceLoader> resource_loader_;
};

#endif