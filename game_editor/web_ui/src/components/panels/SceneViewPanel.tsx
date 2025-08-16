'use client';

import React from 'react';
import { IDockviewPanelProps } from 'dockview';
import { Box, Typography } from '@mui/material';

export interface SceneViewPanelProps {
  // Placeholder for future scene view properties
  cameraMode?: 'perspective' | 'orthographic';
}

export const SceneViewPanel: React.FC<IDockviewPanelProps<SceneViewPanelProps>> = (props) => {
  return (
    <Box
      sx={{
        height: '100%',
        width: '100%',
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        backgroundColor: '#2a2a2a',
        color: '#cccccc',
      }}
    >
      <Typography variant="h5" sx={{ mb: 2 }}>
        Scene View
      </Typography>
      <Typography variant="body2" color="text.secondary">
        3D scene rendering will be displayed here
      </Typography>
    </Box>
  );
};