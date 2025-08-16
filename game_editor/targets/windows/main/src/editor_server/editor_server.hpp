#ifndef NODEC_GAME_EDITOR__EDITOR_SERVER_HPP_
#define NODEC_GAME_EDITOR__EDITOR_SERVER_HPP_

#include <thread>
#include <memory>
#include <queue>
#include <mutex>
#include <functional>

#include <nodec/logging/logging.hpp>
#include <nodec/string_builder.hpp>
#include <nodec/resource_management/resource_registry.hpp>
#include <nodec_world/world.hpp>
#include <nodec_scene/components/hierarchy.hpp>
#include <nodec_scene/components/name.hpp>
#include <nodec_scene/systems/hierarchy_system.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>
#include <nodec_scene_serialization/archive_context.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/details/helpers.hpp>
#include <sstream>

#include <uwebsockets/App.h>

// APIリクエスト情報を保持する構造体
struct APIRequest {
    enum Type {
        GET_ROOT_ENTITIES,
        GET_ENTITY_COMPONENTS,
        GET_ENTITY_INFO
    };
    
    Type type;
    uint32_t entity_id;  // エンティティID（必要に応じて使用）
    std::function<void(const std::string&)> response_callback;
    bool is_valid = true;  // リクエストが有効かどうか
    
    APIRequest(Type t, std::function<void(const std::string&)> callback)
        : type(t), entity_id(0), response_callback(std::move(callback)) {}
    
    APIRequest(Type t, uint32_t id, std::function<void(const std::string&)> callback)
        : type(t), entity_id(id), response_callback(std::move(callback)) {}
};

class EditorServer {
public:
    EditorServer(nodec_world::World* world, 
                 nodec_scene_serialization::SceneSerialization* scene_serialization,
                 nodec::resource_management::ResourceRegistry* resource_registry) 
        : world_(world), scene_serialization_(scene_serialization), resource_registry_(resource_registry),
          logger_(nodec::logging::get_logger("editor_server")) {
        thread_ = std::thread([this]() {
            auto app = uWS::App();
            
            // uWS::Loop ポインタを保存（defer用）
            main_loop_ = uWS::Loop::get();
            
            // CORS Preflight handler for all /api/* routes
            app.options("/api/*", [](auto *res, auto *req) {
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                res->writeHeader("Access-Control-Max-Age", "86400");
                res->end();
            })
            .get("/entities/id/:id", [](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->end("{\"id\": 1}");
            })
            .get("/api/entities/roots", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");
                
                // shared_ptrでレスポンスの状態を管理
                auto response_state = std::make_shared<bool>(true);
                
                // レスポンスが中断された場合のハンドラーを設定（必須）
                res->onAborted([response_state]() {
                    // リクエストが中断された場合、状態を無効にする
                    *response_state = false;
                });
                
                // リクエストを待ち行列に追加
                queue_request(APIRequest::GET_ROOT_ENTITIES, [res, response_state](const std::string& response_data) {
                    // レスポンスが有効かチェックして送信
                    if (*response_state) {
                        res->end(response_data);
                    }
                });
            })
            .get("/api/entities/ids/:id/components", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");
                
                // URLパラメータからエンティティIDを取得
                std::string id_str(req->getParameter(0));
                uint32_t entity_id = 0;
                try {
                    entity_id = std::stoul(id_str);
                } catch (...) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Invalid entity ID\"}");
                    return;
                }
                
                auto response_state = std::make_shared<bool>(true);
                res->onAborted([response_state]() {
                    *response_state = false;
                });
                
                queue_request(APIRequest::GET_ENTITY_COMPONENTS, entity_id, [res, response_state](const std::string& response_data) {
                    if (*response_state) {
                        res->end(response_data);
                    }
                });
            })
            .get("/api/entities/ids/:id", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");
                
                // URLパラメータからエンティティIDを取得
                std::string id_str(req->getParameter(0));
                uint32_t entity_id = 0;
                try {
                    entity_id = std::stoul(id_str);
                } catch (...) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Invalid entity ID\"}");
                    return;
                }
                
                auto response_state = std::make_shared<bool>(true);
                res->onAborted([response_state]() {
                    *response_state = false;
                });
                
                queue_request(APIRequest::GET_ENTITY_INFO, entity_id, [res, response_state](const std::string& response_data) {
                    if (*response_state) {
                        res->end(response_data);
                    }
                });
            })
            .listen(8080, [this](auto *socket) {
                if (socket) {
                    listen_socket_ = socket;
                    logger_->info(__FILE__, __LINE__) << "Editor server listening on port 8080";
                } else {
                    logger_->error(__FILE__, __LINE__) << "Failed to listen on port 8080";
                }
            });
            
            app.run();
        });
    }

    ~EditorServer() {
        if (listen_socket_) {
            us_listen_socket_close(0, listen_socket_);
        }
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    std::string get_root_entities_json() {
        std::string result;
        nodec::StringBuilder json(result);
        json << "{\"entities\":[";
        
        try {
            auto& scene = world_->scene();
            auto& registry = scene.registry();
            
            // Get all entities that don't have a parent (root entities)
            bool first = true;
            auto view = registry.view<nodec_scene::components::Hierarchy>();
            for (auto entity : view) {
                const auto& hierarchy = registry.get_component<nodec_scene::components::Hierarchy>(entity);
                if (hierarchy.parent == nodec::entities::null_entity) {
                    if (!first) json << ",";
                    json << "{\"id\":" << static_cast<uint32_t>(entity);
                    
                    // Nameコンポーネントを直接取得（高速）
                    auto* name = registry.try_get_component<nodec_scene::components::Name>(entity);
                    if (name) {
                        json << ",\"name\":\"" << name->value << "\"";
                    } else {
                        json << ",\"name\":\"Entity_" << static_cast<uint32_t>(entity) << "\"";
                    }
                    
                    // Check if entity has children
                    bool has_children = (hierarchy.first != nodec::entities::null_entity);
                    json << ",\"has_children\":" << (has_children ? "true" : "false");
                    
                    json << "}";
                    first = false;
                }
            }
            
        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error getting root entities: " << e.what();
        }
        
        json << "]}";
        return result;
    }
    
    std::string get_entity_components_json(uint32_t entity_id) {
        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& registry = world_->scene().registry();
        
        if (!registry.is_valid(entity)) {
            return "{\"error\":\"Invalid entity\"}";
        }
        
        std::string result;
        nodec::StringBuilder json(result);
        json << "{\"id\":" << entity_id << ",\"components\":[";
        
        // visitを使って全コンポーネントを収集してシリアライズ
        bool first = true;
        registry.visit(entity, [&](const nodec::type_info& type_info, void* component) {
            if (!scene_serialization_) {
                // SceneSerializationが利用できない場合はインデックスのみ
                if (!first) json << ",";
                json << "{\"type_index\":" << type_info.seq_index() << ",\"data\":null}";
                first = false;
                return;
            }
            
            // SceneSerializationを使ってコンポーネントをシリアライズ
            auto serializable = scene_serialization_->make_serializable_component(type_info, component);
            if (!serializable) {
                // シリアライズ可能でないコンポーネント
                if (!first) json << ",";
                json << "{\"type_index\":" << type_info.seq_index() << ",\"data\":null}";
                first = false;
                return;
            }
            
            // JSONArchiveを使ってシリアライズ
            std::ostringstream oss;
            {
                nodec_scene_serialization::ArchiveContext context(*scene_serialization_, *resource_registry_);
                cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONOutputArchive> archive(context, oss, cereal::JSONOutputArchive::Options::NoIndent());
                archive(cereal::make_nvp("component", serializable));
            }
            
            if (!first) json << ",";
            json << "{\"type_index\":" << type_info.seq_index() << ",\"data\":" << oss.str() << "}";
            first = false;
        });
        
        json << "]}";
        return result;
    }
    
    std::string get_entity_info_json(uint32_t entity_id) {
        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& registry = world_->scene().registry();
        
        if (!registry.is_valid(entity)) {
            return "{\"error\":\"Invalid entity\"}";
        }
        
        std::string result;
        nodec::StringBuilder json(result);
        json << "{\"id\":" << entity_id;
        
        // Name（直接取得）
        auto* name = registry.try_get_component<nodec_scene::components::Name>(entity);
        if (name) {
            json << ",\"name\":\"" << name->value << "\"";
        } else {
            json << ",\"name\":\"Entity_" << entity_id << "\"";
        }
        
        // Hierarchy情報（直接取得）
        auto* hierarchy = registry.try_get_component<nodec_scene::components::Hierarchy>(entity);
        if (hierarchy) {
            json << ",\"hierarchy\":{";
            
            // parent
            if (hierarchy->parent != nodec::entities::null_entity) {
                json << "\"parent\":" << static_cast<uint32_t>(hierarchy->parent);
            } else {
                json << "\"parent\":null";
            }
            
            // children（リンクリストを辿る）
            json << ",\"children\":[";
            if (hierarchy->first != nodec::entities::null_entity) {
                bool first_child = true;
                auto child = hierarchy->first;
                while (child != nodec::entities::null_entity) {
                    if (!first_child) json << ",";
                    json << static_cast<uint32_t>(child);
                    
                    // 次の兄弟を取得
                    auto* child_hierarchy = registry.try_get_component<nodec_scene::components::Hierarchy>(child);
                    child = child_hierarchy ? child_hierarchy->next : nodec::entities::null_entity;
                    first_child = false;
                }
            }
            json << "]}";
        }
        
        json << "}";
        return result;
    }

public:
    // Editor::update()から呼び出される非同期リクエスト処理
    void process_pending_requests() {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        
        while (!request_queue_.empty()) {
            auto request = std::move(request_queue_.front());
            request_queue_.pop();
            
            std::string response_data;
            switch (request.type) {
                case APIRequest::GET_ROOT_ENTITIES:
                    response_data = get_root_entities_json();
                    break;
                case APIRequest::GET_ENTITY_COMPONENTS:
                    response_data = get_entity_components_json(request.entity_id);
                    break;
                case APIRequest::GET_ENTITY_INFO:
                    response_data = get_entity_info_json(request.entity_id);
                    break;
                default:
                    response_data = "{\"error\": \"Unknown request type\"}";
                    break;
            }
            
            // uWebSocketsのメインループでレスポンスを送信
            if (main_loop_) {
                main_loop_->defer([callback = std::move(request.response_callback), response_data]() {
                    callback(response_data);
                });
            }
        }
    }

private:
    // 非同期リクエストを待ち行列に追加
    void queue_request(APIRequest::Type type, std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        request_queue_.emplace(type, std::move(callback));
    }
    
    void queue_request(APIRequest::Type type, uint32_t entity_id, std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        request_queue_.emplace(type, entity_id, std::move(callback));
    }

private:
    nodec_world::World* world_;
    nodec_scene_serialization::SceneSerialization* scene_serialization_;
    nodec::resource_management::ResourceRegistry* resource_registry_;
    std::shared_ptr<nodec::logging::Logger> logger_;
    us_listen_socket_t *listen_socket_{nullptr};
    std::thread thread_;
    
    // 非同期処理用
    uWS::Loop* main_loop_{nullptr};
    std::queue<APIRequest> request_queue_;
    std::mutex request_queue_mutex_;
};

#endif