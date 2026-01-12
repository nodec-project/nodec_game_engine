'use client';

import React, { useState, useEffect, useCallback, useRef } from 'react';
import { Surface, Alert, Spinner, IconButton, Collapse, List, Typography, Icon } from '@/ui';
import { gameEngineAPI, EntityInfo, entityHasChildren, entityHasPrefab, MoveEntityHierarchyRequest } from '../api/gameEngine';
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

  // Drag & Drop state
  type DropType = 'before' | 'child' | 'after';
  const [draggingEntityId, setDraggingEntityId] = useState<string | null>(null);
  const [dropTarget, setDropTarget] = useState<{ entityId: string; dropType: DropType } | null>(null);

  // Refs for selection sync
  const entitiesRef = useRef<EntityNode[]>([]);
  const prevSelectionRef = useRef<number[]>([]);

  // Keep entitiesRef in sync with entities state
  useEffect(() => { entitiesRef.current = entities; }, [entities]);

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

  // Helper: Filter and reorder childNodes based on hierarchy.children
  // Removes children that have moved away (not in hierarchyChildren)
  const reorderChildren = useCallback((
    childNodes: EntityNode[] | undefined,
    hierarchyChildren: number[]
  ): EntityNode[] | undefined => {
    if (!childNodes) return childNodes;

    // Filter out children that are no longer in hierarchy.children
    const hierarchyChildrenSet = new Set(hierarchyChildren);
    const filtered = childNodes.filter(c => hierarchyChildrenSet.has(parseInt(c.id, 10)));

    if (filtered.length === 0) return undefined;
    if (filtered.length === 1) return filtered;

    // Sort remaining children by hierarchy order
    return filtered.sort((a, b) => {
      const aIdx = hierarchyChildren.indexOf(parseInt(a.id, 10));
      const bIdx = hierarchyChildren.indexOf(parseInt(b.id, 10));
      return aIdx - bIdx;
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
      const result = rootInfos.map(info => {
        const existing = existingMap.get(info.id);
        if (existing) {
          // Preserve childNodes but reorder based on new hierarchy.children
          return {
            ...info,
            childNodes: reorderChildren(existing.childNodes, info.hierarchy.children),
            childrenLoaded: existing.childrenLoaded,
          };
        }
        return info;
      });
      return result;
    });
    setLoading(false);
    setError(null);
  }, [reorderChildren]);

  // Helper: Remove entity from tree (returns tree without the entity, and the removed node if found)
  const removeEntityFromTree = useCallback((
    nodes: EntityNode[],
    id: string
  ): { nodes: EntityNode[]; removed: EntityNode | null } => {
    let removed: EntityNode | null = null;

    // Check if entity is in root level
    const rootIndex = nodes.findIndex(n => n.id === id);
    if (rootIndex !== -1) {
      removed = nodes[rootIndex];
      return {
        nodes: [...nodes.slice(0, rootIndex), ...nodes.slice(rootIndex + 1)],
        removed,
      };
    }

    // Recursively search and remove from children
    const newNodes = nodes.map(node => {
      if (!node.childNodes || removed) return node;

      const childIndex = node.childNodes.findIndex(c => c.id === id);
      if (childIndex !== -1) {
        removed = node.childNodes[childIndex];
        const removedIdNum = parseInt(id, 10);
        return {
          ...node,
          hierarchy: {
            ...node.hierarchy,
            children: node.hierarchy.children.filter(cid => cid !== removedIdNum),
          },
          childNodes: [
            ...node.childNodes.slice(0, childIndex),
            ...node.childNodes.slice(childIndex + 1),
          ],
        };
      }

      // Recurse into children
      const result = removeEntityFromTree(node.childNodes, id);
      if (result.removed) {
        removed = result.removed;
        return { ...node, childNodes: result.nodes };
      }

      return node;
    });

    return { nodes: newNodes, removed };
  }, []);

  // Handle entity info update from WebSocket
  const handleEntityInfoUpdate = useCallback((entityInfos: EntityInfo[]) => {
    setEntities(prevEntities => {
      let updated = prevEntities;

      // First pass: Update/move entities
      for (const info of entityInfos) {
        const parentId = info.hierarchy.parent;

        // Remove the entity from its current position (preserving childNodes)
        const { nodes: withoutEntity, removed } = removeEntityFromTree(updated, info.id);
        updated = withoutEntity;

        // Preserve childNodes but reorder based on new hierarchy.children
        const newNode: EntityNode = {
          ...info,
          childNodes: reorderChildren(removed?.childNodes, info.hierarchy.children),
          childrenLoaded: removed?.childrenLoaded,
        };

        if (parentId === null) {
          // Add to root level
          updated = [...updated, newNode];
        } else {
          // Add as child of parent
          const parentIdStr = String(parentId);
          const newNodeId = parseInt(info.id, 10);

          updated = updateEntityInTree(updated, parentIdStr, parentNode => {
            const childNodes = parentNode.childNodes || [];
            // Check if child already exists to avoid duplicates
            const alreadyExists = childNodes.some(c => c.id === info.id);
            if (alreadyExists) {
              // Update existing child instead of adding duplicate
              return {
                ...parentNode,
                childNodes: childNodes.map(c => c.id === info.id ? newNode : c),
              };
            }
            // Also update hierarchy.children to include the new child
            const updatedHierarchyChildren = parentNode.hierarchy.children.includes(newNodeId)
              ? parentNode.hierarchy.children
              : [...parentNode.hierarchy.children, newNodeId];
            return {
              ...parentNode,
              hierarchy: {
                ...parentNode.hierarchy,
                children: updatedHierarchyChildren,
              },
              childNodes: [...childNodes, newNode],
            };
          });

          // If parent not found in tree (not loaded yet), entity won't be visible
          // This is expected - it will appear when parent is expanded
        }
      }

      // Second pass: Reorder parent's children based on hierarchy.children
      // This handles the case where the parent is also in entityInfos
      for (const info of entityInfos) {
        updated = updateEntityInTree(updated, info.id, node => {
          const reordered = reorderChildren(node.childNodes, node.hierarchy.children);
          if (reordered === node.childNodes) return node;
          return { ...node, childNodes: reordered };
        });
      }

      return updated;
    });
  }, [updateEntityInTree, removeEntityFromTree, reorderChildren]);

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
    // - Root entities (always, to track when they move to/from root)
    // - Expanded entities (to get hierarchy.children updates)
    // - Children of expanded entities (to get updates when children change)
    const idsToSubscribe: number[] = [];

    const collectIds = (nodes: EntityNode[], isRoot: boolean) => {
      for (const node of nodes) {
        const isExpanded = expandedEntities.has(node.id);

        // Always subscribe to root entities (even if not expanded)
        // so we can track when they move to become children
        if (isRoot || isExpanded) {
          idsToSubscribe.push(parseInt(node.id, 10));
        }

        if (isExpanded && node.childNodes) {
          // Subscribe to children of expanded nodes
          for (const child of node.childNodes) {
            idsToSubscribe.push(parseInt(child.id, 10));
          }
          // Recursively collect from children (no longer root level)
          collectIds(node.childNodes, false);
        }
      }
    };

    collectIds(entities, true);

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

  // Expand tree to show a selected entity (load ancestors from root down)
  const expandToEntity = useCallback(async (targetEntityId: string) => {
    // Build ancestor path from target to root via API
    const ancestorPath: string[] = [];
    let currentId = targetEntityId;

    while (true) {
      try {
        const info = await gameEngineAPI.getEntityDetails(currentId);
        if (!info.hierarchy.parent) break;  // Reached root
        currentId = String(info.hierarchy.parent);
        ancestorPath.unshift(currentId);  // Add to beginning (root-first)
      } catch {
        break;  // Entity not found
      }
    }

    if (ancestorPath.length === 0) return;  // Target is at root level

    // Expand and load children for each ancestor from root down
    for (const ancestorId of ancestorPath) {
      // Skip if already expanded
      if (expandedEntities.has(ancestorId)) continue;

      // Add to expanded set
      setExpandedEntities(prev => {
        const next = new Set(prev);
        next.add(ancestorId);
        return next;
      });

      // Find node in current tree
      const node = findEntityInTree(entitiesRef.current, ancestorId);
      if (!node) continue;  // Node not in tree yet

      // Load children if not already loaded
      if (entityHasChildren(node) && !node.childrenLoaded) {
        setLoadingChildren(prev => {
          const next = new Set(prev);
          next.add(ancestorId);
          return next;
        });

        try {
          const children = await fetchChildEntities(node);
          setEntities(prev => updateEntityInTree(prev, ancestorId, n => ({
            ...n,
            childNodes: children,
            childrenLoaded: true,
          })));
        } finally {
          setLoadingChildren(prev => {
            const next = new Set(prev);
            next.delete(ancestorId);
            return next;
          });
        }

        // Wait for state update to propagate
        await new Promise(resolve => setTimeout(resolve, 0));
      }
    }
  }, [expandedEntities, findEntityInTree, fetchChildEntities, updateEntityInTree]);

  // Helper: Compare two arrays for equality
  const arraysEqual = useCallback((a: number[], b: number[]): boolean => {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) {
      if (a[i] !== b[i]) return false;
    }
    return true;
  }, []);

  // Subscribe to selection updates from native editor
  useEffect(() => {
    let unsubscribe: (() => void) | null = null;

    const subscribe = async () => {
      try {
        unsubscribe = await gameEngineAPI.subscribeToSelection((selected) => {
          // Only react if selection actually changed
          if (arraysEqual(selected, prevSelectionRef.current)) return;
          prevSelectionRef.current = [...selected];

          // Update local selection state
          if (selected.length > 0) {
            const primaryId = String(selected[0]);
            setSelectedEntityId(primaryId);
            onEntitySelect?.(primaryId);
            // Auto-expand to show selected entity
            expandToEntity(primaryId);
          } else {
            setSelectedEntityId(null);
            onEntitySelect?.(null as unknown as string);
          }
        });
      } catch (err) {
        console.error('Failed to subscribe to selection:', err);
      }
    };

    subscribe();

    return () => {
      if (unsubscribe) unsubscribe();
    };
  }, [arraysEqual, expandToEntity, onEntitySelect]);

  const handleEntityClick = useCallback((entityId: string) => {
    setSelectedEntityId(entityId);
    onEntitySelect?.(entityId);
    // Send selection to server
    gameEngineAPI.sendSelectionUpdate([parseInt(entityId, 10)]);
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

  // Drag & Drop handlers
  const handleDragStart = useCallback((e: React.DragEvent, entityId: string) => {
    e.dataTransfer.setData('text/plain', entityId);
    e.dataTransfer.effectAllowed = 'move';
    setDraggingEntityId(entityId);
  }, []);

  const handleDragEnd = useCallback(() => {
    setDraggingEntityId(null);
    setDropTarget(null);
  }, []);

  const handleDragOver = useCallback((e: React.DragEvent, entityId: string) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';

    // Calculate drop zone based on mouse position
    const rect = e.currentTarget.getBoundingClientRect();
    const y = e.clientY - rect.top;
    const height = rect.height;

    let dropType: DropType;
    if (y < height * 0.25) {
      dropType = 'before';
    } else if (y > height * 0.75) {
      dropType = 'after';
    } else {
      dropType = 'child';
    }

    // Don't allow dropping on self
    if (draggingEntityId === entityId) {
      setDropTarget(null);
      return;
    }

    setDropTarget({ entityId, dropType });
  }, [draggingEntityId]);

  const handleDragLeave = useCallback((e: React.DragEvent) => {
    // Only clear if actually leaving the element (not entering a child)
    const relatedTarget = e.relatedTarget as HTMLElement | null;
    if (!e.currentTarget.contains(relatedTarget)) {
      setDropTarget(null);
    }
  }, []);

  const handleDrop = useCallback(async (e: React.DragEvent, targetEntityId: string) => {
    e.preventDefault();
    e.stopPropagation(); // Prevent bubbling to content container (handleDropOnRoot)
    const draggedEntityId = e.dataTransfer.getData('text/plain');

    if (!draggedEntityId || draggedEntityId === targetEntityId || !dropTarget) {
      setDropTarget(null);
      setDraggingEntityId(null);
      return;
    }

    try {
      const request: MoveEntityHierarchyRequest = {};

      // Find target entity to get its parent
      const targetEntity = findEntityInTree(entities, targetEntityId);

      switch (dropTarget.dropType) {
        case 'before':
          request.insertBefore = parseInt(targetEntityId, 10);
          break;
        case 'after':
          request.insertAfter = parseInt(targetEntityId, 10);
          break;
        case 'child':
          request.parentId = parseInt(targetEntityId, 10);
          break;
      }

      await gameEngineAPI.moveEntityHierarchy(draggedEntityId, request);

      // Refresh will happen automatically via WebSocket subscription
      console.log(`Moved entity ${draggedEntityId} ${dropTarget.dropType} ${targetEntityId}`);
    } catch (err) {
      const error = err as Error & { code?: string };
      if (error.code === 'CIRCULAR_REFERENCE') {
        console.warn('Cannot create circular reference in hierarchy');
      } else {
        console.error('Failed to move entity:', err);
      }
    } finally {
      setDropTarget(null);
      setDraggingEntityId(null);
    }
  }, [dropTarget, entities, findEntityInTree]);

  // Handle drop on empty area (move to root)
  const handleDropOnRoot = useCallback(async (e: React.DragEvent) => {
    e.preventDefault();
    const draggedEntityId = e.dataTransfer.getData('text/plain');

    if (!draggedEntityId) {
      return;
    }

    try {
      await gameEngineAPI.moveEntityHierarchy(draggedEntityId, { parentId: null });
      console.log(`Moved entity ${draggedEntityId} to root`);
    } catch (err) {
      console.error('Failed to move entity to root:', err);
    } finally {
      setDropTarget(null);
      setDraggingEntityId(null);
    }
  }, []);

  const handleDragOverRoot = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
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
    const isDragging = draggingEntityId === entity.id;
    const isDropTarget = dropTarget?.entityId === entity.id;
    const currentDropType = isDropTarget ? dropTarget.dropType : null;

    const itemClasses = [
      styles.entityItem,
      isSelected && styles.entityItemSelected,
      isDragging && styles.entityItemDragging,
      isDropTarget && currentDropType === 'before' && styles.dropBefore,
      isDropTarget && currentDropType === 'after' && styles.dropAfter,
      isDropTarget && currentDropType === 'child' && styles.dropChild,
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
          draggable
          onDragStart={(e) => handleDragStart(e, entity.id)}
          onDragEnd={handleDragEnd}
          onDragOver={(e) => handleDragOver(e, entity.id)}
          onDragLeave={handleDragLeave}
          onDrop={(e) => handleDrop(e, entity.id)}
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
  }, [selectedEntityId, expandedEntities, loadingChildren, getDepthClass, handleEntityClick, handleExpandToggle, draggingEntityId, dropTarget, handleDragStart, handleDragEnd, handleDragOver, handleDragLeave, handleDrop]);

  return (
    <Surface className={styles.container}>
      <div className={styles.header}>
        <IconButton onClick={handleRefresh} disabled={loading} size="small">
          <Icon name="refresh" size={18} />
        </IconButton>
      </div>

      <div
        className={styles.content}
        onDragOver={handleDragOverRoot}
        onDrop={handleDropOnRoot}
      >
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
