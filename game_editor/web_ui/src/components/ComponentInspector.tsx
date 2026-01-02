'use client';

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  Surface,
  Alert,
  Spinner,
  Divider,
  Accordion,
  AccordionItem,
  AccordionHeader,
  AccordionPanel,
  Chip,
  TextField,
  Typography,
  Icon,
  IconButton,
  Button,
  Menu,
  MenuItem,
} from '@/ui';
import { gameEngineAPI, ComponentInfo, EntityDetailsResponse, SerializableComponent, RegisteredComponent } from '../api/gameEngine';
import styles from './ComponentInspector.module.css';

// Helper to extract component display name from polymorphic_name
const getComponentDisplayName = (polymorphicName: string | undefined): string => {
  if (!polymorphicName) return 'Unknown Component';
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
      dirty={isDirty}
    />
  );
};

export const ComponentInspector: React.FC<ComponentInspectorProps> = ({ entityId }) => {
  const [components, setComponents] = useState<ComponentInfo[]>([]);
  const [entityDetails, setEntityDetails] = useState<EntityDetailsResponse | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [patchError, setPatchError] = useState<string | null>(null);
  const [isLiveUpdating, setIsLiveUpdating] = useState(false);
  const unsubscribeRef = useRef<(() => void) | null>(null);
  const isDirtyRef = useRef(false);
  const [expandedAccordions, setExpandedAccordions] = useState<string[]>(['component-0']);
  const [registeredComponents, setRegisteredComponents] = useState<RegisteredComponent[]>([]);
  const [componentSearchQuery, setComponentSearchQuery] = useState('');

  // Load registered components from cache when connection state changes
  useEffect(() => {
    const updateRegisteredComponents = () => {
      const cached = gameEngineAPI.getRegisteredComponents();
      if (cached) {
        setRegisteredComponents(cached);
      }
    };

    // Initial load
    updateRegisteredComponents();

    // Subscribe to connection state changes
    const unsubscribe = gameEngineAPI.onConnectionStateChange((connected) => {
      if (connected) {
        updateRegisteredComponents();
      }
    });

    return unsubscribe;
  }, []);

  useEffect(() => {
    if (entityId === null) {
      setComponents([]);
      setEntityDetails(null);
      return;
    }

    const fetchEntityData = async () => {
      try {
        setLoading(true);
        setError(null);
        setPatchError(null);

        const [details, comps] = await Promise.all([
          gameEngineAPI.getEntityDetails(entityId),
          gameEngineAPI.getEntityComponents(entityId),
        ]);

        setEntityDetails(details);
        setComponents(comps);
        console.log(comps)
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch entity data');
      } finally {
        setLoading(false);
      }
    };

    fetchEntityData();
  }, [entityId]);

  useEffect(() => {
    if (unsubscribeRef.current) {
      unsubscribeRef.current();
      unsubscribeRef.current = null;
    }

    if (entityId === null) {
      setIsLiveUpdating(false);
      return;
    }

    const subscribe = async () => {
      try {
        const unsubscribe = await gameEngineAPI.subscribeToEntityComponents(
          entityId,
          (newComponents) => {
            if (!isDirtyRef.current) {
              setComponents(newComponents);
            }
          }
        );
        unsubscribeRef.current = unsubscribe;
        setIsLiveUpdating(true);
      } catch (err) {
        console.error('Failed to subscribe to entity updates:', err);
        setIsLiveUpdating(false);
      }
    };

    subscribe();

    return () => {
      if (unsubscribeRef.current) {
        unsubscribeRef.current();
        unsubscribeRef.current = null;
      }
      setIsLiveUpdating(false);
    };
  }, [entityId]);

  const handlePropertyChange = useCallback((
    componentIndex: number,
    propertyPath: string[],
    newValue: unknown
  ) => {
    isDirtyRef.current = true;

    setComponents(prev => {
      const newComponents = [...prev];
      const component = { ...newComponents[componentIndex] };

      if (component.data) {
        const clonedData: Record<string, unknown> = JSON.parse(JSON.stringify(component.data));
        component.data = clonedData;

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

  const handleCommit = useCallback(async (componentIndex: number) => {
    if (entityId === null) return;

    const component = components[componentIndex];
    const serializableComponent = gameEngineAPI.extractSerializableComponent(component);

    if (!serializableComponent) {
      console.warn('Cannot extract serializable component for PATCH');
      isDirtyRef.current = false;
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
    } finally {
      isDirtyRef.current = false;
    }
  }, [entityId, components]);

  const handleRemoveComponent = useCallback(async (typeIndex: number) => {
    if (entityId === null) return;

    try {
      setPatchError(null);
      const result = await gameEngineAPI.removeEntityComponent(entityId, typeIndex);

      if (!result.success) {
        setPatchError(result.error || 'Failed to remove component');
      } else {
        // Refresh components after removal
        const comps = await gameEngineAPI.getEntityComponents(entityId);
        setComponents(comps);
      }
    } catch (err) {
      setPatchError(err instanceof Error ? err.message : 'Failed to remove component');
    }
  }, [entityId]);

  const handleAddComponent = useCallback(async (registeredComponent: RegisteredComponent) => {
    if (entityId === null) return;

    try {
      setPatchError(null);

      const component = registeredComponent.data.component;
      const result = await gameEngineAPI.addEntityComponent(entityId, component);

      if (!result.success) {
        setPatchError(result.error || 'Failed to add component');
      } else {
        // Refresh components after addition
        const comps = await gameEngineAPI.getEntityComponents(entityId);
        setComponents(comps);
      }
    } catch (err) {
      setPatchError(err instanceof Error ? err.message : 'Failed to add component');
    }
  }, [entityId]);

  const renderEditableData = (
    data: Record<string, unknown>,
    componentIndex: number,
    basePath: string[] = []
  ): React.ReactNode => {
    return (
      <div className={basePath.length > 0 ? styles.nestedData : undefined}>
        {Object.entries(data).map(([key, value]) => {
          const currentPath = [...basePath, key];

          if (value === null || value === undefined) {
            return (
              <div key={key} className={styles.fieldContainer}>
                <Typography variant="body-small" color="secondary">
                  {key}: <em>null</em>
                </Typography>
              </div>
            );
          }

          if (isPrimitiveValue(value)) {
            return (
              <div key={key} className={styles.fieldContainer}>
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
              </div>
            );
          }

          if (Array.isArray(value)) {
            return (
              <div key={key} className={styles.fieldContainer}>
                <Typography variant="body-small" color="secondary">
                  {key}: [{value.length} items]
                </Typography>
                <div className={styles.nestedData}>
                  {value.map((item, index) => (
                    <div key={index} className={styles.arrayItem}>
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
                        <Typography variant="body-small" color="secondary">
                          [{index}]: {typeof item === 'object' ? JSON.stringify(item) : String(item)}
                        </Typography>
                      )}
                    </div>
                  ))}
                </div>
              </div>
            );
          }

          if (typeof value === 'object') {
            return (
              <div key={key} className={styles.fieldContainer}>
                <Typography variant="body-small" color="secondary" className={styles.propertyLabelBold}>
                  {key}:
                </Typography>
                {renderEditableData(value as Record<string, unknown>, componentIndex, currentPath)}
              </div>
            );
          }

          return null;
        })}
      </div>
    );
  };

  const renderSerializableComponent = (
    component: ComponentInfo,
    componentIndex: number
  ): React.ReactNode => {
    const serializableComponent = gameEngineAPI.extractSerializableComponent(component);

    if (!serializableComponent) {
      return renderReadOnlyData(component.data);
    }

    const displayName = getComponentDisplayName(serializableComponent.polymorphic_name);
    const editableData = serializableComponent.ptr_wrapper?.data || {};

    return (
      <AccordionItem key={componentIndex} value={`component-${componentIndex}`}>
        <AccordionHeader expandIcon={<Icon name="expand_more" />}>
          <div className={styles.componentHeader}>
            <Icon name="settings" size={18} className={styles.componentIcon} />
            <span className={styles.componentName}>{displayName}</span>
            <Chip size="small" variant="outlined" className={styles.typeChip}>
              Type: {component.type_index}
            </Chip>
            <IconButton
              size="small"
              onClick={(e) => {
                e.stopPropagation();
                handleRemoveComponent(component.type_index);
              }}
              className={styles.removeButton}
            >
              <Icon name="delete" size={16} />
            </IconButton>
          </div>
        </AccordionHeader>
        <AccordionPanel>
          {Object.keys(editableData).length > 0 ? (
            renderEditableData(editableData, componentIndex)
          ) : (
            <Typography variant="body-medium" color="secondary">
              No editable properties
            </Typography>
          )}
        </AccordionPanel>
      </AccordionItem>
    );
  };

  const renderReadOnlyData = (data: unknown): React.ReactNode => {
    if (data === null || data === undefined) {
      return <Typography variant="body-medium" color="secondary">No data</Typography>;
    }

    if (typeof data === 'object' && !Array.isArray(data)) {
      return (
        <div className={styles.nestedData}>
          {Object.entries(data).map(([key, value]) => (
            <div key={key} className={styles.fieldContainer}>
              <Typography variant="body-small" color="secondary">
                {key}:
              </Typography>
              <div className={styles.nestedData}>
                {typeof value === 'object' ? (
                  renderReadOnlyData(value)
                ) : (
                  <TextField
                    size="small"
                    value={String(value)}
                    variant="outlined"
                    fullWidth
                    disabled
                  />
                )}
              </div>
            </div>
          ))}
        </div>
      );
    }

    if (Array.isArray(data)) {
      return (
        <div className={styles.nestedData}>
          {data.map((item, index) => (
            <div key={index} className={styles.fieldContainer}>
              <Typography variant="body-small" color="secondary">
                [{index}]
              </Typography>
              <div className={styles.nestedData}>
                {renderReadOnlyData(item)}
              </div>
            </div>
          ))}
        </div>
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

  if (entityId === null) {
    return (
      <Surface className={styles.noEntityContainer}>
        <Alert severity="info">
          Select an entity to view its components
        </Alert>
      </Surface>
    );
  }

  return (
    <Surface className={styles.container}>
      {entityDetails && (
        <div className={styles.header}>
          <div className={styles.entityInfo}>
            <span className={styles.entityName}>Entity: {entityDetails.name}</span>
            <span className={styles.entityId}>ID: {entityDetails.id}</span>
          </div>
          {isLiveUpdating && (
            <Chip
              icon={<Icon name="sync" size={14} className={styles.liveChipIcon} />}
              size="small"
              variant="outlined"
              className={styles.liveChip}
            >
              Live
            </Chip>
          )}
        </div>
      )}

      <div className={styles.content}>
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

        {!loading && !error && entityDetails && (
          <>
            {/* Entity Details */}
            <div className={styles.section}>
              <Typography variant="title-small" className={styles.sectionTitle}>
                Entity Properties
              </Typography>
              <div className={styles.formGrid}>
                <TextField
                  label="Name"
                  value={entityDetails.name}
                  variant="outlined"
                  size="small"
                  fullWidth
                  disabled
                />
                {entityDetails.hierarchy?.parent !== null && entityDetails.hierarchy?.parent !== undefined && (
                  <TextField
                    label="Parent ID"
                    value={String(entityDetails.hierarchy.parent)}
                    variant="outlined"
                    size="small"
                    fullWidth
                    disabled
                  />
                )}
                {entityDetails.hierarchy?.children && entityDetails.hierarchy.children.length > 0 && (
                  <span className={styles.childrenCount}>
                    Children: {entityDetails.hierarchy.children.length}
                  </span>
                )}
              </div>
            </div>

            <Divider className={styles.divider} />

            {/* Components */}
            <div className={styles.section}>
              <div className={styles.sectionHeader}>
                <Typography variant="title-small" className={styles.sectionTitle}>
                  Components ({components.length})
                </Typography>
                <Menu
                  trigger={
                    <Button
                      variant="outlined"
                      size="small"
                      startIcon={<Icon name="add" size={16} />}
                    >
                      Add
                    </Button>
                  }
                  placement="bottom-start"
                >
                  <div className={styles.componentMenuContainer}>
                    <div className={styles.componentSearchContainer}>
                      <TextField
                        size="small"
                        placeholder="Search components..."
                        value={componentSearchQuery}
                        onChange={(e) => setComponentSearchQuery(e.target.value)}
                        onKeyDown={(e) => e.stopPropagation()}
                        fullWidth
                        leadingIcon={<Icon name="search" size={16} />}
                      />
                    </div>
                    <div className={styles.componentMenuList}>
                      {registeredComponents.length === 0 ? (
                        <MenuItem disabled>No components available</MenuItem>
                      ) : (
                        registeredComponents
                          .filter((reg) => {
                            if (!componentSearchQuery) return true;
                            const name = getComponentDisplayName(reg.data.component.polymorphic_name);
                            return name.toLowerCase().includes(componentSearchQuery.toLowerCase());
                          })
                          .map((reg) => {
                            const name = getComponentDisplayName(reg.data.component.polymorphic_name);
                            return (
                              <MenuItem
                                key={reg.runtime_type_index}
                                onClick={() => handleAddComponent(reg)}
                              >
                                {name}
                              </MenuItem>
                            );
                          })
                      )}
                      {registeredComponents.length > 0 &&
                        componentSearchQuery &&
                        registeredComponents.filter((reg) => {
                          const name = getComponentDisplayName(reg.data.component.polymorphic_name);
                          return name.toLowerCase().includes(componentSearchQuery.toLowerCase());
                        }).length === 0 && (
                          <MenuItem disabled>No matching components</MenuItem>
                        )}
                    </div>
                  </div>
                </Menu>
              </div>

              {patchError && (
                <div className={styles.alertContainer}>
                  <Alert severity="error">{patchError}</Alert>
                </div>
              )}

              {components.length === 0 ? (
                <Alert severity="info">
                  No components found for this entity
                </Alert>
              ) : (
                <Accordion
                  value={expandedAccordions}
                  onValueChange={setExpandedAccordions}
                >
                  {components.map((component, index) =>
                    renderSerializableComponent(component, index)
                  )}
                </Accordion>
              )}
            </div>
          </>
        )}
      </div>
    </Surface>
  );
};
