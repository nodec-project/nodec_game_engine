#ifndef NODEC_GAME_ENGINE__RESOURCES__RESOURCE_LOADER_HPP_
#define NODEC_GAME_ENGINE__RESOURCES__RESOURCE_LOADER_HPP_

#include <nodec/concurrent/thread_pool_executor.hpp>
#include <nodec/logging/logging.hpp>
#include <nodec/resource_management/resource_registry.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>

#include <Font/FontBackend.hpp>
#include <Font/FontLibrary.hpp>
#include <graphics/graphics.hpp>

class ResourceLoader {
    using ResourceRegistry = nodec::resource_management::ResourceRegistry;

    template<typename T>
    using ResourceFuture = std::future<std::shared_ptr<T>>;

    template<typename T>
    using ResourcePtr = std::shared_ptr<T>;

    void HandleException(const std::string &identifier) const {
        using namespace nodec;

        std::string details;

        try {
            throw;
        } catch (std::exception &e) {
            details = e.what();
        } catch (...) {
            details = "Unknown";
        }

        logger_->warn(__FILE__, __LINE__)
            << "Failed to make resource (" << identifier << ")\n"
            << "details: \n"
            << details;
    }

public:
    ResourceLoader(Graphics &gfx, ResourceRegistry &registry,
                   FontLibrary &font_library,
                   nodec_scene_serialization::SceneSerialization &scene_serialization)
        : logger_(nodec::logging::get_logger("engine.resources.resource-loader")),
          gfx_{gfx}, registry_{registry}, font_library_{font_library}, scene_serialization_{scene_serialization} {
    }

    // For resource registry
    template<typename Resource, typename ResourceBackend>
    ResourcePtr<Resource> load_direct(const std::string &path) {
        std::shared_ptr<Resource> resource = load_backend<ResourceBackend>(path);
        return resource;
    }

    template<typename Resource, typename ResourceBackend>
    ResourceFuture<Resource> load_async(const std::string &name, const std::string &path, ResourceRegistry::LoadNotifyer<Resource> notifyer) {
        using namespace nodec;

        return executor_.submit(
            [=]() {
                // <https://github.com/microsoft/DirectXTex/issues/163>
                HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
                if (FAILED(hr)) {
                    logger_->warn(__FILE__, __LINE__) << "CoInitializeEx failed.";
                }

                std::shared_ptr<Resource> resource = load_backend<ResourceBackend>(path);
                notifyer.on_loaded(name, resource);

                CoUninitialize();
                return resource;
            });
    }

    template<typename ResourceBackend>
    std::shared_ptr<ResourceBackend> load_backend(const std::string &path) const noexcept;

private:
    Graphics &gfx_;
    ResourceRegistry &registry_;
    FontLibrary &font_library_;
    nodec_scene_serialization::SceneSerialization &scene_serialization_;
    std::shared_ptr<nodec::logging::Logger> logger_;
    nodec::concurrent::ThreadPoolExecutor executor_;
};

#endif