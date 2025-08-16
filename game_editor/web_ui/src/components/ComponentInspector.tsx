'use client';

import React, { useState, useEffect } from 'react';
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
import { gameEngineAPI, ComponentInfo, EntityDetailsResponse } from '../api/gameEngine';

interface ComponentInspectorProps {
  entityId: string | null;
}

export const ComponentInspector: React.FC<ComponentInspectorProps> = ({ entityId }) => {
  const [components, setComponents] = useState<ComponentInfo[]>([]);
  const [entityDetails, setEntityDetails] = useState<EntityDetailsResponse | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

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

  const renderComponentData = (data: unknown): React.ReactNode => {
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
                  renderComponentData(value)
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
                {renderComponentData(item)}
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
        <Typography variant="h6" component="h2" gutterBottom>
          Component Inspector
        </Typography>
        <Alert severity="info">
          Select an entity to view its components
        </Alert>
      </Paper>
    );
  }

  return (
    <Paper sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
      <Box sx={{ p: 2, borderBottom: 1, borderColor: 'divider' }}>
        <Typography variant="h6" component="h2">
          Component Inspector
        </Typography>
        {entityDetails && (
          <Box sx={{ mt: 1 }}>
            <Typography variant="subtitle2" color="text.secondary">
              Entity: {entityDetails.name}
            </Typography>
            <Typography variant="caption" color="text.secondary">
              ID: {entityDetails.entity_id}
            </Typography>
          </Box>
        )}
      </Box>

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
                {entityDetails.parent_id && (
                  <Grid item xs={12}>
                    <TextField
                      label="Parent ID"
                      value={entityDetails.parent_id}
                      variant="outlined"
                      size="small"
                      fullWidth
                      disabled
                    />
                  </Grid>
                )}
                {entityDetails.children.length > 0 && (
                  <Grid item xs={12}>
                    <Typography variant="caption" color="text.secondary">
                      Children: {entityDetails.children.length}
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
              
              {components.length === 0 ? (
                <Alert severity="info">
                  No components found for this entity
                </Alert>
              ) : (
                components.map((component, index) => (
                  <Accordion key={index} defaultExpanded={index === 0}>
                    <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                        <ComponentIcon fontSize="small" />
                        <Typography>
                          Component {component.type_index}
                        </Typography>
                        <Chip 
                          label={`Type: ${component.type_index}`} 
                          size="small" 
                          variant="outlined"
                        />
                      </Box>
                    </AccordionSummary>
                    <AccordionDetails>
                      {component.data !== null ? (
                        renderComponentData(component.data)
                      ) : (
                        <Typography variant="body2" color="text.secondary">
                          Component data not serializable
                        </Typography>
                      )}
                    </AccordionDetails>
                  </Accordion>
                ))
              )}
            </Box>
          </>
        )}
      </Box>
    </Paper>
  );
};