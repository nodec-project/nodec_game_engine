'use client';

import React from 'react';
import { IDockviewPanelProps } from 'dockview';
import { SceneHierarchy } from '../SceneHierarchy';
import { useEditor } from '../../contexts/EditorContext';

// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export interface SceneHierarchyPanelProps {}

export const SceneHierarchyPanel: React.FC<IDockviewPanelProps<SceneHierarchyPanelProps>> = () => {
  const { setSelectedEntityId } = useEditor();
  
  return (
    <div style={{ 
      height: '100%', 
      width: '100%', 
      overflow: 'auto',
      backgroundColor: '#1e1e1e',
      color: '#cccccc'
    }}>
      <SceneHierarchy onEntitySelect={setSelectedEntityId} />
    </div>
  );
};