#include "editor_server.hpp"

#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <sstream>
#include <fstream>
#include <set>
#include <unordered_map>

#include <nodec/logging/logging.hpp>
#include <nodec/string_builder.hpp>
#include <nodec/concurrent/thread_pool_executor.hpp>
#include <nodec_scene/components/hierarchy.hpp>
#include <nodec_scene/components/name.hpp>
#include <nodec_scene_serialization/archive_context.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/details/helpers.hpp>

#include <uwebsockets/App.h>

#include <nodec_animation/resources/animation_clip.hpp>
#include <nodec_animation/serialization/resources/animation_clip.hpp>

// API response with HTTP status
struct APIResponse {
    std::string status;  // HTTP status string, e.g., "200 OK", "400 Bad Request", "500 Internal Server Error"
    std::string body;

    static APIResponse ok(std::string body) {
        return {"200 OK", std::move(body)};
    }
    static APIResponse bad_request(std::string body) {
        return {"400 Bad Request", std::move(body)};
    }
    static APIResponse not_found(std::string body) {
        return {"404 Not Found", std::move(body)};
    }
    static APIResponse internal_error(std::string body) {
        return {"500 Internal Server Error", std::move(body)};
    }
};

// API request info
struct APIRequest {
    enum Type {
        GET_ROOT_ENTITIES,
        GET_ENTITY_COMPONENTS,
        GET_ENTITY_INFO,
        GET_RESOURCE,
        PATCH_ENTITY_COMPONENTS,
        PUT_RESOURCE,
        GET_REGISTERED_COMPONENTS
    };

    Type type;
    uint32_t entity_id;
    std::string resource_type;
    std::string resource_name;
    std::string request_body;
    std::function<void(const APIResponse&)> response_callback;
    bool is_valid = true;

    APIRequest(Type t, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(0), response_callback(std::move(callback)) {}

    APIRequest(Type t, uint32_t id, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(id), response_callback(std::move(callback)) {}

    APIRequest(Type t, std::string res_type, std::string res_name, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(0), resource_type(std::move(res_type)), resource_name(std::move(res_name)), response_callback(std::move(callback)) {}

    APIRequest(Type t, uint32_t id, std::string body, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(id), request_body(std::move(body)), response_callback(std::move(callback)) {}

    // Constructor for PUT_RESOURCE (resource_type, resource_name, body)
    APIRequest(Type t, std::string res_type, std::string res_name, std::string body, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(0), resource_type(std::move(res_type)), resource_name(std::move(res_name)),
          request_body(std::move(body)), response_callback(std::move(callback)) {}
};

// WebSocket per-socket data
struct WebSocketData {
    std::set<uint32_t> subscribed_entities;
};

// WebSocket message structure for cereal deserialization
struct WsSubscribePayload {
    uint32_t entity_id = 0;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::make_nvp("entity_id", entity_id));
    }
};

struct WsMessage {
    std::string event;
    WsSubscribePayload payload;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::make_nvp("event", event));
        archive(cereal::make_nvp("payload", payload));
    }
};

// Pending WebSocket broadcast message
struct WsBroadcastMessage {
    uint32_t entity_id;
    std::string message;
};

class EditorServer::Impl {
public:
    Impl(nodec_world::World* world,
         nodec_scene_serialization::SceneSerialization* scene_serialization,
         nodec_resources::Resources* resources)
        : world_(world), scene_serialization_(scene_serialization), resources_(resources),
          logger_(nodec::logging::get_logger("editor_server")),
          file_write_executor_(1) {
        thread_ = std::thread([this]() {
            auto app = uWS::App();

            // uWS::Loop pointer for defer
            main_loop_ = uWS::Loop::get();

            // CORS Preflight handler for all /api/* routes
            app.options("/api/*", [](auto *res, auto *req) {
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                res->writeHeader("Access-Control-Max-Age", "86400");
                res->end();
            })
            .get("/api/components", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

                auto response_state = std::make_shared<bool>(true);

                res->onAborted([response_state]() {
                    *response_state = false;
                });

                queue_request(APIRequest::GET_REGISTERED_COMPONENTS, [res, response_state](const APIResponse& response) {
                    if (*response_state) {
                        res->writeStatus(response.status);
                        res->end(response.body);
                    }
                });
            })
            .get("/api/entities/roots", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

                auto response_state = std::make_shared<bool>(true);

                res->onAborted([response_state]() {
                    *response_state = false;
                });

                queue_request(APIRequest::GET_ROOT_ENTITIES, [res, response_state](const APIResponse& response) {
                    if (*response_state) {
                        res->writeStatus(response.status);
                        res->end(response.body);
                    }
                });
            })
            .get("/api/entities/ids/:id/components", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

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

                queue_request(APIRequest::GET_ENTITY_COMPONENTS, entity_id, [res, response_state](const APIResponse& response) {
                    if (*response_state) {
                        res->writeStatus(response.status);
                        res->end(response.body);
                    }
                });
            })
            .patch("/api/entities/ids/:id/components", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, PATCH, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

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
                auto body_buffer = std::make_shared<std::string>();
                auto captured_entity_id = entity_id;

                res->onAborted([response_state]() {
                    *response_state = false;
                });

                // Read request body using onData callback
                res->onData([this, res, response_state, body_buffer, captured_entity_id](std::string_view chunk, bool is_last) {
                    body_buffer->append(chunk.data(), chunk.size());

                    if (is_last) {
                        queue_request(APIRequest::PATCH_ENTITY_COMPONENTS, captured_entity_id, std::move(*body_buffer),
                            [res, response_state](const APIResponse& response) {
                                if (*response_state) {
                                    res->writeStatus(response.status);
                                    res->end(response.body);
                                }
                            });
                    }
                });
            })
            .get("/api/entities/ids/:id", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

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

                queue_request(APIRequest::GET_ENTITY_INFO, entity_id, [res, response_state](const APIResponse& response) {
                    if (*response_state) {
                        res->writeStatus(response.status);
                        res->end(response.body);
                    }
                });
            })
            .get("/api/resources/:type/*", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

                std::string_view url = req->getUrl();
                const std::string_view prefix = "/api/resources/";

                if (url.substr(0, prefix.size()) != prefix) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Invalid URL format\"}");
                    return;
                }

                std::string_view rest = url.substr(prefix.size());
                size_t slash_pos = rest.find('/');
                if (slash_pos == std::string_view::npos) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Missing resource name\"}");
                    return;
                }

                std::string resource_type(rest.substr(0, slash_pos));
                std::string resource_name(rest.substr(slash_pos + 1));

                logger_->info(__FILE__, __LINE__) << "Resource request: type=" << resource_type << ", name=" << resource_name;

                if (resource_type.empty() || resource_name.empty()) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Missing resource type or name\"}");
                    return;
                }

                auto response_state = std::make_shared<bool>(true);
                res->onAborted([response_state]() {
                    *response_state = false;
                });

                queue_request(APIRequest::GET_RESOURCE, resource_type, resource_name,
                    [res, response_state](const APIResponse& response) {
                        if (*response_state) {
                            res->writeStatus(response.status);
                            res->end(response.body);
                        }
                    });
            })
            .put("/api/resources/:type/*", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, PUT, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

                std::string_view url = req->getUrl();
                const std::string_view prefix = "/api/resources/";

                if (url.substr(0, prefix.size()) != prefix) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Invalid URL format\"}");
                    return;
                }

                std::string_view rest = url.substr(prefix.size());
                size_t slash_pos = rest.find('/');
                if (slash_pos == std::string_view::npos) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Missing resource name\"}");
                    return;
                }

                std::string resource_type(rest.substr(0, slash_pos));
                std::string resource_name(rest.substr(slash_pos + 1));

                logger_->info(__FILE__, __LINE__) << "Resource PUT request: type=" << resource_type << ", name=" << resource_name;

                if (resource_type.empty() || resource_name.empty()) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Missing resource type or name\"}");
                    return;
                }

                auto response_state = std::make_shared<bool>(true);
                auto body_buffer = std::make_shared<std::string>();
                auto captured_type = resource_type;
                auto captured_name = resource_name;

                res->onAborted([response_state]() {
                    *response_state = false;
                });

                // Read request body using onData callback
                res->onData([this, res, response_state, body_buffer, captured_type, captured_name](std::string_view chunk, bool is_last) {
                    body_buffer->append(chunk.data(), chunk.size());

                    if (is_last) {
                        queue_request(APIRequest::PUT_RESOURCE, captured_type, captured_name, std::move(*body_buffer),
                            [res, response_state](const APIResponse& response) {
                                if (*response_state) {
                                    res->writeStatus(response.status);
                                    res->end(response.body);
                                }
                            });
                    }
                });
            })
            // WebSocket endpoint for real-time updates
            .ws<WebSocketData>("/ws", {
                .compression = uWS::DISABLED,
                .maxPayloadLength = 16 * 1024,
                .idleTimeout = 120,
                .maxBackpressure = 1 * 1024 * 1024,
                .closeOnBackpressureLimit = false,
                .resetIdleTimeoutOnSend = true,
                .sendPingsAutomatically = true,

                .open = [this](auto *ws) {
                    logger_->info(__FILE__, __LINE__) << "WebSocket client connected";
                    std::lock_guard<std::mutex> lock(ws_clients_mutex_);
                    ws_clients_.insert(ws);
                },

                .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
                    handle_ws_message(ws, message);
                },

                .close = [this](auto *ws, int code, std::string_view message) {
                    logger_->info(__FILE__, __LINE__) << "WebSocket client disconnected";
                    std::lock_guard<std::mutex> lock(ws_clients_mutex_);
                    ws_clients_.erase(ws);
                }
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

    ~Impl() {
        // Close all WebSocket clients and listen socket from the uWS thread
        if (main_loop_) {
            main_loop_->defer([this]() {
                // Copy the set before iterating because ws->close() triggers
                // the close callback which erases from ws_clients_ (iterator invalidation)
                auto clients_copy = ws_clients_;
                ws_clients_.clear();

                for (auto* ws : clients_copy) {
                    ws->close();
                }

                // Close the listen socket to stop accepting new connections
                if (listen_socket_) {
                    us_listen_socket_close(0, listen_socket_);
                    listen_socket_ = nullptr;
                }
            });
        }

        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void process_pending_requests() {
        // Process HTTP requests
        {
            std::lock_guard<std::mutex> lock(request_queue_mutex_);

            while (!request_queue_.empty()) {
                auto request = std::move(request_queue_.front());
                request_queue_.pop();

                APIResponse response;
                switch (request.type) {
                    case APIRequest::GET_ROOT_ENTITIES:
                        response = get_root_entities_json();
                        break;
                    case APIRequest::GET_ENTITY_COMPONENTS:
                        response = get_entity_components_json(request.entity_id);
                        break;
                    case APIRequest::GET_ENTITY_INFO:
                        response = get_entity_info_json(request.entity_id);
                        break;
                    case APIRequest::GET_RESOURCE:
                        response = get_resource_json(request.resource_type, request.resource_name);
                        break;
                    case APIRequest::PATCH_ENTITY_COMPONENTS:
                        response = update_entity_components(request.entity_id, request.request_body);
                        break;
                    case APIRequest::PUT_RESOURCE:
                        response = update_resource_json(request.resource_type, request.resource_name, request.request_body);
                        break;
                    case APIRequest::GET_REGISTERED_COMPONENTS:
                        response = get_registered_components_json();
                        break;
                    default:
                        response = APIResponse::bad_request("{\"error\": \"Unknown request type\"}");
                        break;
                }

                if (main_loop_) {
                    main_loop_->defer([callback = std::move(request.response_callback), response = std::move(response)]() {
                        callback(response);
                    });
                }
            }
        }

        // Broadcast component updates to subscribed WebSocket clients
        broadcast_subscribed_updates();
    }

private:
    // WebSocket message handler (runs on uWS thread)
    void handle_ws_message(uWS::WebSocket<false, true, WebSocketData>* ws, std::string_view message) {
        try {
            // Wrap the message for cereal deserialization
            // cereal::JSONInputArchive requires a named root element
            std::string wrapped_json = "{\"message\":" + std::string(message) + "}";
            std::istringstream iss(wrapped_json);
            cereal::JSONInputArchive archive(iss);

            WsMessage msg;
            archive(cereal::make_nvp("message", msg));

            if (msg.event == "subscribe") {
                auto* data = ws->getUserData();
                data->subscribed_entities.insert(msg.payload.entity_id);

                logger_->info(__FILE__, __LINE__) << "Client subscribed to entity " << msg.payload.entity_id;

                std::string response = "{\"event\":\"subscribed\",\"payload\":{\"entity_id\":" +
                                        std::to_string(msg.payload.entity_id) + "}}";
                ws->send(response, uWS::OpCode::TEXT);

            } else if (msg.event == "unsubscribe") {
                auto* data = ws->getUserData();
                data->subscribed_entities.erase(msg.payload.entity_id);

                logger_->info(__FILE__, __LINE__) << "Client unsubscribed from entity " << msg.payload.entity_id;

                std::string response = "{\"event\":\"unsubscribed\",\"payload\":{\"entity_id\":" +
                                        std::to_string(msg.payload.entity_id) + "}}";
                ws->send(response, uWS::OpCode::TEXT);

            } else {
                ws->send("{\"event\":\"error\",\"payload\":{\"message\":\"Unknown event type\"}}", uWS::OpCode::TEXT);
            }

        } catch (const std::exception& e) {
            logger_->warn(__FILE__, __LINE__) << "Failed to parse WebSocket message: " << e.what();
            ws->send("{\"event\":\"error\",\"payload\":{\"message\":\"Invalid JSON format\"}}", uWS::OpCode::TEXT);
        }
    }

    // Broadcast component updates to subscribed clients
    // Called from game thread - queues messages for uWS thread
    void broadcast_subscribed_updates() {
        // Early exit without lock if no pending broadcasts needed
        {
            std::lock_guard<std::mutex> lock(ws_clients_mutex_);
            if (ws_clients_.empty()) return;
        }

        // Collect subscribed entity IDs (lock scope minimized)
        std::set<uint32_t> all_subscribed_entities;
        {
            std::lock_guard<std::mutex> lock(ws_clients_mutex_);
            for (auto* ws : ws_clients_) {
                const auto* data = ws->getUserData();
                all_subscribed_entities.insert(
                    data->subscribed_entities.begin(),
                    data->subscribed_entities.end()
                );
            }
        }

        if (all_subscribed_entities.empty()) return;

        // Generate component JSON for each subscribed entity (no lock needed)
        std::vector<WsBroadcastMessage> messages;
        messages.reserve(all_subscribed_entities.size());

        for (uint32_t entity_id : all_subscribed_entities) {
            auto response = get_entity_components_json(entity_id);
            messages.push_back({
                entity_id,
                "{\"event\":\"component_update\",\"payload\":" + std::move(response.body) + "}"
            });
        }

        // Queue messages for uWS thread
        // The defer callback will look up valid clients at execution time
        if (main_loop_) {
            main_loop_->defer([this, messages = std::move(messages)]() {
                // This runs on uWS thread - safe to access ws_clients_ and send
                // No mutex needed here because uWS is single-threaded
                // and close/open callbacks also run on this thread
                for (auto* ws : ws_clients_) {
                    const auto* data = ws->getUserData();
                    for (const auto& msg : messages) {
                        if (data->subscribed_entities.count(msg.entity_id) > 0) {
                            ws->send(msg.message, uWS::OpCode::TEXT);
                        }
                    }
                }
            });
        }
    }

    APIResponse get_root_entities_json() {
        std::string result;
        nodec::StringBuilder json(result);
        json << "{\"entities\":[";

        try {
            auto& scene = world_->scene();
            auto& registry = scene.registry();

            bool first = true;
            auto view = registry.view<nodec_scene::components::Hierarchy>();
            for (auto entity : view) {
                const auto& hierarchy = registry.get_component<nodec_scene::components::Hierarchy>(entity);
                if (hierarchy.parent == nodec::entities::null_entity) {
                    if (!first) json << ",";
                    json << "{\"id\":" << static_cast<uint32_t>(entity);

                    auto* name = registry.try_get_component<nodec_scene::components::Name>(entity);
                    if (name) {
                        json << ",\"name\":\"" << name->value << "\"";
                    } else {
                        json << ",\"name\":\"Entity_" << static_cast<uint32_t>(entity) << "\"";
                    }

                    bool has_children = (hierarchy.first != nodec::entities::null_entity);
                    json << ",\"has_children\":" << (has_children ? "true" : "false");

                    json << "}";
                    first = false;
                }
            }

        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error getting root entities: " << e.what();
            return APIResponse::internal_error("{\"error\":\"" + std::string(e.what()) + "\"}");
        }

        json << "]}";
        return APIResponse::ok(result);
    }

    APIResponse get_registered_components_json() {
        if (!scene_serialization_ || !resources_) {
            return APIResponse::internal_error("{\"error\":\"Scene serialization or resource registry not available\"}");
        }

        std::string result;
        nodec::StringBuilder json(result);
        json << "{\"components\":[";

        bool first = true;
        nodec_scene_serialization::ArchiveContext context(*scene_serialization_, resources_->registry());

        scene_serialization_->for_each_component_serialization(
            [&](const nodec_scene_serialization::SceneSerialization::BaseComponentSerialization& serialization) {
                auto serializable = serialization.make_serializable_component();
                if (!serializable) return;

                if (!first) json << ",";
                first = false;

                // Serialize the empty component (ostringstream required for cereal)
                std::ostringstream component_oss;
                {
                    cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONOutputArchive>
                        archive(context, component_oss, cereal::JSONOutputArchive::Options::NoIndent());
                    archive(cereal::make_nvp("component", serializable));
                }

                json << "{\"runtime_type_index\":" << serialization.type_info().seq_index()
                     << ",\"data\":" << component_oss.str() << "}";
            });

        json << "]}";
        return APIResponse::ok(result);
    }

    APIResponse get_entity_components_json(uint32_t entity_id) {
        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& registry = world_->scene().registry();

        if (!registry.is_valid(entity)) {
            return APIResponse::not_found("{\"error\":\"Invalid entity\"}");
        }

        std::string result;
        nodec::StringBuilder json(result);
        json << "{\"id\":" << entity_id << ",\"components\":[";

        bool first = true;
        registry.visit(entity, [&](const nodec::type_info& type_info, void* component) {
            if (!scene_serialization_) {
                if (!first) json << ",";
                json << "{\"type_index\":" << type_info.seq_index() << ",\"data\":null}";
                first = false;
                return;
            }

            auto serializable = scene_serialization_->make_serializable_component(type_info, component);
            if (!serializable) {
                if (!first) json << ",";
                json << "{\"type_index\":" << type_info.seq_index() << ",\"data\":null}";
                first = false;
                return;
            }

            std::ostringstream oss;
            {
                nodec_scene_serialization::ArchiveContext context(*scene_serialization_, resources_->registry());
                cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONOutputArchive> archive(context, oss, cereal::JSONOutputArchive::Options::NoIndent());
                archive(cereal::make_nvp("component", serializable));
            }

            if (!first) json << ",";
            json << "{\"type_index\":" << type_info.seq_index() << ",\"data\":" << oss.str() << "}";
            first = false;
        });

        json << "]}";
        return APIResponse::ok(result);
    }

    APIResponse get_entity_info_json(uint32_t entity_id) {
        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& registry = world_->scene().registry();

        if (!registry.is_valid(entity)) {
            return APIResponse::not_found("{\"error\":\"Invalid entity\"}");
        }

        std::string result;
        nodec::StringBuilder json(result);
        json << "{\"id\":" << entity_id;

        auto* name = registry.try_get_component<nodec_scene::components::Name>(entity);
        if (name) {
            json << ",\"name\":\"" << name->value << "\"";
        } else {
            json << ",\"name\":\"Entity_" << entity_id << "\"";
        }

        auto* hierarchy = registry.try_get_component<nodec_scene::components::Hierarchy>(entity);
        if (hierarchy) {
            json << ",\"hierarchy\":{";

            if (hierarchy->parent != nodec::entities::null_entity) {
                json << "\"parent\":" << static_cast<uint32_t>(hierarchy->parent);
            } else {
                json << "\"parent\":null";
            }

            json << ",\"children\":[";
            if (hierarchy->first != nodec::entities::null_entity) {
                bool first_child = true;
                auto child = hierarchy->first;
                while (child != nodec::entities::null_entity) {
                    if (!first_child) json << ",";
                    json << static_cast<uint32_t>(child);

                    auto* child_hierarchy = registry.try_get_component<nodec_scene::components::Hierarchy>(child);
                    child = child_hierarchy ? child_hierarchy->next : nodec::entities::null_entity;
                    first_child = false;
                }
            }
            json << "]}";
        }

        json << "}";
        return APIResponse::ok(result);
    }

    APIResponse get_resource_json(const std::string& resource_type, const std::string& resource_name) {
        if (!resources_ || !scene_serialization_) {
            return APIResponse::internal_error("{\"error\":\"Resource registry or scene serialization not available\"}");
        }

        try {
            if (resource_type == "animation_clip") {
                auto clip = resources_->registry().get_resource_direct<nodec_animation::resources::AnimationClip>(resource_name);
                if (!clip) {
                    return APIResponse::not_found("{\"error\":\"Animation clip not found\",\"name\":\"" + resource_name + "\"}");
                }

                std::ostringstream oss;
                {
                    nodec_scene_serialization::ArchiveContext context(*scene_serialization_, resources_->registry());
                    cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONOutputArchive>
                        archive(context, oss, cereal::JSONOutputArchive::Options::NoIndent());
                    archive(cereal::make_nvp("clip", *clip));
                }

                return APIResponse::ok(oss.str());
            }

            return APIResponse::bad_request("{\"error\":\"Unsupported resource type\",\"type\":\"" + resource_type + "\"}");

        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error getting resource: " << e.what();
            return APIResponse::internal_error("{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    }

    APIResponse update_entity_components(uint32_t entity_id, const std::string& json_body) {
        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& registry = world_->scene().registry();

        if (!registry.is_valid(entity)) {
            return APIResponse::not_found("{\"error\":\"Invalid entity\",\"id\":" + std::to_string(entity_id) + "}");
        }

        if (!scene_serialization_ || !resources_) {
            return APIResponse::internal_error("{\"error\":\"Scene serialization or resource registry not available\"}");
        }

        try {
            std::istringstream iss(json_body);
            nodec_scene_serialization::ArchiveContext context(*scene_serialization_, resources_->registry());
            cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONInputArchive>
                archive(context, iss);

            std::vector<std::unique_ptr<nodec_scene_serialization::BaseSerializableComponent>> components;
            archive(cereal::make_nvp("components", components));

            int updated_count = 0;
            for (const auto& comp : components) {
                if (!comp) continue;

                scene_serialization_->emplace_or_replace_component(comp.get(), entity, registry);
                ++updated_count;
            }

            logger_->info(__FILE__, __LINE__) << "Updated " << updated_count << " components on entity " << entity_id;

            return APIResponse::ok("{\"success\":true,\"id\":" + std::to_string(entity_id) +
                   ",\"updated_count\":" + std::to_string(updated_count) + "}");

        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error updating entity components: " << e.what();
            return APIResponse::bad_request("{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    }

    APIResponse update_resource_json(const std::string& resource_type, const std::string& resource_name, const std::string& json_body) {
        if (!resources_ || !scene_serialization_) {
            return APIResponse::internal_error("{\"error\":\"Resource registry or scene serialization not available\"}");
        }

        try {
            if (resource_type == "animation_clip") {
                // Get the existing clip from the registry
                auto clip = resources_->registry().get_resource_direct<nodec_animation::resources::AnimationClip>(resource_name);
                if (!clip) {
                    return APIResponse::not_found("{\"error\":\"Animation clip not found\",\"name\":\"" + resource_name + "\"}");
                }

                // Deserialize the incoming JSON to update the clip
                // Expected format: { "clip": { "root_entity": { ... } } }
                std::istringstream iss(json_body);
                nodec_scene_serialization::ArchiveContext context(*scene_serialization_, resources_->registry());
                cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONInputArchive>
                    archive(context, iss);

                // Load into the existing clip (this will update the root_entity)
                archive(cereal::make_nvp("clip", *clip));

                // Write json_body directly to file in background thread
                std::string file_path = resources_->resource_path() + "/" + resource_name;
                file_write_executor_.submit([file_path, json_body, logger = logger_]() {
                    std::ofstream ofs(file_path);
                    if (ofs) {
                        ofs << json_body;
                        logger->info(__FILE__, __LINE__) << "Saved animation clip to file: " << file_path;
                    } else {
                        logger->error(__FILE__, __LINE__) << "Failed to open file for writing: " << file_path;
                    }
                });

                logger_->info(__FILE__, __LINE__) << "Updated animation clip: " << resource_name;

                return APIResponse::ok("{\"success\":true,\"type\":\"animation_clip\",\"name\":\"" + resource_name + "\"}");
            }

            return APIResponse::bad_request("{\"error\":\"Unsupported resource type for update\",\"type\":\"" + resource_type + "\"}");

        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error updating resource: " << e.what();
            return APIResponse::bad_request("{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    }

    void queue_request(APIRequest::Type type, std::function<void(const APIResponse&)> callback) {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        request_queue_.emplace(type, std::move(callback));
    }

    void queue_request(APIRequest::Type type, uint32_t entity_id, std::function<void(const APIResponse&)> callback) {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        request_queue_.emplace(type, entity_id, std::move(callback));
    }

    void queue_request(APIRequest::Type type, const std::string& resource_type, const std::string& resource_name,
                       std::function<void(const APIResponse&)> callback) {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        request_queue_.emplace(type, resource_type, resource_name, std::move(callback));
    }

    void queue_request(APIRequest::Type type, uint32_t entity_id, std::string body,
                       std::function<void(const APIResponse&)> callback) {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        request_queue_.emplace(type, entity_id, std::move(body), std::move(callback));
    }

    void queue_request(APIRequest::Type type, const std::string& resource_type, const std::string& resource_name,
                       std::string body, std::function<void(const APIResponse&)> callback) {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        request_queue_.emplace(type, resource_type, resource_name, std::move(body), std::move(callback));
    }

private:
    nodec_world::World* world_;
    nodec_scene_serialization::SceneSerialization* scene_serialization_;
    nodec_resources::Resources* resources_;
    std::shared_ptr<nodec::logging::Logger> logger_;
    us_listen_socket_t* listen_socket_{nullptr};
    std::thread thread_;

    // Async processing
    uWS::Loop* main_loop_{nullptr};
    std::queue<APIRequest> request_queue_;
    std::mutex request_queue_mutex_;

    // WebSocket clients
    std::set<uWS::WebSocket<false, true, WebSocketData>*> ws_clients_;
    std::mutex ws_clients_mutex_;

    // File writing executor (1 thread for sequential writes)
    nodec::concurrent::ThreadPoolExecutor file_write_executor_;
};

EditorServer::EditorServer(nodec_world::World* world,
                           nodec_scene_serialization::SceneSerialization* scene_serialization,
                           nodec_resources::Resources* resources)
    : impl_(std::make_unique<Impl>(world, scene_serialization, resources)) {
}

EditorServer::~EditorServer() = default;

void EditorServer::process_pending_requests() {
    impl_->process_pending_requests();
}
