# Solreno Scene Editor Web UI

Web-based scene editor interface for the Solreno game engine.

## Features

- **Scene Hierarchy View**: Browse and select entities in the scene
- **Component Inspector**: View and inspect entity components with their serialized data
- **Real-time Connection**: Connects to the game engine's Editor Server API

## Prerequisites

- Node.js 18+ and npm
- Game engine running with Editor Server enabled (port 8080)

## Installation

```bash
npm install
```

## Development

Run the development server:

```bash
npm run dev
```

Open [http://localhost:3000](http://localhost:3000) in your browser.

## API Endpoints

The Web UI connects to the following Editor Server API endpoints:

- `GET /api/entities/roots` - Get root entities in the scene
- `GET /api/entities/ids/:id` - Get entity details
- `GET /api/entities/ids/:id/components` - Get entity components

## Building for Production

```bash
npm run build
npm start
```

## Configuration

The API base URL is configured in `src/api/gameEngine.ts`:

```typescript
const API_BASE_URL = 'http://localhost:8080';
```

## Architecture

- **Next.js 14**: React framework with App Router
- **Material-UI**: Component library for consistent UI
- **TypeScript**: Type-safe development
- **API Client**: Singleton pattern for API communication

## Project Structure

```
web_ui/
├── app/                    # Next.js app directory
│   ├── layout.tsx         # Root layout
│   └── page.tsx           # Main page
├── src/
│   ├── api/               # API client
│   │   └── gameEngine.ts  # Game engine API interface
│   ├── components/        # React components
│   │   ├── SceneHierarchy.tsx    # Entity tree view
│   │   └── ComponentInspector.tsx # Component details view
│   └── lib/               # Utilities
│       └── theme.ts       # MUI theme configuration
└── package.json           # Dependencies
```

## Troubleshooting

### "Game engine server is not running"

Make sure:
1. The game engine is running in editor mode
2. The Editor Server is listening on port 8080
3. Check the console output for "Editor server listening on port 8080"

### CORS errors

The Editor Server includes CORS headers for local development. If you're running the Web UI on a different port or host, you may need to update the CORS configuration in the Editor Server.