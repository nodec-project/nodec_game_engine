# Animation Web Editor Implementation Plan

## Overview
Implement a web-based animation editor that leverages web ecosystem UI components for rich curve editing capabilities, integrated with the existing Editor Server architecture. The editor must handle the relationship between AnimationClips, Animator components, and entity hierarchies.

## Architecture Overview

```
Game Engine (C++)                Web UI (React/Next.js)
┌─────────────────────┐          ┌────────────────────────┐
│ AnimatorSystem      │          │ Animation Editor       │
│                     │          │ ┌────────────────────┐ │
│ Entity Hierarchy    │◄────────►│ │ Context Manager    │ │
│                     │   HTTP   │ │ - Entity binding   │ │
│ Animation Clips     │   API    │ │ - Clip management  │ │
│                     │          │ └────────────────────┘ │
│ Editor Server       │          │ ┌────────────────────┐ │
│ (port 8080)         │          │ │ Curve Editor       │ │
│                     │          │ │ - Keyframe edit    │ │
│ Session Manager     │◄────────►│ │ - Bezier curves    │ │
└─────────────────────┘          │ └────────────────────┘ │
                                │ ┌────────────────────┐ │
                                │ │ Property Selector  │ │
                                │ │ - Component list   │ │
                                │ │ - Property tree    │ │
                                │ └────────────────────┘ │
                                └────────────────────────┘
```

## Phase 1: Animation Editing Context API (Week 1)

### Objective
Establish the foundation for animation editing by exposing entity-clip relationships through the Editor Server API.

### Implementation Tasks

#### 1.1 C++ Editor Server Extensions
```cpp
// Add to editor_server.hpp
class EditorServer {
    // New member for animation editing
    AnimationEditorState animation_editor_state_;
    
    // New API handlers
    void handle_get_editing_context(Request* req, Response* res);
    void handle_create_session(Request* req, Response* res);
    void handle_get_animatable_properties(Request* req, Response* res);
};
```

#### 1.2 Entity Hierarchy Collection
- Implement recursive entity traversal
- Collect component type information
- Build property path mappings
- Match clip entity paths to actual entities

#### 1.3 Property Discovery System
- Leverage ComponentRegistry for animatable components
- Use PropertyWriter traversal mechanism
- Cache property paths per component type
- Generate human-readable property names

### API Endpoints to Implement
1. `GET /api/animations/editing-context?entityId=<id>`
2. `GET /api/animations/animatable-properties?entityId=<id>`
3. `GET /api/animations/clips`
4. `GET /api/animations/clip?path=<resource_path>`

### Testing Strategy
- Create test entities with Animator components
- Generate sample AnimationClips programmatically
- Verify entity-clip binding logic
- Test property discovery accuracy

## Phase 2: Minimal Animation Viewer (Week 2)

### Objective
Display animation data in the Web UI with entity context awareness.

### UI Components Structure
```tsx
<AnimationEditor>
  <EntityContextPanel>
    <EntitySelector />        // Select entity with Animator
    <HierarchyView />         // Show entity tree
    <ComponentList />         // List animatable components
  </EntityContextPanel>
  
  <AnimationPanel>
    <Timeline />              // Playback control
    <CurveList>              // List of animated properties
      <CurveViewer />        // Display curve
    </CurveList>
  </AnimationPanel>
</AnimationEditor>
```

### Implementation Tasks

#### 2.1 Entity Context Management
```typescript
interface AnimationContext {
  animatorEntity: Entity;
  clip: AnimationClip | null;
  hierarchy: EntityHierarchy;
  bindings: Record<string, number>;
  session?: string;
}

const useAnimationContext = () => {
  const [context, setContext] = useState<AnimationContext>();
  
  const loadContext = async (entityId: number) => {
    const data = await api.getEditingContext(entityId);
    setContext(data);
  };
  
  return { context, loadContext };
};
```

#### 2.2 Property Tree Component
```tsx
const PropertyTree: React.FC<{hierarchy: EntityHierarchy}> = ({hierarchy}) => {
  return (
    <TreeView>
      {Object.entries(hierarchy).map(([path, node]) => (
        <TreeItem key={path} label={node.name}>
          {Object.entries(node.components).map(([comp, info]) => (
            <TreeItem key={comp} label={comp}>
              {info.animatableProperties.map(prop => (
                <TreeItem 
                  key={prop} 
                  label={prop}
                  onClick={() => addPropertyCurve(path, comp, prop)}
                />
              ))}
            </TreeItem>
          ))}
        </TreeItem>
      ))}
    </TreeView>
  );
};
```

#### 2.3 Curve Visualization
- Use Recharts for initial implementation
- Display keyframes as points
- Show interpolated curve
- Support zoom/pan on timeline

### Dependencies
- `recharts`: For curve visualization
- `@mui/lab`: For TreeView component
- `react-use`: For hooks utilities

## Phase 3: Interactive Curve Editing (Week 3-4)

### Objective
Enable curve editing with awareness of entity property constraints.

### Features to Implement

#### 3.1 Property-Aware Editing
- Property type validation (float ranges, etc.)
- Component-specific constraints
- Real-time value preview on entities

#### 3.2 Curve Editor Integration
```tsx
interface CurveEditorProps {
  entityPath: string;
  componentType: string;
  propertyPath: string;
  curve: AnimationCurve;
  entityId: number;  // For real-time preview
  onUpdate: (curve: AnimationCurve) => void;
}

const CurveEditor: React.FC<CurveEditorProps> = ({
  entityPath,
  componentType,
  propertyPath,
  curve,
  entityId,
  onUpdate
}) => {
  // Property-specific constraints
  const constraints = usePropertyConstraints(componentType, propertyPath);
  
  return (
    <BezierEditor
      keyframes={curve.keyframes}
      min={constraints.min}
      max={constraints.max}
      onChange={(keyframes) => {
        const newCurve = { ...curve, keyframes };
        onUpdate(newCurve);
        previewValue(entityId, componentType, propertyPath, currentTime);
      }}
    />
  );
};
```

#### 3.3 Session Management
- Create editing sessions on server
- Track changes in working copy
- Support undo/redo operations
- Auto-save to prevent data loss

### Technology Stack
- **Primary**: Custom implementation with D3.js/Canvas
- **Alternative**: Adapt `react-curve-editor` library
- **Fallback**: Use modified Recharts with drag support

### API Endpoints to Add
1. `POST /api/animations/sessions/create`
2. `PUT /api/animations/sessions/:id/curves`
3. `DELETE /api/animations/sessions/:id`
4. `POST /api/animations/sessions/:id/save`

## Phase 4: Real-time Preview Integration (Week 5)

### Objective
Preview animations on actual entities while editing.

### Implementation Approach

#### 4.1 WebSocket Protocol
```typescript
// Client-side WebSocket handler
class AnimationPreviewSocket {
  private ws: WebSocket;
  private sessionId: string;
  
  connect(sessionId: string) {
    this.ws = new WebSocket('ws://localhost:8080/api/animations/preview-stream');
    this.sessionId = sessionId;
  }
  
  updateTime(time: number) {
    this.ws.send(JSON.stringify({
      type: 'PREVIEW_TIME',
      sessionId: this.sessionId,
      time
    }));
  }
  
  onEntityUpdate(callback: (entities: EntityUpdateMap) => void) {
    this.ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (data.type === 'PREVIEW_UPDATE') {
        callback(data.entities);
      }
    };
  }
}
```

#### 4.2 Server-side Preview System
```cpp
// Animation preview handler
void handle_preview_websocket(WebSocket* ws, std::string_view message) {
    auto data = parse_json(message);
    
    if (data.type == "PREVIEW_TIME") {
        auto* session = animation_editor_state_.getSession(data.sessionId);
        if (!session) return;
        
        // Apply animation to entities
        auto updates = session->evaluateAtTime(data.time);
        
        // Send updates back
        ws->send(serialize_entity_updates(updates));
    }
}
```

#### 4.3 UI Integration
- Sync timeline scrubbing with preview
- Highlight animated properties in scene
- Show before/after comparison
- Support play/pause/step controls

## Phase 5: Advanced Features (Week 6+)

### 5.1 Multi-Property Editing
- Select multiple properties
- Batch operations (scale, offset)
- Copy/paste curves between properties

### 5.2 Curve Templates
- Save curve presets
- Apply easing functions
- Import/export curve data

### 5.3 Animation Blending
- Preview transitions between clips
- Blend weight adjustment
- Cross-fade visualization

## Technology Recommendations

### Core Libraries

#### Animation & Curves
1. **Build Custom with D3.js** (Recommended)
   - Full control over interaction
   - Good performance
   - Extensive documentation

2. **react-spring** + Canvas
   - Smooth animations
   - Good for interactive feedback
   - Combine with curve rendering

3. **Theatre.js** (Alternative)
   - Complete animation solution
   - May be overkill but very polished

#### UI Components
- **@mui/material**: Base UI components
- **react-window**: Virtualized lists for performance
- **react-resizable-panels**: Layout management
- **@dnd-kit**: Drag and drop for keyframes

### State Management
```typescript
// Zustand store for animation editor state
interface AnimationEditorStore {
  context: AnimationContext | null;
  session: string | null;
  selectedCurves: CurveSelection[];
  currentTime: number;
  isPlaying: boolean;
  
  // Actions
  loadEntity: (entityId: number) => Promise<void>;
  createSession: () => Promise<void>;
  updateCurve: (path: string, curve: AnimationCurve) => void;
  setCurrentTime: (time: number) => void;
  play: () => void;
  pause: () => void;
}
```

## Minimal MVP Implementation Steps

### Week 1: Foundation
1. ✅ Design API endpoints
2. Implement editing context endpoint
3. Create test animation data
4. Basic property discovery

### Week 2: Visualization
1. Entity context panel
2. Property tree view
3. Simple curve display
4. Timeline component

### Week 3: Editing
1. Keyframe manipulation
2. Curve updates via API
3. Session management
4. Save functionality

### Week 4: Preview
1. WebSocket connection
2. Real-time updates
3. Playback controls
4. Visual feedback

## Success Metrics

### Phase 1
- [ ] Can fetch editing context for entity with Animator
- [ ] Property discovery returns correct animatable properties
- [ ] Entity hierarchy properly mapped to clip structure

### Phase 2
- [ ] Curves displayed with entity context
- [ ] Property tree shows all animatable properties
- [ ] Can select properties to add curves

### Phase 3
- [ ] Can drag keyframes to new positions
- [ ] Changes persist through session
- [ ] Can save modified clips

### Phase 4
- [ ] Real-time preview while editing
- [ ] Smooth playback in scene view
- [ ] WebSocket connection stable

## Risk Mitigation

### Entity-Clip Mismatch
- **Risk**: Clip references entities that don't exist
- **Mitigation**: Validation and warning system, orphaned curve highlighting

### Performance with Large Hierarchies
- **Risk**: Deep entity hierarchies slow down API
- **Mitigation**: Lazy loading, pagination, caching

### Session State Synchronization
- **Risk**: Client-server state mismatch
- **Mitigation**: Version tracking, conflict resolution, auto-save

### Browser Compatibility
- **Risk**: Advanced canvas features not supported
- **Mitigation**: Progressive enhancement, fallback renderers

## Future Enhancements

1. **Animation Layers**: Multiple clips on same entity
2. **Procedural Animation**: Node-based curve generation
3. **Motion Capture Import**: BVH/FBX animation import
4. **Animation Compression**: Optimize clip file sizes
5. **Collaborative Editing**: Multiple users editing same clip
6. **Version Control Integration**: Git-friendly animation format