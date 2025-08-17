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
  Button,
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
  Settings as PropertyIcon,
} from '@mui/icons-material';
import { useEditor } from '../../contexts/EditorContext';
import { gameEngineAPI, AnimationEditingContext, AnimationCurve } from '../../api/gameEngine';
import { CurveViewer } from '../animation/CurveViewer';

// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export interface AnimationEditorPanelProps {}

export const AnimationEditorPanel: React.FC<IDockviewPanelProps<AnimationEditorPanelProps>> = () => {
  const { selectedEntityId } = useEditor();
  const [context, setContext] = useState<AnimationEditingContext | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [tabValue, setTabValue] = useState(0);
  const [selectedCurves, setSelectedCurves] = useState<Set<string>>(new Set());
  const [currentTime, setCurrentTime] = useState(0);
  
  useEffect(() => {
    if (!selectedEntityId) {
      setContext(null);
      setError(null);
      return;
    }
    
    const fetchContext = async () => {
      try {
        setLoading(true);
        setError(null);
        const animContext = await gameEngineAPI.getAnimationEditingContext(selectedEntityId);
        setContext(animContext);
        
        if (animContext.error && !animContext.hasAnimator) {
          setError(animContext.error);
        }
        
        // Auto-select all curves initially
        if (animContext.clipData?.curves) {
          setSelectedCurves(new Set(animContext.clipData.curves.map(c => c.propertyPath)));
        }
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch animation context');
        setContext(null);
      } finally {
        setLoading(false);
      }
    };
    
    fetchContext();
  }, [selectedEntityId]);
  
  const handleTabChange = (event: React.SyntheticEvent, newValue: number) => {
    setTabValue(newValue);
  };
  
  const handleCurveToggle = (propertyPath: string) => {
    setSelectedCurves(prev => {
      const next = new Set(prev);
      if (next.has(propertyPath)) {
        next.delete(propertyPath);
      } else {
        next.add(propertyPath);
      }
      return next;
    });
  };
  
  const getFilteredCurves = (): AnimationCurve[] => {
    if (!context?.clipData?.curves) return [];
    return context.clipData.curves.filter(c => selectedCurves.has(c.propertyPath));
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
      
      {!loading && error && (
        <Box sx={{ p: 2 }}>
          <Alert severity="warning">
            {error}
          </Alert>
        </Box>
      )}
      
      {!loading && context && context.hasAnimator && (
        <Box sx={{ display: 'flex', flexDirection: 'column', flex: 1, overflow: 'hidden' }}>
          {/* Entity and Clip Info Bar */}
          <Box sx={{ px: 2, pb: 1 }}>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, flexWrap: 'wrap' }}>
              <Chip 
                label={context.entityName} 
                size="small" 
                icon={<AnimationIcon />}
                variant="outlined"
              />
              {context.hasClip && (
                <>
                  <Chip 
                    label={context.clipPath || 'Clip loaded'} 
                    size="small" 
                    icon={<ClipIcon />}
                    variant="outlined"
                  />
                  <Chip 
                    label={`Duration: ${context.clipData?.duration.toFixed(2)}s`} 
                    size="small" 
                    variant="outlined"
                  />
                  <Chip 
                    label={`Curves: ${context.clipData?.curveCount}`} 
                    size="small" 
                    variant="outlined"
                  />
                </>
              )}
            </Box>
          </Box>
          
          <Divider />
          
          {context.hasClip && context.clipData ? (
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
                <Tab label="Properties" icon={<PropertyIcon />} iconPosition="start" />
              </Tabs>
              
              {tabValue === 0 && (
                <Box sx={{ display: 'flex', flex: 1, overflow: 'hidden' }}>
                  {/* Curve Selection List */}
                  <Paper sx={{ 
                    width: 250, 
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
                      {context.clipData.curves.map((curve, index) => (
                        <ListItem 
                          key={index}
                          onClick={() => handleCurveToggle(curve.propertyPath)}
                          sx={{ cursor: 'pointer' }}
                        >
                          <ListItemIcon sx={{ minWidth: 32 }}>
                            <Checkbox
                              edge="start"
                              checked={selectedCurves.has(curve.propertyPath)}
                              size="small"
                            />
                          </ListItemIcon>
                          <ListItemText 
                            primary={curve.propertyPath}
                            primaryTypographyProps={{ fontSize: '0.875rem' }}
                            secondary={`${curve.keyframes.length} keys`}
                            secondaryTypographyProps={{ fontSize: '0.75rem' }}
                          />
                        </ListItem>
                      ))}
                    </List>
                  </Paper>
                  
                  {/* Curve Viewer */}
                  <Box sx={{ flex: 1, p: 2 }}>
                    {getFilteredCurves().length > 0 ? (
                      <CurveViewer
                        curves={getFilteredCurves()}
                        duration={context.clipData.duration}
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
              
              {tabValue === 1 && (
                <Box sx={{ p: 2, overflow: 'auto' }}>
                  <Typography variant="subtitle2" gutterBottom>
                    Available Properties for Animation
                  </Typography>
                  {context.clipData.availableProperties.length > 0 ? (
                    <List>
                      {context.clipData.availableProperties.map((prop, index) => (
                        <ListItem key={index}>
                          <ListItemText
                            primary={prop.componentName}
                            secondary={
                              prop.properties.length > 0 
                                ? `Properties: ${prop.properties.join(', ')}`
                                : 'No animatable properties detected'
                            }
                          />
                        </ListItem>
                      ))}
                    </List>
                  ) : (
                    <Alert severity="info">
                      No animatable properties found. Properties will be populated when property reflection is implemented.
                    </Alert>
                  )}
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