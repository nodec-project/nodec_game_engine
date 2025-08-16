'use client';

import React, { useEffect, useState } from 'react';
import {
  DockviewReact,
  DockviewReadyEvent,
  IDockviewPanelProps,
  SerializedDockview,
} from 'dockview';
import { SceneHierarchyPanel } from './panels/SceneHierarchyPanel';
import { ComponentInspectorPanel } from './panels/ComponentInspectorPanel';
import { AnimationEditorPanel } from './panels/AnimationEditorPanel';
import { EditorProvider } from '../contexts/EditorContext';

// Import dockview styles
import '../styles/dockview-theme.css';

const EditorLayoutContent: React.FC = () => {
  const [api, setApi] = useState<DockviewReadyEvent['api'] | null>(null);

  // Component registry for panels
  const components = {
    sceneHierarchy: SceneHierarchyPanel,
    componentInspector: ComponentInspectorPanel,
    animationEditor: AnimationEditorPanel,
  };

  // Initialize dockview layout
  const onReady = (event: DockviewReadyEvent) => {
    console.log('Dockview ready');
    setApi(event.api);

    // Check for saved layout in localStorage
    const savedLayout = localStorage.getItem('editorLayout');
    if (savedLayout) {
      try {
        const layout = JSON.parse(savedLayout) as SerializedDockview;
        event.api.fromJSON(layout);
        return;
      } catch (e) {
        console.warn('Failed to restore layout:', e);
      }
    }

    // Create default layout if no saved layout exists
    createDefaultLayout(event);
  };

  // Create default Unity/Unreal-style layout
  const createDefaultLayout = (event: DockviewReadyEvent) => {
    // First panel - Scene Hierarchy (becomes the root)
    const hierarchyPanel = event.api.addPanel({
      id: 'hierarchy',
      component: 'sceneHierarchy',
      title: 'Scene Hierarchy',
    });

    // Right panel - Component Inspector
    const inspectorPanel = event.api.addPanel({
      id: 'inspector',
      component: 'componentInspector',
      title: 'Inspector',
      position: { 
        referencePanel: hierarchyPanel,
        direction: 'right' 
      },
    });

    // Bottom - Animation Editor
    const animationPanel = event.api.addPanel({
      id: 'animation',
      component: 'animationEditor',
      title: 'Animation',
      position: { 
        referencePanel: hierarchyPanel,
        direction: 'below' 
      },
    });

    // Set initial sizes - commented out for now as API needs verification
    // TODO: Verify correct Dockview API for panel sizing
  };

  // Save layout to localStorage when it changes
  useEffect(() => {
    if (!api) return;

    const saveLayout = () => {
      const layout = api.toJSON();
      localStorage.setItem('editorLayout', JSON.stringify(layout));
    };

    // Subscribe to layout changes
    const disposable = api.onDidLayoutChange(() => {
      saveLayout();
    });

    return () => {
      disposable.dispose();
    };
  }, [api]);

  // Handle keyboard shortcuts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Ctrl/Cmd + Shift + R: Reset layout
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'R') {
        e.preventDefault();
        if (api) {
          api.clear();
          // Re-create default layout inline to avoid dependency issues
          // First panel - Scene Hierarchy (becomes the root)
          const hierarchyPanel = api.addPanel({
            id: 'hierarchy',
            component: 'sceneHierarchy',
            title: 'Scene Hierarchy',
          });

          // Right panel - Component Inspector
          const inspectorPanel = api.addPanel({
            id: 'inspector',
            component: 'componentInspector',
            title: 'Inspector',
            position: { 
              referencePanel: hierarchyPanel,
              direction: 'right' 
            },
          });

          // Bottom - Animation Editor
          const animationPanel = api.addPanel({
            id: 'animation',
            component: 'animationEditor',
            title: 'Animation',
            position: { 
              referencePanel: hierarchyPanel,
              direction: 'below' 
            },
          });

          // Set initial sizes - commented out for now as API needs verification
          // TODO: Verify correct Dockview API for panel sizing
        }
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [api]);

  return (
    <div style={{ 
      height: '100vh', 
      width: '100vw',
      display: 'flex',
      flexDirection: 'column',
      backgroundColor: '#1e1e1e'
    }}>
      {/* Optional toolbar */}
      <div style={{
        height: '35px',
        backgroundColor: '#2d2d30',
        borderBottom: '1px solid #3e3e42',
        display: 'flex',
        alignItems: 'center',
        padding: '0 12px',
        color: '#cccccc',
        fontSize: '13px',
        fontWeight: 500
      }}>
        nodec Game Editor
      </div>

      {/* Dockview container */}
      <div style={{ flex: 1, overflow: 'hidden' }}>
        <DockviewReact
          components={components}
          onReady={onReady}
          className="dockview-theme-dark"
          disableFloatingGroups={false}
          floatingGroupBounds="boundedWithinViewport"
        />
      </div>
    </div>
  );
};

export const EditorLayout: React.FC = () => {
  return (
    <EditorProvider>
      <EditorLayoutContent />
    </EditorProvider>
  );
};