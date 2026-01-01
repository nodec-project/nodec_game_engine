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
} from '@/ui';
import { gameEngineAPI, ComponentInfo, EntityDetailsResponse, SerializableComponent } from '../api/gameEngine';
import styles from './ComponentInspector.module.css';

// Icons
const ExpandMoreIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor" width="24" height="24">
    <path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" />
  </svg>
);

const ComponentIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor" className={styles.componentIcon}>
    <path d="M19.14 12.94c.04-.31.06-.63.06-.94 0-.31-.02-.63-.06-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.04.31-.06.63-.06.94s.02.63.06.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z" />
  </svg>
);

const SyncIcon = ({ className }: { className?: string }) => (
  <svg viewBox="0 0 24 24" fill="currentColor" className={className}>
    <path d="M12 4V1L8 5l4 4V6c3.31 0 6 2.69 6 6 0 1.01-.25 1.97-.7 2.8l1.46 1.46C19.54 15.03 20 13.57 20 12c0-4.42-3.58-8-8-8zm0 14c-3.31 0-6-2.69-6-6 0-1.01.25-1.97.7-2.8L5.24 7.74C4.46 8.97 4 10.43 4 12c0 4.42 3.58 8 8 8v3l4-4-4-4v3z" />
  </svg>
);

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
        <AccordionHeader expandIcon={<ExpandMoreIcon />}>
          <div className={styles.componentHeader}>
            <ComponentIcon />
            <span className={styles.componentName}>{displayName}</span>
            <Chip size="small" variant="outlined" className={styles.typeChip}>
              Type: {component.type_index}
            </Chip>
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
              icon={<SyncIcon className={styles.liveChipIcon} />}
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
              <Typography variant="title-small" className={styles.sectionTitle}>
                Components ({components.length})
              </Typography>

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
