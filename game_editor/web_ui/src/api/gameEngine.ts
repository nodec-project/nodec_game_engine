// Game Engine API Client

// Entity info for root entities list
export interface EntityInfo {
  id: string;  // Entity ID as string
  name: string;
  has_children: boolean;
}

// Response from /api/entities/roots
export interface RootEntitiesResponse {
  entities: EntityInfo[];
}

// Component info
export interface ComponentInfo {
  type_index: number;
  data: Record<string, unknown> | null; // JSON serialized component data
}

// Serializable component in cereal polymorphic format
export interface SerializableComponent {
  polymorphic_id: number;
  polymorphic_name?: string;
  ptr_wrapper: {
    valid: number;
    data: Record<string, unknown>;
  };
}

// Request body for PATCH /api/entities/ids/:id/components
export interface PatchComponentsRequest {
  components: SerializableComponent[];
}

// Response from PATCH /api/entities/ids/:id/components
export interface PatchComponentsResponse {
  success: boolean;
  id: number;
  updated_count: number;
  error?: string;
}

// Response from /api/entities/ids/:id/components
export interface EntityComponentsResponse {
  id: string;  // Changed from entity_id to id to match server response
  components: ComponentInfo[];
}

// Response from /api/entities/ids/:id
export interface EntityDetailsResponse {
  id: string;  // Entity ID as string (server returns number but we convert to string)
  name: string;
  hierarchy?: {
    parent: number | null;
    children: number[];  // Array of child entity IDs
  };
}

// Keyframe data for animation curves
export interface Keyframe {
  time: number;
  value: number;
}

// Animation curve data (from .anim file format)
export interface AnimationCurveData {
  wrap_mode: number;
  keyframes: Keyframe[];
}

// Animated property in clip
export interface AnimatedProperty {
  key: string;  // property path like "position.x", "rotation.z"
  value: {
    curve: AnimationCurveData;
  };
}

// Animated component placeholder (cereal polymorphic format)
export interface AnimatedComponentPlaceholder {
  polymorphic_id: number;
  polymorphic_name?: string;  // Optional - may not be present for some types
  ptr_wrapper: {
    valid: number;
    data: Record<string, unknown>;
  };
}

// Animated component in clip
export interface AnimatedComponentData {
  placeholder: AnimatedComponentPlaceholder;
  properties: AnimatedProperty[];
}

// Animated entity child in clip
export interface AnimatedEntityChild {
  key: string;  // entity name
  value: {
    components: AnimatedComponentData[];
    children: AnimatedEntityChild[];
  };
}

// AnimationClip resource response from /api/resources/animation_clip/*
export interface AnimationClipResponse {
  clip: {
    root_entity: {
      components: AnimatedComponentData[];
      children: AnimatedEntityChild[];
    };
  };
}

// Flattened curve for UI display
export interface AnimationCurve {
  entityPath: string;
  propertyPath: string;
  keyframes: Keyframe[];
  wrapMode: number;
}

// Registered component from /api/components
export interface RegisteredComponent {
  runtime_type_index: number;
  data: {
    component: AnimatedComponentPlaceholder;
  };
}

// Response from /api/components
export interface RegisteredComponentsResponse {
  components: RegisteredComponent[];
}

// Polymorphic type registry for cereal serialization
// Maps stripped polymorphic_id (without MSB) to polymorphic_name
export type PolymorphicTypeRegistry = Map<number, string>;

// cereal's MSB marker for first occurrence of a polymorphic type
const CEREAL_MSB_32BIT = 0x80000000;

const API_BASE_URL = 'http://localhost:8080';
const WS_BASE_URL = 'ws://localhost:8080';

// WebSocket message types
export interface WsSubscribeMessage {
  event: 'subscribe' | 'unsubscribe';
  payload: { entity_id: number };
}

export interface WsComponentUpdateMessage {
  event: 'component_update';
  payload: {
    id: number;
    components: ComponentInfo[];
  };
}

export interface WsSubscribedMessage {
  event: 'subscribed' | 'unsubscribed';
  payload: { entity_id: number };
}

export interface WsErrorMessage {
  event: 'error';
  payload: { message: string };
}

export type WsServerMessage = WsComponentUpdateMessage | WsSubscribedMessage | WsErrorMessage;

// Response from POST /api/entities/ids/:id/components
export interface AddComponentResponse {
  success: boolean;
  id: number;
  error?: string;
}

// Response from DELETE /api/entities/ids/:id/components/:type_index
export interface RemoveComponentResponse {
  success: boolean;
  id: number;
  error?: string;
}

export class GameEngineAPI {
  private static instance: GameEngineAPI;
  private ws: WebSocket | null = null;
  private wsListeners: Map<number, Set<(components: ComponentInfo[]) => void>> = new Map();
  private wsConnecting: boolean = false;
  private wsReconnectTimeout: NodeJS.Timeout | null = null;
  private connectionStateListeners: Set<(connected: boolean) => void> = new Set();
  private registeredComponentsCache: RegisteredComponent[] | null = null;

  private constructor() {}

  /**
   * Subscribe to WebSocket connection state changes
   * @returns Unsubscribe function
   */
  onConnectionStateChange(callback: (connected: boolean) => void): () => void {
    this.connectionStateListeners.add(callback);
    // Immediately notify current state
    callback(this.ws !== null && this.ws.readyState === WebSocket.OPEN);
    return () => {
      this.connectionStateListeners.delete(callback);
    };
  }

  private notifyConnectionState(connected: boolean) {
    this.connectionStateListeners.forEach(cb => cb(connected));
  }

  /**
   * Check if WebSocket is currently connected
   */
  isConnected(): boolean {
    return this.ws !== null && this.ws.readyState === WebSocket.OPEN;
  }

  /**
   * Connect to WebSocket (public method for eager connection)
   */
  async connect(): Promise<void> {
    try {
      await this.connectWebSocket();
    } catch (e) {
      // Connection failed, state already notified via onclose/onerror
      console.error('Failed to connect to engine:', e);
    }
  }

  public static getInstance(): GameEngineAPI {
    if (!GameEngineAPI.instance) {
      GameEngineAPI.instance = new GameEngineAPI();
    }
    return GameEngineAPI.instance;
  }

  /**
   * Connect to WebSocket for real-time component updates
   */
  private connectWebSocket(): Promise<WebSocket> {
    return new Promise((resolve, reject) => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        resolve(this.ws);
        return;
      }

      if (this.wsConnecting) {
        // Wait for existing connection attempt
        const checkConnection = setInterval(() => {
          if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            clearInterval(checkConnection);
            resolve(this.ws);
          }
        }, 100);
        return;
      }

      this.wsConnecting = true;
      const ws = new WebSocket(`${WS_BASE_URL}/ws`);

      ws.onopen = async () => {
        console.log('WebSocket connected');
        this.ws = ws;
        this.wsConnecting = false;

        // Fetch and cache registered components BEFORE notifying connected state
        try {
          const components = await this.getRegisteredComponents();
          this.registeredComponentsCache = components;
          console.log(`Cached ${components.length} registered components`);
        } catch (e) {
          console.error('Failed to fetch registered components on connect:', e);
        }

        // Notify connected state after components are cached
        this.notifyConnectionState(true);
        resolve(ws);
      };

      ws.onmessage = (event) => {
        try {
          const msg: WsServerMessage = JSON.parse(event.data);
          if (msg.event === 'component_update') {
            const entityId = msg.payload.id;
            const listeners = this.wsListeners.get(entityId);
            if (listeners) {
              listeners.forEach(cb => cb(msg.payload.components));
            }
          }
        } catch (e) {
          console.error('Failed to parse WebSocket message:', e);
        }
      };

      ws.onclose = () => {
        console.log('WebSocket disconnected');
        this.ws = null;
        this.wsConnecting = false;
        this.notifyConnectionState(false);
        // Attempt reconnect if there are active listeners
        if (this.wsListeners.size > 0 && !this.wsReconnectTimeout) {
          this.wsReconnectTimeout = setTimeout(() => {
            this.wsReconnectTimeout = null;
            this.reconnectAndResubscribe();
          }, 2000);
        }
      };

      ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        this.wsConnecting = false;
        reject(error);
      };
    });
  }

  private async reconnectAndResubscribe() {
    try {
      await this.connectWebSocket();
      // Re-subscribe to all entities
      const entityIds = Array.from(this.wsListeners.keys());
      for (const entityId of entityIds) {
        this.sendSubscribe(entityId);
      }
    } catch (e) {
      console.error('Failed to reconnect WebSocket:', e);
    }
  }

  private sendSubscribe(entityId: number) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      const msg: WsSubscribeMessage = {
        event: 'subscribe',
        payload: { entity_id: entityId }
      };
      this.ws.send(JSON.stringify(msg));
      console.log(`Subscribed to entity ${entityId}`);
    }
  }

  private sendUnsubscribe(entityId: number) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      const msg: WsSubscribeMessage = {
        event: 'unsubscribe',
        payload: { entity_id: entityId }
      };
      this.ws.send(JSON.stringify(msg));
    }
  }

  /**
   * Subscribe to real-time component updates for an entity
   * @returns Unsubscribe function
   */
  async subscribeToEntityComponents(
    entityId: string,
    callback: (components: ComponentInfo[]) => void
  ): Promise<() => void> {
    const id = parseInt(entityId, 10);

    // Add listener
    if (!this.wsListeners.has(id)) {
      this.wsListeners.set(id, new Set());
    }
    this.wsListeners.get(id)!.add(callback);

    // Connect and subscribe
    try {
      await this.connectWebSocket();
      this.sendSubscribe(id);
    } catch (e) {
      console.error('Failed to subscribe:', e);
    }

    // Return unsubscribe function
    return () => {
      const listeners = this.wsListeners.get(id);
      if (listeners) {
        listeners.delete(callback);
        if (listeners.size === 0) {
          this.wsListeners.delete(id);
          this.sendUnsubscribe(id);
        }
      }
    };
  }

  /**
   * Get root entities from the game engine
   */
  async getRootEntities(): Promise<EntityInfo[]> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/entities/roots`, {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();
      // Convert id to string (server returns number)
      return data.entities.map((e: { id: number; name: string; has_children: boolean }) => ({
        ...e,
        id: String(e.id)
      }));
    } catch (error) {
      console.error('Failed to fetch root entities:', error);
      throw error;
    }
  }

  /**
   * Get cached registered components (fetched on WebSocket connect)
   * Returns null if not yet fetched (WebSocket not connected)
   */
  get registeredComponents(): RegisteredComponent[] | null {
    return this.registeredComponentsCache;
  }

  /**
   * Get all registered component types from the game engine via API
   */
  async getRegisteredComponents(): Promise<RegisteredComponent[]> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/components`, {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data: RegisteredComponentsResponse = await response.json();
      return data.components;
    } catch (error) {
      console.error('Failed to fetch registered components:', error);
      throw error;
    }
  }

  /**
   * Get components for a specific entity
   */
  async getEntityComponents(entityId: string): Promise<ComponentInfo[]> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/entities/ids/${entityId}/components`, {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data: EntityComponentsResponse = await response.json();
      return data.components;
    } catch (error) {
      console.error(`Failed to fetch components for entity ${entityId}:`, error);
      throw error;
    }
  }

  /**
   * Get entity details
   */
  async getEntityDetails(entityId: string): Promise<EntityDetailsResponse> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/entities/ids/${entityId}`, {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();
      // Convert id to string if it's a number
      return {
        ...data,
        id: String(data.id)
      };
    } catch (error) {
      console.error(`Failed to fetch details for entity ${entityId}:`, error);
      throw error;
    }
  }

  /**
   * Get a resource by type and name
   */
  async getResource<T>(resourceType: string, resourceName: string): Promise<T> {
    try {
      console.log(`${API_BASE_URL}/api/resources/${resourceType}/${resourceName}`)
      const response = await fetch(`${API_BASE_URL}/api/resources/${resourceType}/${resourceName}`, {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();
      return data;
    } catch (error) {
      console.error(`Failed to fetch resource ${resourceType}/${resourceName}:`, error);
      throw error;
    }
  }

  /**
   * Get an animation clip by resource name
   */
  async getAnimationClip(clipName: string): Promise<AnimationClipResponse> {
    return this.getResource<AnimationClipResponse>('animation_clip', clipName);
  }

  /**
   * Restore polymorphic_name for all placeholders using the type registry.
   * Call this once after receiving clip data from the server.
   * Modifies clip in-place for performance (no deep clone).
   *
   * @param clip - The clip to restore names in (modified in-place)
   * @param typeRegistry - Registry mapping base_id -> polymorphic_name
   */
  restorePolymorphicNames(clip: AnimationClipResponse, typeRegistry: PolymorphicTypeRegistry): void {
    const processEntity = (entity: { components: AnimatedComponentData[]; children: AnimatedEntityChild[] }): void => {
      for (const component of entity.components) {
        const placeholder = component.placeholder;
        if (!placeholder.polymorphic_name) {
          // Restore name from registry
          const baseId = placeholder.polymorphic_id >= CEREAL_MSB_32BIT
            ? placeholder.polymorphic_id - CEREAL_MSB_32BIT
            : placeholder.polymorphic_id;
          const typeName = typeRegistry.get(baseId);
          if (typeName) {
            placeholder.polymorphic_name = typeName;
          }
        }
      }
      for (const child of entity.children) {
        processEntity(child.value);
      }
    };

    processEntity(clip.clip.root_entity);
  }

  /**
   * Remap polymorphic_ids in an AnimationClipResponse for cereal serialization.
   *
   * cereal uses MSB-based scheme for polymorphic types:
   * - First occurrence: polymorphic_id = 0x80000000 + base_id, with polymorphic_name present
   * - Subsequent occurrences: polymorphic_id = base_id, without polymorphic_name
   *
   * Precondition: All placeholders must have polymorphic_name set.
   * Call restorePolymorphicNames() after loading clip data to ensure this.
   */
  remapPolymorphicIds(clip: AnimationClipResponse): AnimationClipResponse {
    // Deep clone to avoid mutating the original (needed for Save)
    const clonedClip: AnimationClipResponse = JSON.parse(JSON.stringify(clip));

    const typeToNewBaseId = new Map<string, number>();
    let nextBaseId = 1;

    const processPlaceholder = (placeholder: AnimatedComponentPlaceholder): void => {
      const typeName = placeholder.polymorphic_name;

      if (!typeName) {
        // This should not happen if restorePolymorphicNames was called after loading
        console.warn(`Missing polymorphic_name for polymorphic_id ${placeholder.polymorphic_id}`);
        return;
      }

      if (typeToNewBaseId.has(typeName)) {
        // Subsequent occurrence - use base id without MSB, remove polymorphic_name
        placeholder.polymorphic_id = typeToNewBaseId.get(typeName)!;
        delete placeholder.polymorphic_name;
      } else {
        // First occurrence - assign new base id with MSB, keep polymorphic_name
        const newBaseId = nextBaseId++;
        typeToNewBaseId.set(typeName, newBaseId);
        placeholder.polymorphic_id = CEREAL_MSB_32BIT + newBaseId;
      }
    };

    const processEntity = (entity: { components: AnimatedComponentData[]; children: AnimatedEntityChild[] }): void => {
      for (const component of entity.components) {
        processPlaceholder(component.placeholder);
      }
      for (const child of entity.children) {
        processEntity(child.value);
      }
    };

    processEntity(clonedClip.clip.root_entity);

    return clonedClip;
  }

  /**
   * Update an animation clip resource via PUT
   * Automatically remaps polymorphic_ids for cereal serialization.
   *
   * @param clipName - The resource name (e.g., "org.solreno.solreno/animations/title.anim")
   * @param clipData - The animation clip data to save
   */
  async updateAnimationClip(clipName: string, clipData: AnimationClipResponse): Promise<void> {
    try {
      // Remap polymorphic_ids before sending
      const remappedClip = this.remapPolymorphicIds(clipData);

      const response = await fetch(`${API_BASE_URL}/api/resources/animation_clip/${clipName}`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(remappedClip),
      });

      if (!response.ok) {
        const errorText = await response.text();
        throw new Error(`HTTP error! status: ${response.status}, message: ${errorText}`);
      }
    } catch (error) {
      console.error(`Failed to update animation clip ${clipName}:`, error);
      throw error;
    }
  }

  /**
   * Update entity components via PATCH
   * @param entityId - The entity ID
   * @param components - Array of serializable components in cereal polymorphic format
   */
  async patchEntityComponents(entityId: string, components: SerializableComponent[]): Promise<PatchComponentsResponse> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/entities/ids/${entityId}/components`, {
        method: 'PATCH',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ components }),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data: PatchComponentsResponse = await response.json();
      return data;
    } catch (error) {
      console.error(`Failed to patch components for entity ${entityId}:`, error);
      throw error;
    }
  }

  /**
   * Add a component to an entity via POST
   * @param entityId - The entity ID
   * @param component - The serializable component to add
   */
  async addEntityComponent(entityId: string, component: SerializableComponent): Promise<AddComponentResponse> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/entities/ids/${entityId}/components`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ component }),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data: AddComponentResponse = await response.json();
      return data;
    } catch (error) {
      console.error(`Failed to add component to entity ${entityId}:`, error);
      throw error;
    }
  }

  /**
   * Remove a component from an entity via DELETE
   * @param entityId - The entity ID
   * @param typeIndex - The runtime type index of the component to remove
   */
  async removeEntityComponent(entityId: string, typeIndex: number): Promise<RemoveComponentResponse> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/entities/ids/${entityId}/components/${typeIndex}`, {
        method: 'DELETE',
        headers: {
          'Content-Type': 'application/json',
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data: RemoveComponentResponse = await response.json();
      return data;
    } catch (error) {
      console.error(`Failed to remove component ${typeIndex} from entity ${entityId}:`, error);
      throw error;
    }
  }

  /**
   * Helper: Extract SerializableComponent from ComponentInfo
   * Returns null if the component data is not in the expected format
   */
  extractSerializableComponent(componentInfo: ComponentInfo): SerializableComponent | null {
    if (!componentInfo.data) return null;

    const data = componentInfo.data as {
      component?: SerializableComponent;
    };

    if (data.component && typeof data.component.polymorphic_id === 'number' && data.component.ptr_wrapper) {
      return data.component;
    }

    return null;
  }

  /**
   * Helper: Flatten AnimationClip into a list of curves for UI display
   */
  flattenAnimationClip(clip: AnimationClipResponse): AnimationCurve[] {
    const curves: AnimationCurve[] = [];

    const processEntity = (
      entity: { components: AnimatedComponentData[]; children: AnimatedEntityChild[] },
      entityPath: string
    ) => {
      // Process components in this entity
      for (const component of entity.components) {
        for (const prop of component.properties) {
          curves.push({
            entityPath,
            propertyPath: prop.key,
            keyframes: prop.value.curve.keyframes,
            wrapMode: prop.value.curve.wrap_mode,
          });
        }
      }

      // Process children recursively
      for (const child of entity.children) {
        const childPath = entityPath ? `${entityPath}/${child.key}` : child.key;
        processEntity(child.value, childPath);
      }
    };

    processEntity(clip.clip.root_entity, '');
    return curves;
  }

  /**
   * Helper: Calculate clip duration from curves
   */
  getClipDuration(curves: AnimationCurve[]): number {
    let maxTime = 0;
    for (const curve of curves) {
      for (const kf of curve.keyframes) {
        if (kf.time > maxTime) {
          maxTime = kf.time;
        }
      }
    }
    return maxTime;
  }

  /**
   * Helper: Build polymorphic type registry from AnimationClip
   *
   * cereal serialization uses polymorphic_id with MSB (0x80000000) set for first occurrence,
   * and includes polymorphic_name. Subsequent occurrences use just the stripped ID without name.
   *
   * This function traverses the clip and builds a registry mapping stripped IDs to names.
   */
  buildPolymorphicTypeRegistry(clip: AnimationClipResponse): PolymorphicTypeRegistry {
    const registry: PolymorphicTypeRegistry = new Map();

    const processPlaceholder = (placeholder: AnimatedComponentPlaceholder) => {
      const id = placeholder.polymorphic_id;
      const name = placeholder.polymorphic_name;

      // If MSB is set, this is a first occurrence with name
      if (id >= CEREAL_MSB_32BIT && name) {
        const strippedId = id - CEREAL_MSB_32BIT;  // Same as id & ~MSB
        registry.set(strippedId, name);
      }
    };

    const processEntity = (
      entity: { components: AnimatedComponentData[]; children: AnimatedEntityChild[] }
    ) => {
      for (const component of entity.components) {
        processPlaceholder(component.placeholder);
      }

      for (const child of entity.children) {
        processEntity(child.value);
      }
    };

    processEntity(clip.clip.root_entity);
    return registry;
  }

  /**
   * Helper: Get component type name from placeholder using registry
   *
   * @param placeholder The component placeholder from AnimationClip
   * @param registry The polymorphic type registry built from the clip
   * @returns Component type name (e.g., "SerializableImageRenderer")
   */
  getComponentTypeName(placeholder: AnimatedComponentPlaceholder, registry: PolymorphicTypeRegistry): string {
    let fullName: string | undefined = placeholder.polymorphic_name;

    // If no name in placeholder, look up in registry
    if (!fullName) {
      const id = placeholder.polymorphic_id;
      fullName = registry.get(id);
    }

    if (fullName) {
      // Extract short name from full qualified name
      // e.g., "nodec_rendering::components::SerializableImageRenderer" -> "SerializableImageRenderer"
      return fullName.split('::').pop() || fullName;
    }

    // Fallback to showing the ID
    return `Component[${placeholder.polymorphic_id}]`;
  }

  /**
   * Helper: Find animator clip name from entity components
   * Returns null if no animator or no clip assigned
   *
   * Component structure from API:
   * {
   *   type_index: number,
   *   data: {
   *     component: {
   *       polymorphic_name: "nodec_animation::components::SerializableAnimator",
   *       ptr_wrapper: {
   *         valid: 1,
   *         data: {
   *           clip: "resource_name"
   *         }
   *       }
   *     }
   *   }
   * }
   */
  findAnimatorClipName(components: ComponentInfo[]): string | null {
    for (const comp of components) {
      if (comp.data && typeof comp.data === 'object') {
        // Check if this is a SerializableAnimator component
        const componentData = comp.data as {
          component?: {
            polymorphic_name?: string;
            ptr_wrapper?: {
              valid?: number;
              data?: {
                clip?: string;
              };
            };
          };
        };

        // Check for SerializableAnimator by polymorphic_name
        if (componentData.component?.polymorphic_name === 'nodec_animation::components::SerializableAnimator') {
          const clipName = componentData.component.ptr_wrapper?.data?.clip;
          // Return empty string if animator exists but no clip, or the clip name
          return clipName ?? '';
        }
      }
    }
    return null;
  }

  /**
   * Check if the game engine server is running
   */
  async healthCheck(): Promise<boolean> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/entities/roots`, {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json',
        },
      });
      return response.ok;
    } catch (error) {
      return false;
    }
  }
}

export const gameEngineAPI = GameEngineAPI.getInstance();