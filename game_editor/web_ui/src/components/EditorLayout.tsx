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
import { IconButton, Tooltip, Chip, Button, Menu, MenuItem } from '@mui/material';
import {
  PictureInPictureAlt as FloatIcon,
  Circle as CircleIcon,
  ViewQuilt as ViewIcon,
} from '@mui/icons-material';
import { SceneHierarchyPanel } from './panels/SceneHierarchyPanel';
import { ComponentInspectorPanel } from './panels/ComponentInspectorPanel';
import { AnimationEditorPanel } from './panels/AnimationEditorPanel';
import { EditorProvider, useEditor } from '../contexts/EditorContext';

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
        sx={{
          color: '#cccccc',
          padding: '2px',
          '&:hover': { backgroundColor: 'rgba(255,255,255,0.1)' }
        }}
      >
        <FloatIcon fontSize="small" sx={{ fontSize: 16 }} />
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
  const { engineConnected } = useEditor();
  const [api, setApi] = useState<DockviewReadyEvent['api'] | null>(null);
  const [viewMenuAnchor, setViewMenuAnchor] = useState<null | HTMLElement>(null);

  // Set browser tab title
  useEffect(() => {
    document.title = 'nodec Game Editor';
  }, []);

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

    setViewMenuAnchor(null);
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

  return (
    <div style={{ 
      height: '100vh', 
      width: '100vw',
      display: 'flex',
      flexDirection: 'column',
      backgroundColor: '#1e1e1e'
    }}>
      {/* Toolbar */}
      <div style={{
        height: '35px',
        backgroundColor: '#2d2d30',
        borderBottom: '1px solid #3e3e42',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        padding: '0 12px',
        color: '#cccccc',
        fontSize: '13px',
        fontWeight: 500
      }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
          <span>nodec Game Editor</span>
          <Button
            size="small"
            startIcon={<ViewIcon sx={{ fontSize: 16 }} />}
            onClick={(e) => setViewMenuAnchor(e.currentTarget)}
            sx={{
              color: '#cccccc',
              textTransform: 'none',
              fontSize: '12px',
              minWidth: 'auto',
              padding: '2px 8px',
              '&:hover': { backgroundColor: 'rgba(255,255,255,0.1)' }
            }}
          >
            View
          </Button>
          <Menu
            anchorEl={viewMenuAnchor}
            open={Boolean(viewMenuAnchor)}
            onClose={() => setViewMenuAnchor(null)}
            sx={{
              '& .MuiPaper-root': {
                backgroundColor: '#2d2d30',
                color: '#cccccc',
                border: '1px solid #3e3e42',
              }
            }}
          >
            {PANEL_DEFINITIONS.map((panel) => (
              <MenuItem
                key={panel.id}
                onClick={() => openPanel(panel.id, panel.component, panel.title)}
                disabled={isPanelOpen(panel.id)}
                sx={{
                  fontSize: '13px',
                  '&:hover': { backgroundColor: 'rgba(255,255,255,0.1)' },
                  '&.Mui-disabled': { color: '#666666' }
                }}
              >
                {panel.title}
              </MenuItem>
            ))}
          </Menu>
        </div>
        <Chip
          icon={<CircleIcon sx={{ fontSize: 10 }} />}
          label={engineConnected ? 'Engine Connected' : 'Engine Disconnected'}
          size="small"
          sx={{
            height: 22,
            backgroundColor: engineConnected ? 'rgba(76, 175, 80, 0.2)' : 'rgba(244, 67, 54, 0.2)',
            color: engineConnected ? '#81c784' : '#e57373',
            border: `1px solid ${engineConnected ? '#4caf50' : '#f44336'}`,
            '& .MuiChip-icon': {
              color: engineConnected ? '#4caf50' : '#f44336',
            },
            '& .MuiChip-label': {
              fontSize: '11px',
              px: 1,
            },
          }}
        />
      </div>

      {/* Dockview container */}
      <div style={{ flex: 1, overflow: 'hidden' }}>
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