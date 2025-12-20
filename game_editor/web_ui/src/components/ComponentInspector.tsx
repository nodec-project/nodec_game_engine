'use client';

import React, { useState, useEffect, useCallback } from 'react';
import {
  Box,
  Typography,
  Paper,
  Alert,
  CircularProgress,
  Divider,
  Accordion,
  AccordionSummary,
  AccordionDetails,
  Chip,
  TextField,
  Grid,
} from '@mui/material';
import {
  ExpandMore as ExpandMoreIcon,
  Settings as ComponentIcon,
} from '@mui/icons-material';
import { gameEngineAPI, ComponentInfo, EntityDetailsResponse, SerializableComponent } from '../api/gameEngine';

// Helper to extract component display name from polymorphic_name
const getComponentDisplayName = (polymorphicName: string | undefined): string => {
  if (!polymorphicName) return 'Unknown Component';
  // Extract last part after :: (e.g., "SerializableLocalTransform" from "nodec_scene::components::SerializableLocalTransform")
  const parts = polymorphicName.split('::');
  return parts[parts.length - 1];
};

// Helper to determine if a value is a primitive that can be edited
const isPrimitiveValue = (value: unknown): boolean => {
  return typeof value === 'string' || typeof value === 'number' || typeof value === 'boolean';
};

// Helper to parse input value back to the appropriate type
const parseInputValue = (value: string, originalValue: unknown): unknown => {
  if (typeof originalValue === 'number') {
    const parsed = parseFloat(value);
    return isNaN(parsed) ? originalValue : parsed;
  }
  if (typeof originalValue === 'boolean') {
    return value.toLowerCase() === 'true';
  }
  return value;
};

interface ComponentInspectorProps {
  entityId: string | null;
}

// Props for the editable property field
interface EditablePropertyFieldProps {
  label: string;
  value: unknown;
  onChange: (newValue: unknown) => void;
  onCommit: () => void;
}

const EditablePropertyField: React.FC<EditablePropertyFieldProps> = ({
  label,
  value,
  onChange,
  onCommit,
}) => {
  const [localValue, setLocalValue] = useState(String(value));
  const [isDirty, setIsDirty] = useState(false);

  // Update local value when prop changes (but not if we're editing)
  useEffect(() => {
    if (!isDirty) {
      setLocalValue(String(value));
    }
  }, [value, isDirty]);

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setLocalValue(e.target.value);
    setIsDirty(true);
    onChange(parseInputValue(e.target.value, value));
  };

  const handleBlur = () => {
    if (isDirty) {
      onCommit();
      setIsDirty(false);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter') {
      if (isDirty) {
        onCommit();
        setIsDirty(false);
      }
      (e.target as HTMLInputElement).blur();
    }
  };

  return (
    <TextField
      label={label}
      size="small"
      value={localValue}
      onChange={handleChange}
      onBlur={handleBlur}
      onKeyDown={handleKeyDown}
      variant="outlined"
      fullWidth
      sx={{
        mt: 0.5,
        '& .MuiOutlinedInput-root': isDirty ? {
          '& fieldset': { borderColor: 'warning.main' },
        } : {},
      }}
    />
  );
};

export const ComponentInspector: React.FC<ComponentInspectorProps> = ({ entityId }) => {
  const [components, setComponents] = useState<ComponentInfo[]>([]);
  const [entityDetails, setEntityDetails] = useState<EntityDetailsResponse | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [patchError, setPatchError] = useState<string | null>(null);

  useEffect(() => {
    if (!entityId) {
      setComponents([]);
      setEntityDetails(null);
      return;
    }

    const fetchEntityData = async () => {
      try {
        setLoading(true);
        setError(null);
        setPatchError(null);

        // Fetch both entity details and components in parallel
        const [details, comps] = await Promise.all([
          gameEngineAPI.getEntityDetails(entityId),
          gameEngineAPI.getEntityComponents(entityId),
        ]);

        setEntityDetails(details);
        setComponents(comps);
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch entity data');
      } finally {
        setLoading(false);
      }
    };

    fetchEntityData();
  }, [entityId]);

  // Update a property in the component data and commit via PATCH
  const handlePropertyChange = useCallback((
    componentIndex: number,
    propertyPath: string[],
    newValue: unknown
  ) => {
    setComponents(prev => {
      const newComponents = [...prev];
      const component = { ...newComponents[componentIndex] };

      // Deep clone the data
      if (component.data) {
        const clonedData: Record<string, unknown> = JSON.parse(JSON.stringify(component.data));
        component.data = clonedData;

        // Navigate to the property and update it
        // Path is like: component.ptr_wrapper.data.{propertyKey}
        let current: Record<string, unknown> = clonedData;
        for (let i = 0; i < propertyPath.length - 1; i++) {
          current = current[propertyPath[i]] as Record<string, unknown>;
        }
        current[propertyPath[propertyPath.length - 1]] = newValue;
      }

      newComponents[componentIndex] = component;
      return newComponents;
    });
  }, []);

  // Commit the component changes via PATCH
  const handleCommit = useCallback(async (componentIndex: number) => {
    if (!entityId) return;

    const component = components[componentIndex];
    const serializableComponent = gameEngineAPI.extractSerializableComponent(component);

    if (!serializableComponent) {
      console.warn('Cannot extract serializable component for PATCH');
      return;
    }

    try {
      setPatchError(null);
      const result = await gameEngineAPI.patchEntityComponents(entityId, [serializableComponent]);

      if (!result.success) {
        setPatchError(result.error || 'Failed to update component');
      }
    } catch (err) {
      setPatchError(err instanceof Error ? err.message : 'Failed to update component');
    }
  }, [entityId, components]);

  // Render editable properties for ptr_wrapper.data
  const renderEditableData = (
    data: Record<string, unknown>,
    componentIndex: number,
    basePath: string[] = []
  ): React.ReactNode => {
    return (
      <Box sx={{ pl: basePath.length > 0 ? 2 : 0 }}>
        {Object.entries(data).map(([key, value]) => {
          const currentPath = [...basePath, key];

          if (value === null || value === undefined) {
            return (
              <Box key={key} sx={{ mb: 1 }}>
                <Typography variant="caption" color="text.secondary">
                  {key}: <em>null</em>
                </Typography>
              </Box>
            );
          }

          if (isPrimitiveValue(value)) {
            return (
              <Box key={key} sx={{ mb: 1 }}>
                <EditablePropertyField
                  label={key}
                  value={value}
                  onChange={(newValue) => handlePropertyChange(
                    componentIndex,
                    ['component', 'ptr_wrapper', 'data', ...currentPath],
                    newValue
                  )}
                  onCommit={() => handleCommit(componentIndex)}
                />
              </Box>
            );
          }

          if (Array.isArray(value)) {
            return (
              <Box key={key} sx={{ mb: 1 }}>
                <Typography variant="caption" color="text.secondary">
                  {key}: [{value.length} items]
                </Typography>
                <Box sx={{ pl: 2 }}>
                  {value.map((item, index) => (
                    <Box key={index} sx={{ mb: 0.5 }}>
                      {isPrimitiveValue(item) ? (
                        <EditablePropertyField
                          label={`[${index}]`}
                          value={item}
                          onChange={(newValue) => {
                            const newArray = [...value];
                            newArray[index] = newValue;
                            handlePropertyChange(
                              componentIndex,
                              ['component', 'ptr_wrapper', 'data', ...currentPath],
                              newArray
                            );
                          }}
                          onCommit={() => handleCommit(componentIndex)}
                        />
                      ) : (
                        <Typography variant="caption" color="text.secondary">
                          [{index}]: {typeof item === 'object' ? JSON.stringify(item) : String(item)}
                        </Typography>
                      )}
                    </Box>
                  ))}
                </Box>
              </Box>
            );
          }

          if (typeof value === 'object') {
            return (
              <Box key={key} sx={{ mb: 1 }}>
                <Typography variant="caption" color="text.secondary" sx={{ fontWeight: 'bold' }}>
                  {key}:
                </Typography>
                {renderEditableData(value as Record<string, unknown>, componentIndex, currentPath)}
              </Box>
            );
          }

          return null;
        })}
      </Box>
    );
  };

  // Render a serializable component with editable properties
  const renderSerializableComponent = (
    component: ComponentInfo,
    componentIndex: number
  ): React.ReactNode => {
    const serializableComponent = gameEngineAPI.extractSerializableComponent(component);

    if (!serializableComponent) {
      // Fall back to read-only display for non-serializable components
      return renderReadOnlyData(component.data);
    }

    const displayName = getComponentDisplayName(serializableComponent.polymorphic_name);
    const editableData = serializableComponent.ptr_wrapper?.data || {};

    return (
      <Accordion key={componentIndex} defaultExpanded={componentIndex === 0}>
        <AccordionSummary expandIcon={<ExpandMoreIcon />}>
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
            <ComponentIcon fontSize="small" />
            <Typography>{displayName}</Typography>
            <Chip
              label={`Type: ${component.type_index}`}
              size="small"
              variant="outlined"
            />
          </Box>
        </AccordionSummary>
        <AccordionDetails>
          {Object.keys(editableData).length > 0 ? (
            renderEditableData(editableData, componentIndex)
          ) : (
            <Typography variant="body2" color="text.secondary">
              No editable properties
            </Typography>
          )}
        </AccordionDetails>
      </Accordion>
    );
  };

  // Render read-only data for non-serializable components
  const renderReadOnlyData = (data: unknown): React.ReactNode => {
    if (data === null || data === undefined) {
      return <Typography variant="body2" color="text.secondary">No data</Typography>;
    }

    if (typeof data === 'object' && !Array.isArray(data)) {
      return (
        <Box sx={{ pl: 2 }}>
          {Object.entries(data).map(([key, value]) => (
            <Box key={key} sx={{ mb: 1 }}>
              <Typography variant="caption" color="text.secondary">
                {key}:
              </Typography>
              <Box sx={{ pl: 2 }}>
                {typeof value === 'object' ? (
                  renderReadOnlyData(value)
                ) : (
                  <TextField
                    size="small"
                    value={String(value)}
                    variant="outlined"
                    fullWidth
                    disabled
                    sx={{ mt: 0.5 }}
                  />
                )}
              </Box>
            </Box>
          ))}
        </Box>
      );
    }

    if (Array.isArray(data)) {
      return (
        <Box sx={{ pl: 2 }}>
          {data.map((item, index) => (
            <Box key={index} sx={{ mb: 1 }}>
              <Typography variant="caption" color="text.secondary">
                [{index}]
              </Typography>
              <Box sx={{ pl: 2 }}>
                {renderReadOnlyData(item)}
              </Box>
            </Box>
          ))}
        </Box>
      );
    }

    return (
      <TextField
        size="small"
        value={String(data)}
        variant="outlined"
        fullWidth
        disabled
      />
    );
  };

  if (!entityId) {
    return (
      <Paper sx={{ height: '100%', p: 2 }}>
        <Alert severity="info">
          Select an entity to view its components
        </Alert>
      </Paper>
    );
  }

  return (
    <Paper sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
      {entityDetails && (
        <Box sx={{ px: 2, py: 1, borderBottom: 1, borderColor: 'divider' }}>
          <Typography variant="subtitle2" color="text.secondary">
            Entity: {entityDetails.name}
          </Typography>
          <Typography variant="caption" color="text.secondary">
            ID: {entityDetails.id}
          </Typography>
        </Box>
      )}

      <Box sx={{ flex: 1, overflow: 'auto', p: 2 }}>
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

        {!loading && !error && entityDetails && (
          <>
            {/* Entity Details */}
            <Box sx={{ mb: 3 }}>
              <Typography variant="subtitle1" gutterBottom>
                Entity Properties
              </Typography>
              <Grid container spacing={2}>
                <Grid item xs={12}>
                  <TextField
                    label="Name"
                    value={entityDetails.name}
                    variant="outlined"
                    size="small"
                    fullWidth
                    disabled
                  />
                </Grid>
                {entityDetails.hierarchy?.parent !== null && entityDetails.hierarchy?.parent !== undefined && (
                  <Grid item xs={12}>
                    <TextField
                      label="Parent ID"
                      value={entityDetails.hierarchy.parent}
                      variant="outlined"
                      size="small"
                      fullWidth
                      disabled
                    />
                  </Grid>
                )}
                {entityDetails.hierarchy?.children && entityDetails.hierarchy.children.length > 0 && (
                  <Grid item xs={12}>
                    <Typography variant="caption" color="text.secondary">
                      Children: {entityDetails.hierarchy.children.length}
                    </Typography>
                  </Grid>
                )}
              </Grid>
            </Box>

            <Divider sx={{ my: 2 }} />

            {/* Components */}
            <Box>
              <Typography variant="subtitle1" gutterBottom>
                Components ({components.length})
              </Typography>

              {patchError && (
                <Alert severity="error" sx={{ mb: 2 }} onClose={() => setPatchError(null)}>
                  {patchError}
                </Alert>
              )}

              {components.length === 0 ? (
                <Alert severity="info">
                  No components found for this entity
                </Alert>
              ) : (
                components.map((component, index) =>
                  renderSerializableComponent(component, index)
                )
              )}
            </Box>
          </>
        )}
      </Box>
    </Paper>
  );
};