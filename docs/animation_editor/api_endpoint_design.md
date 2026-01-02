# Animation Editor API Endpoint Design

## Overview

This document defines the API endpoints for the animation editor feature in the nodec_game_editor. The API is designed to handle the relationship between AnimationClips, Animator components, and the entity hierarchy they animate.

## Core Concepts

### Animation Editing Context
Animation editing requires three interconnected pieces of information:
1. **AnimationClip**: The animation data (curves, keyframes)
2. **Animator Component**: The component that references the clip
3. **Entity Hierarchy**: The actual entities and components being animated

### Resource Path Challenge
Animation clips are identified by resource paths like `org.solreno.solreno/animations/sample.clip`, which are problematic in URL paths. We use query parameters to handle these paths cleanly.

## API Endpoints

### 1. Get Animation Editing Context
Retrieves complete context for editing an animation, including the clip, entity hierarchy, and bindings.

```http
GET /api/animations/editing-context?entityId=<animator_entity_id>
```

**Query Parameters:**
- `entityId` (required): Entity ID that has the Animator component

**Response:**
```json
{
  "animatorEntity": {
    "id": 12345,
    "name": "Player",
    "clipPath": "org.solreno.solreno/animations/player_walk.clip",
    "hasClip": true
  },
  "clip": {
    "path": "org.solreno.solreno/animations/player_walk.clip",
    "duration": 2.5,
    "entities": {
      "": {
        "components": {
          "Transform": {
            "properties": {
              "position.x": {
                "keyframes": [
                  { "time": 0.0, "value": 0.0 },
                  { "time": 1.0, "value": 100.0 }
                ],
                "wrapMode": "Loop"
              }
            }
          }
        }
      },
      "weapon": {
        "components": {
          "Transform": {
            "properties": {
              "rotation.z": {
                "keyframes": [
                  { "time": 0.0, "value": 0.0 },
                  { "time": 1.0, "value": 360.0 }
                ],
                "wrapMode": "Loop"
              }
            }
          }
        }
      }
    }
  },
  "targetHierarchy": {
    "": {
      "entityId": 12345,
      "name": "Player",
      "components": {
        "Transform": {
          "typeIndex": 1,
          "animatableProperties": [
            "position.x",
            "position.y",
            "position.z",
            "rotation.x",
            "rotation.y",
            "rotation.z",
            "scale.x",
            "scale.y",
            "scale.z"
          ]
        },
        "SpriteRenderer": {
          "typeIndex": 5,
          "animatableProperties": [
            "opacity",
            "tint.r",
            "tint.g",
            "tint.b"
          ]
        }
      }
    },
    "weapon": {
      "entityId": 12346,
      "name": "weapon",
      "components": {
        "Transform": {
          "typeIndex": 1,
          "animatableProperties": [
            "position.x",
            "position.y",
            "position.z",
            "rotation.x",
            "rotation.y",
            "rotation.z"
          ]
        }
      }
    }
  },
  "bindings": {
    "": 12345,
    "weapon": 12346
  }
}
```

### 2. List Available Animation Clips
Lists all animation clips available in the resource system.

```http
GET /api/animations/clips
```

**Response:**
```json
{
  "clips": [
    {
      "path": "org.solreno.solreno/animations/player_walk.clip",
      "name": "player_walk",
      "duration": 1.5,
      "entityCount": 2
    },
    {
      "path": "org.solreno.solreno/animations/enemy_idle.clip",
      "name": "enemy_idle",
      "duration": 2.0,
      "entityCount": 1
    }
  ]
}
```

### 3. Get Animation Clip Details
Retrieves detailed data for a specific animation clip.

```http
GET /api/animations/clip?path=<resource_path>
```

**Query Parameters:**
- `path` (required): Resource path of the animation clip

**Response:**
```json
{
  "path": "org.solreno.solreno/animations/sample.clip",
  "name": "sample",
  "duration": 2.5,
  "entities": {
    "": {
      "components": {
        "Transform": {
          "properties": {
            "position.x": {
              "keyframes": [
                { "time": 0.0, "value": 0.0 },
                { "time": 1.0, "value": 100.0 }
              ],
              "wrapMode": "Loop"
            }
          }
        }
      }
    }
  }
}
```

### 4. Get Animatable Properties
Discovers all animatable properties for a specific entity.

```http
GET /api/animations/animatable-properties?entityId=<entity_id>
```

**Query Parameters:**
- `entityId` (required): Entity ID to query

**Response:**
```json
{
  "entityId": 12345,
  "name": "Player",
  "components": [
    {
      "typeName": "Transform",
      "typeIndex": 1,
      "properties": [
        {
          "path": "position.x",
          "type": "float",
          "currentValue": 50.0
        },
        {
          "path": "position.y",
          "type": "float",
          "currentValue": 100.0
        }
      ]
    }
  ]
}
```

### 5. Create Animation Editing Session
Creates a new editing session for creating or modifying animations.

```http
POST /api/animations/sessions/create
```

**Request Body:**
```json
{
  "baseEntityId": 12345,
  "clipPath": "org.solreno.solreno/animations/sample.clip"
}
```

**Response:**
```json
{
  "sessionId": "session_001",
  "baseEntityId": 12345,
  "clipPath": "org.solreno.solreno/animations/sample.clip",
  "workingCopy": {
    "entities": {}
  }
}
```

### 6. Update Animation Curve
Updates a curve in the working copy of an animation.

```http
PUT /api/animations/sessions/:sessionId/curves
```

**Request Body:**
```json
{
  "entityPath": "",
  "componentType": "Transform",
  "propertyPath": "position.x",
  "keyframes": [
    { "time": 0.0, "value": 0.0 },
    { "time": 0.5, "value": 75.0 },
    { "time": 1.0, "value": 100.0 }
  ],
  "wrapMode": "Loop"
}
```

### 7. Preview Animation
Applies animation at a specific time for preview.

```http
POST /api/animations/preview
```

**Request Body:**
```json
{
  "sessionId": "session_001",
  "time": 0.75,
  "targetEntityId": 12345
}
```

**Response:**
```json
{
  "applied": true,
  "evaluatedValues": {
    "12345": {
      "Transform": {
        "position.x": 87.5,
        "position.y": 0.0
      }
    },
    "12346": {
      "Transform": {
        "rotation.z": 270.0
      }
    }
  }
}
```

### 8. Save Animation Clip
Saves the working copy back to the resource system.

```http
POST /api/animations/sessions/:sessionId/save
```

**Request Body:**
```json
{
  "path": "org.solreno.solreno/animations/new_animation.clip",
  "overwrite": false
}
```

### 9. WebSocket for Real-time Preview
Establishes WebSocket connection for real-time animation preview.

```
WS /api/animations/preview-stream
```

**Client → Server:**
```json
{
  "type": "PREVIEW_TIME",
  "sessionId": "session_001",
  "time": 0.5
}
```

**Server → Client:**
```json
{
  "type": "PREVIEW_UPDATE",
  "entities": {
    "12345": {
      "Transform": {
        "position": { "x": 50.0, "y": 0.0, "z": 0.0 }
      }
    }
  }
}
```

## Implementation Considerations

### C++ Side

#### Query Parameter Parsing
```cpp
std::string parseQueryParam(std::string_view query, const std::string& key) {
    // Parse "?path=org.solreno.solreno/animations/sample.clip"
    size_t key_pos = query.find(key + "=");
    if (key_pos == std::string_view::npos) return "";
    
    size_t value_start = key_pos + key.length() + 1;
    size_t value_end = query.find('&', value_start);
    
    if (value_end == std::string_view::npos) {
        return std::string(query.substr(value_start));
    }
    return std::string(query.substr(value_start, value_end - value_start));
}
```

#### Entity Hierarchy Collection
```cpp
struct EntityHierarchyInfo {
    nodec::entities::Entity id;
    std::string name;
    std::map<std::string, ComponentInfo> components;
    std::map<std::string, EntityHierarchyInfo> children;
};

EntityHierarchyInfo collectEntityHierarchy(
    nodec::entities::Entity root,
    nodec_scene::SceneRegistry& registry,
    ComponentRegistry& comp_registry
) {
    EntityHierarchyInfo info;
    info.id = root;
    
    // Get entity name
    if (auto* name = registry.try_get_component<Name>(root)) {
        info.name = name->value;
    }
    
    // Collect animatable components
    registry.visit(root, [&](const type_info& type, void* component) {
        if (comp_registry.get_handler(type)) {
            // This component is animatable
            auto properties = discoverProperties(type, component);
            info.components[type.name()] = ComponentInfo{type, properties};
        }
    });
    
    // Recursively collect children
    if (auto* hierarchy = registry.try_get_component<Hierarchy>(root)) {
        // ... traverse children
    }
    
    return info;
}
```

#### Session Management
```cpp
class AnimationEditingSession {
    std::string session_id_;
    nodec::entities::Entity base_entity_;
    std::shared_ptr<AnimationClip> working_copy_;
    std::string original_clip_path_;
    
public:
    void updateCurve(
        const std::string& entity_path,
        const std::string& component_type,
        const std::string& property_path,
        const AnimationCurve& curve
    );
    
    void applyPreview(float time, SceneRegistry& registry);
    
    void save(ResourceRegistry& resources, const std::string& path);
};

class AnimationEditorState {
    std::unordered_map<std::string, AnimationEditingSession> sessions_;
    
public:
    std::string createSession(Entity base_entity, const std::string& clip_path);
    AnimationEditingSession* getSession(const std::string& session_id);
    void removeSession(const std::string& session_id);
};
```

### Web UI Integration

#### TypeScript Types
```typescript
interface AnimationEditingContext {
  animatorEntity: {
    id: number;
    name: string;
    clipPath: string | null;
    hasClip: boolean;
  };
  clip: AnimationClip | null;
  targetHierarchy: Record<string, EntityHierarchyNode>;
  bindings: Record<string, number>;
}

interface EntityHierarchyNode {
  entityId: number;
  name: string;
  components: Record<string, ComponentInfo>;
}

interface ComponentInfo {
  typeIndex: number;
  animatableProperties: string[];
}
```

#### API Client
```typescript
export class AnimationEditorAPI {
  async getEditingContext(entityId: number): Promise<AnimationEditingContext> {
    const params = new URLSearchParams({ entityId: entityId.toString() });
    const response = await fetch(`${API_BASE_URL}/api/animations/editing-context?${params}`);
    return response.json();
  }
  
  async createSession(baseEntityId: number, clipPath?: string): Promise<string> {
    const response = await fetch(`${API_BASE_URL}/api/animations/sessions/create`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ baseEntityId, clipPath })
    });
    const data = await response.json();
    return data.sessionId;
  }
}
```

## Error Handling

### Common Error Responses
```json
{
  "error": "Entity not found",
  "details": "Entity with ID 12345 does not exist"
}
```

```json
{
  "error": "No animator component",
  "details": "Entity 12345 does not have an Animator component"
}
```

```json
{
  "error": "Invalid clip path",
  "details": "Resource 'invalid/path.clip' not found"
}
```

## Security Considerations

1. **Path Validation**: Validate resource paths to prevent directory traversal
2. **Session Timeout**: Automatically clean up inactive editing sessions
3. **Rate Limiting**: Limit preview updates to prevent server overload
4. **Input Validation**: Validate keyframe times and values