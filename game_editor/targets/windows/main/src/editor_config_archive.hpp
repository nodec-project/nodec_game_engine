#ifndef NODEC_GAME_EDITOR__EDITOR_CONFIG_ARCHIVE_HPP_
#define NODEC_GAME_EDITOR__EDITOR_CONFIG_ARCHIVE_HPP_

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include <nodec/concurrent/thread_pool_executor.hpp>
#include <nodec/logging/logging.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/unordered_map.hpp>

struct BaseEditorConfigBlock {
    virtual ~BaseEditorConfigBlock() = 0;
};
inline BaseEditorConfigBlock::~BaseEditorConfigBlock() {}

#include <cereal/archives/json.hpp>

#define NODEC_GAME_EDITOR_REGISTER_EDITOR_CONFIG_BLOCK(T) \
    CEREAL_REGISTER_TYPE(T) \
    CEREAL_REGISTER_POLYMORPHIC_RELATION(BaseEditorConfigBlock, T)

struct EditorConfig {
    std::string resource_path;

    struct FontConfig {
        std::string path;
        float pixel_size{10};

        template<class Archive>
        void serialize(Archive &archive) {
            archive(cereal::make_nvp("path", path),
                    cereal::make_nvp("pixel_size", pixel_size));
        }
    };

    FontConfig font;

    std::unordered_map<std::string, std::unique_ptr<BaseEditorConfigBlock>> blocks;

    template<class Archive>
    void serialize(Archive &archive) {
        archive(cereal::make_nvp("resource_path", resource_path),
                cereal::make_nvp("font", font));
        archive(cereal::make_nvp("blocks", blocks));
    }
};

class EditorConfigArchive {
public:
    EditorConfigArchive(const std::string &file_path)
        : logger_(nodec::logging::get_logger("editor.editor-config-archive")),
          file_path_(file_path),
          executor_(1) {
    }

    std::future<std::unique_ptr<EditorConfig>> load() {
        return executor_.submit([this]() {
            std::unique_ptr<EditorConfig> config = std::make_unique<EditorConfig>();
            
            std::ifstream file(file_path_);

            if (!file) {
                return config;
            }

            try {
                cereal::JSONInputArchive archive(file);
                archive(*config);
            } catch (std::exception &e) {
                logger_->warn(__FILE__, __LINE__)
                    << "Failed to load editor configuration.\n"
                    << "details: \n"
                    << e.what();
                return config;
            }

            return config;
        });
    }

    std::future<bool> save() {
        return executor_.submit([this]() {
            std::ofstream file(file_path_);

            if (!file) {
                logger_->warn(__FILE__, __LINE__)
                    << "Failed to open editor configuration file for saving.\n"
                    << "details: \n"
                    << strerror(errno);
                return false;
            }

            try {
                cereal::JSONOutputArchive archive(file);
                archive(*config_);
            } catch (std::exception &e) {
                logger_->warn(__FILE__, __LINE__)
                    << "Failed to save editor configuration.\n"
                    << "details: \n"
                    << e.what();
                return false;
            }

            return true;
        });
    }

    void set_config(std::unique_ptr<EditorConfig> config) {
        config_ = std::move(config);
    }

    /**
     * @brief Get current EditorConfig.
     * @warning All accessors should be called from the same thread.
     *
     * @return EditorConfig&
     */
    EditorConfig &config() {
        return *config_;
    }

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    std::string file_path_;
    std::unique_ptr<EditorConfig> config_;
    nodec::concurrent::ThreadPoolExecutor executor_;
};

#endif