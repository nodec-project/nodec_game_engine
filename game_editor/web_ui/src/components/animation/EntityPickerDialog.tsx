'use client';

import React, { useState, useEffect, useCallback } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Typography,
  Spinner,
  Alert,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  Collapse,
  IconButton,
  Icon,
} from '@/ui';
import { gameEngineAPI, EntityInfo, entityHasChildren } from '../../api/gameEngine';
import styles from './EntityPickerDialog.module.css';

interface EntityTreeNode {
  id: string;
  name: string;
  hasChildren: boolean;
  children?: EntityTreeNode[];
  loaded: boolean;
}

interface EntityPickerDialogProps {
  open: boolean;
  onClose: () => void;
  onSelect: (entityId: string, entityName: string) => void;
  excludeEntityIds?: string[];
  /** If provided, start hierarchy from this entity's children instead of scene roots */
  rootEntityId?: string;
}

export const EntityPickerDialog: React.FC<EntityPickerDialogProps> = ({
  open,
  onClose,
  onSelect,
  excludeEntityIds = [],
  rootEntityId,
}) => {
  const [rootNodes, setRootNodes] = useState<EntityTreeNode[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [expandedNodes, setExpandedNodes] = useState<Set<string>>(new Set());
  const [selectedEntityId, setSelectedEntityId] = useState<string | null>(null);
  const [selectedEntityName, setSelectedEntityName] = useState<string>('');

  // Fetch initial entities when dialog opens
  useEffect(() => {
    if (!open) {
      setSelectedEntityId(null);
      setSelectedEntityName('');
      return;
    }

    const fetchInitialEntities = async () => {
      try {
        setLoading(true);
        setError(null);

        let nodes: EntityTreeNode[] = [];

        if (rootEntityId) {
          // Start from specified entity's children
          const rootDetails = await gameEngineAPI.getEntityDetails(rootEntityId);
          const childIds = rootDetails.hierarchy.children;

          if (childIds.length > 0) {
            nodes = await Promise.all(
              childIds.map(async (childId: number) => {
                const childDetails = await gameEngineAPI.getEntityDetails(String(childId));
                return {
                  id: childDetails.id,
                  name: childDetails.name,
                  hasChildren: entityHasChildren(childDetails),
                  loaded: false,
                };
              })
            );
          }
        } else {
          // Start from scene roots
          const entities = await gameEngineAPI.getRootEntities();
          nodes = entities.map((e: EntityInfo) => ({
            id: e.id,
            name: e.name,
            hasChildren: entityHasChildren(e),
            loaded: false,
          }));
        }

        setRootNodes(nodes);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch entities');
      } finally {
        setLoading(false);
      }
    };

    fetchInitialEntities();
  }, [open, rootEntityId]);

  // Load children for an entity
  const loadChildren = useCallback(async (node: EntityTreeNode): Promise<EntityTreeNode[]> => {
    try {
      const details = await gameEngineAPI.getEntityDetails(node.id);

      if (details.hierarchy.children.length === 0) {
        return [];
      }

      // Fetch details for each child
      const childNodes: EntityTreeNode[] = await Promise.all(
        details.hierarchy.children.map(async (childId: number) => {
          const childDetails = await gameEngineAPI.getEntityDetails(String(childId));
          return {
            id: childDetails.id,
            name: childDetails.name,
            hasChildren: entityHasChildren(childDetails),
            loaded: false,
          };
        })
      );

      return childNodes;
    } catch (err) {
      console.error('Failed to load children:', err);
      return [];
    }
  }, []);

  // Update a node in the tree by path
  const updateNodeInTree = useCallback((
    nodes: EntityTreeNode[],
    path: string[],
    updatedNode: EntityTreeNode
  ): EntityTreeNode[] => {
    if (path.length === 0) return nodes;

    const [currentId, ...restPath] = path;

    return nodes.map(node => {
      if (node.id !== currentId) return node;

      if (restPath.length === 0) {
        return updatedNode;
      }

      return {
        ...node,
        children: node.children ? updateNodeInTree(node.children, restPath, updatedNode) : node.children,
      };
    });
  }, []);

  // Toggle expand/collapse for a node
  const handleToggleExpand = useCallback(async (node: EntityTreeNode, nodePath: string[]) => {
    const nodeId = node.id;

    if (expandedNodes.has(nodeId)) {
      // Collapse
      setExpandedNodes(prev => {
        const next = new Set(prev);
        next.delete(nodeId);
        return next;
      });
    } else {
      // Expand - load children if not loaded
      if (!node.loaded && node.hasChildren) {
        const children = await loadChildren(node);

        // Update the tree with loaded children
        setRootNodes(prev => updateNodeInTree(prev, nodePath, { ...node, children, loaded: true }));
      }

      setExpandedNodes(prev => {
        const next = new Set(prev);
        next.add(nodeId);
        return next;
      });
    }
  }, [expandedNodes, loadChildren, updateNodeInTree]);

  // Handle entity selection
  const handleSelectEntity = (entityId: string, entityName: string) => {
    setSelectedEntityId(entityId);
    setSelectedEntityName(entityName);
  };

  // Handle confirm
  const handleConfirm = () => {
    if (selectedEntityId && selectedEntityName) {
      onSelect(selectedEntityId, selectedEntityName);
      onClose();
    }
  };

  // Get indent style for depth
  const getIndentStyle = (depth: number): React.CSSProperties => ({
    paddingLeft: `${depth * 16}px`,
  });

  // Render an entity node
  const renderNode = (node: EntityTreeNode, path: string[], depth: number) => {
    const isExcluded = excludeEntityIds.includes(node.id);
    const isExpanded = expandedNodes.has(node.id);
    const isSelected = selectedEntityId === node.id;

    return (
      <React.Fragment key={node.id}>
        <ListItem>
          <ListItemButton
            style={getIndentStyle(depth)}
            selected={isSelected}
            disabled={isExcluded}
            onClick={() => !isExcluded && handleSelectEntity(node.id, node.name)}
          >
            <ListItemIcon className={styles.expandIcon}>
              {node.hasChildren ? (
                <IconButton
                  size="small"
                  onClick={(e) => {
                    e.stopPropagation();
                    handleToggleExpand(node, path);
                  }}
                >
                  {isExpanded ? <Icon name="expand_more" /> : <Icon name="chevron_right" />}
                </IconButton>
              ) : (
                <span className={styles.spacer} />
              )}
            </ListItemIcon>
            <ListItemIcon className={`${styles.entityIcon} ${isExcluded ? styles.iconDisabled : styles.iconPrimary}`}>
              {isExpanded ? <Icon name="folder_open" /> : <Icon name="folder" />}
            </ListItemIcon>
            <ListItemText
              primary={node.name}
              secondary={isExcluded ? 'Already in animation' : undefined}
            />
          </ListItemButton>
        </ListItem>

        {node.hasChildren && node.children && (
          <Collapse in={isExpanded}>
            <List dense>
              {node.children.map(child =>
                renderNode(child, [...path, child.id], depth + 1)
              )}
            </List>
          </Collapse>
        )}
      </React.Fragment>
    );
  };

  return (
    <Dialog open={open} onClose={onClose}>
      <DialogTitle>Select Entity to Animate</DialogTitle>
      <DialogContent dividers>
        {loading && (
          <div className={styles.loadingContainer}>
            <Spinner />
          </div>
        )}

        {error && (
          <div className={styles.alertContainer}>
            <Alert severity="error">{error}</Alert>
          </div>
        )}

        {!loading && !error && rootNodes.length === 0 && (
          <Typography color="onSurfaceVariant" className={styles.emptyMessage}>
            {rootEntityId ? 'No child entities found' : 'No entities found in scene'}
          </Typography>
        )}

        {!loading && !error && rootNodes.length > 0 && (
          <List dense className={styles.listContainer}>
            {rootNodes.map(node => renderNode(node, [node.id], 0))}
          </List>
        )}

        {selectedEntityId && (
          <div className={styles.selectedContainer}>
            <Typography variant="bodySmall">
              Selected: <strong>{selectedEntityName}</strong>
            </Typography>
          </div>
        )}
      </DialogContent>
      <DialogActions>
        <Button variant="text" onClick={onClose}>Cancel</Button>
        <Button
          variant="filled"
          onClick={handleConfirm}
          disabled={!selectedEntityId}
        >
          Add Entity
        </Button>
      </DialogActions>
    </Dialog>
  );
};
