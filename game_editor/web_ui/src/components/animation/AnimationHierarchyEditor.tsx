'use client';

import React, { useState, useCallback } from 'react';
import {
  Typography,
  IconButton,
  Tooltip,
  Collapse,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
} from '@/ui';
import {
  AddIcon,
  DeleteIcon,
  ExpandMoreIcon,
  ChevronRightIcon,
  FolderIcon,
  ExtensionIcon,
  TimelineIcon,
} from '@/ui/icons';
import {
  AnimationClipResponse,
  AnimatedComponentData,
  AnimatedEntityChild,
  AnimatedProperty,
  PolymorphicTypeRegistry,
  gameEngineAPI,
} from '../../api/gameEngine';
import styles from './AnimationHierarchyEditor.module.css';

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

  // Get indent style for depth
  const getIndentStyle = (depth: number): React.CSSProperties => ({
    paddingLeft: `${depth * 16}px`,
  });

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
                <DeleteIcon />
              </IconButton>
            </Tooltip>
          )
        }
      >
        <ListItemButton
          style={getIndentStyle(depth)}
          selected={isSelected(node)}
          onClick={() => onSelectNode(node)}
        >
          <ListItemIcon className={styles.expandIcon}>
            <span className={styles.spacer} />
          </ListItemIcon>
          <ListItemIcon>
            <TimelineIcon />
          </ListItemIcon>
          <ListItemText
            primary={prop.key}
            secondary={`${keyCount} keys`}
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
          secondaryAction={
            <div className={styles.actionButtons}>
              {onAddProperty && (
                <Tooltip title="Add property">
                  <IconButton
                    size="small"
                    onClick={(e) => {
                      e.stopPropagation();
                      onAddProperty(entityPath, componentIndex);
                    }}
                  >
                    <AddIcon />
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
                    <DeleteIcon />
                  </IconButton>
                </Tooltip>
              )}
            </div>
          }
        >
          <ListItemButton
            style={getIndentStyle(depth)}
            selected={isSelected(node)}
            onClick={() => onSelectNode(node)}
          >
            <ListItemIcon className={styles.expandIcon}>
              {hasProperties ? (
                <IconButton
                  size="small"
                  onClick={(e) => {
                    e.stopPropagation();
                    toggleExpand(nodeId);
                  }}
                >
                  {expanded ? <ExpandMoreIcon /> : <ChevronRightIcon />}
                </IconButton>
              ) : (
                <span className={styles.spacer} />
              )}
            </ListItemIcon>
            <ListItemIcon>
              <ExtensionIcon />
            </ListItemIcon>
            <ListItemText
              primary={typeName}
              secondary={`${component.properties.length} properties`}
            />
          </ListItemButton>
        </ListItem>
        {hasProperties && (
          <Collapse in={expanded}>
            <List dense>
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
          secondaryAction={
            <div className={styles.actionButtons}>
              {onAddComponent && (
                <Tooltip title="Add component">
                  <IconButton
                    size="small"
                    onClick={(e) => {
                      e.stopPropagation();
                      onAddComponent(entityPath);
                    }}
                  >
                    <AddIcon />
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
                    <DeleteIcon />
                  </IconButton>
                </Tooltip>
              )}
            </div>
          }
        >
          <ListItemButton
            style={getIndentStyle(depth)}
            selected={isSelected(node)}
            onClick={() => onSelectNode(node)}
          >
            <ListItemIcon className={styles.expandIcon}>
              {hasContent ? (
                <IconButton
                  size="small"
                  onClick={(e) => {
                    e.stopPropagation();
                    toggleExpand(nodeId);
                  }}
                >
                  {expanded ? <ExpandMoreIcon /> : <ChevronRightIcon />}
                </IconButton>
              ) : (
                <span className={styles.spacer} />
              )}
            </ListItemIcon>
            <ListItemIcon>
              <FolderIcon />
            </ListItemIcon>
            <ListItemText
              primary={entityName}
              secondary={`${entity.components.length} components`}
            />
          </ListItemButton>
        </ListItem>
        {hasContent && (
          <Collapse in={expanded}>
            <List dense>
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
        <ListItem>
          <ListItemButton onClick={() => toggleExpand('root')}>
            <ListItemIcon className={styles.expandIcon}>
              {hasContent ? (
                expanded ? <ExpandMoreIcon /> : <ChevronRightIcon />
              ) : (
                <span className={styles.spacer} />
              )}
            </ListItemIcon>
            <ListItemIcon>
              <FolderIcon />
            </ListItemIcon>
            <ListItemText
              primary="Root Entity"
              secondary={`${root.components.length} components, ${root.children.length} children`}
            />
          </ListItemButton>
        </ListItem>
        {hasContent && (
          <Collapse in={expanded}>
            <List dense>
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
    <div className={styles.container}>
      {/* Header */}
      <div className={styles.header}>
        <Typography variant="titleSmall">Animation Hierarchy</Typography>
        {onAddEntity && (
          <Tooltip title="Add Entity">
            <IconButton size="small" onClick={onAddEntity}>
              <AddIcon />
            </IconButton>
          </Tooltip>
        )}
      </div>

      {/* Tree View */}
      <div className={styles.treeContainer}>
        <List dense>
          {renderRootEntity()}
        </List>
      </div>

      {/* Selected Node Info */}
      {selectedNode && (
        <div className={styles.selectedInfo}>
          <Typography variant="labelSmall" color="onSurfaceVariant">
            Selected: {selectedNode.type} - {selectedNode.entityPath || 'root'}
            {selectedNode.componentIndex !== undefined && ` [${selectedNode.componentIndex}]`}
            {selectedNode.propertyKey && ` > ${selectedNode.propertyKey}`}
          </Typography>
        </div>
      )}
    </div>
  );
};
