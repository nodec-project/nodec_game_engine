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
#include <nodec_scene_serialization/components/prefab.hpp>
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
    static APIResponse conflict(std::string body) {
        return {"409 Conflict", std::move(body)};
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
        PATCH_ENTITY_HIERARCHY,
        PUT_RESOURCE,
        GET_REGISTERED_COMPONENTS,
        POST_ENTITY_COMPONENT,
        DELETE_ENTITY_COMPONENT
    };

    Type type;
    uint32_t entity_id;
    uint32_t type_index;  // For DELETE_ENTITY_COMPONENT
    std::string resource_type;
    std::string resource_name;
    std::string request_body;
    std::function<void(const APIResponse&)> response_callback;
    bool is_valid = true;

    APIRequest(Type t, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(0), type_index(0), response_callback(std::move(callback)) {}

    APIRequest(Type t, uint32_t id, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(id), type_index(0), response_callback(std::move(callback)) {}

    APIRequest(Type t, std::string res_type, std::string res_name, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(0), type_index(0), resource_type(std::move(res_type)), resource_name(std::move(res_name)), response_callback(std::move(callback)) {}

    APIRequest(Type t, uint32_t id, std::string body, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(id), type_index(0), request_body(std::move(body)), response_callback(std::move(callback)) {}

    // Constructor for PUT_RESOURCE (resource_type, resource_name, body)
    APIRequest(Type t, std::string res_type, std::string res_name, std::string body, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(0), type_index(0), resource_type(std::move(res_type)), resource_name(std::move(res_name)),
          request_body(std::move(body)), response_callback(std::move(callback)) {}

    // Constructor for DELETE_ENTITY_COMPONENT (entity_id, type_index)
    APIRequest(Type t, uint32_t id, uint32_t type_idx, std::function<void(const APIResponse&)> callback)
        : type(t), entity_id(id), type_index(type_idx), response_callback(std::move(callback)) {}
};

// WebSocket per-socket data
struct WebSocketData {
    std::set<uint32_t> subscribed_components;   // For component updates
    std::set<uint32_t> subscribed_entity_info;  // For entity_info updates
    bool subscribed_root_infos{false};          // For root entities updates
};

// Pending WebSocket broadcast message
struct WsBroadcastMessage {
    uint32_t entity_id;
    std::string message;
};

// WebSocket message payloads for cereal deserialization
struct WsComponentsPayload {
    uint32_t entity_id = 0;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::make_nvp("entity_id", entity_id));
    }
};

struct WsEntityInfoPayload {
    std::vector<uint32_t> entities;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::make_nvp("entities", entities));
    }
};

// Generic message with event name only (payload parsed separately based on event)
struct WsEventHeader {
    std::string event;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::make_nvp("event", event));
    }
};

// Full message structures for each event type
struct WsComponentsMessage {
    std::string event;
    WsComponentsPayload payload;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::make_nvp("event", event));
        archive(cereal::make_nvp("payload", payload));
    }
};

struct WsEntityInfoMessage {
    std::string event;
    WsEntityInfoPayload payload;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::make_nvp("event", event));
        archive(cereal::make_nvp("payload", payload));
    }
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
            .patch("/api/entities/ids/:id/hierarchy", [this](auto *res, auto *req) {
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

                res->onData([this, res, response_state, body_buffer, captured_entity_id](std::string_view chunk, bool is_last) {
                    body_buffer->append(chunk.data(), chunk.size());

                    if (is_last) {
                        queue_request(APIRequest::PATCH_ENTITY_HIERARCHY, captured_entity_id, std::move(*body_buffer),
                            [res, response_state](const APIResponse& response) {
                                if (*response_state) {
                                    res->writeStatus(response.status);
                                    res->end(response.body);
                                }
                            });
                    }
                });
            })
            .post("/api/entities/ids/:id/components", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
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
                        queue_request(APIRequest::POST_ENTITY_COMPONENT, captured_entity_id, std::move(*body_buffer),
                            [res, response_state](const APIResponse& response) {
                                if (*response_state) {
                                    res->writeStatus(response.status);
                                    res->end(response.body);
                                }
                            });
                    }
                });
            })
            .del("/api/entities/ids/:id/components/:type_index", [this](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/json");
                res->writeHeader("Access-Control-Allow-Origin", "*");
                res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
                res->writeHeader("Access-Control-Allow-Headers", "Content-Type");

                std::string id_str(req->getParameter(0));
                std::string type_index_str(req->getParameter(1));
                uint32_t entity_id = 0;
                uint32_t type_index = 0;
                try {
                    entity_id = std::stoul(id_str);
                    type_index = std::stoul(type_index_str);
                } catch (...) {
                    res->writeStatus("400 Bad Request");
                    res->end("{\"error\":\"Invalid entity ID or type index\"}");
                    return;
                }

                auto response_state = std::make_shared<bool>(true);
                res->onAborted([response_state]() {
                    *response_state = false;
                });

                queue_request(APIRequest::DELETE_ENTITY_COMPONENT, entity_id, type_index,
                    [res, response_state](const APIResponse& response) {
                        if (*response_state) {
                            res->writeStatus(response.status);
                            res->end(response.body);
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
                    case APIRequest::PATCH_ENTITY_HIERARCHY:
                        response = update_entity_hierarchy(request.entity_id, request.request_body);
                        break;
                    case APIRequest::PUT_RESOURCE:
                        response = update_resource_json(request.resource_type, request.resource_name, request.request_body);
                        break;
                    case APIRequest::GET_REGISTERED_COMPONENTS:
                        response = get_registered_components_json();
                        break;
                    case APIRequest::POST_ENTITY_COMPONENT:
                        response = add_entity_component(request.entity_id, request.request_body);
                        break;
                    case APIRequest::DELETE_ENTITY_COMPONENT:
                        response = remove_entity_component(request.entity_id, request.type_index);
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
            std::string wrapped_json = "{\"message\":" + std::string(message) + "}";

            // First, parse just the event name
            std::string event;
            {
                std::istringstream iss(wrapped_json);
                cereal::JSONInputArchive archive(iss);
                WsEventHeader header;
                archive(cereal::make_nvp("message", header));
                event = header.event;
            }

            auto* data = ws->getUserData();

            // Handle component subscription events
            if (event == "subscribe_components_update") {
                std::istringstream iss(wrapped_json);
                cereal::JSONInputArchive archive(iss);
                WsComponentsMessage msg;
                archive(cereal::make_nvp("message", msg));

                data->subscribed_components.insert(msg.payload.entity_id);
                logger_->info(__FILE__, __LINE__) << "Client subscribed to components of entity " << msg.payload.entity_id;

                std::string response = "{\"event\":\"subscribed_components_update\",\"payload\":{\"entity_id\":" +
                                        std::to_string(msg.payload.entity_id) + "}}";
                ws->send(response, uWS::OpCode::TEXT);

            } else if (event == "unsubscribe_components_update") {
                std::istringstream iss(wrapped_json);
                cereal::JSONInputArchive archive(iss);
                WsComponentsMessage msg;
                archive(cereal::make_nvp("message", msg));

                data->subscribed_components.erase(msg.payload.entity_id);
                logger_->info(__FILE__, __LINE__) << "Client unsubscribed from components of entity " << msg.payload.entity_id;

                std::string response = "{\"event\":\"unsubscribed_components_update\",\"payload\":{\"entity_id\":" +
                                        std::to_string(msg.payload.entity_id) + "}}";
                ws->send(response, uWS::OpCode::TEXT);

            } else if (event == "subscribe_entity_info") {
                std::istringstream iss(wrapped_json);
                cereal::JSONInputArchive archive(iss);
                WsEntityInfoMessage msg;
                archive(cereal::make_nvp("message", msg));

                // Replace current subscriptions with new list
                data->subscribed_entity_info.clear();
                data->subscribed_entity_info.insert(msg.payload.entities.begin(), msg.payload.entities.end());

                logger_->info(__FILE__, __LINE__) << "Client subscribed to entity_info for " << msg.payload.entities.size() << " entities";

                // Build response with subscribed entities
                std::string response = "{\"event\":\"subscribed_entity_info\",\"payload\":{\"entities\":[";
                bool first = true;
                for (uint32_t id : msg.payload.entities) {
                    if (!first) response += ",";
                    response += std::to_string(id);
                    first = false;
                }
                response += "]}}";
                ws->send(response, uWS::OpCode::TEXT);

            } else if (event == "subscribe_root_infos") {
                // No payload needed - subscribes to all root entities
                data->subscribed_root_infos = true;
                logger_->info(__FILE__, __LINE__) << "Client subscribed to root_infos";
                ws->send("{\"event\":\"subscribed_root_infos\"}", uWS::OpCode::TEXT);

            } else if (event == "unsubscribe_root_infos") {
                data->subscribed_root_infos = false;
                logger_->info(__FILE__, __LINE__) << "Client unsubscribed from root_infos";
                ws->send("{\"event\":\"unsubscribed_root_infos\"}", uWS::OpCode::TEXT);

            } else {
                ws->send("{\"event\":\"error\",\"payload\":{\"message\":\"Unknown event type\"}}", uWS::OpCode::TEXT);
            }

        } catch (const std::exception& e) {
            logger_->warn(__FILE__, __LINE__) << "Failed to parse WebSocket message: " << e.what();
            ws->send("{\"event\":\"error\",\"payload\":{\"message\":\"Invalid JSON format\"}}", uWS::OpCode::TEXT);
        }
    }

    // Broadcast component updates, entity_info updates, and root_infos to subscribed clients
    // Called from game thread - queues messages for uWS thread
    void broadcast_subscribed_updates() {
        // Early exit without lock if no pending broadcasts needed
        {
            std::lock_guard<std::mutex> lock(ws_clients_mutex_);
            if (ws_clients_.empty()) return;
        }

        // Collect subscribed entity IDs for components, entity_info, and root_infos (lock scope minimized)
        std::set<uint32_t> all_subscribed_components;
        std::set<uint32_t> all_subscribed_entity_info;
        bool any_subscribed_root_infos = false;
        {
            std::lock_guard<std::mutex> lock(ws_clients_mutex_);
            for (auto* ws : ws_clients_) {
                const auto* data = ws->getUserData();
                all_subscribed_components.insert(
                    data->subscribed_components.begin(),
                    data->subscribed_components.end()
                );
                all_subscribed_entity_info.insert(
                    data->subscribed_entity_info.begin(),
                    data->subscribed_entity_info.end()
                );
                if (data->subscribed_root_infos) {
                    any_subscribed_root_infos = true;
                }
            }
        }

        std::vector<WsBroadcastMessage> component_messages;
        std::vector<WsBroadcastMessage> entity_info_messages;
        std::string root_infos_message;

        // Generate component JSON for each subscribed entity
        if (!all_subscribed_components.empty()) {
            component_messages.reserve(all_subscribed_components.size());
            for (uint32_t entity_id : all_subscribed_components) {
                auto response = get_entity_components_json(entity_id);
                component_messages.push_back({
                    entity_id,
                    "{\"event\":\"notify_components_update\",\"payload\":" + std::move(response.body) + "}"
                });
            }
        }

        auto& registry = world_->scene().registry();

        // Generate entity_info JSON for each subscribed entity
        if (!all_subscribed_entity_info.empty()) {
            // Build single message with array of entity_info for each client's subscription
            // For efficiency, pre-generate entity_info for all requested entities
            std::unordered_map<uint32_t, std::string> entity_info_cache;
            entity_info_cache.reserve(all_subscribed_entity_info.size());

            for (uint32_t entity_id : all_subscribed_entity_info) {
                auto entity = static_cast<nodec::entities::Entity>(entity_id);
                if (registry.is_valid(entity)) {
                    std::string result;
                    nodec::StringBuilder json(result);
                    build_entity_info_json(json, entity, registry);
                    entity_info_cache[entity_id] = std::move(result);
                }
            }

            // Store cache for use in defer callback
            entity_info_messages.reserve(all_subscribed_entity_info.size());
            for (const auto& [entity_id, json] : entity_info_cache) {
                entity_info_messages.push_back({entity_id, json});
            }
        }

        // Generate root_infos JSON (array of root entity infos)
        // Use hierarchy_system's root_hierarchy for proper ordering
        // (not EnTT view iteration which has unpredictable order)
        if (any_subscribed_root_infos) {
            nodec::StringBuilder json(root_infos_message);
            json << "{\"event\":\"notify_root_infos\",\"payload\":[";

            const auto& root_hierarchy = world_->scene().hierarchy_system().root_hierarchy();
            bool first = true;
            auto entity = root_hierarchy.first;
            while (entity != nodec::entities::null_entity) {
                if (!first) json << ",";
                build_entity_info_json(json, entity, registry);
                first = false;
                // Follow linked list to next root entity
                const auto& hierarchy = registry.get_component<nodec_scene::components::Hierarchy>(entity);
                entity = hierarchy.next;
            }

            json << "]}";
        }

        // Queue messages for uWS thread - batch all events into single message per client
        if (main_loop_ && (!component_messages.empty() || !entity_info_messages.empty() || !root_infos_message.empty())) {
            main_loop_->defer([this,
                               component_messages = std::move(component_messages),
                               entity_info_messages = std::move(entity_info_messages),
                               root_infos_message = std::move(root_infos_message)]() {
                // This runs on uWS thread - safe to access ws_clients_ and send
                for (auto* ws : ws_clients_) {
                    const auto* data = ws->getUserData();

                    // Build batched message: array of events
                    std::string batch_message = "[";
                    bool first_event = true;

                    // Add component updates
                    for (const auto& msg : component_messages) {
                        if (data->subscribed_components.count(msg.entity_id) > 0) {
                            if (!first_event) batch_message += ",";
                            batch_message += msg.message;
                            first_event = false;
                        }
                    }

                    // Add entity_info updates
                    if (!data->subscribed_entity_info.empty()) {
                        std::string entity_info_json = "{\"event\":\"notify_entity_info\",\"payload\":[";
                        bool first_entity = true;
                        for (const auto& msg : entity_info_messages) {
                            if (data->subscribed_entity_info.count(msg.entity_id) > 0) {
                                if (!first_entity) entity_info_json += ",";
                                entity_info_json += msg.message;
                                first_entity = false;
                            }
                        }
                        entity_info_json += "]}";

                        if (!first_entity) {  // Only add if there's at least one entity
                            if (!first_event) batch_message += ",";
                            batch_message += entity_info_json;
                            first_event = false;
                        }
                    }

                    // Add root_infos updates
                    if (data->subscribed_root_infos && !root_infos_message.empty()) {
                        if (!first_event) batch_message += ",";
                        batch_message += root_infos_message;
                        first_event = false;
                    }

                    batch_message += "]";

                    // Send only if there are events to send
                    if (!first_event) {
                        ws->send(batch_message, uWS::OpCode::TEXT);
                    }
                }
            });
        }
    }

    // Helper: Build entity_info JSON for a single entity
    // Format: {"id":<id>,"name":"...","hierarchy":{"parent":null|<id>,"children":[...]},"prefab":{}}
    void build_entity_info_json(nodec::StringBuilder& json, nodec::entities::Entity entity,
                                 nodec_scene::SceneRegistry& registry) {
        uint32_t entity_id = static_cast<uint32_t>(entity);
        json << "{\"id\":" << entity_id;

        // Name
        auto* name = registry.try_get_component<nodec_scene::components::Name>(entity);
        if (name) {
            json << ",\"name\":\"" << name->value << "\"";
        } else {
            json << ",\"name\":\"Entity_" << entity_id << "\"";
        }

        // Hierarchy
        json << ",\"hierarchy\":{";
        auto* hierarchy = registry.try_get_component<nodec_scene::components::Hierarchy>(entity);
        if (hierarchy) {
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
            json << "]";
        } else {
            json << "\"parent\":null,\"children\":[]";
        }
        json << "}";

        // Prefab (only if component exists - empty object to indicate presence)
        auto* prefab = registry.try_get_component<nodec_scene_serialization::components::Prefab>(entity);
        if (prefab) {
            json << ",\"prefab\":{}";
        }

        json << "}";
    }

    APIResponse get_root_entities_json() {
        std::string result;
        nodec::StringBuilder json(result);
        json << "[";

        try {
            auto& scene = world_->scene();
            auto& registry = scene.registry();

            // Use hierarchy_system's root_hierarchy for proper ordering
            // (not EnTT view iteration which has unpredictable order)
            const auto& root_hierarchy = scene.hierarchy_system().root_hierarchy();
            bool first = true;
            auto entity = root_hierarchy.first;
            while (entity != nodec::entities::null_entity) {
                if (!first) json << ",";
                build_entity_info_json(json, entity, registry);
                first = false;
                // Follow linked list to next root entity
                const auto& hierarchy = registry.get_component<nodec_scene::components::Hierarchy>(entity);
                entity = hierarchy.next;
            }

        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error getting root entities: " << e.what();
            return APIResponse::internal_error("{\"error\":\"" + std::string(e.what()) + "\"}");
        }

        json << "]";
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
        build_entity_info_json(json, entity, registry);
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

    APIResponse update_entity_hierarchy(uint32_t entity_id, const std::string& json_body) {
        using namespace nodec::entities;
        using namespace nodec_scene::components;

        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& scene = world_->scene();
        auto& registry = scene.registry();
        auto& hierarchy_system = scene.hierarchy_system();

        if (!registry.is_valid(entity)) {
            return APIResponse::not_found("{\"error\":\"Entity not found\",\"id\":" + std::to_string(entity_id) + "}");
        }

        try {
            // Parse JSON manually using cereal
            // Expected format: { "parentId": number|null, "insertBefore": number, "insertAfter": number }
            std::optional<std::optional<uint32_t>> parent_id;  // outer optional = specified, inner optional = null or value
            std::optional<uint32_t> insert_before;
            std::optional<uint32_t> insert_after;

            // Simple JSON parsing using cereal
            {
                std::istringstream iss(json_body);
                cereal::JSONInputArchive archive(iss);

                // Try to read each field
                try {
                    // parentId can be null or a number
                    // Check if parentId exists in JSON
                    if (json_body.find("\"parentId\"") != std::string::npos) {
                        if (json_body.find("\"parentId\":null") != std::string::npos ||
                            json_body.find("\"parentId\": null") != std::string::npos) {
                            parent_id = std::optional<uint32_t>(std::nullopt);  // null means move to root
                        } else {
                            uint32_t pid;
                            archive(cereal::make_nvp("parentId", pid));
                            parent_id = std::optional<uint32_t>(pid);
                        }
                    }
                } catch (...) {}

                try {
                    if (json_body.find("\"insertBefore\"") != std::string::npos) {
                        uint32_t ib;
                        std::istringstream iss2(json_body);
                        cereal::JSONInputArchive archive2(iss2);
                        archive2(cereal::make_nvp("insertBefore", ib));
                        insert_before = ib;
                    }
                } catch (...) {}

                try {
                    if (json_body.find("\"insertAfter\"") != std::string::npos) {
                        uint32_t ia;
                        std::istringstream iss3(json_body);
                        cereal::JSONInputArchive archive3(iss3);
                        archive3(cereal::make_nvp("insertAfter", ia));
                        insert_after = ia;
                    }
                } catch (...) {}
            }

            // Validate: insertBefore and insertAfter are mutually exclusive
            if (insert_before && insert_after) {
                return APIResponse::bad_request("{\"error\":\"Cannot specify both insertBefore and insertAfter\",\"code\":\"INVALID_REQUEST\"}");
            }

            // Ensure entity has Hierarchy component
            registry.emplace_component<Hierarchy>(entity);

            // Handle insert_before or insert_after (sibling reordering)
            if (insert_before) {
                auto dest = static_cast<nodec::entities::Entity>(*insert_before);
                if (!registry.is_valid(dest)) {
                    return APIResponse::not_found("{\"error\":\"insertBefore entity not found\",\"id\":" + std::to_string(*insert_before) + "}");
                }
                registry.emplace_component<Hierarchy>(dest);

                // If parentId is specified, first move to that parent
                if (parent_id) {
                    if (*parent_id) {
                        auto parent = static_cast<nodec::entities::Entity>(**parent_id);
                        if (!registry.is_valid(parent)) {
                            return APIResponse::not_found("{\"error\":\"Parent entity not found\",\"id\":" + std::to_string(**parent_id) + "}");
                        }
                        registry.emplace_component<Hierarchy>(parent);
                        hierarchy_system.append_child(parent, entity);
                    }
                    // If parentId is null, the insert_before will handle the parent
                }

                hierarchy_system.insert_before(entity, dest);
            } else if (insert_after) {
                auto dest = static_cast<nodec::entities::Entity>(*insert_after);
                if (!registry.is_valid(dest)) {
                    return APIResponse::not_found("{\"error\":\"insertAfter entity not found\",\"id\":" + std::to_string(*insert_after) + "}");
                }
                registry.emplace_component<Hierarchy>(dest);

                // If parentId is specified, first move to that parent
                if (parent_id) {
                    if (*parent_id) {
                        auto parent = static_cast<nodec::entities::Entity>(**parent_id);
                        if (!registry.is_valid(parent)) {
                            return APIResponse::not_found("{\"error\":\"Parent entity not found\",\"id\":" + std::to_string(**parent_id) + "}");
                        }
                        registry.emplace_component<Hierarchy>(parent);
                        hierarchy_system.append_child(parent, entity);
                    }
                }

                hierarchy_system.insert_after(entity, dest);
            } else if (parent_id) {
                // Only parentId specified - move to new parent or root
                if (*parent_id) {
                    // Move to new parent
                    auto parent = static_cast<nodec::entities::Entity>(**parent_id);
                    if (!registry.is_valid(parent)) {
                        return APIResponse::not_found("{\"error\":\"Parent entity not found\",\"id\":" + std::to_string(**parent_id) + "}");
                    }
                    registry.emplace_component<Hierarchy>(parent);
                    hierarchy_system.append_child(parent, entity);
                } else {
                    // Move to root (parentId is null)
                    auto& entity_hierarchy = registry.get_component<Hierarchy>(entity);
                    if (entity_hierarchy.parent != null_entity) {
                        hierarchy_system.remove_child(entity_hierarchy.parent, entity);
                    }
                }
            } else {
                return APIResponse::bad_request("{\"error\":\"No operation specified. Provide parentId, insertBefore, or insertAfter\",\"code\":\"INVALID_REQUEST\"}");
            }

            // Build response with updated hierarchy info
            auto& updated_hierarchy = registry.get_component<Hierarchy>(entity);
            std::ostringstream response;
            response << "{\"id\":" << entity_id << ",\"hierarchy\":{";
            response << "\"parent\":";
            if (updated_hierarchy.parent == null_entity) {
                response << "null";
            } else {
                response << static_cast<uint32_t>(updated_hierarchy.parent);
            }
            response << ",\"children\":[";

            bool first = true;
            auto child = updated_hierarchy.first;
            while (child != null_entity) {
                if (!first) response << ",";
                response << static_cast<uint32_t>(child);
                first = false;
                child = registry.get_component<Hierarchy>(child).next;
            }
            response << "]}}";

            logger_->info(__FILE__, __LINE__) << "Updated hierarchy for entity " << entity_id;
            return APIResponse::ok(response.str());

        } catch (const std::runtime_error& e) {
            // Circular reference errors from hierarchy_system throw runtime_error
            std::string error_msg = e.what();
            if (error_msg.find("cannot set itself as a parent") != std::string::npos) {
                return APIResponse::conflict("{\"error\":\"Circular reference detected\",\"code\":\"CIRCULAR_REFERENCE\"}");
            }
            logger_->error(__FILE__, __LINE__) << "Error updating entity hierarchy: " << e.what();
            return APIResponse::bad_request("{\"error\":\"" + error_msg + "\"}");
        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error updating entity hierarchy: " << e.what();
            return APIResponse::internal_error("{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    }

    APIResponse add_entity_component(uint32_t entity_id, const std::string& json_body) {
        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& registry = world_->scene().registry();

        if (!registry.is_valid(entity)) {
            return APIResponse::not_found("{\"error\":\"Invalid entity\",\"id\":" + std::to_string(entity_id) + "}");
        }

        if (!scene_serialization_ || !resources_) {
            return APIResponse::internal_error("{\"error\":\"Scene serialization or resource registry not available\"}");
        }

        try {
            // Expected format: { "component": { "polymorphic_id": ..., "ptr_wrapper": ... } }
            std::istringstream iss(json_body);
            nodec_scene_serialization::ArchiveContext context(*scene_serialization_, resources_->registry());
            cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONInputArchive>
                archive(context, iss);

            std::unique_ptr<nodec_scene_serialization::BaseSerializableComponent> component;
            archive(cereal::make_nvp("component", component));

            if (!component) {
                return APIResponse::bad_request("{\"error\":\"Failed to deserialize component\"}");
            }

            scene_serialization_->emplace_or_replace_component(component.get(), entity, registry);

            logger_->info(__FILE__, __LINE__) << "Added component to entity " << entity_id;

            return APIResponse::ok("{\"success\":true,\"id\":" + std::to_string(entity_id) + "}");

        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error adding entity component: " << e.what();
            return APIResponse::bad_request("{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    }

    APIResponse remove_entity_component(uint32_t entity_id, uint32_t type_index) {
        auto entity = static_cast<nodec::entities::Entity>(entity_id);
        auto& registry = world_->scene().registry();

        if (!registry.is_valid(entity)) {
            return APIResponse::not_found("{\"error\":\"Invalid entity\",\"id\":" + std::to_string(entity_id) + "}");
        }

        if (!scene_serialization_) {
            return APIResponse::internal_error("{\"error\":\"Scene serialization not available\"}");
        }

        try {
            bool removed = scene_serialization_->remove_component_by_type_index(
                static_cast<nodec::type_seq_index_type>(type_index), entity, registry);

            if (!removed) {
                return APIResponse::not_found("{\"error\":\"Component type not found\",\"type_index\":" + std::to_string(type_index) + "}");
            }

            logger_->info(__FILE__, __LINE__) << "Removed component (type_index=" << type_index << ") from entity " << entity_id;

            return APIResponse::ok("{\"success\":true,\"id\":" + std::to_string(entity_id) +
                   ",\"type_index\":" + std::to_string(type_index) + "}");

        } catch (const std::exception& e) {
            logger_->error(__FILE__, __LINE__) << "Error removing entity component: " << e.what();
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

    void queue_request(APIRequest::Type type, uint32_t entity_id, uint32_t type_index,
                       std::function<void(const APIResponse&)> callback) {
        std::lock_guard<std::mutex> lock(request_queue_mutex_);
        request_queue_.emplace(type, entity_id, type_index, std::move(callback));
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
