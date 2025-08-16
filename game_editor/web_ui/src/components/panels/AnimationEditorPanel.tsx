'use client';

import React from 'react';
import { IDockviewPanelProps } from 'dockview';
import { Box, Typography } from '@mui/material';
import { useEditor } from '../../contexts/EditorContext';

// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export interface AnimationEditorPanelProps {}

export const AnimationEditorPanel: React.FC<IDockviewPanelProps<AnimationEditorPanelProps>> = () => {
  const { selectedEntityId } = useEditor();
  
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
      }}
    >
      <Typography variant="h6" sx={{ mb: 2 }}>
        Animation Editor
      </Typography>
      {selectedEntityId ? (
        <Typography variant="body2">
          Editing animations for entity: {selectedEntityId}
        </Typography>
      ) : (
        <Typography variant="body2" color="text.secondary">
          Select an entity with an Animator component to begin editing
        </Typography>
      )}
    </Box>
  );
};