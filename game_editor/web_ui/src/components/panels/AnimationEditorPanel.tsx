'use client';

import React, { useState, useEffect } from 'react';
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
} from '@mui/icons-material';
import { useEditor } from '../../contexts/EditorContext';
import { gameEngineAPI, AnimationCurve } from '../../api/gameEngine';
import { CurveViewer } from '../animation/CurveViewer';

// Animation editing state
interface AnimationState {
  entityName: string;
  clipName: string;
  curves: AnimationCurve[];
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
        console.log(clipResponse)

        // Step 4: Flatten curves for UI display
        const curves = gameEngineAPI.flattenAnimationClip(clipResponse);
        const duration = gameEngineAPI.getClipDuration(curves);

        setAnimState({
          entityName: `Entity_${selectedEntityId}`,
          clipName,
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

  const getFilteredCurves = (): AnimationCurve[] => {
    if (!animState?.curves) return [];
    return animState.curves.filter(c => selectedCurves.has(getCurveKey(c)));
  };

  // Convert AnimationCurve to the format expected by CurveViewer
  const getCurvesForViewer = () => {
    return getFilteredCurves().map(c => ({
      propertyPath: getCurveDisplayName(c),
      keyframes: c.keyframes,
      wrapMode: String(c.wrapMode),
    }));
  };

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
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, p: 2, borderBottom: 1, borderColor: 'divider' }}>
        <AnimationIcon />
        <Typography variant="h6" component="h2">
          Animation Editor
        </Typography>
      </Box>

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
                        onTimeChange={setCurrentTime}
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
    </Paper>
  );
};
