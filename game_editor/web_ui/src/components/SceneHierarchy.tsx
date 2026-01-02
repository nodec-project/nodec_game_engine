'use client';

import React, { useState, useEffect } from 'react';
import { Surface, Alert, Spinner, IconButton, Collapse, List, Typography, Icon } from '@/ui';
import { gameEngineAPI, EntityInfo } from '../api/gameEngine';
import { useEditor } from '../contexts/EditorContext';
import styles from './SceneHierarchy.module.css';

interface SceneHierarchyProps {
  onEntitySelect?: (entityId: string) => void;
}

interface EntityNode extends EntityInfo {
  children?: EntityNode[];
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

  const fetchRootEntities = async () => {
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

  const getDepthClass = (depth: number): string => {
    if (depth >= 5) return styles.depth5;
    return styles[`depth${depth}` as keyof typeof styles] || '';
  };

  const renderEntity = (entity: EntityNode, depth: number = 0): React.ReactNode => {
    const isSelected = selectedEntityId === entity.id;
    const isExpanded = expandedEntities.has(entity.id);
    const isLoadingChildren = loadingChildren.has(entity.id);

    const itemClasses = [
      styles.entityItem,
      isSelected && styles.entityItemSelected,
      getDepthClass(depth),
    ].filter(Boolean).join(' ');

    const nameClasses = [
      styles.entityName,
      isSelected && styles.entityNameSelected,
    ].filter(Boolean).join(' ');

    return (
      <React.Fragment key={entity.id}>
        <div
          className={itemClasses}
          onClick={() => handleEntityClick(entity.id)}
        >
          <div className={styles.entityIcon}>
            {entity.has_children ? (
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
          {entity.has_children && (
            <div className={styles.folderIcon}>
              {isExpanded ? <Icon name="folder_open" size={18} /> : <Icon name="folder" size={18} />}
            </div>
          )}
        </div>

        {/* Render children if expanded */}
        {isExpanded && entity.children && (
          <Collapse in={isExpanded} unmountOnExit>
            {entity.children.map((child) => renderEntity(child, depth + 1))}
          </Collapse>
        )}
      </React.Fragment>
    );
  };

  return (
    <Surface className={styles.container}>
      <div className={styles.header}>
        <IconButton onClick={fetchRootEntities} disabled={loading} size="small">
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
