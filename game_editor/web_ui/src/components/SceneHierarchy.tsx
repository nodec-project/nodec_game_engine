'use client';

import React, { useState, useEffect } from 'react';
import { Surface, Alert, Spinner, IconButton, Collapse, List, Typography } from '@/ui';
import { gameEngineAPI, EntityInfo } from '../api/gameEngine';
import { useEditor } from '../contexts/EditorContext';
import styles from './SceneHierarchy.module.css';

// Icons
const FolderIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M10 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z" />
  </svg>
);

const FolderOpenIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M20 6h-8l-2-2H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2zm0 12H4V8h16v10z" />
  </svg>
);

const EntityIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M14 2H6c-1.1 0-1.99.9-1.99 2L4 20c0 1.1.89 2 1.99 2H18c1.1 0 2-.9 2-2V8l-6-6zm2 16H8v-2h8v2zm0-4H8v-2h8v2zm-3-5V3.5L18.5 9H13z" />
  </svg>
);

const RefreshIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M17.65 6.35C16.2 4.9 14.21 4 12 4c-4.42 0-7.99 3.58-7.99 8s3.57 8 7.99 8c3.73 0 6.84-2.55 7.73-6h-2.08c-.82 2.33-3.04 4-5.65 4-3.31 0-6-2.69-6-6s2.69-6 6-6c1.66 0 3.14.69 4.22 1.78L13 11h7V4l-2.35 2.35z" />
  </svg>
);

const ExpandMoreIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" />
  </svg>
);

const ChevronRightIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M10 6L8.59 7.41 13.17 12l-4.58 4.59L10 18l6-6z" />
  </svg>
);

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
                  <ExpandMoreIcon />
                ) : (
                  <ChevronRightIcon />
                )}
              </IconButton>
            ) : (
              <EntityIcon />
            )}
          </div>
          <span className={nameClasses}>{entity.name}</span>
          {entity.has_children && (
            <div className={styles.folderIcon}>
              {isExpanded ? <FolderOpenIcon /> : <FolderIcon />}
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
          <RefreshIcon />
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
