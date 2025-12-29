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
  IconButton,
  Tooltip,
} from '@mui/material';
import {
  Animation as AnimationIcon,
  MovieFilter as ClipIcon,
  Warning as WarningIcon,
  Timeline as TimelineIcon,
  AccountTree as HierarchyIcon,
  Lock as LockIcon,
  LockOpen as LockOpenIcon,
  Save as SaveIcon,
} from '@mui/icons-material';
import { useEditor } from '../../contexts/EditorContext';
import {
  gameEngineAPI,
  AnimationCurve,
  AnimationClipResponse,
  AnimatedEntityChild,
  AnimatedComponentData,
  AnimatedComponentPlaceholder,
  Keyframe,
  PolymorphicTypeRegistry,
} from '../../api/gameEngine';
import { CurveViewer, CurveData } from '../animation/CurveViewer';
import { AnimationHierarchyEditor, SelectedNode } from '../animation/AnimationHierarchyEditor';
import { EntityPickerDialog } from '../animation/EntityPickerDialog';
import { ComponentPickerDialog } from '../animation/ComponentPickerDialog';
import { PropertyPickerDialog } from '../animation/PropertyPickerDialog';

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
  const [componentPickerOpen, setComponentPickerOpen] = useState(false);
  const [propertyPickerOpen, setPropertyPickerOpen] = useState(false);
  // Track which entity/component we're adding to
  const [addComponentTargetPath, setAddComponentTargetPath] = useState<string>('');
  const [addPropertyTarget, setAddPropertyTarget] = useState<{ entityPath: string; componentIndex: number } | null>(null);

  // Lock feature - when locked, don't switch entity on selection change
  const [isLocked, setIsLocked] = useState(false);
  const [lockedEntityId, setLockedEntityId] = useState<string | null>(null);

  // Save state
  const [isSaving, setIsSaving] = useState(false);

  // The entity ID to use for fetching animation data
  const effectiveEntityId = isLocked ? lockedEntityId : selectedEntityId;

  // Toggle lock state
  const handleToggleLock = useCallback(() => {
    if (isLocked) {
      // Unlocking - clear locked entity
      setIsLocked(false);
      setLockedEntityId(null);
    } else {
      // Locking - store current entity
      setIsLocked(true);
      setLockedEntityId(selectedEntityId);
    }
  }, [isLocked, selectedEntityId]);

  // Save animation clip to server
  const handleSave = useCallback(async () => {
    if (!animState) return;

    try {
      setIsSaving(true);
      await gameEngineAPI.updateAnimationClip(animState.clipName, animState.clipData);
      console.log(`Animation clip saved: ${animState.clipName}`);
    } catch (err) {
      console.error('Failed to save animation clip:', err);
      setError(err instanceof Error ? err.message : 'Failed to save animation clip');
    } finally {
      setIsSaving(false);
    }
  }, [animState]);

  useEffect(() => {
    if (!effectiveEntityId) {
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
        const components = await gameEngineAPI.getEntityComponents(effectiveEntityId);

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
          entityName: `Entity_${effectiveEntityId}`,
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
  }, [effectiveEntityId]);

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

  // Helper to update keyframes in clipData for a given entityPath and propertyPath
  const updateClipDataKeyframes = useCallback((
    clipData: AnimationClipResponse,
    entityPath: string,
    propertyPath: string,
    newKeyframes: Keyframe[]
  ): AnimationClipResponse => {
    const pathParts = entityPath.split('/').filter(p => p.length > 0);

    // Helper to update keyframes in an entity's components
    const updateInComponents = (components: AnimatedComponentData[]): AnimatedComponentData[] => {
      return components.map(comp => {
        const propIndex = comp.properties.findIndex(p => p.key === propertyPath);
        if (propIndex === -1) return comp;
        return {
          ...comp,
          properties: comp.properties.map((p, idx) => {
            if (idx !== propIndex) return p;
            return {
              ...p,
              value: {
                curve: {
                  ...p.value.curve,
                  keyframes: newKeyframes,
                },
              },
            };
          }),
        };
      });
    };

    // Helper to update recursively in children
    const updateInChildren = (
      children: AnimatedEntityChild[],
      parts: string[]
    ): AnimatedEntityChild[] => {
      if (parts.length === 0) return children;

      const [currentName, ...restParts] = parts;

      return children.map(child => {
        if (child.key !== currentName) return child;

        if (restParts.length === 0) {
          // Found the target entity, update its components
          return {
            ...child,
            value: {
              ...child.value,
              components: updateInComponents(child.value.components),
            },
          };
        }

        // Recurse
        return {
          ...child,
          value: {
            ...child.value,
            children: updateInChildren(child.value.children, restParts),
          },
        };
      });
    };

    if (pathParts.length === 0) {
      // Root entity
      return {
        clip: {
          root_entity: {
            ...clipData.clip.root_entity,
            components: updateInComponents(clipData.clip.root_entity.components),
          },
        },
      };
    }

    return {
      clip: {
        root_entity: {
          ...clipData.clip.root_entity,
          children: updateInChildren(clipData.clip.root_entity.children, pathParts),
        },
      },
    };
  }, []);

  // Keyframe mutation handlers
  const handleKeyframeUpdate = useCallback((curveId: string, keyframeIndex: number, keyframe: Keyframe) => {
    if (!animState) return;

    const curveIndex = findCurveIndex(curveId);
    if (curveIndex === -1) return;

    // Get the curve to extract entityPath and propertyPath
    const curve = animState.curves[curveIndex];

    // Update the curves array immutably
    const newCurves = [...animState.curves];
    const newCurve = { ...newCurves[curveIndex] };
    const newKeyframes = [...newCurve.keyframes];
    newKeyframes[keyframeIndex] = keyframe;
    newCurve.keyframes = newKeyframes;
    newCurves[curveIndex] = newCurve;

    // Also update clipData
    const newClipData = updateClipDataKeyframes(
      animState.clipData,
      curve.entityPath,
      curve.propertyPath,
      newKeyframes
    );

    // Recalculate duration
    const duration = gameEngineAPI.getClipDuration(newCurves);

    setAnimState({
      ...animState,
      clipData: newClipData,
      curves: newCurves,
      duration,
    });
  }, [animState, findCurveIndex, updateClipDataKeyframes]);

  const handleKeyframeAdd = useCallback((curveId: string, keyframe: Keyframe) => {
    if (!animState) return;

    const curveIndex = findCurveIndex(curveId);
    if (curveIndex === -1) return;

    // Get the curve to extract entityPath and propertyPath
    const curve = animState.curves[curveIndex];

    // Update the curves array immutably
    const newCurves = [...animState.curves];
    const newCurve = { ...newCurves[curveIndex] };

    // Insert keyframe in sorted order by time
    const newKeyframes = [...newCurve.keyframes, keyframe].sort((a, b) => a.time - b.time);
    newCurve.keyframes = newKeyframes;
    newCurves[curveIndex] = newCurve;

    // Also update clipData
    const newClipData = updateClipDataKeyframes(
      animState.clipData,
      curve.entityPath,
      curve.propertyPath,
      newKeyframes
    );

    // Recalculate duration
    const duration = gameEngineAPI.getClipDuration(newCurves);

    setAnimState({
      ...animState,
      clipData: newClipData,
      curves: newCurves,
      duration,
    });
  }, [animState, findCurveIndex, updateClipDataKeyframes]);

  const handleKeyframeDelete = useCallback((curveId: string, keyframeIndex: number) => {
    if (!animState) return;

    const curveIndex = findCurveIndex(curveId);
    if (curveIndex === -1) return;

    // Get the curve to extract entityPath and propertyPath
    const curve = animState.curves[curveIndex];

    // Update the curves array immutably
    const newCurves = [...animState.curves];
    const newCurve = { ...newCurves[curveIndex] };
    const newKeyframes = newCurve.keyframes.filter((_, i) => i !== keyframeIndex);
    newCurve.keyframes = newKeyframes;
    newCurves[curveIndex] = newCurve;

    // Also update clipData
    const newClipData = updateClipDataKeyframes(
      animState.clipData,
      curve.entityPath,
      curve.propertyPath,
      newKeyframes
    );

    // Recalculate duration
    const duration = gameEngineAPI.getClipDuration(newCurves);

    setAnimState({
      ...animState,
      clipData: newClipData,
      curves: newCurves,
      duration,
    });
  }, [animState, findCurveIndex, updateClipDataKeyframes]);

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

  // Open component picker for a specific entity
  const handleOpenComponentPicker = useCallback((entityPath: string) => {
    setAddComponentTargetPath(entityPath);
    setComponentPickerOpen(true);
  }, []);

  // Add component to entity in the animation hierarchy
  const handleAddComponent = useCallback((placeholder: AnimatedComponentPlaceholder, displayName: string) => {
    if (!animState) return;

    // Create new component data with empty properties
    const newComponent: AnimatedComponentData = {
      placeholder,
      properties: [],
    };

    // Helper to add component to entity by path
    const addComponentToEntity = (
      children: AnimatedEntityChild[],
      pathParts: string[]
    ): AnimatedEntityChild[] => {
      if (pathParts.length === 0) return children;

      const [currentName, ...restPath] = pathParts;

      return children.map(child => {
        if (child.key !== currentName) return child;

        if (restPath.length === 0) {
          // Add component at this entity
          return {
            ...child,
            value: {
              ...child.value,
              components: [...child.value.components, newComponent],
            },
          };
        }

        // Recurse
        return {
          ...child,
          value: {
            ...child.value,
            children: addComponentToEntity(child.value.children, restPath),
          },
        };
      });
    };

    const pathParts = addComponentTargetPath.split('/').filter(p => p.length > 0);

    let newClipData: AnimationClipResponse;

    if (pathParts.length === 0) {
      // Adding to root entity
      newClipData = {
        clip: {
          root_entity: {
            ...animState.clipData.clip.root_entity,
            components: [...animState.clipData.clip.root_entity.components, newComponent],
          },
        },
      };
    } else {
      newClipData = {
        clip: {
          root_entity: {
            ...animState.clipData.clip.root_entity,
            children: addComponentToEntity(animState.clipData.clip.root_entity.children, pathParts),
          },
        },
      };
    }

    // Rebuild type registry with the new component
    const newTypeRegistry = gameEngineAPI.buildPolymorphicTypeRegistry(newClipData);

    // Re-flatten curves
    const newCurves = gameEngineAPI.flattenAnimationClip(newClipData);
    const newDuration = gameEngineAPI.getClipDuration(newCurves);

    setAnimState({
      ...animState,
      clipData: newClipData,
      typeRegistry: newTypeRegistry,
      curves: newCurves,
      duration: newDuration,
    });
  }, [animState, addComponentTargetPath]);

  // Open property picker for a specific component
  const handleOpenPropertyPicker = useCallback((entityPath: string, componentIndex: number) => {
    setAddPropertyTarget({ entityPath, componentIndex });
    setPropertyPickerOpen(true);
  }, []);

  // Get component data for property picker
  const getComponentDataForPropertyPicker = useCallback((): Record<string, unknown> => {
    if (!animState || !addPropertyTarget) return {};

    const { entityPath, componentIndex } = addPropertyTarget;
    const pathParts = entityPath.split('/').filter(p => p.length > 0);

    // Helper to find component in hierarchy
    const findComponent = (
      children: AnimatedEntityChild[],
      parts: string[]
    ): AnimatedComponentData | null => {
      if (parts.length === 0) return null;

      const [currentName, ...restParts] = parts;

      for (const child of children) {
        if (child.key !== currentName) continue;

        if (restParts.length === 0) {
          // Found the entity, return the component
          return child.value.components[componentIndex] || null;
        }

        // Recurse
        return findComponent(child.value.children, restParts);
      }

      return null;
    };

    let component: AnimatedComponentData | null = null;

    if (pathParts.length === 0) {
      // Root entity
      component = animState.clipData.clip.root_entity.components[componentIndex] || null;
    } else {
      component = findComponent(animState.clipData.clip.root_entity.children, pathParts);
    }

    if (!component?.placeholder?.ptr_wrapper?.data) {
      return {};
    }

    return component.placeholder.ptr_wrapper.data as Record<string, unknown>;
  }, [animState, addPropertyTarget]);

  // Get already added property paths for exclusion in property picker
  const getExcludedPropertyPaths = useCallback((): string[] => {
    if (!animState || !addPropertyTarget) return [];

    const { entityPath, componentIndex } = addPropertyTarget;
    const pathParts = entityPath.split('/').filter(p => p.length > 0);

    // Helper to find component in hierarchy
    const findComponent = (
      children: AnimatedEntityChild[],
      parts: string[]
    ): AnimatedComponentData | null => {
      if (parts.length === 0) return null;

      const [currentName, ...restParts] = parts;

      for (const child of children) {
        if (child.key !== currentName) continue;

        if (restParts.length === 0) {
          return child.value.components[componentIndex] || null;
        }

        return findComponent(child.value.children, restParts);
      }

      return null;
    };

    let component: AnimatedComponentData | null = null;

    if (pathParts.length === 0) {
      component = animState.clipData.clip.root_entity.components[componentIndex] || null;
    } else {
      component = findComponent(animState.clipData.clip.root_entity.children, pathParts);
    }

    if (!component) return [];

    return component.properties.map(p => p.key);
  }, [animState, addPropertyTarget]);

  // Add property to component in the animation hierarchy
  const handleAddProperty = useCallback((propertyPath: string) => {
    if (!animState || !addPropertyTarget) return;

    const { entityPath, componentIndex } = addPropertyTarget;

    // Create new property with empty curve (no keyframes initially)
    const newProperty = {
      key: propertyPath,
      value: {
        curve: {
          wrap_mode: 0,  // Once
          keyframes: [],  // Empty - user adds keyframes manually
        },
      },
    };

    // Helper to add property to component by path
    const addPropertyToComponent = (
      children: AnimatedEntityChild[],
      pathParts: string[],
      compIndex: number
    ): AnimatedEntityChild[] => {
      if (pathParts.length === 0) return children;

      const [currentName, ...restPath] = pathParts;

      return children.map(child => {
        if (child.key !== currentName) return child;

        if (restPath.length === 0) {
          // Found the entity, add property to the component
          return {
            ...child,
            value: {
              ...child.value,
              components: child.value.components.map((comp, idx) => {
                if (idx !== compIndex) return comp;
                return {
                  ...comp,
                  properties: [...comp.properties, newProperty],
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
            children: addPropertyToComponent(child.value.children, restPath, compIndex),
          },
        };
      });
    };

    const pathParts = entityPath.split('/').filter(p => p.length > 0);

    let newClipData: AnimationClipResponse;

    if (pathParts.length === 0) {
      // Adding to root entity's component
      newClipData = {
        clip: {
          root_entity: {
            ...animState.clipData.clip.root_entity,
            components: animState.clipData.clip.root_entity.components.map((comp, idx) => {
              if (idx !== componentIndex) return comp;
              return {
                ...comp,
                properties: [...comp.properties, newProperty],
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
            children: addPropertyToComponent(animState.clipData.clip.root_entity.children, pathParts, componentIndex),
          },
        },
      };
    }

    // Re-flatten curves
    const newCurves = gameEngineAPI.flattenAnimationClip(newClipData);
    const newDuration = gameEngineAPI.getClipDuration(newCurves);

    // Auto-select the new curve
    const newCurveKey = entityPath ? `${entityPath}/${propertyPath}` : propertyPath;

    setSelectedCurves(prev => {
      const next = new Set(prev);
      next.add(newCurveKey);
      return next;
    });

    setAnimState({
      ...animState,
      clipData: newClipData,
      curves: newCurves,
      duration: newDuration,
    });
  }, [animState, addPropertyTarget]);

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
              {/* Lock Button */}
              <Tooltip title={isLocked ? 'Unlock (follow selection)' : 'Lock (keep current entity)'}>
                <IconButton
                  size="small"
                  onClick={handleToggleLock}
                  color={isLocked ? 'primary' : 'default'}
                  sx={{ mr: 0.5 }}
                >
                  {isLocked ? <LockIcon fontSize="small" /> : <LockOpenIcon fontSize="small" />}
                </IconButton>
              </Tooltip>
              {/* Save Button */}
              <Tooltip title="Save animation clip">
                <span>
                  <IconButton
                    size="small"
                    onClick={handleSave}
                    disabled={!animState || isSaving}
                    color="primary"
                    sx={{ mr: 0.5 }}
                  >
                    {isSaving ? <CircularProgress size={18} /> : <SaveIcon fontSize="small" />}
                  </IconButton>
                </span>
              </Tooltip>
              <Chip
                label={animState?.entityName || `Entity_${effectiveEntityId}`}
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
                    onAddComponent={handleOpenComponentPicker}
                    onRemoveComponent={handleRemoveComponent}
                    onAddProperty={handleOpenPropertyPicker}
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

      {/* Entity Picker Dialog - starts from selected entity's children */}
      <EntityPickerDialog
        open={entityPickerOpen}
        onClose={() => setEntityPickerOpen(false)}
        onSelect={handleAddEntity}
        excludeEntityIds={getAnimatedEntityNames()}
        rootEntityId={effectiveEntityId || undefined}
      />

      {/* Component Picker Dialog */}
      <ComponentPickerDialog
        open={componentPickerOpen}
        onClose={() => setComponentPickerOpen(false)}
        onSelect={handleAddComponent}
      />

      {/* Property Picker Dialog */}
      <PropertyPickerDialog
        open={propertyPickerOpen}
        onClose={() => setPropertyPickerOpen(false)}
        onSelect={handleAddProperty}
        componentData={getComponentDataForPropertyPicker()}
        excludePropertyPaths={getExcludedPropertyPaths()}
      />
    </Paper>
  );
};
