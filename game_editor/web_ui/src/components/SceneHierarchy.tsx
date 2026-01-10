'use client';

import React, { useState, useEffect, useCallback, useRef } from 'react';
import { Surface, Alert, Spinner, IconButton, Collapse, List, Typography, Icon } from '@/ui';
import { gameEngineAPI, EntityInfo, entityHasChildren, entityHasPrefab } from '../api/gameEngine';
import { useEditor } from '../contexts/EditorContext';
import styles from './SceneHierarchy.module.css';

interface SceneHierarchyProps {
  onEntitySelect?: (entityId: string) => void;
}

interface EntityNode extends EntityInfo {
  childNodes?: EntityNode[];
  childrenLoaded?: boolean;
}

export const SceneHierarchy: React.FC<SceneHierarchyProps> = ({ onEntitySelect }) => {
  const { engineConnected } = useEditor();
  const [entities, setEntities] = useState<EntityNode[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [selectedEntityId, setSelectedEntityId] = useState<string | null>(null);
  const [expandedEntities, setExpandedEntities] = useState<Set<string>>(new Set());
  const [loadingChildren, setLoadingChildren] = useState<Set<string>>(new Set());

  // Ref to track entities for subscription updates (avoids stale closure)
  const entitiesRef = useRef<EntityNode[]>([]);
  entitiesRef.current = entities;

  // Ref to track expanded entities for subscription management
  const expandedEntitiesRef = useRef<Set<string>>(new Set());
  expandedEntitiesRef.current = expandedEntities;

  // Helper: Find entity by ID in tree
  const findEntityInTree = useCallback((nodes: EntityNode[], id: string): EntityNode | null => {
    for (const node of nodes) {
      if (node.id === id) return node;
      if (node.childNodes) {
        const found = findEntityInTree(node.childNodes, id);
        if (found) return found;
      }
    }
    return null;
  }, []);

  // Helper: Update entity in tree by ID (preserves childNodes)
  const updateEntityInTree = useCallback((
    nodes: EntityNode[],
    id: string,
    updater: (node: EntityNode) => EntityNode
  ): EntityNode[] => {
    return nodes.map(node => {
      if (node.id === id) {
        return updater(node);
      }
      if (node.childNodes) {
        return { ...node, childNodes: updateEntityInTree(node.childNodes, id, updater) };
      }
      return node;
    });
  }, []);

  // Handle root entities update from WebSocket
  const handleRootInfosUpdate = useCallback((rootInfos: EntityInfo[]) => {
    setEntities(prevEntities => {
      // Create a map of existing entities for quick lookup (preserve childNodes)
      const existingMap = new Map<string, EntityNode>();
      for (const entity of prevEntities) {
        existingMap.set(entity.id, entity);
      }

      // Merge new root infos with existing childNodes
      return rootInfos.map(info => {
        const existing = existingMap.get(info.id);
        if (existing) {
          // Preserve childNodes and childrenLoaded state
          return {
            ...info,
            childNodes: existing.childNodes,
            childrenLoaded: existing.childrenLoaded,
          };
        }
        return info;
      });
    });
    setLoading(false);
    setError(null);
  }, []);

  // Handle entity info update from WebSocket
  const handleEntityInfoUpdate = useCallback((entityInfos: EntityInfo[]) => {
    setEntities(prevEntities => {
      let updated = prevEntities;

      for (const info of entityInfos) {
        // Find the parent entity that should contain this as child
        const parentId = info.hierarchy.parent;

        if (parentId === null) {
          // This is a root entity, update in root list
          updated = updated.map(node => {
            if (node.id === info.id) {
              return {
                ...info,
                childNodes: node.childNodes,
                childrenLoaded: node.childrenLoaded,
              };
            }
            return node;
          });
        } else {
          // This is a child entity, find and update in tree
          updated = updateEntityInTree(updated, info.id, node => ({
            ...info,
            childNodes: node.childNodes,
            childrenLoaded: node.childrenLoaded,
          }));

          // Also update parent's childNodes if this entity is new
          const parentIdStr = String(parentId);
          updated = updateEntityInTree(updated, parentIdStr, parentNode => {
            if (!parentNode.childNodes) return parentNode;

            // Check if child exists
            const childExists = parentNode.childNodes.some(c => c.id === info.id);
            if (childExists) {
              // Update existing child
              return {
                ...parentNode,
                childNodes: parentNode.childNodes.map(c =>
                  c.id === info.id
                    ? { ...info, childNodes: c.childNodes, childrenLoaded: c.childrenLoaded }
                    : c
                ),
              };
            } else {
              // Add new child
              return {
                ...parentNode,
                childNodes: [...parentNode.childNodes, info],
              };
            }
          });
        }
      }

      return updated;
    });
  }, [updateEntityInTree]);

  // Subscribe to root infos on mount
  useEffect(() => {
    let unsubscribe: (() => void) | null = null;

    const subscribe = async () => {
      try {
        setLoading(true);
        unsubscribe = await gameEngineAPI.subscribeToRootInfos(handleRootInfosUpdate);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to subscribe to root entities');
        setLoading(false);
      }
    };

    subscribe();

    return () => {
      if (unsubscribe) unsubscribe();
    };
  }, [handleRootInfosUpdate]);

  // Subscribe to entity info for expanded entities' children
  useEffect(() => {
    let unsubscribe: (() => void) | null = null;

    const subscribe = async () => {
      try {
        unsubscribe = await gameEngineAPI.subscribeToEntityInfo(handleEntityInfoUpdate);
      } catch (err) {
        console.error('Failed to subscribe to entity info:', err);
      }
    };

    subscribe();

    return () => {
      if (unsubscribe) unsubscribe();
    };
  }, [handleEntityInfoUpdate]);

  // Update entity info subscription when expanded entities change
  useEffect(() => {
    // Collect all entity IDs we need to subscribe to:
    // - Children of expanded entities (to get updates when children change)
    const idsToSubscribe: number[] = [];

    const collectChildIds = (nodes: EntityNode[]) => {
      for (const node of nodes) {
        if (expandedEntities.has(node.id) && node.childNodes) {
          // Subscribe to children of expanded nodes
          for (const child of node.childNodes) {
            idsToSubscribe.push(parseInt(child.id, 10));
          }
          // Recursively collect from children
          collectChildIds(node.childNodes);
        }
      }
    };

    collectChildIds(entities);

    // Update subscription
    gameEngineAPI.updateEntityInfoSubscription(idsToSubscribe);
  }, [expandedEntities, entities]);

  const fetchChildEntities = useCallback(async (parentEntity: EntityNode): Promise<EntityNode[]> => {
    try {
      const childNodes: EntityNode[] = [];

      // Use hierarchy.children from the parent entity
      for (const childId of parentEntity.hierarchy.children) {
        const childDetails = await gameEngineAPI.getEntityDetails(String(childId));
        childNodes.push({
          ...childDetails,
          childrenLoaded: false,
        });
      }

      return childNodes;
    } catch (err) {
      console.error(`Failed to fetch children for entity ${parentEntity.id}:`, err);
      return [];
    }
  }, []);

  const handleEntityClick = useCallback((entityId: string) => {
    setSelectedEntityId(entityId);
    onEntitySelect?.(entityId);
  }, [onEntitySelect]);

  const handleExpandToggle = useCallback(async (entity: EntityNode) => {
    const entityId = entity.id;

    setExpandedEntities(prev => {
      const next = new Set(prev);
      if (next.has(entityId)) {
        next.delete(entityId);
      } else {
        next.add(entityId);
      }
      return next;
    });

    // Load children if not loaded yet and expanding
    if (!expandedEntities.has(entityId) && entityHasChildren(entity) && !entity.childrenLoaded) {
      setLoadingChildren(prev => {
        const next = new Set(prev);
        next.add(entityId);
        return next;
      });

      const children = await fetchChildEntities(entity);

      // Update the entity tree
      setEntities(prev => updateEntityInTree(prev, entityId, node => ({
        ...node,
        childNodes: children,
        childrenLoaded: true,
      })));

      setLoadingChildren(prev => {
        const next = new Set(prev);
        next.delete(entityId);
        return next;
      });
    }
  }, [expandedEntities, fetchChildEntities, updateEntityInTree]);

  const handleRefresh = useCallback(async () => {
    try {
      setLoading(true);
      setError(null);
      const rootEntities = await gameEngineAPI.getRootEntities();
      setEntities(rootEntities);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch entities');
    } finally {
      setLoading(false);
    }
  }, []);

  const getDepthClass = useCallback((depth: number): string => {
    if (depth >= 5) return styles.depth5;
    return styles[`depth${depth}` as keyof typeof styles] || '';
  }, []);

  const renderEntity = useCallback((entity: EntityNode, depth: number = 0): React.ReactNode => {
    const isSelected = selectedEntityId === entity.id;
    const isExpanded = expandedEntities.has(entity.id);
    const isLoadingChildren = loadingChildren.has(entity.id);
    const hasChildren = entityHasChildren(entity);
    const isPrefab = entityHasPrefab(entity);

    const itemClasses = [
      styles.entityItem,
      isSelected && styles.entityItemSelected,
      getDepthClass(depth),
    ].filter(Boolean).join(' ');

    const nameClasses = [
      styles.entityName,
      isSelected && styles.entityNameSelected,
      isPrefab && styles.entityNamePrefab,
    ].filter(Boolean).join(' ');

    return (
      <React.Fragment key={entity.id}>
        <div
          className={itemClasses}
          onClick={() => handleEntityClick(entity.id)}
        >
          <div className={styles.entityIcon}>
            {hasChildren ? (
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  handleExpandToggle(entity);
                }}
                className={styles.expandButton}
                disabled={isLoadingChildren}
              >
                {isLoadingChildren ? (
                  <Spinner size={16} />
                ) : isExpanded ? (
                  <Icon name="expand_more" size={18} />
                ) : (
                  <Icon name="chevron_right" size={18} />
                )}
              </IconButton>
            ) : (
              <Icon name="description" size={18} />
            )}
          </div>
          <span className={nameClasses}>{entity.name}</span>
          {isPrefab && (
            <Icon name="deployed_code" size={16} className={styles.prefabIcon} />
          )}
          {hasChildren && (
            <div className={styles.folderIcon}>
              {isExpanded ? <Icon name="folder_open" size={18} /> : <Icon name="folder" size={18} />}
            </div>
          )}
        </div>

        {/* Render children if expanded */}
        {isExpanded && entity.childNodes && (
          <Collapse in={isExpanded} unmountOnExit>
            {entity.childNodes.map((child) => renderEntity(child, depth + 1))}
          </Collapse>
        )}
      </React.Fragment>
    );
  }, [selectedEntityId, expandedEntities, loadingChildren, getDepthClass, handleEntityClick, handleExpandToggle]);

  return (
    <Surface className={styles.container}>
      <div className={styles.header}>
        <IconButton onClick={handleRefresh} disabled={loading} size="small">
          <Icon name="refresh" size={18} />
        </IconButton>
      </div>

      <div className={styles.content}>
        {loading && (
          <div className={styles.loadingContainer}>
            <Spinner />
          </div>
        )}

        {error && (
          <div className={styles.messageContainer}>
            <Alert severity="error">
              {error}
              {!engineConnected && (
                <Typography variant="body-small" className={styles.messageCaption}>
                  Make sure the game engine is running and the Inspector Server is enabled.
                </Typography>
              )}
            </Alert>
          </div>
        )}

        {!loading && !error && entities.length === 0 && (
          <div className={styles.messageContainer}>
            <Alert severity="info">
              No root entities found in the scene.
            </Alert>
          </div>
        )}

        {!loading && !error && entities.length > 0 && (
          <List dense>
            {entities.map((entity) => renderEntity(entity))}
          </List>
        )}
      </div>
    </Surface>
  );
};
