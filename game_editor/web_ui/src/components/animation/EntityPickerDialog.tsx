'use client';

import React, { useState, useEffect, useCallback } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Typography,
  CircularProgress,
  Alert,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  Collapse,
  IconButton,
} from '@mui/material';
import {
  ExpandMore,
  ChevronRight,
  Folder as EntityIcon,
  FolderOpen as EntityOpenIcon,
} from '@mui/icons-material';
import { gameEngineAPI, EntityInfo, EntityDetailsResponse } from '../../api/gameEngine';

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
}

export const EntityPickerDialog: React.FC<EntityPickerDialogProps> = ({
  open,
  onClose,
  onSelect,
  excludeEntityIds = [],
}) => {
  const [rootNodes, setRootNodes] = useState<EntityTreeNode[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [expandedNodes, setExpandedNodes] = useState<Set<string>>(new Set());
  const [selectedEntityId, setSelectedEntityId] = useState<string | null>(null);
  const [selectedEntityName, setSelectedEntityName] = useState<string>('');

  // Fetch root entities when dialog opens
  useEffect(() => {
    if (!open) {
      setSelectedEntityId(null);
      setSelectedEntityName('');
      return;
    }

    const fetchRootEntities = async () => {
      try {
        setLoading(true);
        setError(null);
        const entities = await gameEngineAPI.getRootEntities();

        const nodes: EntityTreeNode[] = entities.map((e: EntityInfo) => ({
          id: e.id,
          name: e.name,
          hasChildren: e.has_children,
          loaded: false,
        }));

        setRootNodes(nodes);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch entities');
      } finally {
        setLoading(false);
      }
    };

    fetchRootEntities();
  }, [open]);

  // Load children for an entity
  const loadChildren = useCallback(async (node: EntityTreeNode): Promise<EntityTreeNode[]> => {
    try {
      const details = await gameEngineAPI.getEntityDetails(node.id);

      if (!details.hierarchy?.children || details.hierarchy.children.length === 0) {
        return [];
      }

      // Fetch details for each child
      const childNodes: EntityTreeNode[] = await Promise.all(
        details.hierarchy.children.map(async (childId: number) => {
          const childDetails = await gameEngineAPI.getEntityDetails(String(childId));
          return {
            id: childDetails.id,
            name: childDetails.name,
            hasChildren: (childDetails.hierarchy?.children?.length ?? 0) > 0,
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

  // Render an entity node
  const renderNode = (node: EntityTreeNode, path: string[], depth: number) => {
    const isExcluded = excludeEntityIds.includes(node.id);
    const isExpanded = expandedNodes.has(node.id);
    const isSelected = selectedEntityId === node.id;

    return (
      <React.Fragment key={node.id}>
        <ListItem disablePadding>
          <ListItemButton
            sx={{ pl: depth * 2 }}
            selected={isSelected}
            disabled={isExcluded}
            onClick={() => !isExcluded && handleSelectEntity(node.id, node.name)}
          >
            <ListItemIcon sx={{ minWidth: 24 }}>
              {node.hasChildren ? (
                <IconButton
                  size="small"
                  onClick={(e) => {
                    e.stopPropagation();
                    handleToggleExpand(node, path);
                  }}
                >
                  {isExpanded ? <ExpandMore fontSize="small" /> : <ChevronRight fontSize="small" />}
                </IconButton>
              ) : (
                <Box sx={{ width: 24 }} />
              )}
            </ListItemIcon>
            <ListItemIcon sx={{ minWidth: 32 }}>
              {isExpanded ? (
                <EntityOpenIcon fontSize="small" color={isExcluded ? 'disabled' : 'primary'} />
              ) : (
                <EntityIcon fontSize="small" color={isExcluded ? 'disabled' : 'primary'} />
              )}
            </ListItemIcon>
            <ListItemText
              primary={node.name}
              secondary={isExcluded ? 'Already in animation' : undefined}
              primaryTypographyProps={{
                fontSize: '0.875rem',
                color: isExcluded ? 'text.disabled' : 'text.primary',
              }}
              secondaryTypographyProps={{ fontSize: '0.75rem' }}
            />
          </ListItemButton>
        </ListItem>

        {node.hasChildren && node.children && (
          <Collapse in={isExpanded}>
            <List disablePadding>
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
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle>Select Entity to Animate</DialogTitle>
      <DialogContent dividers>
        {loading && (
          <Box sx={{ display: 'flex', justifyContent: 'center', p: 4 }}>
            <CircularProgress />
          </Box>
        )}

        {error && (
          <Alert severity="error" sx={{ mb: 2 }}>
            {error}
          </Alert>
        )}

        {!loading && !error && rootNodes.length === 0 && (
          <Typography color="text.secondary" align="center" sx={{ py: 4 }}>
            No entities found in scene
          </Typography>
        )}

        {!loading && !error && rootNodes.length > 0 && (
          <List dense sx={{ maxHeight: 400, overflow: 'auto' }}>
            {rootNodes.map(node => renderNode(node, [node.id], 0))}
          </List>
        )}

        {selectedEntityId && (
          <Box sx={{ mt: 2, p: 1, backgroundColor: 'action.selected', borderRadius: 1 }}>
            <Typography variant="body2">
              Selected: <strong>{selectedEntityName}</strong>
            </Typography>
          </Box>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>Cancel</Button>
        <Button
          variant="contained"
          onClick={handleConfirm}
          disabled={!selectedEntityId}
        >
          Add Entity
        </Button>
      </DialogActions>
    </Dialog>
  );
};
