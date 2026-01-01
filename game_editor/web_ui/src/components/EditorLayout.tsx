'use client';

import React, { useEffect, useState, useCallback } from 'react';
import {
  DockviewReact,
  DockviewReadyEvent,
  DockviewApi,
  IDockviewPanelProps,
  IDockviewHeaderActionsProps,
  SerializedDockview,
} from 'dockview';
import { IconButton, Tooltip, Chip, Menu } from '@/ui';
import { SceneHierarchyPanel } from './panels/SceneHierarchyPanel';
import { ComponentInspectorPanel } from './panels/ComponentInspectorPanel';
import { AnimationEditorPanel } from './panels/AnimationEditorPanel';
import { EditorProvider, useEditor } from '../contexts/EditorContext';
import { gameEngineAPI } from '../api/gameEngine';
import styles from './EditorLayout.module.css';

// Import dockview styles
import '../styles/dockview-theme.css';

// Icons
const FloatIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor" className={styles.floatButtonIcon}>
    <path d="M19 7h-8v6h8V7zm-2 4h-4V9h4v2zm4-8H3c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h18c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 16H3V5h18v14z" />
  </svg>
);

const ViewIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor" className={styles.viewButtonIcon}>
    <path d="M3 5v14h19V5H3zm2 2h15v4H5V7zm0 10v-4h4v4H5zm6 0v-4h9v4h-9z" />
  </svg>
);

const CircleIcon = ({ className }: { className?: string }) => (
  <svg viewBox="0 0 24 24" fill="currentColor" className={className}>
    <circle cx="12" cy="12" r="8" />
  </svg>
);

// Right header actions - adds float button to panel headers
const RightHeaderActions: React.FC<IDockviewHeaderActionsProps> = ({ containerApi, group }) => {
  const handleFloat = useCallback(() => {
    // Convert this group to a floating group within the same window
    containerApi.addFloatingGroup(group, {
      width: 500,
      height: 400,
    });
  }, [containerApi, group]);

  return (
    <Tooltip title="Float panel">
      <IconButton
        size="small"
        onClick={handleFloat}
        className={styles.floatButton}
      >
        <FloatIcon />
      </IconButton>
    </Tooltip>
  );
};

// Panel definitions for the View menu
const PANEL_DEFINITIONS = [
  { id: 'hierarchy', component: 'sceneHierarchy', title: 'Scene Hierarchy' },
  { id: 'inspector', component: 'componentInspector', title: 'Inspector' },
  { id: 'animation', component: 'animationEditor', title: 'Animation' },
] as const;

const EditorLayoutContent: React.FC = () => {
  const { engineConnected, setEngineConnected } = useEditor();
  const [api, setApi] = useState<DockviewReadyEvent['api'] | null>(null);

  // Set browser tab title
  useEffect(() => {
    document.title = 'nodec Game Editor';
  }, []);

  // Subscribe to WebSocket connection state and connect immediately
  useEffect(() => {
    const unsubscribe = gameEngineAPI.onConnectionStateChange((connected) => {
      setEngineConnected(connected);
    });

    // Connect to engine WebSocket on mount
    gameEngineAPI.connect();

    return unsubscribe;
  }, [setEngineConnected]);

  // Component registry for panels
  const components = {
    sceneHierarchy: SceneHierarchyPanel,
    componentInspector: ComponentInspectorPanel,
    animationEditor: AnimationEditorPanel,
  };

  // Check if a panel is currently open
  const isPanelOpen = useCallback((panelId: string): boolean => {
    if (!api) return false;
    try {
      return api.getPanel(panelId) !== undefined;
    } catch {
      return false;
    }
  }, [api]);

  // Open a panel
  const openPanel = useCallback((panelId: string, component: string, title: string) => {
    console.log('[EditorLayout] openPanel called:', { panelId, component, title, hasApi: !!api });
    if (!api) return;

    // Check if panel already exists
    const existingPanel = api.getPanel(panelId);
    if (existingPanel) {
      // Panel exists, just focus it
      existingPanel.api.setActive();
      return;
    }

    // Add the panel
    api.addPanel({
      id: panelId,
      component,
      title,
    });
  }, [api]);

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

  const connectionChipClass = engineConnected
    ? styles.connectionChipConnected
    : styles.connectionChipDisconnected;

  const connectionIconClass = engineConnected
    ? styles.connectionIconConnected
    : styles.connectionIconDisconnected;

  return (
    <div className={styles.container}>
      {/* Toolbar */}
      <div className={styles.toolbar}>
        <div className={styles.toolbarLeft}>
          <span>nodec Game Editor</span>
          <Menu
            trigger={
              <button className={styles.viewButton}>
                <ViewIcon />
                View
              </button>
            }
            placement="bottom-start"
          >
            {PANEL_DEFINITIONS.map((panel) => (
              <Menu.Item
                key={panel.id}
                onClick={() => openPanel(panel.id, panel.component, panel.title)}
                disabled={isPanelOpen(panel.id)}
                className={isPanelOpen(panel.id) ? styles.menuItemDisabled : styles.menuItem}
              >
                {panel.title}
              </Menu.Item>
            ))}
          </Menu>
        </div>
        <Chip
          icon={<CircleIcon className={`${styles.connectionIcon} ${connectionIconClass}`} />}
          className={`${styles.connectionChip} ${connectionChipClass}`}
        >
          {engineConnected ? 'Engine Connected' : 'Engine Disconnected'}
        </Chip>
      </div>

      {/* Dockview container */}
      <div className={styles.dockviewContainer}>
        <DockviewReact
          components={components}
          onReady={onReady}
          className="dockview-theme-dark"
          disableFloatingGroups={false}
          rightHeaderActionsComponent={RightHeaderActions}
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
