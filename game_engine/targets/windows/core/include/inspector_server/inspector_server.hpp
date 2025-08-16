
#ifndef NODEC_GAME_ENGINE__INSPECTOR_SERVER_HPP_
#define NODEC_GAME_ENGINE__INSPECTOR_SERVER_HPP_

#include <thread>
#include <memory>
#include <queue>
#include <mutex>
#include <functional>

#include <nodec/logging/logger.hpp>
#include <nodec/string_builder.hpp>
#include <nodec_world/world.hpp>
#include <nodec_scene/components/hierarchy.hpp>
#include <nodec_scene/systems/hierarchy_system.hpp>

#include <uwebsockets/App.h>

// APIリクエスト情報を保持する構造体
struct APIRequest {
    enum Type {
        GET_ROOT_ENTITIES
    };
    
    Type type;
    std::function<void(const std::string&)> response_callback;
    bool is_valid = true;  // リクエストが有効かどうか
    
    APIRequest(Type t, std::function<void(const std::string&)> callback)
        : type(t), response_callback(std::move(callback)) {}
};

class InspectorServer {
public:
    InspectorServer(std::shared_ptr<nodec_world::World> world) 
        : world_(world), logger_(nodec::logging::get_logger("inspector_server")) {
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
            .listen(8080, [this](auto *socket) {
                if (socket) {
                    listen_socket_ = socket;
                    logger_->info(__FILE__, __LINE__) << "Inspector server listening on port 8080";
                } else {
                    logger_->error(__FILE__, __LINE__) << "Failed to listen on port 8080";
                }
            });
            
            app.run();
        });
    }

    ~InspectorServer() {
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
                    json << "{";
                    json << "\"id\":\"" << static_cast<uint32_t>(entity) << "\",";
                    json << "\"name\":\"Entity_" << static_cast<uint32_t>(entity) << "\",";
                    json << "\"has_children\":" << (hierarchy.first != nodec::entities::null_entity ? "true" : "false");
                    json << "}";
                    first = false;
                }
            }
            
            // Also include entities without Hierarchy component (they are also root entities)
            // This is more complex as we need to iterate all entities and check for Hierarchy component
            // For now, we'll focus on entities with Hierarchy component only
            
        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error getting root entities: " << e.what();
        }
        
        json << "]}";
        return result;
    }

public:
    // Engine::frame_end()から呼び出される非同期リクエスト処理
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

private:
    std::shared_ptr<nodec_world::World> world_;
    std::shared_ptr<nodec::logging::Logger> logger_;
    us_listen_socket_t *listen_socket_{nullptr};
    std::thread thread_;
    
    // 非同期処理用
    uWS::Loop* main_loop_{nullptr};
    std::queue<APIRequest> request_queue_;
    std::mutex request_queue_mutex_;
};

#endif