'use client';

import React from 'react';
import { IDockviewPanelProps } from 'dockview';
import { Typography } from '@/ui';
import styles from './SceneViewPanel.module.css';

export interface SceneViewPanelProps {
  // Placeholder for future scene view properties
  cameraMode?: 'perspective' | 'orthographic';
}

export const SceneViewPanel: React.FC<IDockviewPanelProps<SceneViewPanelProps>> = () => {
  return (
    <div className={styles.container}>
      <Typography variant="headlineSmall" className={styles.title}>
        Scene View
      </Typography>
      <Typography variant="bodySmall" color="onSurfaceVariant">
        3D scene rendering will be displayed here
      </Typography>
    </div>
  );
};
