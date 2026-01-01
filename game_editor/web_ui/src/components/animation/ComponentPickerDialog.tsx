'use client';

import React, { useState, useEffect } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Typography,
  Spinner,
  Alert,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  TextField,
} from '@/ui';
import { ExtensionIcon, SearchIcon } from '@/ui/icons';
import {
  gameEngineAPI,
  RegisteredComponent,
  AnimatedComponentPlaceholder,
} from '../../api/gameEngine';
import styles from './ComponentPickerDialog.module.css';

interface ComponentPickerDialogProps {
  open: boolean;
  onClose: () => void;
  onSelect: (placeholder: AnimatedComponentPlaceholder, displayName: string) => void;
  /** Component type names to exclude (already added) */
  excludeComponentNames?: string[];
}

/**
 * Extract component name from polymorphic_name
 * e.g., "nodec_rendering::components::SerializableImageRenderer" -> "SerializableImageRenderer"
 */
function getComponentDisplayName(placeholder: AnimatedComponentPlaceholder): string {
  const fullName = placeholder.polymorphic_name;
  if (!fullName) {
    return `Component[${placeholder.polymorphic_id}]`;
  }

  // Extract last part after "::"
  return fullName.split('::').pop() || fullName;
}

export const ComponentPickerDialog: React.FC<ComponentPickerDialogProps> = ({
  open,
  onClose,
  onSelect,
  excludeComponentNames = [],
}) => {
  const [components, setComponents] = useState<RegisteredComponent[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedComponent, setSelectedComponent] = useState<RegisteredComponent | null>(null);

  // Fetch registered components when dialog opens
  useEffect(() => {
    if (!open) {
      setSelectedComponent(null);
      setSearchQuery('');
      return;
    }

    const fetchComponents = async () => {
      try {
        setLoading(true);
        setError(null);
        const registeredComponents = await gameEngineAPI.getRegisteredComponents();
        setComponents(registeredComponents);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch components');
      } finally {
        setLoading(false);
      }
    };

    fetchComponents();
  }, [open]);

  // Filter components by search query
  const filteredComponents = components.filter(comp => {
    const displayName = getComponentDisplayName(comp.data.component);
    const isExcluded = excludeComponentNames.includes(displayName);
    if (isExcluded) return false;

    if (!searchQuery) return true;
    return displayName.toLowerCase().includes(searchQuery.toLowerCase());
  });

  // Handle confirm
  const handleConfirm = () => {
    if (selectedComponent) {
      const displayName = getComponentDisplayName(selectedComponent.data.component);
      onSelect(selectedComponent.data.component, displayName);
      onClose();
    }
  };

  return (
    <Dialog open={open} onClose={onClose}>
      <DialogTitle>Select Component Type</DialogTitle>
      <DialogContent dividers>
        {/* Search input */}
        <div className={styles.searchContainer}>
          <TextField
            fullWidth
            size="small"
            placeholder="Search components..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            leadingIcon={<SearchIcon size={18} />}
          />
        </div>

        {loading && (
          <div className={styles.loadingContainer}>
            <Spinner />
          </div>
        )}

        {error && (
          <div className={styles.alertContainer}>
            <Alert severity="error">{error}</Alert>
          </div>
        )}

        {!loading && !error && filteredComponents.length === 0 && (
          <Typography color="onSurfaceVariant" className={styles.emptyMessage}>
            {searchQuery ? 'No matching components found' : 'No components available'}
          </Typography>
        )}

        {!loading && !error && filteredComponents.length > 0 && (
          <List dense className={styles.listContainer}>
            {filteredComponents.map((comp, index) => {
              const displayName = getComponentDisplayName(comp.data.component);
              const isSelected = selectedComponent === comp;

              return (
                <ListItem key={index}>
                  <ListItemButton
                    selected={isSelected}
                    onClick={() => setSelectedComponent(comp)}
                  >
                    <ListItemIcon className={styles.componentIcon}>
                      <ExtensionIcon />
                    </ListItemIcon>
                    <ListItemText primary={displayName} />
                  </ListItemButton>
                </ListItem>
              );
            })}
          </List>
        )}

        {selectedComponent && (
          <div className={styles.selectedContainer}>
            <Typography variant="bodySmall">
              Selected: <strong>{getComponentDisplayName(selectedComponent.data.component)}</strong>
            </Typography>
          </div>
        )}
      </DialogContent>
      <DialogActions>
        <Button variant="text" onClick={onClose}>Cancel</Button>
        <Button
          variant="filled"
          onClick={handleConfirm}
          disabled={!selectedComponent}
        >
          Add Component
        </Button>
      </DialogActions>
    </Dialog>
  );
};
