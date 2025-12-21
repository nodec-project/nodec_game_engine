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

export class GameEngineAPI {
  private static instance: GameEngineAPI;
  private ws: WebSocket | null = null;
  private wsListeners: Map<number, Set<(components: ComponentInfo[]) => void>> = new Map();
  private wsConnecting: boolean = false;
  private wsReconnectTimeout: NodeJS.Timeout | null = null;
  private connectionStateListeners: Set<(connected: boolean) => void> = new Set();

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

      ws.onopen = () => {
        console.log('WebSocket connected');
        this.ws = ws;
        this.wsConnecting = false;
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