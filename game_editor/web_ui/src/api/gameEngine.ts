// Game Engine API Client

// Hierarchy info for entities
export interface EntityHierarchy {
  parent: number | null;
  children: number[];
}

// Prefab info (empty object - only presence indicates Prefab component attached)
// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export interface EntityPrefab { }

// Entity info (unified format for /api/entities/roots and /api/entities/ids/:id)
export interface EntityInfo {
  id: string;  // Entity ID as string (server returns number, we convert)
  name: string;
  hierarchy: EntityHierarchy;
  prefab?: EntityPrefab;
}

// Helper: Check if entity has children
export function entityHasChildren(entity: EntityInfo): boolean {
  return entity.hierarchy.children.length > 0;
}

// Helper: Check if entity has prefab component
export function entityHasPrefab(entity: EntityInfo): boolean {
  return entity.prefab !== undefined;
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

// Response from /api/entities/ids/:id (same as EntityInfo)
export type EntityDetailsResponse = EntityInfo;

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
  event: 'subscribe_components_update' | 'unsubscribe_components_update';
  payload: { entity_id: number };
}

export interface WsComponentUpdateMessage {
  event: 'notify_components_update';
  payload: {
    id: number;
    components: ComponentInfo[];
  };
}

export interface WsSubscribedMessage {
  event: 'subscribed_components_update' | 'unsubscribed_components_update';
  payload: { entity_id: number };
}

export interface WsErrorMessage {
  event: 'error';
  payload: { message: string };
}

// Root infos subscription messages
export interface WsRootInfosSubscribeMessage {
  event: 'subscribe_root_infos' | 'unsubscribe_root_infos';
}

export interface WsRootInfosMessage {
  event: 'notify_root_infos';
  payload: Array<{
    id: number;
    name: string;
    hierarchy: EntityHierarchy;
    prefab?: EntityPrefab;
  }>;
}

export interface WsRootInfosSubscribedMessage {
  event: 'subscribed_root_infos' | 'unsubscribed_root_infos';
}

// Entity info subscription messages
export interface WsEntityInfoSubscribeMessage {
  event: 'subscribe_entity_info';
  payload: { entities: number[] };
}

export interface WsEntityInfoMessage {
  event: 'notify_entity_info';
  payload: Array<{
    id: number;
    name: string;
    hierarchy: EntityHierarchy;
    prefab?: EntityPrefab;
  }>;
}

export interface WsEntityInfoSubscribedMessage {
  event: 'subscribed_entity_info';
  payload: { entities: number[] };
}

// Selection update messages
export interface WsSelectionUpdateMessage {
  event: 'notify_selection_update';
  payload: { selected: number[] };
}

export interface WsSelectionSubscribedMessage {
  event: 'subscribed_selection_update';
}

// All server event types (individual event in batch)
export type WsServerEvent =
  | WsComponentUpdateMessage
  | WsSubscribedMessage
  | WsRootInfosMessage
  | WsRootInfosSubscribedMessage
  | WsEntityInfoMessage
  | WsEntityInfoSubscribedMessage
  | WsSelectionUpdateMessage
  | WsSelectionSubscribedMessage
  | WsErrorMessage;

// Batched message from server (array of events)
export type WsBatchMessage = WsServerEvent[];

// Legacy single message type (for backwards compatibility)
export type WsServerMessage = WsServerEvent;

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

// Request for PATCH /api/entities/ids/:id/hierarchy
export interface MoveEntityHierarchyRequest {
  parentId?: number | null;  // null = move to root, undefined = no change
  insertBefore?: number;     // Insert before this sibling
  insertAfter?: number;      // Insert after this sibling
}

// Response from PATCH /api/entities/ids/:id/hierarchy
export interface MoveEntityHierarchyResponse {
  id: number;
  hierarchy: EntityHierarchy;
}

// Error response from hierarchy API
export interface HierarchyErrorResponse {
  error: string;
  code?: string;  // "CIRCULAR_REFERENCE", "INVALID_REQUEST", etc.
}

export class GameEngineAPI {
  private static instance: GameEngineAPI;
  private ws: WebSocket | null = null;
  private wsConnecting: boolean = false;
  private wsReconnectTimeout: NodeJS.Timeout | null = null;
  private connectionStateListeners: Set<(connected: boolean) => void> = new Set();
  private registeredComponentsCache: RegisteredComponent[] | null = null;

  // Component update listeners (per entity)
  private componentListeners: Map<number, Set<(components: ComponentInfo[]) => void>> = new Map();

  // Root infos listeners
  private rootInfosListeners: Set<(entities: EntityInfo[]) => void> = new Set();
  private rootInfosSubscribed: boolean = false;

  // Entity info listeners (single callback for all subscribed entities)
  private entityInfoListeners: Set<(entities: EntityInfo[]) => void> = new Set();
  private entityInfoSubscribedIds: Set<number> = new Set();

  // Selection listeners
  private selectionListeners: Set<(selected: number[]) => void> = new Set();
  private selectionSubscribed: boolean = false;
  private lastSentSelection: number[] = [];

  private constructor() { }

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
          const data = JSON.parse(event.data);

          // Handle batched messages (array of events)
          const events: WsServerEvent[] = Array.isArray(data) ? data : [data];

          for (const msg of events) {
            this.handleWsEvent(msg);
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
        const hasActiveListeners =
          this.componentListeners.size > 0 ||
          this.rootInfosListeners.size > 0 ||
          this.entityInfoListeners.size > 0;
        if (hasActiveListeners && !this.wsReconnectTimeout) {
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

      // Re-subscribe to component updates
      Array.from(this.componentListeners.keys()).forEach(entityId => {
        this.sendComponentSubscribe(entityId);
      });

      // Re-subscribe to root infos
      if (this.rootInfosSubscribed) {
        this.sendRootInfosSubscribe();
      }

      // Re-subscribe to entity infos
      if (this.entityInfoSubscribedIds.size > 0) {
        this.sendEntityInfoSubscribe(Array.from(this.entityInfoSubscribedIds));
      }

      // Re-subscribe to selection
      if (this.selectionSubscribed) {
        this.sendSelectionSubscribe();
      }
    } catch (e) {
      console.error('Failed to reconnect WebSocket:', e);
    }
  }

  /**
   * Handle a single WebSocket event
   */
  private handleWsEvent(msg: WsServerEvent): void {
    switch (msg.event) {
      case 'notify_components_update': {
        const entityId = msg.payload.id;
        const listeners = this.componentListeners.get(entityId);
        if (listeners) {
          listeners.forEach((cb) => cb(msg.payload.components));
        }
        break;
      }
      case 'notify_root_infos': {
        // Convert to EntityInfo (id as string)
        const entities: EntityInfo[] = msg.payload.map((e) => ({
          ...e,
          id: String(e.id),
        }));
        this.rootInfosListeners.forEach((cb) => cb(entities));
        break;
      }
      case 'notify_entity_info': {
        // Convert to EntityInfo (id as string)
        const entities: EntityInfo[] = msg.payload.map((e) => ({
          ...e,
          id: String(e.id),
        }));
        this.entityInfoListeners.forEach((cb) => cb(entities));
        break;
      }
      case 'notify_selection_update': {
        const selected: number[] = msg.payload.selected;
        this.selectionListeners.forEach((cb) => cb(selected));
        break;
      }
      // Confirmation messages - can be logged if needed
      case 'subscribed_components_update':
      case 'unsubscribed_components_update':
      case 'subscribed_root_infos':
      case 'unsubscribed_root_infos':
      case 'subscribed_entity_info':
      case 'subscribed_selection_update':
        // Subscriptions confirmed
        break;
      case 'error':
        console.error('WebSocket error from server:', msg.payload.message);
        break;
    }
  }

  // Component subscription send methods
  private sendComponentSubscribe(entityId: number) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      const msg: WsSubscribeMessage = {
        event: 'subscribe_components_update',
        payload: { entity_id: entityId },
      };
      this.ws.send(JSON.stringify(msg));
    }
  }

  private sendComponentUnsubscribe(entityId: number) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      const msg: WsSubscribeMessage = {
        event: 'unsubscribe_components_update',
        payload: { entity_id: entityId },
      };
      this.ws.send(JSON.stringify(msg));
    }
  }

  // Root infos subscription send methods
  private sendRootInfosSubscribe() {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      const msg: WsRootInfosSubscribeMessage = { event: 'subscribe_root_infos' };
      this.ws.send(JSON.stringify(msg));
    }
  }

  private sendRootInfosUnsubscribe() {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      const msg: WsRootInfosSubscribeMessage = { event: 'unsubscribe_root_infos' };
      this.ws.send(JSON.stringify(msg));
    }
  }

  // Entity info subscription send methods
  private sendEntityInfoSubscribe(entityIds: number[]) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      const msg: WsEntityInfoSubscribeMessage = {
        event: 'subscribe_entity_info',
        payload: { entities: entityIds },
      };
      this.ws.send(JSON.stringify(msg));
    }
  }

  // Selection subscription send methods
  private sendSelectionSubscribe() {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({ event: 'subscribe_selection_update' }));
    }
  }

  private sendSelectionUnsubscribe() {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({ event: 'unsubscribe_selection_update' }));
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
    if (!this.componentListeners.has(id)) {
      this.componentListeners.set(id, new Set());
    }
    this.componentListeners.get(id)!.add(callback);

    // Connect and subscribe
    try {
      await this.connectWebSocket();
      this.sendComponentSubscribe(id);
    } catch (e) {
      console.error('Failed to subscribe:', e);
    }

    // Return unsubscribe function
    return () => {
      const listeners = this.componentListeners.get(id);
      if (listeners) {
        listeners.delete(callback);
        if (listeners.size === 0) {
          this.componentListeners.delete(id);
          this.sendComponentUnsubscribe(id);
        }
      }
    };
  }

  /**
   * Subscribe to real-time root entity info updates
   * @param callback Called whenever root entities change
   * @returns Unsubscribe function
   */
  async subscribeToRootInfos(
    callback: (entities: EntityInfo[]) => void
  ): Promise<() => void> {
    // Add listener
    this.rootInfosListeners.add(callback);

    // Connect and subscribe if first listener
    if (!this.rootInfosSubscribed) {
      try {
        await this.connectWebSocket();
        this.sendRootInfosSubscribe();
        this.rootInfosSubscribed = true;
      } catch (e) {
        console.error('Failed to subscribe to root infos:', e);
      }
    }

    // Return unsubscribe function
    return () => {
      this.rootInfosListeners.delete(callback);
      if (this.rootInfosListeners.size === 0 && this.rootInfosSubscribed) {
        this.sendRootInfosUnsubscribe();
        this.rootInfosSubscribed = false;
      }
    };
  }

  /**
   * Subscribe to real-time entity info updates for specific entities
   * @param callback Called whenever any subscribed entity info changes
   * @returns Unsubscribe function
   */
  async subscribeToEntityInfo(
    callback: (entities: EntityInfo[]) => void
  ): Promise<() => void> {
    // Add listener
    this.entityInfoListeners.add(callback);

    // Connect WebSocket if not connected
    try {
      await this.connectWebSocket();
    } catch (e) {
      console.error('Failed to connect for entity info subscription:', e);
    }

    // Return unsubscribe function
    return () => {
      this.entityInfoListeners.delete(callback);
    };
  }

  /**
   * Update the set of entity IDs to subscribe to for entity info updates
   * Call this when the expanded entities change in the hierarchy
   * @param entityIds The complete set of entity IDs to subscribe to
   */
  updateEntityInfoSubscription(entityIds: number[]): void {
    const newIds = new Set(entityIds);

    // Check if the set has changed
    const hasChanged = entityIds.length !== this.entityInfoSubscribedIds.size ||
      entityIds.some(id => !this.entityInfoSubscribedIds.has(id));

    // Update the tracked set
    this.entityInfoSubscribedIds = newIds;

    // Send FULL subscription list to server (server replaces its subscription)
    if (hasChanged && this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.sendEntityInfoSubscribe(entityIds);
    }
  }

  /**
   * Subscribe to real-time selection updates
   * @param callback Called whenever selection changes
   * @returns Unsubscribe function
   */
  async subscribeToSelection(
    callback: (selected: number[]) => void
  ): Promise<() => void> {
    // Add listener
    this.selectionListeners.add(callback);

    // Connect and subscribe if first listener
    if (!this.selectionSubscribed) {
      try {
        await this.connectWebSocket();
        this.sendSelectionSubscribe();
        this.selectionSubscribed = true;
      } catch (e) {
        console.error('Failed to subscribe to selection:', e);
      }
    }

    // Return unsubscribe function
    return () => {
      this.selectionListeners.delete(callback);
      if (this.selectionListeners.size === 0 && this.selectionSubscribed) {
        this.sendSelectionUnsubscribe();
        this.selectionSubscribed = false;
      }
    };
  }

  /**
   * Send selection update to server
   * @param selected Array of selected entity IDs
   */
  sendSelectionUpdate(selected: number[]): void {
    // Check if selection actually changed to avoid duplicate sends
    if (this.arraysEqual(selected, this.lastSentSelection)) return;
    this.lastSentSelection = [...selected];

    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({
        event: 'update_selection',
        payload: { selected }
      }));
    }
  }

  /**
   * Helper: Compare two arrays for equality
   */
  private arraysEqual(a: number[], b: number[]): boolean {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) {
      if (a[i] !== b[i]) return false;
    }
    return true;
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

      // API now returns array directly (not { entities: [...] })
      const data: Array<{
        id: number;
        name: string;
        hierarchy: EntityHierarchy;
        prefab?: EntityPrefab;
      }> = await response.json();

      // Convert id to string (server returns number)
      return data.map((e) => ({
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

      const data: {
        id: number;
        name: string;
        hierarchy: EntityHierarchy;
        prefab?: EntityPrefab;
      } = await response.json();

      // Convert id to string (server returns number)
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
   * Move an entity in the hierarchy (change parent, reorder siblings)
   * @param entityId - The entity ID to move
   * @param request - The move operation parameters
   * @returns The updated hierarchy info
   * @throws Error with code "CIRCULAR_REFERENCE" if move would create a cycle
   */
  async moveEntityHierarchy(entityId: string, request: MoveEntityHierarchyRequest): Promise<MoveEntityHierarchyResponse> {
    const response = await fetch(`${API_BASE_URL}/api/entities/ids/${entityId}/hierarchy`, {
      method: 'PATCH',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(request),
    });

    const data = await response.json();

    if (!response.ok) {
      const errorData = data as HierarchyErrorResponse;
      const error = new Error(errorData.error || `HTTP error! status: ${response.status}`);
      (error as Error & { code?: string }).code = errorData.code;
      throw error;
    }

    return data as MoveEntityHierarchyResponse;
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