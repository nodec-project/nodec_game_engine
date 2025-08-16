// Game Engine API Client

export interface EntityInfo {
  id: string;
  name: string;
  has_children: boolean;
}

export interface RootEntitiesResponse {
  entities: EntityInfo[];
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