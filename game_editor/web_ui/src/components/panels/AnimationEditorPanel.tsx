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
  Button 
} from '@mui/material';
import {
  Animation as AnimationIcon,
  MovieFilter as ClipIcon,
  Warning as WarningIcon,
} from '@mui/icons-material';
import { useEditor } from '../../contexts/EditorContext';
import { gameEngineAPI, AnimationEditingContext } from '../../api/gameEngine';

// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export interface AnimationEditorPanelProps {}

export const AnimationEditorPanel: React.FC<IDockviewPanelProps<AnimationEditorPanelProps>> = () => {
  const { selectedEntityId } = useEditor();
  const [context, setContext] = useState<AnimationEditingContext | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  
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
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch animation context');
        setContext(null);
      } finally {
        setLoading(false);
      }
    };
    
    fetchContext();
  }, [selectedEntityId]);
  
  return (
    <Box
      sx={{
        height: '100%',
        width: '100%',
        display: 'flex',
        flexDirection: 'column',
        backgroundColor: '#2a2a2a',
        color: '#cccccc',
        p: 2,
        overflow: 'auto',
      }}
    >
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
        <AnimationIcon />
        <Typography variant="h6">
          Animation Editor
        </Typography>
      </Box>
      
      {loading && (
        <Box sx={{ display: 'flex', justifyContent: 'center', p: 4 }}>
          <CircularProgress />
        </Box>
      )}
      
      {!loading && !selectedEntityId && (
        <Alert severity="info" sx={{ backgroundColor: '#1e3a5f', color: '#b3d9ff' }}>
          Select an entity to view animation information
        </Alert>
      )}
      
      {!loading && error && (
        <Alert severity="warning" sx={{ backgroundColor: '#5f4b1e', color: '#ffd9b3' }}>
          {error}
        </Alert>
      )}
      
      {!loading && context && context.hasAnimator && (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
          {/* Entity Info */}
          <Paper sx={{ p: 2, backgroundColor: '#1e1e1e' }}>
            <Typography variant="subtitle2" color="text.secondary" gutterBottom>
              Entity
            </Typography>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
              <Typography variant="body1">
                {context.entityName}
              </Typography>
              <Chip 
                label={`ID: ${context.entityId}`} 
                size="small" 
                variant="outlined" 
                sx={{ color: '#999' }}
              />
              <Chip 
                icon={<AnimationIcon />}
                label="Has Animator" 
                size="small" 
                color="success" 
                variant="outlined"
              />
            </Box>
          </Paper>
          
          <Divider sx={{ borderColor: '#444' }} />
          
          {/* Animation Clip Info */}
          <Paper sx={{ p: 2, backgroundColor: '#1e1e1e' }}>
            <Typography variant="subtitle2" color="text.secondary" gutterBottom>
              Animation Clip
            </Typography>
            
            {context.hasClip ? (
              <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1 }}>
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                  <ClipIcon fontSize="small" />
                  <Typography variant="body2">
                    {context.clipPath || 'Clip loaded (path unknown)'}
                  </Typography>
                </Box>
                
                {context.clipData && (
                  <Box sx={{ pl: 3, display: 'flex', gap: 3 }}>
                    <Typography variant="caption" color="text.secondary">
                      Duration: {context.clipData.duration.toFixed(2)}s
                    </Typography>
                    <Typography variant="caption" color="text.secondary">
                      Curves: {context.clipData.curveCount}
                    </Typography>
                  </Box>
                )}
                
                <Box sx={{ mt: 2 }}>
                  <Button 
                    variant="outlined" 
                    size="small"
                    disabled
                    sx={{ mr: 1 }}
                  >
                    View Curves (Coming Soon)
                  </Button>
                  <Button 
                    variant="outlined" 
                    size="small"
                    disabled
                  >
                    Edit Properties (Coming Soon)
                  </Button>
                </Box>
              </Box>
            ) : (
              <Alert 
                severity="info" 
                icon={<WarningIcon />}
                sx={{ backgroundColor: '#3d3d1e', color: '#ffff99' }}
              >
                No animation clip assigned to this Animator
              </Alert>
            )}
          </Paper>
          
          {/* Future: Timeline and Curve Editor */}
          <Paper sx={{ p: 2, backgroundColor: '#1e1e1e', flex: 1, minHeight: 200 }}>
            <Typography variant="subtitle2" color="text.secondary" gutterBottom>
              Timeline & Curves
            </Typography>
            <Box 
              sx={{ 
                height: '100%', 
                display: 'flex', 
                alignItems: 'center', 
                justifyContent: 'center',
                border: '1px dashed #444',
                borderRadius: 1,
                mt: 1,
              }}
            >
              <Typography variant="body2" color="text.secondary">
                Timeline and curve editor will appear here (Step 2)
              </Typography>
            </Box>
          </Paper>
        </Box>
      )}
    </Box>
  );
};