'use client';

import React, { useState, useMemo } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Typography,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  TextField,
  InputAdornment,
} from '@mui/material';
import {
  Timeline as PropertyIcon,
  Search as SearchIcon,
} from '@mui/icons-material';

interface PropertyPickerDialogProps {
  open: boolean;
  onClose: () => void;
  onSelect: (propertyPath: string) => void;
  /** Component data to extract properties from */
  componentData: Record<string, unknown>;
  /** Property paths to exclude (already added) */
  excludePropertyPaths?: string[];
}

/**
 * Recursively extract animatable property paths from component data
 * Only extracts numeric fields (which can be animated)
 */
function extractPropertyPaths(
  data: Record<string, unknown>,
  prefix: string = ''
): string[] {
  const paths: string[] = [];

  for (const [key, value] of Object.entries(data)) {
    const currentPath = prefix ? `${prefix}.${key}` : key;

    if (typeof value === 'number') {
      // Numeric field - can be animated
      paths.push(currentPath);
    } else if (typeof value === 'object' && value !== null && !Array.isArray(value)) {
      // Nested object - recurse
      paths.push(...extractPropertyPaths(value as Record<string, unknown>, currentPath));
    }
    // Skip arrays, strings, booleans, etc. as they cannot be animated
  }

  return paths;
}

export const PropertyPickerDialog: React.FC<PropertyPickerDialogProps> = ({
  open,
  onClose,
  onSelect,
  componentData,
  excludePropertyPaths = [],
}) => {
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedProperty, setSelectedProperty] = useState<string | null>(null);

  // Reset state when dialog closes
  React.useEffect(() => {
    if (!open) {
      setSelectedProperty(null);
      setSearchQuery('');
    }
  }, [open]);

  // Extract all animatable property paths from component data
  const allProperties = useMemo(() => {
    return extractPropertyPaths(componentData);
  }, [componentData]);

  // Filter out excluded and apply search
  const filteredProperties = useMemo(() => {
    return allProperties.filter(path => {
      if (excludePropertyPaths.includes(path)) return false;
      if (!searchQuery) return true;
      return path.toLowerCase().includes(searchQuery.toLowerCase());
    });
  }, [allProperties, excludePropertyPaths, searchQuery]);

  // Handle confirm
  const handleConfirm = () => {
    if (selectedProperty) {
      onSelect(selectedProperty);
      onClose();
    }
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle>Select Property to Animate</DialogTitle>
      <DialogContent dividers>
        {/* Search input */}
        <TextField
          fullWidth
          size="small"
          placeholder="Search properties..."
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

        {filteredProperties.length === 0 && (
          <Typography color="text.secondary" align="center" sx={{ py: 4 }}>
            {searchQuery
              ? 'No matching properties found'
              : allProperties.length === 0
                ? 'No animatable properties in this component'
                : 'All properties already added'}
          </Typography>
        )}

        {filteredProperties.length > 0 && (
          <List dense sx={{ maxHeight: 400, overflow: 'auto' }}>
            {filteredProperties.map((path) => {
              const isSelected = selectedProperty === path;

              return (
                <ListItem key={path} disablePadding>
                  <ListItemButton
                    selected={isSelected}
                    onClick={() => setSelectedProperty(path)}
                  >
                    <ListItemIcon sx={{ minWidth: 32 }}>
                      <PropertyIcon fontSize="small" color="action" />
                    </ListItemIcon>
                    <ListItemText
                      primary={path}
                      primaryTypographyProps={{ fontSize: '0.875rem', fontFamily: 'monospace' }}
                    />
                  </ListItemButton>
                </ListItem>
              );
            })}
          </List>
        )}

        {selectedProperty && (
          <Box sx={{ mt: 2, p: 1, backgroundColor: 'action.selected', borderRadius: 1 }}>
            <Typography variant="body2">
              Selected: <strong style={{ fontFamily: 'monospace' }}>{selectedProperty}</strong>
            </Typography>
          </Box>
        )}
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>Cancel</Button>
        <Button
          variant="contained"
          onClick={handleConfirm}
          disabled={!selectedProperty}
        >
          Add Property
        </Button>
      </DialogActions>
    </Dialog>
  );
};
