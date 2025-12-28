'use client';

import React, { useState, useEffect, useCallback } from 'react';
import { IDockviewPanelProps } from 'dockview';
import {
  Box,
  Typography,
  Alert,
  CircularProgress,
  Paper,
  Chip,
  Divider,
  Tabs,
  Tab,
  List,
  ListItem,
  ListItemText,
  ListItemIcon,
  Checkbox,
} from '@mui/material';
import {
  Animation as AnimationIcon,
  MovieFilter as ClipIcon,
  Warning as WarningIcon,
  Timeline as TimelineIcon,
  AccountTree as HierarchyIcon,
} from '@mui/icons-material';
import { useEditor } from '../../contexts/EditorContext';
import {
  gameEngineAPI,
  AnimationCurve,
  AnimationClipResponse,
  AnimatedEntityChild,
  Keyframe,
  PolymorphicTypeRegistry,
} from '../../api/gameEngine';
import { CurveViewer, CurveData } from '../animation/CurveViewer';
import { AnimationHierarchyEditor, SelectedNode } from '../animation/AnimationHierarchyEditor';
import { EntityPickerDialog } from '../animation/EntityPickerDialog';

// Animation editing state - stores full clip for mutation
interface AnimationState {
  entityName: string;
  clipName: string;
  clipData: AnimationClipResponse;  // Full clip data for mutations
  typeRegistry: PolymorphicTypeRegistry;  // Registry for polymorphic type names
  curves: AnimationCurve[];         // Flattened curves for display
  duration: number;
}

// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export interface AnimationEditorPanelProps {}

export const AnimationEditorPanel: React.FC<IDockviewPanelProps<AnimationEditorPanelProps>> = () => {
  const { selectedEntityId } = useEditor();
  const [animState, setAnimState] = useState<AnimationState | null>(null);
  const [hasAnimator, setHasAnimator] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [tabValue, setTabValue] = useState(0);
  const [selectedCurves, setSelectedCurves] = useState<Set<string>>(new Set());
  const [currentTime, setCurrentTime] = useState(0);
  const [selectedHierarchyNode, setSelectedHierarchyNode] = useState<SelectedNode | null>(null);
  const [entityPickerOpen, setEntityPickerOpen] = useState(false);

  useEffect(() => {
    if (!selectedEntityId) {
      setAnimState(null);
      setHasAnimator(false);
      setError(null);
      return;
    }

    const fetchAnimationData = async () => {
      try {
        setLoading(true);
        setError(null);

        // Step 1: Get entity components
        const components = await gameEngineAPI.getEntityComponents(selectedEntityId);

        // Step 2: Find animator clip name from components
        const clipName = gameEngineAPI.findAnimatorClipName(components);

        if (clipName === null) {
          // No animator component or no clip assigned
          setHasAnimator(false);
          setAnimState(null);
          setError('Entity does not have Animator component or no clip assigned');
          return;
        }

        setHasAnimator(true);

        if (clipName === '') {
          // Has animator but no clip assigned
          setAnimState(null);
          return;
        }

        // Step 3: Fetch the animation clip resource
        const clipResponse = await gameEngineAPI.getAnimationClip(clipName);
        console.log(clipResponse);

        // Step 4: Build polymorphic type registry for component name lookup
        const typeRegistry = gameEngineAPI.buildPolymorphicTypeRegistry(clipResponse);

        // Step 5: Flatten curves for UI display
        const curves = gameEngineAPI.flattenAnimationClip(clipResponse);
        const duration = gameEngineAPI.getClipDuration(curves);

        setAnimState({
          entityName: `Entity_${selectedEntityId}`,
          clipName,
          clipData: clipResponse,
          typeRegistry,
          curves,
          duration,
        });

        // Auto-select all curves initially
        const curveKeys = curves.map(c => getCurveKey(c));
        setSelectedCurves(new Set(curveKeys));

      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch animation data');
        setAnimState(null);
        setHasAnimator(false);
      } finally {
        setLoading(false);
      }
    };

    fetchAnimationData();
  }, [selectedEntityId]);

  // Helper to get unique key for a curve
  const getCurveKey = (curve: AnimationCurve): string => {
    return curve.entityPath ? `${curve.entityPath}/${curve.propertyPath}` : curve.propertyPath;
  };

  // Helper to get display name for a curve
  const getCurveDisplayName = (curve: AnimationCurve): string => {
    if (curve.entityPath) {
      return `${curve.entityPath} > ${curve.propertyPath}`;
    }
    return curve.propertyPath;
  };

  const handleTabChange = (_event: React.SyntheticEvent, newValue: number) => {
    setTabValue(newValue);
  };

  const handleCurveToggle = (curveKey: string) => {
    setSelectedCurves(prev => {
      const next = new Set(prev);
      if (next.has(curveKey)) {
        next.delete(curveKey);
      } else {
        next.add(curveKey);
      }
      return next;
    });
  };

  const getFilteredCurves = useCallback((): AnimationCurve[] => {
    if (!animState?.curves) return [];
    return animState.curves.filter(c => selectedCurves.has(getCurveKey(c)));
  }, [animState?.curves, selectedCurves]);

  // Convert AnimationCurve to the format expected by CurveViewer
  const getCurvesForViewer = useCallback((): CurveData[] => {
    return getFilteredCurves().map(c => ({
      id: getCurveKey(c),
      propertyPath: getCurveDisplayName(c),
      keyframes: [...c.keyframes],
      wrapMode: c.wrapMode,
    }));
  }, [getFilteredCurves]);

  // Find curve index in animState.curves by curveId
  const findCurveIndex = useCallback((curveId: string): number => {
    if (!animState?.curves) return -1;
    return animState.curves.findIndex(c => getCurveKey(c) === curveId);
  }, [animState?.curves]);

  // Keyframe mutation handlers
  const handleKeyframeUpdate = useCallback((curveId: string, keyframeIndex: number, keyframe: Keyframe) => {
    if (!animState) return;

    const curveIndex = findCurveIndex(curveId);
    if (curveIndex === -1) return;

    // Update the curves array immutably
    const newCurves = [...animState.curves];
    const curve = { ...newCurves[curveIndex] };
    const newKeyframes = [...curve.keyframes];
    newKeyframes[keyframeIndex] = keyframe;
    curve.keyframes = newKeyframes;
    newCurves[curveIndex] = curve;

    // Recalculate duration
    const duration = gameEngineAPI.getClipDuration(newCurves);

    setAnimState({
      ...animState,
      curves: newCurves,
      duration,
    });
  }, [animState, findCurveIndex]);

  const handleKeyframeAdd = useCallback((curveId: string, keyframe: Keyframe) => {
    if (!animState) return;

    const curveIndex = findCurveIndex(curveId);
    if (curveIndex === -1) return;

    // Update the curves array immutably
    const newCurves = [...animState.curves];
    const curve = { ...newCurves[curveIndex] };

    // Insert keyframe in sorted order by time
    const newKeyframes = [...curve.keyframes, keyframe].sort((a, b) => a.time - b.time);
    curve.keyframes = newKeyframes;
    newCurves[curveIndex] = curve;

    // Recalculate duration
    const duration = gameEngineAPI.getClipDuration(newCurves);

    setAnimState({
      ...animState,
      curves: newCurves,
      duration,
    });
  }, [animState, findCurveIndex]);

  const handleKeyframeDelete = useCallback((curveId: string, keyframeIndex: number) => {
    if (!animState) return;

    const curveIndex = findCurveIndex(curveId);
    if (curveIndex === -1) return;

    // Update the curves array immutably
    const newCurves = [...animState.curves];
    const curve = { ...newCurves[curveIndex] };
    const newKeyframes = curve.keyframes.filter((_, i) => i !== keyframeIndex);
    curve.keyframes = newKeyframes;
    newCurves[curveIndex] = curve;

    // Recalculate duration
    const duration = gameEngineAPI.getClipDuration(newCurves);

    setAnimState({
      ...animState,
      curves: newCurves,
      duration,
    });
  }, [animState, findCurveIndex]);

  // Get list of entity names already in the animation (for exclusion in picker)
  const getAnimatedEntityNames = useCallback((): string[] => {
    if (!animState?.clipData) return [];

    const names: string[] = [];

    const collectNames = (children: { key: string; value: { children: typeof children } }[]) => {
      for (const child of children) {
        names.push(child.key);
        if (child.value.children) {
          collectNames(child.value.children as typeof children);
        }
      }
    };

    collectNames(animState.clipData.clip.root_entity.children);
    return names;
  }, [animState?.clipData]);

  // Add entity to animation hierarchy
  const handleAddEntity = useCallback((entityId: string, entityName: string) => {
    if (!animState) return;

    // Create a new child entity entry with empty components
    const newChild = {
      key: entityName,
      value: {
        components: [],
        children: [],
      },
    };

    // Update clipData immutably
    const newClipData: AnimationClipResponse = {
      clip: {
        root_entity: {
          ...animState.clipData.clip.root_entity,
          children: [...animState.clipData.clip.root_entity.children, newChild],
        },
      },
    };

    // Re-flatten curves
    const newCurves = gameEngineAPI.flattenAnimationClip(newClipData);
    const newDuration = gameEngineAPI.getClipDuration(newCurves);

    setAnimState({
      ...animState,
      clipData: newClipData,
      curves: newCurves,
      duration: newDuration,
    });

    // Switch to Hierarchy tab to show the new entity
    setTabValue(1);
  }, [animState]);

  // Remove entity from animation hierarchy
  const handleRemoveEntity = useCallback((entityPath: string) => {
    if (!animState) return;

    // Helper to remove entity from children array by path
    const removeFromChildren = (
      children: AnimatedEntityChild[],
      pathParts: string[]
    ): AnimatedEntityChild[] => {
      if (pathParts.length === 0) return children;

      const [currentName, ...restPath] = pathParts;

      if (restPath.length === 0) {
        // Remove the entity at this level
        return children.filter(child => child.key !== currentName);
      }

      // Recurse into child
      return children.map(child => {
        if (child.key !== currentName) return child;
        return {
          ...child,
          value: {
            ...child.value,
            children: removeFromChildren(child.value.children, restPath),
          },
        };
      });
    };

    const pathParts = entityPath.split('/').filter(p => p.length > 0);

    const newClipData: AnimationClipResponse = {
      clip: {
        root_entity: {
          ...animState.clipData.clip.root_entity,
          children: removeFromChildren(animState.clipData.clip.root_entity.children, pathParts),
        },
      },
    };

    // Re-flatten curves
    const newCurves = gameEngineAPI.flattenAnimationClip(newClipData);
    const newDuration = gameEngineAPI.getClipDuration(newCurves);

    // Update selected curves - remove any that belonged to the removed entity
    const curveKeys = new Set(newCurves.map(c => getCurveKey(c)));
    setSelectedCurves(prev => {
      const next = new Set<string>();
      prev.forEach(key => {
        if (curveKeys.has(key)) next.add(key);
      });
      return next;
    });

    setAnimState({
      ...animState,
      clipData: newClipData,
      curves: newCurves,
      duration: newDuration,
    });

    // Clear selection if removed entity was selected
    if (selectedHierarchyNode?.entityPath.startsWith(entityPath)) {
      setSelectedHierarchyNode(null);
    }
  }, [animState, selectedHierarchyNode]);

  // Remove component from an entity in the animation hierarchy
  const handleRemoveComponent = useCallback((entityPath: string, componentIndex: number) => {
    if (!animState) return;

    // Helper to update entity's components
    const updateEntityComponents = (
      children: AnimatedEntityChild[],
      pathParts: string[],
      compIndex: number
    ): AnimatedEntityChild[] => {
      if (pathParts.length === 0) return children;

      const [currentName, ...restPath] = pathParts;

      return children.map(child => {
        if (child.key !== currentName) return child;

        if (restPath.length === 0) {
          // Remove component at this entity
          return {
            ...child,
            value: {
              ...child.value,
              components: child.value.components.filter((_, idx) => idx !== compIndex),
            },
          };
        }

        // Recurse
        return {
          ...child,
          value: {
            ...child.value,
            children: updateEntityComponents(child.value.children, restPath, compIndex),
          },
        };
      });
    };

    const pathParts = entityPath.split('/').filter(p => p.length > 0);

    let newClipData: AnimationClipResponse;

    if (pathParts.length === 0) {
      // Removing from root entity
      newClipData = {
        clip: {
          root_entity: {
            ...animState.clipData.clip.root_entity,
            components: animState.clipData.clip.root_entity.components.filter((_, idx) => idx !== componentIndex),
          },
        },
      };
    } else {
      newClipData = {
        clip: {
          root_entity: {
            ...animState.clipData.clip.root_entity,
            children: updateEntityComponents(animState.clipData.clip.root_entity.children, pathParts, componentIndex),
          },
        },
      };
    }

    // Re-flatten curves
    const newCurves = gameEngineAPI.flattenAnimationClip(newClipData);
    const newDuration = gameEngineAPI.getClipDuration(newCurves);

    // Update selected curves
    const curveKeys = new Set(newCurves.map(c => getCurveKey(c)));
    setSelectedCurves(prev => {
      const next = new Set<string>();
      prev.forEach(key => {
        if (curveKeys.has(key)) next.add(key);
      });
      return next;
    });

    setAnimState({
      ...animState,
      clipData: newClipData,
      curves: newCurves,
      duration: newDuration,
    });

    // Clear selection if removed component was selected
    if (selectedHierarchyNode?.entityPath === entityPath && selectedHierarchyNode?.componentIndex === componentIndex) {
      setSelectedHierarchyNode(null);
    }
  }, [animState, selectedHierarchyNode]);

  // Remove property from a component in the animation hierarchy
  const handleRemoveProperty = useCallback((entityPath: string, componentIndex: number, propertyKey: string) => {
    if (!animState) return;

    // Helper to update component's properties
    const updateComponentProperties = (
      children: AnimatedEntityChild[],
      pathParts: string[],
      compIndex: number,
      propKey: string
    ): AnimatedEntityChild[] => {
      if (pathParts.length === 0) return children;

      const [currentName, ...restPath] = pathParts;

      return children.map(child => {
        if (child.key !== currentName) return child;

        if (restPath.length === 0) {
          // Update properties at this entity's component
          return {
            ...child,
            value: {
              ...child.value,
              components: child.value.components.map((comp, idx) => {
                if (idx !== compIndex) return comp;
                return {
                  ...comp,
                  properties: comp.properties.filter(p => p.key !== propKey),
                };
              }),
            },
          };
        }

        // Recurse
        return {
          ...child,
          value: {
            ...child.value,
            children: updateComponentProperties(child.value.children, restPath, compIndex, propKey),
          },
        };
      });
    };

    const pathParts = entityPath.split('/').filter(p => p.length > 0);

    let newClipData: AnimationClipResponse;

    if (pathParts.length === 0) {
      // Removing from root entity's component
      newClipData = {
        clip: {
          root_entity: {
            ...animState.clipData.clip.root_entity,
            components: animState.clipData.clip.root_entity.components.map((comp, idx) => {
              if (idx !== componentIndex) return comp;
              return {
                ...comp,
                properties: comp.properties.filter(p => p.key !== propertyKey),
              };
            }),
          },
        },
      };
    } else {
      newClipData = {
        clip: {
          root_entity: {
            ...animState.clipData.clip.root_entity,
            children: updateComponentProperties(animState.clipData.clip.root_entity.children, pathParts, componentIndex, propertyKey),
          },
        },
      };
    }

    // Re-flatten curves
    const newCurves = gameEngineAPI.flattenAnimationClip(newClipData);
    const newDuration = gameEngineAPI.getClipDuration(newCurves);

    // Update selected curves
    const curveKeys = new Set(newCurves.map(c => getCurveKey(c)));
    setSelectedCurves(prev => {
      const next = new Set<string>();
      prev.forEach(key => {
        if (curveKeys.has(key)) next.add(key);
      });
      return next;
    });

    setAnimState({
      ...animState,
      clipData: newClipData,
      curves: newCurves,
      duration: newDuration,
    });

    // Clear selection if removed property was selected
    if (selectedHierarchyNode?.entityPath === entityPath &&
        selectedHierarchyNode?.componentIndex === componentIndex &&
        selectedHierarchyNode?.propertyKey === propertyKey) {
      setSelectedHierarchyNode(null);
    }
  }, [animState, selectedHierarchyNode]);

  return (
    <Paper
      sx={{
        height: '100%',
        width: '100%',
        display: 'flex',
        flexDirection: 'column',
        overflow: 'hidden',
      }}
    >

      {loading && (
        <Box sx={{ display: 'flex', justifyContent: 'center', p: 4 }}>
          <CircularProgress />
        </Box>
      )}

      {!loading && !selectedEntityId && (
        <Box sx={{ p: 2 }}>
          <Alert severity="info">
            Select an entity to view animation information
          </Alert>
        </Box>
      )}

      {!loading && selectedEntityId && error && !hasAnimator && (
        <Box sx={{ p: 2 }}>
          <Alert severity="warning">
            {error}
          </Alert>
        </Box>
      )}

      {!loading && hasAnimator && (
        <Box sx={{ display: 'flex', flexDirection: 'column', flex: 1, overflow: 'hidden' }}>
          {/* Entity and Clip Info Bar */}
          <Box sx={{ px: 2, pb: 1 }}>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, flexWrap: 'wrap' }}>
              <Chip
                label={animState?.entityName || `Entity_${selectedEntityId}`}
                size="small"
                icon={<AnimationIcon />}
                variant="outlined"
              />
              {animState && (
                <>
                  <Chip
                    label={animState.clipName}
                    size="small"
                    icon={<ClipIcon />}
                    variant="outlined"
                  />
                  <Chip
                    label={`Duration: ${animState.duration.toFixed(2)}s`}
                    size="small"
                    variant="outlined"
                  />
                  <Chip
                    label={`Curves: ${animState.curves.length}`}
                    size="small"
                    variant="outlined"
                  />
                </>
              )}
            </Box>
          </Box>

          <Divider />

          {animState ? (
            <Box sx={{ display: 'flex', flexDirection: 'column', flex: 1, overflow: 'hidden' }}>
              <Tabs
                value={tabValue}
                onChange={handleTabChange}
                sx={{
                  borderBottom: 1,
                  borderColor: 'divider',
                  minHeight: 40,
                  '& .MuiTab-root': { minHeight: 40 }
                }}
              >
                <Tab label="Curves" icon={<TimelineIcon />} iconPosition="start" />
                <Tab label="Hierarchy" icon={<HierarchyIcon />} iconPosition="start" />
              </Tabs>

              {tabValue === 0 && (
                <Box sx={{ display: 'flex', flex: 1, overflow: 'hidden' }}>
                  {/* Curve Selection List */}
                  <Paper sx={{
                    width: 280,
                    borderRight: 1,
                    borderColor: 'divider',
                    borderRadius: 0,
                    display: 'flex',
                    flexDirection: 'column'
                  }}>
                    <Typography variant="subtitle2" sx={{ p: 1 }}>
                      Select Curves to Display
                    </Typography>
                    <List dense sx={{ flex: 1, overflow: 'auto' }}>
                      {animState.curves.map((curve, index) => {
                        const curveKey = getCurveKey(curve);
                        return (
                          <ListItem
                            key={index}
                            onClick={() => handleCurveToggle(curveKey)}
                            sx={{ cursor: 'pointer' }}
                          >
                            <ListItemIcon sx={{ minWidth: 32 }}>
                              <Checkbox
                                edge="start"
                                checked={selectedCurves.has(curveKey)}
                                size="small"
                              />
                            </ListItemIcon>
                            <ListItemText
                              primary={getCurveDisplayName(curve)}
                              primaryTypographyProps={{ fontSize: '0.875rem' }}
                              secondary={`${curve.keyframes.length} keys`}
                              secondaryTypographyProps={{ fontSize: '0.75rem' }}
                            />
                          </ListItem>
                        );
                      })}
                    </List>
                  </Paper>

                  {/* Curve Viewer */}
                  <Box sx={{ flex: 1, p: 2 }}>
                    {getFilteredCurves().length > 0 ? (
                      <CurveViewer
                        curves={getCurvesForViewer()}
                        duration={animState.duration}
                        currentTime={currentTime}
                        editable={true}
                        onTimeChange={setCurrentTime}
                        onKeyframeUpdate={handleKeyframeUpdate}
                        onKeyframeAdd={handleKeyframeAdd}
                        onKeyframeDelete={handleKeyframeDelete}
                      />
                    ) : (
                      <Box
                        sx={{
                          height: '100%',
                          display: 'flex',
                          alignItems: 'center',
                          justifyContent: 'center',
                          border: '2px dashed',
                          borderColor: 'divider',
                          borderRadius: 1,
                        }}
                      >
                        <Typography variant="body2" color="text.secondary">
                          Select curves from the list to visualize them
                        </Typography>
                      </Box>
                    )}
                  </Box>
                </Box>
              )}

              {tabValue === 1 && (
                <Box sx={{ flex: 1, overflow: 'hidden' }}>
                  <AnimationHierarchyEditor
                    clipData={animState.clipData}
                    typeRegistry={animState.typeRegistry}
                    selectedNode={selectedHierarchyNode}
                    onSelectNode={setSelectedHierarchyNode}
                    onAddEntity={() => setEntityPickerOpen(true)}
                    onRemoveEntity={handleRemoveEntity}
                    onRemoveComponent={handleRemoveComponent}
                    onRemoveProperty={handleRemoveProperty}
                  />
                </Box>
              )}
            </Box>
          ) : (
            <Box sx={{ p: 2, flex: 1 }}>
              <Alert
                severity="info"
                icon={<WarningIcon />}
              >
                No animation clip assigned to this Animator
              </Alert>
            </Box>
          )}
        </Box>
      )}

      {/* Entity Picker Dialog */}
      <EntityPickerDialog
        open={entityPickerOpen}
        onClose={() => setEntityPickerOpen(false)}
        onSelect={handleAddEntity}
        excludeEntityIds={getAnimatedEntityNames()}
      />
    </Paper>
  );
};
