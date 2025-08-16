'use client'

import { SnackbarProvider } from 'notistack'

export function AppLayout({ children }: { children: React.ReactNode }) {
  return (
    <SnackbarProvider
      maxSnack={3}
      anchorOrigin={{
        vertical: 'bottom',
        horizontal: 'left',
      }}
    >
      {children}
    </SnackbarProvider>
  )
}