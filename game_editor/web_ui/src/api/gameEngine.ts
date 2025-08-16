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

// Response from /api/animations/editing-context
export interface AnimationEditingContext {
  hasAnimator: boolean;
  entityId: number;
  entityName: string;
  error?: string;
  hasClip?: boolean;
  clipPath?: string | null;
  clipData?: {
    duration: number;
    curveCount: number;
  } | null;
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
   * Get animation editing context for an entity with Animator component
   */
  async getAnimationEditingContext(entityId: string): Promise<AnimationEditingContext> {
    try {
      const response = await fetch(`${API_BASE_URL}/api/animations/editing-context?entityId=${entityId}`, {
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
      console.error(`Failed to fetch animation editing context for entity ${entityId}:`, error);
      throw error;
    }
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