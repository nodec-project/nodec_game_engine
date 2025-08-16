
'use client';

import { Box, Container, Typography, Grid } from '@mui/material';
import { SceneHierarchy } from '../src/components/SceneHierarchy';

export default function Home() {
  const handleEntitySelect = (entityId: string) => {
    console.log('Selected entity:', entityId);
    // TODO: Implement entity selection handling
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
          <Box
            sx={{
              height: '100%',
              border: 1,
              borderColor: 'divider',
              borderRadius: 1,
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              backgroundColor: 'grey.50',
            }}
          >
            <Typography variant="h6" color="text.secondary">
              Entity Inspector (Coming Soon)
            </Typography>
          </Box>
        </Grid>
      </Grid>
    </Container>
  );
}