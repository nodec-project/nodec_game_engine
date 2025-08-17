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

// Animation module includes
#include <nodec_animation/components/animator.hpp>
#include <nodec_animation/resources/animation_clip.hpp>

// APIリクエスト情報を保持する構造体
struct APIRequest {
    enum Type {
        GET_ROOT_ENTITIES,
        GET_ENTITY_COMPONENTS,
        GET_ENTITY_INFO,
        GET_ANIMATION_EDITING_CONTEXT
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
            .get("/api/animations/editing-context", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");
                
                // Get entityId from query parameter
                std::string_view query = req->getQuery();
                std::string entity_id_str = parseQueryParam(query, "entityId");
                
                if (entity_id_str.empty()) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Missing entityId parameter\"}");
                    return;
                }
                
                uint32_t entity_id = 0;
                try {
                    entity_id = std::stoul(entity_id_str);
                } catch (...) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Invalid entityId\"}");
                    return;
                }
                
                auto response_state = std::make_shared<bool>(true);
                res->onAborted([response_state]() {
                    *response_state = false;
                });
                
                queue_request(APIRequest::GET_ANIMATION_EDITING_CONTEXT, entity_id, [res, response_state](const std::string& response_data) {
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
    
    // Parse query parameter from query string
    std::string parseQueryParam(std::string_view query, const std::string& key) {
        size_t key_pos = query.find(key + "=");
        if (key_pos == std::string_view::npos) return "";
        
        size_t value_start = key_pos + key.length() + 1;
        size_t value_end = query.find('&', value_start);
        
        if (value_end == std::string_view::npos) {
            return std::string(query.substr(value_start));
        }
        return std::string(query.substr(value_start, value_end - value_start));
    }
    
    // Helper function to count curves in an AnimatedEntity recursively
    int count_curves_recursive(const nodec_animation::resources::AnimatedEntity& entity) {
        int count = 0;
        
        // Count curves in this entity's components
        for (const auto& [type, component] : entity.components) {
            count += static_cast<int>(component.properties.size());
        }
        
        // Count curves in children
        for (const auto& [child_name, child_entity] : entity.children) {
            count += count_curves_recursive(child_entity);
        }
        
        return count;
    }
    
    // Helper function to get the maximum time from all curves
    float get_clip_duration(const nodec_animation::resources::AnimatedEntity& entity) {
        float max_time = 0.0f;
        
        // Check all properties in this entity
        for (const auto& [type, component] : entity.components) {
            for (const auto& [prop_name, prop] : component.properties) {
                if (!prop.curve.keyframes().empty()) {
                    max_time = std::max(max_time, prop.curve.keyframes().back().time);
                }
            }
        }
        
        // Check children recursively
        for (const auto& [child_name, child_entity] : entity.children) {
            max_time = std::max(max_time, get_clip_duration(child_entity));
        }
        
        return max_time;
    }
    
    // Helper function to serialize curve data to JSON
    void serialize_curve_json(nodec::StringBuilder& json, const std::string& property_path, 
                             const nodec_animation::AnimationCurve& curve) {
        json << "{";
        json << "\"propertyPath\":\"" << property_path << "\",";
        json << "\"keyframes\":[";
        
        bool first = true;
        for (const auto& keyframe : curve.keyframes()) {
            if (!first) json << ",";
            json << "{\"time\":" << keyframe.time << ",\"value\":" << keyframe.value << "}";
            first = false;
        }
        
        json << "],";
        json << "\"wrapMode\":\"" << static_cast<int>(curve.wrap_mode()) << "\"";
        json << "}";
    }
    
    // Helper function to collect all curves from AnimatedEntity
    void collect_curves_json(nodec::StringBuilder& json, 
                            const nodec_animation::resources::AnimatedEntity& entity,
                            const std::string& entity_path,
                            bool& first_curve) {
        // Collect curves from this entity's components
        for (const auto& [type_info, component] : entity.components) {
            std::string component_name = "Component_" + std::to_string(type_info.seq_index());
            
            // Try to get the actual component type name if possible
            // This would require a type registry, so we'll use the index for now
            
            for (const auto& [prop_name, prop] : component.properties) {
                if (!first_curve) json << ",";
                
                std::string full_path = entity_path.empty() ? 
                    component_name + "/" + prop_name :
                    entity_path + "/" + component_name + "/" + prop_name;
                    
                serialize_curve_json(json, full_path, prop.curve);
                first_curve = false;
            }
        }
        
        // Collect curves from children recursively
        for (const auto& [child_name, child_entity] : entity.children) {
            std::string child_path = entity_path.empty() ? child_name : entity_path + "/" + child_name;
            collect_curves_json(json, child_entity, child_path, first_curve);
        }
    }
    
    // Get animation editing context for an entity with Animator component
    std::string get_animation_editing_context_json(uint32_t entity_id) {
        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& registry = world_->scene().registry();
        
        if (!registry.is_valid(entity)) {
            return "{\"error\":\"Invalid entity\"}";
        }
        
        std::string result;
        nodec::StringBuilder json(result);
        json << "{";
        
        // Get entity name
        auto* name = registry.try_get_component<nodec_scene::components::Name>(entity);
        std::string entity_name = name ? name->value : ("Entity_" + std::to_string(entity_id));
        
        // Check for Animator component
        auto* animator = registry.try_get_component<nodec_animation::components::Animator>(entity);
        if (!animator) {
            json << "\"hasAnimator\":false,";
            json << "\"entityId\":" << entity_id << ",";
            json << "\"entityName\":\"" << entity_name << "\",";
            json << "\"error\":\"Entity does not have Animator component\"";
            json << "}";
            return result;
        }
        
        // Basic response with Animator info
        json << "\"hasAnimator\":true,";
        json << "\"entityId\":" << entity_id << ",";
        json << "\"entityName\":\"" << entity_name << "\",";
        
        // Check if clip exists
        if (animator->clip) {
            json << "\"hasClip\":true,";
            
            // Try to get the resource path from resource registry
            std::string clip_path = "[clip loaded]";
            // TODO: Get actual resource path from resource registry when available
            // clip_path = resource_registry_->get_resource_path(animator->clip);
            
            json << "\"clipPath\":\"" << clip_path << "\",";
            
            // Get detailed clip data
            const auto& root_entity = animator->clip->root_entity();
            float duration = get_clip_duration(root_entity);
            int curve_count = count_curves_recursive(root_entity);
            
            json << "\"clipData\":{";
            json << "\"duration\":" << duration << ",";
            json << "\"curveCount\":" << curve_count << ",";
            
            // Add curves array
            json << "\"curves\":[";
            bool first_curve = true;
            collect_curves_json(json, root_entity, "", first_curve);
            json << "],";
            
            // Add property paths for UI to show available properties
            json << "\"availableProperties\":[";
            
            // Step 3: Add property reflection
            // This will use the scene serialization to discover available properties
            bool first_prop = true;
            registry.visit(entity, [&](const nodec::type_info& type_info, void* component) {
                if (!scene_serialization_) return;
                
                auto serializable = scene_serialization_->make_serializable_component(type_info, component);
                if (!serializable) return;
                
                // For now, just list the component types that have serializable properties
                if (!first_prop) json << ",";
                json << "{";
                json << "\"componentType\":\"" << type_info.seq_index() << "\",";
                json << "\"componentName\":\"Component_" << type_info.seq_index() << "\",";
                
                // TODO: Step 4 - Use property visitor to enumerate actual property names
                json << "\"properties\":[]";
                json << "}";
                first_prop = false;
            });
            
            json << "]";  // End availableProperties
            json << "}";  // End clipData
        } else {
            json << "\"hasClip\":false,";
            json << "\"clipPath\":null,";
            json << "\"clipData\":null";
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
                case APIRequest::GET_ANIMATION_EDITING_CONTEXT:
                    response_data = get_animation_editing_context_json(request.entity_id);
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