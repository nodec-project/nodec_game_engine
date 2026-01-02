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
import { IconButton, Tooltip, Chip, Menu, Icon } from '@/ui';
import { SceneHierarchyPanel } from './panels/SceneHierarchyPanel';
import { ComponentInspectorPanel } from './panels/ComponentInspectorPanel';
import { AnimationEditorPanel } from './panels/AnimationEditorPanel';
import { EditorProvider, useEditor } from '../contexts/EditorContext';
import { gameEngineAPI } from '../api/gameEngine';
import styles from './EditorLayout.module.css';

// Import dockview styles
import '../styles/dockview-theme.css';

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
        <Icon name="picture_in_picture" size={18} />
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
                <Icon name="view_quilt" size={18} />
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
          icon={<Icon name="circle" size={10} fill className={`${styles.connectionIcon} ${connectionIconClass}`} />}
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
