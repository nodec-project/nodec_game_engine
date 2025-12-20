'use client';

import React, { useRef } from 'react';
import {
  DockviewReact,
  DockviewReadyEvent,
  DockviewApi,
} from 'dockview';
import { SceneHierarchyPanel } from '../../src/components/panels/SceneHierarchyPanel';
import { ComponentInspectorPanel } from '../../src/components/panels/ComponentInspectorPanel';
import { AnimationEditorPanel } from '../../src/components/panels/AnimationEditorPanel';
import { EditorProvider } from '../../src/contexts/EditorContext';
import { ThemeProvider } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';
import { theme } from '../../src/lib/theme';

// Import dockview styles
import '../../src/styles/dockview-theme.css';

const PopoutContent: React.FC = () => {
  const apiRef = useRef<DockviewApi | null>(null);

  // Component registry - must match main window exactly
  const components = {
    sceneHierarchy: SceneHierarchyPanel,
    componentInspector: ComponentInspectorPanel,
    animationEditor: AnimationEditorPanel,
  };

  const onReady = (event: DockviewReadyEvent) => {
    console.log('Popout dockview ready', event.api);
    apiRef.current = event.api;

    // Expose the API to the window for dockview's popout communication
    (window as unknown as { dockviewApi?: DockviewApi }).dockviewApi = event.api;
  };

  return (
    <div style={{
      height: '100vh',
      width: '100vw',
      backgroundColor: '#1e1e1e',
      overflow: 'hidden',
    }}>
      <DockviewReact
        components={components}
        onReady={onReady}
        className="dockview-theme-dark"
        disableFloatingGroups={false}
      />
    </div>
  );
};

export default function PopoutPage() {
  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <EditorProvider>
        <PopoutContent />
      </EditorProvider>
    </ThemeProvider>
  );
}
