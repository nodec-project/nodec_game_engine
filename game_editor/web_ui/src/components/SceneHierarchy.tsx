'use client';

import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  List,
  ListItem,
  ListItemText,
  ListItemIcon,
  Paper,
  Alert,
  CircularProgress,
  IconButton,
  Collapse,
} from '@mui/material';
import {
  Folder as FolderIcon,
  FolderOpen as FolderOpenIcon,
  Description as EntityIcon,
  Refresh as RefreshIcon,
  ExpandMore as ExpandMoreIcon,
  ChevronRight as ChevronRightIcon,
} from '@mui/icons-material';
import { gameEngineAPI, EntityInfo, EntityDetailsResponse } from '../api/gameEngine';

interface SceneHierarchyProps {
  onEntitySelect?: (entityId: string) => void;
}

interface EntityNode extends EntityInfo {
  children?: EntityNode[];
  childrenLoaded?: boolean;
}

export const SceneHierarchy: React.FC<SceneHierarchyProps> = ({ onEntitySelect }) => {
  const [entities, setEntities] = useState<EntityNode[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [engineConnected, setEngineConnected] = useState(false);
  const [selectedEntityId, setSelectedEntityId] = useState<string | null>(null);
  const [expandedEntities, setExpandedEntities] = useState<Set<string>>(new Set());
  const [loadingChildren, setLoadingChildren] = useState<Set<string>>(new Set());

  const fetchRootEntities = async () => {
    try {
      setLoading(true);
      setError(null);
      
      // Check if engine is connected
      const isConnected = await gameEngineAPI.healthCheck();
      setEngineConnected(isConnected);
      
      if (!isConnected) {
        setError('Game engine server is not running on localhost:8080');
        return;
      }

      const rootEntities = await gameEngineAPI.getRootEntities();
      setEntities(rootEntities);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch entities');
      setEngineConnected(false);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchRootEntities();
  }, []);

  const fetchChildEntities = async (parentEntity: EntityNode): Promise<EntityNode[]> => {
    try {
      const details = await gameEngineAPI.getEntityDetails(parentEntity.id);
      const childNodes: EntityNode[] = [];
      
      // Check if hierarchy exists and has children
      if (details.hierarchy && details.hierarchy.children) {
        for (const childId of details.hierarchy.children) {
          const childDetails = await gameEngineAPI.getEntityDetails(String(childId));
          childNodes.push({
            id: String(childId),
            name: childDetails.name,
            has_children: childDetails.hierarchy ? childDetails.hierarchy.children.length > 0 : false,
            childrenLoaded: false,
          });
        }
      }
      
      return childNodes;
    } catch (err) {
      console.error(`Failed to fetch children for entity ${parentEntity.id}:`, err);
      return [];
    }
  };

  const handleEntityClick = (entityId: string) => {
    setSelectedEntityId(entityId);
    onEntitySelect?.(entityId);
  };

  const handleExpandToggle = async (entity: EntityNode) => {
    const entityId = entity.id;
    const newExpanded = new Set(expandedEntities);
    
    if (newExpanded.has(entityId)) {
      newExpanded.delete(entityId);
    } else {
      newExpanded.add(entityId);
      
      // Load children if not loaded yet
      if (entity.has_children && !entity.childrenLoaded) {
        setLoadingChildren(prev => {
          const next = new Set(prev);
          next.add(entityId);
          return next;
        });
        const children = await fetchChildEntities(entity);
        
        // Update the entity tree
        const updateEntityChildren = (nodes: EntityNode[]): EntityNode[] => {
          return nodes.map(node => {
            if (node.id === entityId) {
              return { ...node, children, childrenLoaded: true };
            }
            if (node.children) {
              return { ...node, children: updateEntityChildren(node.children) };
            }
            return node;
          });
        };
        
        setEntities(updateEntityChildren(entities));
        setLoadingChildren(prev => {
          const next = new Set(prev);
          next.delete(entityId);
          return next;
        });
      }
    }
    setExpandedEntities(newExpanded);
  };

  const renderEntity = (entity: EntityNode, depth: number = 0): React.ReactNode => {
    const isSelected = selectedEntityId === entity.id;
    const isExpanded = expandedEntities.has(entity.id);
    const isLoadingChildren = loadingChildren.has(entity.id);

    return (
      <React.Fragment key={entity.id}>
        <ListItem
          sx={{
            pl: 2 + depth * 2,
            backgroundColor: isSelected ? 'action.selected' : 'transparent',
            '&:hover': {
              backgroundColor: 'action.hover',
            },
            cursor: 'pointer',
          }}
          onClick={() => handleEntityClick(entity.id)}
        >
          <ListItemIcon sx={{ minWidth: 32 }}>
            {entity.has_children ? (
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  handleExpandToggle(entity);
                }}
                sx={{ p: 0.5 }}
                disabled={isLoadingChildren}
              >
                {isLoadingChildren ? (
                  <CircularProgress size={16} />
                ) : isExpanded ? (
                  <ExpandMoreIcon />
                ) : (
                  <ChevronRightIcon />
                )}
              </IconButton>
            ) : (
              <EntityIcon fontSize="small" />
            )}
          </ListItemIcon>
          <ListItemText
            primary={entity.name}
            primaryTypographyProps={{
              fontSize: '0.875rem',
              fontWeight: isSelected ? 'bold' : 'normal',
            }}
          />
          {entity.has_children && (
            <ListItemIcon sx={{ minWidth: 'auto', ml: 1 }}>
              {isExpanded ? <FolderOpenIcon fontSize="small" /> : <FolderIcon fontSize="small" />}
            </ListItemIcon>
          )}
        </ListItem>
        
        {/* Render children if expanded */}
        {isExpanded && entity.children && (
          <Collapse in={isExpanded} timeout="auto" unmountOnExit>
            {entity.children.map((child) => renderEntity(child, depth + 1))}
          </Collapse>
        )}
      </React.Fragment>
    );
  };

  return (
    <Paper sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
      <Box sx={{ p: 2, borderBottom: 1, borderColor: 'divider' }}>
        <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
          <Typography variant="h6" component="h2">
            Scene Hierarchy
          </Typography>
          <IconButton onClick={fetchRootEntities} disabled={loading} size="small">
            <RefreshIcon />
          </IconButton>
        </Box>
        
        {/* Connection Status */}
        <Box sx={{ mt: 1 }}>
          <Typography variant="caption" color={engineConnected ? 'success.main' : 'error.main'}>
            Engine: {engineConnected ? 'Connected' : 'Disconnected'}
          </Typography>
        </Box>
      </Box>

      <Box sx={{ flex: 1, overflow: 'auto' }}>
        {loading && (
          <Box sx={{ display: 'flex', justifyContent: 'center', p: 4 }}>
            <CircularProgress />
          </Box>
        )}

        {error && (
          <Box sx={{ p: 2 }}>
            <Alert severity="error">
              {error}
              {!engineConnected && (
                <Box sx={{ mt: 1 }}>
                  <Typography variant="caption">
                    Make sure the game engine is running and the Inspector Server is enabled.
                  </Typography>
                </Box>
              )}
            </Alert>
          </Box>
        )}

        {!loading && !error && entities.length === 0 && (
          <Box sx={{ p: 2 }}>
            <Alert severity="info">
              No root entities found in the scene.
            </Alert>
          </Box>
        )}

        {!loading && !error && entities.length > 0 && (
          <List dense sx={{ py: 0 }}>
            {entities.map((entity) => renderEntity(entity))}
          </List>
        )}
      </Box>
    </Paper>
  );
};