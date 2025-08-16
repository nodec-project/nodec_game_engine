// Game Engine API Client

export interface EntityInfo {
  id: string;
  name: string;
  has_children: boolean;
}

export interface RootEntitiesResponse {
  entities: EntityInfo[];
}

export interface ComponentInfo {
  type_index: number;
  data: Record<string, unknown> | null; // JSON serialized component data
}

export interface EntityComponentsResponse {
  entity_id: string;
  components: ComponentInfo[];
}

export interface EntityDetailsResponse {
  entity_id: string;
  name: string;
  parent_id?: string;
  children: string[];
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

      const data: EntityDetailsResponse = await response.json();
      return data;
    } catch (error) {
      console.error(`Failed to fetch details for entity ${entityId}:`, error);
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