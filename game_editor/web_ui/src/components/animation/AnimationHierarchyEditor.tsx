'use client';

import React, { useState, useCallback } from 'react';
import {
  Box,
  Typography,
  IconButton,
  Tooltip,
  Collapse,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
} from '@mui/material';
import {
  Add,
  Delete,
  ExpandMore,
  ChevronRight,
  Folder as EntityIcon,
  Extension as ComponentIcon,
  Timeline as PropertyIcon,
} from '@mui/icons-material';
import {
  AnimationClipResponse,
  AnimatedComponentData,
  AnimatedEntityChild,
  AnimatedProperty,
  AnimatedComponentPlaceholder,
  PolymorphicTypeRegistry,
  gameEngineAPI,
} from '../../api/gameEngine';

// Tree node types
type NodeType = 'entity' | 'component' | 'property';

// Selected node info
export interface SelectedNode {
  type: NodeType;
  entityPath: string;
  componentIndex?: number;
  propertyKey?: string;
}

interface AnimationHierarchyEditorProps {
  clipData: AnimationClipResponse;
  typeRegistry: PolymorphicTypeRegistry;
  selectedNode: SelectedNode | null;
  onSelectNode: (node: SelectedNode | null) => void;
  onAddEntity?: () => void;
  onRemoveEntity?: (entityPath: string) => void;
  onAddComponent?: (entityPath: string) => void;
  onRemoveComponent?: (entityPath: string, componentIndex: number) => void;
  onAddProperty?: (entityPath: string, componentIndex: number) => void;
  onRemoveProperty?: (entityPath: string, componentIndex: number, propertyKey: string) => void;
}

export const AnimationHierarchyEditor: React.FC<AnimationHierarchyEditorProps> = ({
  clipData,
  typeRegistry,
  selectedNode,
  onSelectNode,
  onAddEntity,
  onRemoveEntity,
  onAddComponent,
  onRemoveComponent,
  onAddProperty,
  onRemoveProperty,
}) => {
  const [expandedNodes, setExpandedNodes] = useState<Set<string>>(new Set(['root']));

  const toggleExpand = useCallback((nodeId: string) => {
    setExpandedNodes(prev => {
      const next = new Set(prev);
      if (next.has(nodeId)) {
        next.delete(nodeId);
      } else {
        next.add(nodeId);
      }
      return next;
    });
  }, []);

  const isExpanded = (nodeId: string) => expandedNodes.has(nodeId);

  const isSelected = (node: SelectedNode) => {
    if (!selectedNode) return false;
    if (node.type !== selectedNode.type) return false;
    if (node.entityPath !== selectedNode.entityPath) return false;
    if (node.componentIndex !== selectedNode.componentIndex) return false;
    if (node.propertyKey !== selectedNode.propertyKey) return false;
    return true;
  };

  // Render a property node
  const renderProperty = (
    entityPath: string,
    componentIndex: number,
    prop: AnimatedProperty,
    depth: number
  ) => {
    const node: SelectedNode = {
      type: 'property',
      entityPath,
      componentIndex,
      propertyKey: prop.key,
    };
    const keyCount = prop.value.curve.keyframes.length;

    return (
      <ListItem
        key={`${entityPath}:${componentIndex}:${prop.key}`}
        disablePadding
        secondaryAction={
          onRemoveProperty && (
            <Tooltip title="Remove property">
              <IconButton
                size="small"
                onClick={(e) => {
                  e.stopPropagation();
                  onRemoveProperty(entityPath, componentIndex, prop.key);
                }}
              >
                <Delete fontSize="small" />
              </IconButton>
            </Tooltip>
          )
        }
      >
        <ListItemButton
          sx={{ pl: depth * 2 }}
          selected={isSelected(node)}
          onClick={() => onSelectNode(node)}
        >
          <ListItemIcon sx={{ minWidth: 32 }}>
            <PropertyIcon fontSize="small" color="action" />
          </ListItemIcon>
          <ListItemText
            primary={prop.key}
            secondary={`${keyCount} keys`}
            primaryTypographyProps={{ fontSize: '0.875rem' }}
            secondaryTypographyProps={{ fontSize: '0.75rem' }}
          />
        </ListItemButton>
      </ListItem>
    );
  };

  // Render a component node
  const renderComponent = (
    entityPath: string,
    componentIndex: number,
    component: AnimatedComponentData,
    depth: number
  ) => {
    const nodeId = `${entityPath}:${componentIndex}`;
    const node: SelectedNode = {
      type: 'component',
      entityPath,
      componentIndex,
    };
    const typeName = gameEngineAPI.getComponentTypeName(component.placeholder, typeRegistry);
    const hasProperties = component.properties.length > 0;
    const expanded = isExpanded(nodeId);

    return (
      <React.Fragment key={nodeId}>
        <ListItem
          disablePadding
          secondaryAction={
            <Box sx={{ display: 'flex', gap: 0.5 }}>
              {onAddProperty && (
                <Tooltip title="Add property">
                  <IconButton
                    size="small"
                    onClick={(e) => {
                      e.stopPropagation();
                      onAddProperty(entityPath, componentIndex);
                    }}
                  >
                    <Add fontSize="small" />
                  </IconButton>
                </Tooltip>
              )}
              {onRemoveComponent && (
                <Tooltip title="Remove component">
                  <IconButton
                    size="small"
                    onClick={(e) => {
                      e.stopPropagation();
                      onRemoveComponent(entityPath, componentIndex);
                    }}
                  >
                    <Delete fontSize="small" />
                  </IconButton>
                </Tooltip>
              )}
            </Box>
          }
        >
          <ListItemButton
            sx={{ pl: depth * 2 }}
            selected={isSelected(node)}
            onClick={() => onSelectNode(node)}
          >
            <ListItemIcon sx={{ minWidth: 24 }}>
              {hasProperties ? (
                <IconButton
                  size="small"
                  onClick={(e) => {
                    e.stopPropagation();
                    toggleExpand(nodeId);
                  }}
                >
                  {expanded ? <ExpandMore fontSize="small" /> : <ChevronRight fontSize="small" />}
                </IconButton>
              ) : (
                <Box sx={{ width: 24 }} />
              )}
            </ListItemIcon>
            <ListItemIcon sx={{ minWidth: 32 }}>
              <ComponentIcon fontSize="small" color="primary" />
            </ListItemIcon>
            <ListItemText
              primary={typeName}
              secondary={`${component.properties.length} properties`}
              primaryTypographyProps={{ fontSize: '0.875rem' }}
              secondaryTypographyProps={{ fontSize: '0.75rem' }}
            />
          </ListItemButton>
        </ListItem>
        {hasProperties && (
          <Collapse in={expanded}>
            <List disablePadding>
              {component.properties.map((prop) =>
                renderProperty(entityPath, componentIndex, prop, depth + 1)
              )}
            </List>
          </Collapse>
        )}
      </React.Fragment>
    );
  };

  // Render an entity node (recursive for children)
  const renderEntity = (
    entityName: string,
    entity: { components: AnimatedComponentData[]; children: AnimatedEntityChild[] },
    parentPath: string,
    depth: number
  ) => {
    const entityPath = parentPath ? `${parentPath}/${entityName}` : entityName;
    const nodeId = `entity:${entityPath}`;
    const node: SelectedNode = {
      type: 'entity',
      entityPath,
    };
    const hasContent = entity.components.length > 0 || entity.children.length > 0;
    const expanded = isExpanded(nodeId);

    return (
      <React.Fragment key={entityPath}>
        <ListItem
          disablePadding
          secondaryAction={
            <Box sx={{ display: 'flex', gap: 0.5 }}>
              {onAddComponent && (
                <Tooltip title="Add component">
                  <IconButton
                    size="small"
                    onClick={(e) => {
                      e.stopPropagation();
                      onAddComponent(entityPath);
                    }}
                  >
                    <Add fontSize="small" />
                  </IconButton>
                </Tooltip>
              )}
              {onRemoveEntity && (
                <Tooltip title="Remove entity">
                  <IconButton
                    size="small"
                    onClick={(e) => {
                      e.stopPropagation();
                      onRemoveEntity(entityPath);
                    }}
                  >
                    <Delete fontSize="small" />
                  </IconButton>
                </Tooltip>
              )}
            </Box>
          }
        >
          <ListItemButton
            sx={{ pl: depth * 2 }}
            selected={isSelected(node)}
            onClick={() => onSelectNode(node)}
          >
            <ListItemIcon sx={{ minWidth: 24 }}>
              {hasContent ? (
                <IconButton
                  size="small"
                  onClick={(e) => {
                    e.stopPropagation();
                    toggleExpand(nodeId);
                  }}
                >
                  {expanded ? <ExpandMore fontSize="small" /> : <ChevronRight fontSize="small" />}
                </IconButton>
              ) : (
                <Box sx={{ width: 24 }} />
              )}
            </ListItemIcon>
            <ListItemIcon sx={{ minWidth: 32 }}>
              <EntityIcon fontSize="small" color="secondary" />
            </ListItemIcon>
            <ListItemText
              primary={entityName}
              secondary={`${entity.components.length} components`}
              primaryTypographyProps={{ fontSize: '0.875rem', fontWeight: 500 }}
              secondaryTypographyProps={{ fontSize: '0.75rem' }}
            />
          </ListItemButton>
        </ListItem>
        {hasContent && (
          <Collapse in={expanded}>
            <List disablePadding>
              {/* Render components */}
              {entity.components.map((comp, idx) =>
                renderComponent(entityPath, idx, comp, depth + 1)
              )}
              {/* Render child entities */}
              {entity.children.map((child) =>
                renderEntity(child.key, child.value, entityPath, depth + 1)
              )}
            </List>
          </Collapse>
        )}
      </React.Fragment>
    );
  };

  // Render root entity (special case - no name)
  const renderRootEntity = () => {
    const root = clipData.clip.root_entity;
    const hasContent = root.components.length > 0 || root.children.length > 0;
    const expanded = isExpanded('root');

    return (
      <>
        <ListItem disablePadding>
          <ListItemButton onClick={() => toggleExpand('root')}>
            <ListItemIcon sx={{ minWidth: 24 }}>
              {hasContent ? (
                expanded ? <ExpandMore fontSize="small" /> : <ChevronRight fontSize="small" />
              ) : (
                <Box sx={{ width: 24 }} />
              )}
            </ListItemIcon>
            <ListItemIcon sx={{ minWidth: 32 }}>
              <EntityIcon fontSize="small" color="secondary" />
            </ListItemIcon>
            <ListItemText
              primary="Root Entity"
              secondary={`${root.components.length} components, ${root.children.length} children`}
              primaryTypographyProps={{ fontSize: '0.875rem', fontWeight: 600 }}
              secondaryTypographyProps={{ fontSize: '0.75rem' }}
            />
          </ListItemButton>
        </ListItem>
        {hasContent && (
          <Collapse in={expanded}>
            <List disablePadding>
              {/* Root components */}
              {root.components.map((comp, idx) =>
                renderComponent('', idx, comp, 1)
              )}
              {/* Child entities */}
              {root.children.map((child) =>
                renderEntity(child.key, child.value, '', 1)
              )}
            </List>
          </Collapse>
        )}
      </>
    );
  };

  return (
    <Box sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
      {/* Header */}
      <Box
        sx={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          p: 1,
          borderBottom: 1,
          borderColor: 'divider',
        }}
      >
        <Typography variant="subtitle2">Animation Hierarchy</Typography>
        {onAddEntity && (
          <Tooltip title="Add Entity">
            <IconButton size="small" onClick={onAddEntity}>
              <Add fontSize="small" />
            </IconButton>
          </Tooltip>
        )}
      </Box>

      {/* Tree View */}
      <Box sx={{ flex: 1, overflow: 'auto' }}>
        <List dense disablePadding>
          {renderRootEntity()}
        </List>
      </Box>

      {/* Selected Node Info */}
      {selectedNode && (
        <Box
          sx={{
            p: 1,
            borderTop: 1,
            borderColor: 'divider',
            backgroundColor: 'action.hover',
          }}
        >
          <Typography variant="caption" color="text.secondary">
            Selected: {selectedNode.type} - {selectedNode.entityPath || 'root'}
            {selectedNode.componentIndex !== undefined && ` [${selectedNode.componentIndex}]`}
            {selectedNode.propertyKey && ` > ${selectedNode.propertyKey}`}
          </Typography>
        </Box>
      )}
    </Box>
  );
};
