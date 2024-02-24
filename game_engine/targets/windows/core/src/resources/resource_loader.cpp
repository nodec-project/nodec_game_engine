#include <resources/resource_loader.hpp>

#include <fstream>

#include <nodec_animation/serialization/resources/animation_clip.hpp>
#include <nodec_rendering/serialization/resources/material.hpp>
#include <nodec_rendering/serialization/resources/mesh.hpp>
#include <nodec_rendering/serialization/resources/shader.hpp>
#include <nodec_scene_serialization/archive_context.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>
#include <nodec_scene_serialization/serializable_entity.hpp>

#include <SceneAudio/AudioClipBackend.hpp>
#include <rendering/image_texture.hpp>
#include <rendering/material_backend.hpp>
#include <rendering/mesh_backend.hpp>
#include <rendering/shader_backend.hpp>
#include <rendering/texture_backend.hpp>

template<>
std::shared_ptr<MeshBackend>
ResourceLoader::load_backend<MeshBackend>(const std::string &path) const noexcept {
    using namespace nodec_rendering::resources;
    using namespace nodec;

    std::ifstream file(path, std::ios::binary);

    if (!file) {
        logger_->warn(__FILE__, __LINE__) << "Failed to open resource file. path: " << path;
        // return empty mesh.
        return {};
    }

    cereal::PortableBinaryInputArchive archive(file);

    SerializableMesh source;
    try {
        archive(source);
    } catch (...) {
        HandleException(Formatter() << "Mesh::" << path);
        return {};
    }

    auto mesh = std::make_shared<MeshBackend>();
    mesh->vertices.reserve(source.vertices.size());
    for (auto &&src : source.vertices) {
        mesh->vertices.push_back({src.position, src.normal, src.uv, src.tangent});
    }

    mesh->triangles = source.triangles;

    try {
        mesh->update_device_memory(&gfx_);
    } catch (...) {
        HandleException(Formatter() << "Mesh::" << path);
        return {};
    }

    return mesh;
}

template<>
std::shared_ptr<ShaderBackend>
ResourceLoader::load_backend<ShaderBackend>(const std::string &path) const noexcept {
    using namespace nodec;
    using namespace nodec_rendering::resources;

    std::ifstream file(Formatter() << path << "/shader.meta", std::ios::binary);

    if (!file) {
        logger_->warn(__FILE__, __LINE__) << "Failed to open resource file. path: " << path;
        // return empty mesh.
        return {};
    }

    ShaderMetaInfo metaInfo;
    try {
        cereal::JSONInputArchive archive(file);
        archive(metaInfo);
    } catch (...) {
        HandleException(Formatter() << "Shader::" << path);
        return {};
    }

    std::vector<SubShaderMetaInfo> subShaderMetaInfos;
    for (const auto &subShaderName : metaInfo.pass) {
        std::ifstream file(Formatter() << path << "/" << subShaderName << ".meta");
        if (!file) return {};
        SubShaderMetaInfo info;
        try {
            cereal::JSONInputArchive archive(file);
            archive(info);
            subShaderMetaInfos.push_back(std::move(info));
        } catch (...) {
            HandleException(Formatter() << "Shader::" << path);
            return {};
        }
    }

    std::shared_ptr<ShaderBackend> shader;
    try {
        shader = std::make_shared<ShaderBackend>(&gfx_, path, metaInfo, subShaderMetaInfos);
    } catch (...) {
        HandleException(Formatter() << "Shader::" << path);
        return {};
    }

    return shader;
}

template<>
std::shared_ptr<MaterialBackend>
ResourceLoader::load_backend<MaterialBackend>(const std::string &path) const noexcept {
    using namespace nodec;
    using namespace nodec::resource_management;
    using namespace nodec_rendering::resources;

    std::ifstream file(path, std::ios::binary);

    if (!file) {
        logger_->warn(__FILE__, __LINE__) << "Failed to open resource file. path: " << path;
        // return empty mesh.
        return {};
    }

    SerializableMaterial source;

    try {
        cereal::JSONInputArchive archive(file);
        archive(source);
    } catch (...) {
        HandleException(Formatter() << "Material::" << path);
        return {};
    }

    auto material = std::make_shared<MaterialBackend>(&gfx_);

    try {
        source.apply_to(*material, registry_);
    } catch (...) {
        HandleException(Formatter() << "Material::" << path);
        return {};
    }

    return material;
}

template<>
std::shared_ptr<TextureBackend>
ResourceLoader::load_backend<TextureBackend>(const std::string &path) const noexcept {
    using namespace nodec;

    try {
        auto texture = std::make_shared<ImageTexture>(&gfx_, path);
        return texture;
    } catch (...) {
        HandleException(Formatter() << "Texture::" << path);
        return {};
    }

    return {};
}

template<>
std::shared_ptr<nodec_scene_serialization::SerializableEntity>
ResourceLoader::load_backend<nodec_scene_serialization::SerializableEntity>(const std::string &path) const noexcept {
    using namespace nodec;
    using namespace nodec_scene_serialization;

    std::ifstream file(path, std::ios::binary);

    if (!file) {
        logger_->warn(__FILE__, __LINE__) << "Failed to open resource file. path: " << path;
        // return empty mesh.
        return {};
    }

    SerializableEntity ser_entity;

    try {
        ArchiveContext context{scene_serialization_, registry_};
        cereal::UserDataAdapter<ArchiveContext, cereal::JSONInputArchive> archive(context, file);
        archive(ser_entity);
    } catch (...) {
        HandleException(Formatter() << "SerializableEntity::" << path);
        return {};
    }

    return std::make_shared<SerializableEntity>(std::move(ser_entity));
}

template<>
std::shared_ptr<AudioClipBackend>
ResourceLoader::load_backend<AudioClipBackend>(const std::string &path) const noexcept {
    using namespace nodec;

    std::shared_ptr<AudioClipBackend> clip;

    try {
        clip = std::make_shared<AudioClipBackend>(path);
    } catch (...) {
        HandleException(Formatter() << "AudioClip::" << path);
        return {};
    }

    return clip;
}

template<>
std::shared_ptr<FontBackend>
ResourceLoader::load_backend(const std::string &path) const noexcept {
    using namespace nodec;

    std::shared_ptr<FontBackend> font;
    try {
        font = std::make_shared<FontBackend>(font_library_, path);
    } catch (...) {
        HandleException(Formatter() << "Font::" << path);
        return {};
    }

    return font;
}

template<>
std::shared_ptr<nodec_animation::resources::AnimationClip>
ResourceLoader::load_backend<nodec_animation::resources::AnimationClip>(const std::string &path) const noexcept {
    using namespace nodec;
    using namespace nodec_animation::resources;
    using namespace nodec_scene_serialization;

    std::ifstream file(path, std::ios::binary);

    if (!file) {
        logger_->warn(__FILE__, __LINE__) << "Failed to open resource file. path: " << path;
        // return empty mesh.
        return {};
    }

    std::shared_ptr<AnimationClip> clip = std::make_shared<AnimationClip>();

    try {
        ArchiveContext context{scene_serialization_, registry_};
        cereal::UserDataAdapter<ArchiveContext, cereal::JSONInputArchive> archive(context, file);
        archive(*clip);
    } catch (...) {
        HandleException(Formatter() << "AnimationClip::" << path);
        return {};
    }

    return clip;
}