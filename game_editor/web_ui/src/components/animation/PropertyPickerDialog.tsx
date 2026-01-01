'use client';

import React, { useState, useMemo } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Typography,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  TextField,
  Icon,
} from '@/ui';
import styles from './PropertyPickerDialog.module.css';

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
    <Dialog open={open} onClose={onClose}>
      <DialogTitle>Select Property to Animate</DialogTitle>
      <DialogContent dividers>
        {/* Search input */}
        <div className={styles.searchContainer}>
          <TextField
            fullWidth
            size="small"
            placeholder="Search properties..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            leadingIcon={<Icon name="search" size={18} />}
          />
        </div>

        {filteredProperties.length === 0 && (
          <Typography color="onSurfaceVariant" className={styles.emptyMessage}>
            {searchQuery
              ? 'No matching properties found'
              : allProperties.length === 0
                ? 'No animatable properties in this component'
                : 'All properties already added'}
          </Typography>
        )}

        {filteredProperties.length > 0 && (
          <List dense className={styles.listContainer}>
            {filteredProperties.map((path) => {
              const isSelected = selectedProperty === path;

              return (
                <ListItem key={path}>
                  <ListItemButton
                    selected={isSelected}
                    onClick={() => setSelectedProperty(path)}
                  >
                    <ListItemIcon className={styles.propertyIcon}>
                      <Icon name="timeline" />
                    </ListItemIcon>
                    <ListItemText
                      primary={<span className={styles.propertyPath}>{path}</span>}
                    />
                  </ListItemButton>
                </ListItem>
              );
            })}
          </List>
        )}

        {selectedProperty && (
          <div className={styles.selectedContainer}>
            <Typography variant="bodySmall">
              Selected: <strong className={styles.propertyPath}>{selectedProperty}</strong>
            </Typography>
          </div>
        )}
      </DialogContent>
      <DialogActions>
        <Button variant="text" onClick={onClose}>Cancel</Button>
        <Button
          variant="filled"
          onClick={handleConfirm}
          disabled={!selectedProperty}
        >
          Add Property
        </Button>
      </DialogActions>
    </Dialog>
  );
};
