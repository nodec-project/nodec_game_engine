
'use client';

import { useState } from 'react';
import { Box, Container, Typography, Grid } from '@mui/material';
import { SceneHierarchy } from '../src/components/SceneHierarchy';
import { ComponentInspector } from '../src/components/ComponentInspector';

export default function Home() {
  const [selectedEntityId, setSelectedEntityId] = useState<string | null>(null);

  const handleEntitySelect = (entityId: string) => {
    console.log('Selected entity:', entityId);
    setSelectedEntityId(entityId);
  };

  return (
    <Container maxWidth="xl" sx={{ py: 4 }}>
      <Typography variant="h4" component="h1" gutterBottom>
        Solreno Scene Editor
      </Typography>
      
      <Grid container spacing={3} sx={{ height: 'calc(100vh - 200px)' }}>
        <Grid item xs={12} md={4} lg={3}>
          <SceneHierarchy onEntitySelect={handleEntitySelect} />
        </Grid>
        
        <Grid item xs={12} md={8} lg={9}>
          <ComponentInspector entityId={selectedEntityId} />
        </Grid>
      </Grid>
    </Container>
  );
}