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

// Animated component in clip
export interface AnimatedComponentData {
  placeholder: {
    type_info: {
      seq_index: number;
    };
  };
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

export class GameEngineAPI {
  private static instance: GameEngineAPI;

  private constructor() {}

  public static getInstance(): GameEngineAPI {
    if (!GameEngineAPI.instance) {
      GameEngineAPI.instance = new GameEngineAPI();
    }
    return GameEngineAPI.instance;
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

      const data: RootEntitiesResponse = await response.json();
      return data.entities;
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