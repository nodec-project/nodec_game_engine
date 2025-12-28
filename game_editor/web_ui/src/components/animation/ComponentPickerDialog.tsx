'use client';

import React, { useState, useEffect } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Typography,
  CircularProgress,
  Alert,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  TextField,
  InputAdornment,
} from '@mui/material';
import {
  Extension as ComponentIcon,
  Search as SearchIcon,
} from '@mui/icons-material';
import {
  gameEngineAPI,
  RegisteredComponent,
  AnimatedComponentPlaceholder,
} from '../../api/gameEngine';

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
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle>Select Component Type</DialogTitle>
      <DialogContent dividers>
        {/* Search input */}
        <TextField
          fullWidth
          size="small"
          placeholder="Search components..."
          value={searchQuery}
          onChange={(e) => setSearchQuery(e.target.value)}
          sx={{ mb: 2 }}
          InputProps={{
            startAdornment: (
              <InputAdornment position="start">
                <SearchIcon fontSize="small" />
              </InputAdornment>
            ),
          }}
        />

        {loading && (
          <Box sx={{ display: 'flex', justifyContent: 'center', p: 4 }}>
            <CircularProgress />
          </Box>
        )}

        {error && (
          <Alert severity="error" sx={{ mb: 2 }}>
            {error}
          </Alert>
        )}

        {!loading && !error && filteredComponents.length === 0 && (
          <Typography color="text.secondary" align="center" sx={{ py: 4 }}>
            {searchQuery ? 'No matching components found' : 'No components available'}
          </Typography>
        )}

        {!loading && !error && filteredComponents.length > 0 && (
          <List dense sx={{ maxHeight: 400, overflow: 'auto' }}>
            {filteredComponents.map((comp, index) => {
              const displayName = getComponentDisplayName(comp.data.component);
              const isSelected = selectedComponent === comp;

              return (
                <ListItem key={index} disablePadding>
                  <ListItemButton
                    selected={isSelected}
                    onClick={() => setSelectedComponent(comp)}
                  >
                    <ListItemIcon sx={{ minWidth: 32 }}>
                      <ComponentIcon fontSize="small" color="primary" />
                    </ListItemIcon>
                    <ListItemText
                      primary={displayName}
                      primaryTypographyProps={{ fontSize: '0.875rem' }}
                    />
                  </ListItemButton>
                </ListItem>
              );
            })}
          </List>
        )}

        {selectedComponent && (
          <Box sx={{ mt: 2, p: 1, backgroundColor: 'action.selected', borderRadius: 1 }}>
            <Typography variant="body2">
              Selected: <strong>{getComponentDisplayName(selectedComponent.data.component)}</strong>
            </Typography>
          </Box>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>Cancel</Button>
        <Button
          variant="contained"
          onClick={handleConfirm}
          disabled={!selectedComponent}
        >
          Add Component
        </Button>
      </DialogActions>
    </Dialog>
  );
};
