import './globals.css'
import '@/ui/styles/tokens.css'
import '@/ui/styles/base.css'
import '@/ui/styles/icons.css'
import { AppLayout } from './app-layout'

export const metadata = {
  title: 'nodec Game Editor',
  description: 'Game editor for nodec game engine',
}

export default function RootLayout({
  children,
}: {
  children: React.ReactNode
}) {
  return (
    <html lang="en">
      <body>
        <AppLayout>{children}</AppLayout>
      </body>
    </html>
  )
}
