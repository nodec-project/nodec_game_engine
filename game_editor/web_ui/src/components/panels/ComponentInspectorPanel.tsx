'use client';

import React from 'react';
import { IDockviewPanelProps } from 'dockview';
import { ComponentInspector } from '../ComponentInspector';
import { useEditor } from '../../contexts/EditorContext';

// eslint-disable-next-line @typescript-eslint/no-empty-object-type
export interface ComponentInspectorPanelProps {}

export const ComponentInspectorPanel: React.FC<IDockviewPanelProps<ComponentInspectorPanelProps>> = () => {
  const { selectedEntityId } = useEditor();
  
  return (
    <div style={{ 
      height: '100%', 
      width: '100%', 
      overflow: 'auto',
      backgroundColor: '#1e1e1e',
      color: '#cccccc'
    }}>
      <ComponentInspector entityId={selectedEntityId} />
    </div>
  );
};