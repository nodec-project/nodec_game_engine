'use client';

import React, { useRef, useEffect, useLayoutEffect, useState, useCallback } from 'react';
import { Typography, IconButton, Tooltip, Icon } from '@/ui';
import styles from './CurveViewer.module.css';

// Keyframe data
interface Keyframe {
  time: number;
  value: number;
}

// Wrap mode constants
export const WrapMode = {
  Once: 0,
  Loop: 1,
  PingPong: 2,
} as const;

export type WrapModeType = typeof WrapMode[keyof typeof WrapMode];

export const WrapModeLabels: Record<WrapModeType, string> = {
  [WrapMode.Once]: 'Once',
  [WrapMode.Loop]: 'Loop',
  [WrapMode.PingPong]: 'Ping Pong',
};

// Curve data for display
export interface CurveData {
  id: string; // Unique identifier for the curve
  propertyPath: string;
  keyframes: Keyframe[];
  wrapMode: number;
}

// Selected keyframe info
interface SelectedKeyframe {
  curveId: string;
  keyframeIndex: number;
}

// ViewState for coordinate system management
interface ViewState {
  timeMin: number;  // Left edge time
  timeMax: number;  // Right edge time
  valueMin: number; // Bottom edge value
  valueMax: number; // Top edge value
}

// Canvas colors for theming
interface CanvasColors {
  bg: string;
  grid: string;
  axes: string;
  label: string;
  scrubber: string;
  addOverlay: string;
  addText: string;
}

// Pan state
interface PanState {
  x: number;
  y: number;
  view: ViewState | null;
}

interface CurveViewerProps {
  curves: CurveData[];
  duration: number;
  currentTime?: number;
  editable?: boolean;
  onTimeChange?: (time: number) => void;
  onKeyframeUpdate?: (curveId: string, keyframeIndex: number, keyframe: Keyframe) => void;
  onKeyframeAdd?: (curveId: string, keyframe: Keyframe) => void;
  onKeyframeDelete?: (curveId: string, keyframeIndex: number) => void;
  onWrapModeChange?: (curveId: string, wrapMode: WrapModeType) => void;
}

// Calculate default view state from curves
const getDefaultViewState = (curves: CurveData[], duration: number): ViewState => {
  let valueMin = 0;
  let valueMax = 1;

  curves.forEach(curve => {
    curve.keyframes.forEach(kf => {
      valueMin = Math.min(valueMin, kf.value);
      valueMax = Math.max(valueMax, kf.value);
    });
  });

  // Add padding
  const valuePadding = (valueMax - valueMin) * 0.1 || 0.5;

  return {
    timeMin: 0,
    timeMax: duration || 1,
    valueMin: valueMin - valuePadding,
    valueMax: valueMax + valuePadding,
  };
};

// Calculate nice tick interval (1, 2, 5, 10, 20, 50, 100...)
const calculateNiceInterval = (range: number, targetTickCount: number): number => {
  if (range <= 0 || targetTickCount <= 0) return 1;

  const rawInterval = range / targetTickCount;
  const magnitude = Math.pow(10, Math.floor(Math.log10(rawInterval)));
  const normalized = rawInterval / magnitude;

  let niceNormalized: number;
  if (normalized <= 1) niceNormalized = 1;
  else if (normalized <= 2) niceNormalized = 2;
  else if (normalized <= 5) niceNormalized = 5;
  else niceNormalized = 10;

  return niceNormalized * magnitude;
};

export const CurveViewer: React.FC<CurveViewerProps> = ({
  curves,
  duration,
  currentTime = 0,
  editable = false,
  onTimeChange,
  onKeyframeUpdate,
  onKeyframeAdd,
  onKeyframeDelete,
  onWrapModeChange,
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const drawRequestRef = useRef<number | null>(null);

  // View state (replaces zoom/offset)
  const [viewState, setViewState] = useState<ViewState>(() =>
    getDefaultViewState(curves, duration)
  );

  // Selection and interaction states
  const [selectedCurveIndex, setSelectedCurveIndex] = useState<number | null>(null);
  const [selectedKeyframe, setSelectedKeyframe] = useState<SelectedKeyframe | null>(null);
  const [isDraggingKeyframe, setIsDraggingKeyframe] = useState(false);
  const [isAddMode, setIsAddMode] = useState(false);
  const [scrubberTime, setScrubberTime] = useState(currentTime);

  // Pan state
  const [isPanning, setIsPanning] = useState(false);
  const [panStart, setPanStart] = useState<PanState>({ x: 0, y: 0, view: null });

  // Colors for different curves (stable reference)
  const curveColors = React.useMemo(() => [
    '#ff6b6b', '#4ecdc4', '#45b7d1', '#f9ca24',
    '#f0932b', '#eb4d4b', '#6ab04c', '#130f40'
  ], []);

  // Get CSS variable colors from container
  const getCanvasColors = useCallback((): CanvasColors => {
    const container = containerRef.current;
    if (!container) {
      return {
        bg: '#fafafa',
        grid: '#e0e0e0',
        axes: '#9e9e9e',
        label: '#616161',
        scrubber: '#ffd700',
        addOverlay: 'rgba(76, 175, 80, 0.1)',
        addText: '#4caf50',
      };
    }
    const computedStyles = getComputedStyle(container);
    return {
      bg: computedStyles.getPropertyValue('--curve-bg').trim() || '#fafafa',
      grid: computedStyles.getPropertyValue('--curve-grid').trim() || '#e0e0e0',
      axes: computedStyles.getPropertyValue('--curve-axes').trim() || '#9e9e9e',
      label: computedStyles.getPropertyValue('--curve-label').trim() || '#616161',
      scrubber: computedStyles.getPropertyValue('--curve-scrubber').trim() || '#ffd700',
      addOverlay: computedStyles.getPropertyValue('--curve-add-overlay').trim() || 'rgba(76, 175, 80, 0.1)',
      addText: computedStyles.getPropertyValue('--curve-add-text').trim() || '#4caf50',
    };
  }, []);

  // Data coordinates → Canvas coordinates
  const dataToCanvas = useCallback((time: number, value: number): { x: number; y: number } => {
    const canvas = canvasRef.current;
    if (!canvas) return { x: 0, y: 0 };

    const { timeMin, timeMax, valueMin, valueMax } = viewState;
    const timeRange = timeMax - timeMin;
    const valueRange = valueMax - valueMin;

    // Prevent division by zero
    const x = timeRange > 0 ? ((time - timeMin) / timeRange) * canvas.width : 0;
    const y = valueRange > 0 ? ((valueMax - value) / valueRange) * canvas.height : canvas.height / 2;

    return { x, y };
  }, [viewState]);

  // Canvas coordinates → Data coordinates
  const canvasToData = useCallback((x: number, y: number): { time: number; value: number } => {
    const canvas = canvasRef.current;
    if (!canvas) return { time: 0, value: 0 };

    const { timeMin, timeMax, valueMin, valueMax } = viewState;
    const timeRange = timeMax - timeMin;
    const valueRange = valueMax - valueMin;

    const time = (x / canvas.width) * timeRange + timeMin;
    const value = valueMax - (y / canvas.height) * valueRange;

    return { time, value };
  }, [viewState]);

  // Find keyframe at position
  const findKeyframeAtPosition = useCallback((canvasX: number, canvasY: number): SelectedKeyframe | null => {
    const canvas = canvasRef.current;
    if (!canvas) return null;

    const hitRadius = 8;

    for (let curveIdx = 0; curveIdx < curves.length; curveIdx++) {
      const curve = curves[curveIdx];
      for (let kfIdx = 0; kfIdx < curve.keyframes.length; kfIdx++) {
        const kf = curve.keyframes[kfIdx];
        const { x, y } = dataToCanvas(kf.time, kf.value);

        const dx = canvasX - x;
        const dy = canvasY - y;
        if (Math.sqrt(dx * dx + dy * dy) < hitRadius) {
          return { curveId: curve.id, keyframeIndex: kfIdx };
        }
      }
    }
    return null;
  }, [curves, dataToCanvas]);

  // Draw grid with adaptive ticks
  const drawGrid = useCallback((ctx: CanvasRenderingContext2D, canvas: HTMLCanvasElement, colors: CanvasColors) => {
    const { timeMin, timeMax, valueMin, valueMax } = viewState;

    const timeRange = timeMax - timeMin;
    const valueRange = valueMax - valueMin;

    // Calculate tick intervals
    const timeTickInterval = calculateNiceInterval(timeRange, canvas.width / 100);
    const valueTickInterval = calculateNiceInterval(valueRange, canvas.height / 50);

    ctx.strokeStyle = colors.grid;
    ctx.lineWidth = 0.5;

    // Vertical grid lines (time axis)
    const timeStart = Math.ceil(timeMin / timeTickInterval) * timeTickInterval;
    for (let t = timeStart; t <= timeMax; t += timeTickInterval) {
      const { x } = dataToCanvas(t, 0);
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
    }

    // Horizontal grid lines (value axis)
    const valueStart = Math.ceil(valueMin / valueTickInterval) * valueTickInterval;
    for (let v = valueStart; v <= valueMax; v += valueTickInterval) {
      const { y } = dataToCanvas(0, v);
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(canvas.width, y);
      ctx.stroke();
    }

    // Draw axes (if visible)
    ctx.strokeStyle = colors.axes;
    ctx.lineWidth = 1;

    // Time axis (y = 0)
    if (valueMin <= 0 && valueMax >= 0) {
      const { y: zeroY } = dataToCanvas(0, 0);
      ctx.beginPath();
      ctx.moveTo(0, zeroY);
      ctx.lineTo(canvas.width, zeroY);
      ctx.stroke();
    }

    // Value axis (t = 0)
    if (timeMin <= 0 && timeMax >= 0) {
      const { x: zeroX } = dataToCanvas(0, 0);
      ctx.beginPath();
      ctx.moveTo(zeroX, 0);
      ctx.lineTo(zeroX, canvas.height);
      ctx.stroke();
    }

    // Draw tick labels
    ctx.fillStyle = colors.label;
    ctx.font = '10px monospace';

    // Time labels (bottom)
    ctx.textAlign = 'center';
    for (let t = timeStart; t <= timeMax; t += timeTickInterval) {
      const { x } = dataToCanvas(t, 0);
      if (x >= 0 && x <= canvas.width) {
        ctx.fillText(t.toFixed(1) + 's', x, canvas.height - 4);
      }
    }

    // Value labels (left)
    ctx.textAlign = 'left';
    for (let v = valueStart; v <= valueMax; v += valueTickInterval) {
      const { y } = dataToCanvas(0, v);
      if (y >= 10 && y <= canvas.height - 10) {
        ctx.fillText(v.toFixed(2), 4, y + 3);
      }
    }
  }, [viewState, dataToCanvas]);

  // Draw curves
  const drawCurves = useCallback(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const colors = getCanvasColors();

    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Fill background
    ctx.fillStyle = colors.bg;
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // Draw grid
    drawGrid(ctx, canvas, colors);

    // Draw curves
    curves.forEach((curve, index) => {
      if (curve.keyframes.length < 1) return;

      const color = curveColors[index % curveColors.length];
      ctx.strokeStyle = color;
      ctx.lineWidth = selectedCurveIndex === index ? 3 : 2;
      ctx.globalAlpha = selectedCurveIndex === null || selectedCurveIndex === index ? 1 : 0.3;

      // Draw curve line
      if (curve.keyframes.length >= 2) {
        ctx.beginPath();
        curve.keyframes.forEach((keyframe, i) => {
          const { x, y } = dataToCanvas(keyframe.time, keyframe.value);
          if (i === 0) {
            ctx.moveTo(x, y);
          } else {
            ctx.lineTo(x, y);
          }
        });
        ctx.stroke();
      }

      // Draw keyframe points
      curve.keyframes.forEach((keyframe, kfIndex) => {
        const { x, y } = dataToCanvas(keyframe.time, keyframe.value);
        const isSelected = selectedKeyframe?.curveId === curve.id && selectedKeyframe?.keyframeIndex === kfIndex;

        // Draw keyframe marker
        ctx.fillStyle = isSelected ? '#fff' : color;
        ctx.strokeStyle = color;
        ctx.lineWidth = isSelected ? 3 : 2;

        ctx.beginPath();
        if (isSelected) {
          ctx.arc(x, y, 6, 0, Math.PI * 2);
          ctx.fill();
          ctx.stroke();
        } else {
          ctx.fillRect(x - 4, y - 4, 8, 8);
        }
      });

      ctx.globalAlpha = 1;
    });

    // Draw current time indicator (scrubber)
    if (currentTime >= 0) {
      const { x } = dataToCanvas(currentTime, 0);

      ctx.strokeStyle = colors.scrubber;
      ctx.lineWidth = 2;
      ctx.setLineDash([5, 5]);
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
      ctx.setLineDash([]);
    }

    // Draw add mode indicator
    if (isAddMode) {
      ctx.fillStyle = colors.addOverlay;
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.fillStyle = colors.addText;
      ctx.font = '14px sans-serif';
      ctx.fillText('Click to add keyframe', 10, 20);
    }
  }, [curves, selectedCurveIndex, selectedKeyframe, currentTime, isAddMode, curveColors, getCanvasColors, dataToCanvas, drawGrid]);

  // Request redraw with requestAnimationFrame
  const requestRedraw = useCallback(() => {
    if (drawRequestRef.current !== null) return;
    drawRequestRef.current = requestAnimationFrame(() => {
      drawCurves();
      drawRequestRef.current = null;
    });
  }, [drawCurves]);

  // Cleanup animation frame on unmount
  useEffect(() => {
    return () => {
      if (drawRequestRef.current !== null) {
        cancelAnimationFrame(drawRequestRef.current);
      }
    };
  }, []);

  // Trigger redraw when dependencies change
  useEffect(() => {
    requestRedraw();
  }, [requestRedraw]);

  // Track canvas size for ResizeObserver
  const canvasSizeRef = useRef({ width: 0, height: 0 });

  // Store drawCurves in a ref to avoid effect re-running on every render
  const drawCurvesRef = useRef(drawCurves);
  useLayoutEffect(() => {
    drawCurvesRef.current = drawCurves;
  });

  // ResizeObserver for parent element size changes
  useLayoutEffect(() => {
    const container = containerRef.current;
    const canvas = canvasRef.current;
    if (!container || !canvas) return;

    const updateCanvasSize = (width: number, height: number) => {
      if (width > 0 && height > 0 &&
          (canvasSizeRef.current.width !== width || canvasSizeRef.current.height !== height)) {
        canvasSizeRef.current = { width, height };
        canvas.width = width;
        canvas.height = height;
        // Force immediate redraw after resize
        if (drawRequestRef.current !== null) {
          cancelAnimationFrame(drawRequestRef.current);
          drawRequestRef.current = null;
        }
        drawCurvesRef.current();
      }
    };

    // Set initial size
    const rect = container.getBoundingClientRect();
    updateCanvasSize(rect.width, rect.height);

    const resizeObserver = new ResizeObserver((entries) => {
      for (const entry of entries) {
        const { width, height } = entry.contentRect;
        updateCanvasSize(width, height);
      }
    });

    resizeObserver.observe(container);
    return () => resizeObserver.disconnect();
  }, []); // No dependencies - runs once and uses ref for latest drawCurves

  // Sync scrubber time with external currentTime
  useEffect(() => {
    setScrubberTime(currentTime);
  }, [currentTime]);

  // Mouse wheel zoom
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const handleWheel = (e: WheelEvent) => {
      e.preventDefault();

      const rect = canvas.getBoundingClientRect();
      const mouseX = e.clientX - rect.left;
      const mouseY = e.clientY - rect.top;

      // Get data position under mouse
      const { time: mouseTime, value: mouseValue } = canvasToData(mouseX, mouseY);

      // Zoom factor
      const zoomFactor = e.deltaY > 0 ? 1.1 : 0.9;

      setViewState(prev => {
        let { timeMin, timeMax, valueMin, valueMax } = prev;

        // Determine which axes to zoom
        const zoomTime = !e.ctrlKey;   // Ctrl = vertical only
        const zoomValue = !e.shiftKey; // Shift = horizontal only

        if (zoomTime) {
          const timeRange = timeMax - timeMin;
          const newTimeRange = timeRange * zoomFactor;
          const mouseTimeRatio = (mouseTime - timeMin) / timeRange;
          timeMin = mouseTime - newTimeRange * mouseTimeRatio;
          timeMax = mouseTime + newTimeRange * (1 - mouseTimeRatio);
        }

        if (zoomValue) {
          const valueRange = valueMax - valueMin;
          const newValueRange = valueRange * zoomFactor;
          const mouseValueRatio = (mouseValue - valueMin) / valueRange;
          valueMin = mouseValue - newValueRange * mouseValueRatio;
          valueMax = mouseValue + newValueRange * (1 - mouseValueRatio);
        }

        return { timeMin, timeMax, valueMin, valueMax };
      });
    };

    canvas.addEventListener('wheel', handleWheel, { passive: false });
    return () => canvas.removeEventListener('wheel', handleWheel);
  }, [canvasToData]);

  // Keyboard events
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Fit view with F key
      if (e.key === 'f' || e.key === 'F') {
        setViewState(getDefaultViewState(curves, duration));
        return;
      }

      if (!editable) return;

      if (e.key === 'Delete' || e.key === 'Backspace') {
        if (selectedKeyframe && onKeyframeDelete) {
          onKeyframeDelete(selectedKeyframe.curveId, selectedKeyframe.keyframeIndex);
          setSelectedKeyframe(null);
        }
      } else if (e.key === 'Escape') {
        setSelectedKeyframe(null);
        setIsAddMode(false);
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [editable, selectedKeyframe, onKeyframeDelete, curves, duration]);

  // Mouse event handlers
  const handleMouseDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const canvasX = e.clientX - rect.left;
    const canvasY = e.clientY - rect.top;

    // Middle button = pan
    if (e.button === 1) {
      e.preventDefault();
      setIsPanning(true);
      setPanStart({ x: e.clientX, y: e.clientY, view: viewState });
      return;
    }

    // Left button
    if (e.button === 0) {
      if (editable && isAddMode && selectedCurveIndex !== null) {
        const { time, value } = canvasToData(canvasX, canvasY);
        const curve = curves[selectedCurveIndex];
        if (curve && onKeyframeAdd) {
          onKeyframeAdd(curve.id, { time: Math.max(0, time), value });
        }
        setIsAddMode(false);
        return;
      }

      const hit = findKeyframeAtPosition(canvasX, canvasY);
      if (hit) {
        setSelectedKeyframe(hit);
        if (editable) {
          setIsDraggingKeyframe(true);
        }
      } else {
        setSelectedKeyframe(null);
        if (onTimeChange) {
          const { time } = canvasToData(canvasX, canvasY);
          const clampedTime = Math.max(0, Math.min(duration, time));
          onTimeChange(clampedTime);
          setScrubberTime(clampedTime);
        }
      }
    }
  };

  const handleMouseMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    // Pan mode
    if (isPanning && panStart.view) {
      const dx = e.clientX - panStart.x;
      const dy = e.clientY - panStart.y;

      const { timeMin, timeMax, valueMin, valueMax } = panStart.view;
      const timeRange = timeMax - timeMin;
      const valueRange = valueMax - valueMin;

      const timeDelta = -(dx / canvas.width) * timeRange;
      const valueDelta = (dy / canvas.height) * valueRange;

      setViewState({
        timeMin: timeMin + timeDelta,
        timeMax: timeMax + timeDelta,
        valueMin: valueMin + valueDelta,
        valueMax: valueMax + valueDelta,
      });
      return;
    }

    // Keyframe dragging
    if (isDraggingKeyframe && selectedKeyframe && editable) {
      const rect = canvas.getBoundingClientRect();
      const canvasX = e.clientX - rect.left;
      const canvasY = e.clientY - rect.top;

      const { time, value } = canvasToData(canvasX, canvasY);

      if (onKeyframeUpdate) {
        onKeyframeUpdate(selectedKeyframe.curveId, selectedKeyframe.keyframeIndex, {
          time: Math.max(0, time),
          value
        });
      }
    }
  };

  const handleMouseUp = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (e.button === 1) {
      setIsPanning(false);
      return;
    }
    setIsDraggingKeyframe(false);
  };

  const handleMouseLeave = () => {
    setIsPanning(false);
    setIsDraggingKeyframe(false);
  };

  const handleDoubleClick = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (!editable || selectedCurveIndex === null) return;

    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const canvasX = e.clientX - rect.left;
    const canvasY = e.clientY - rect.top;

    const { time, value } = canvasToData(canvasX, canvasY);
    const curve = curves[selectedCurveIndex];
    if (curve && onKeyframeAdd) {
      onKeyframeAdd(curve.id, { time: Math.max(0, time), value });
    }
  };

  // Toolbar handlers
  const handleZoomIn = () => {
    setViewState(prev => {
      const timeCenter = (prev.timeMin + prev.timeMax) / 2;
      const valueCenter = (prev.valueMin + prev.valueMax) / 2;
      const timeHalfRange = (prev.timeMax - prev.timeMin) / 2 * 0.8;
      const valueHalfRange = (prev.valueMax - prev.valueMin) / 2 * 0.8;
      return {
        timeMin: timeCenter - timeHalfRange,
        timeMax: timeCenter + timeHalfRange,
        valueMin: valueCenter - valueHalfRange,
        valueMax: valueCenter + valueHalfRange,
      };
    });
  };

  const handleZoomOut = () => {
    setViewState(prev => {
      const timeCenter = (prev.timeMin + prev.timeMax) / 2;
      const valueCenter = (prev.valueMin + prev.valueMax) / 2;
      const timeHalfRange = (prev.timeMax - prev.timeMin) / 2 * 1.2;
      const valueHalfRange = (prev.valueMax - prev.valueMin) / 2 * 1.2;
      return {
        timeMin: timeCenter - timeHalfRange,
        timeMax: timeCenter + timeHalfRange,
        valueMin: valueCenter - valueHalfRange,
        valueMax: valueCenter + valueHalfRange,
      };
    });
  };

  const handleFitView = () => {
    setViewState(getDefaultViewState(curves, duration));
  };

  const handleDeleteKeyframe = () => {
    if (selectedKeyframe && onKeyframeDelete) {
      onKeyframeDelete(selectedKeyframe.curveId, selectedKeyframe.keyframeIndex);
      setSelectedKeyframe(null);
    }
  };

  const getCursorClass = () => {
    if (isPanning) return styles.cursorGrabbing;
    if (isDraggingKeyframe) return styles.cursorGrabbing;
    if (isAddMode) return styles.cursorCell;
    return styles.cursorCrosshair;
  };

  return (
    <div className={styles.container}>
      {/* Toolbar */}
      <div className={styles.toolbar}>
        <Typography variant="labelSmall" style={{ marginRight: 16 }}>
          Curves: {curves.length}
        </Typography>

        <Tooltip title="Zoom In">
          <IconButton size="small" onClick={handleZoomIn}>
            <Icon name="zoom_in" />
          </IconButton>
        </Tooltip>

        <Tooltip title="Zoom Out">
          <IconButton size="small" onClick={handleZoomOut}>
            <Icon name="zoom_out" />
          </IconButton>
        </Tooltip>

        <Tooltip title="Fit to Screen (F)">
          <IconButton size="small" onClick={handleFitView}>
            <Icon name="fit_screen" />
          </IconButton>
        </Tooltip>

        {editable && (
          <>
            <div className={styles.toolbarDivider} />
            <Tooltip title="Add Keyframe (select curve first, then click)">
              <IconButton
                size="small"
                onClick={() => setIsAddMode(!isAddMode)}
                color={isAddMode ? 'primary' : 'default'}
                disabled={selectedCurveIndex === null}
              >
                <Icon name="add" />
              </IconButton>
            </Tooltip>
            <Tooltip title="Delete Selected Keyframe">
              <IconButton
                size="small"
                onClick={handleDeleteKeyframe}
                disabled={selectedKeyframe === null}
                color="error"
              >
                <Icon name="delete" />
              </IconButton>
            </Tooltip>
          </>
        )}

        <Typography variant="labelSmall" className={styles.timeDisplay}>
          Time: {scrubberTime.toFixed(2)}s / {duration.toFixed(2)}s
        </Typography>

        {selectedKeyframe && (
          <Typography variant="labelSmall" className={styles.selectedDisplay}>
            Selected: keyframe {selectedKeyframe.keyframeIndex}
          </Typography>
        )}
      </div>

      {/* Canvas */}
      <div ref={containerRef} className={styles.canvasContainer}>
        <canvas
          ref={canvasRef}
          className={`${styles.canvas} ${getCursorClass()}`}
          onMouseDown={handleMouseDown}
          onMouseMove={handleMouseMove}
          onMouseUp={handleMouseUp}
          onMouseLeave={handleMouseLeave}
          onDoubleClick={handleDoubleClick}
        />
      </div>

      {/* Curve List */}
      <div className={styles.curveList}>
        <Typography variant="labelSmall" className={styles.curveListTitle}>
          Select curve to edit:
        </Typography>
        <div className={styles.curveListItems}>
          {curves.map((curve, index) => (
            <div key={curve.id} className={styles.curveItem}>
              <button
                onClick={() => setSelectedCurveIndex(selectedCurveIndex === index ? null : index)}
                className={`${styles.curveChip} ${selectedCurveIndex === index ? styles.curveChipSelected : ''}`}
                style={{
                  borderColor: curveColors[index % curveColors.length],
                  backgroundColor: selectedCurveIndex === index ? curveColors[index % curveColors.length] : 'transparent',
                  color: selectedCurveIndex === index ? '#fff' : 'inherit',
                }}
              >
                {curve.propertyPath} ({curve.keyframes.length} keys)
              </button>
              {editable && onWrapModeChange && (
                <select
                  className={styles.wrapModeSelect}
                  value={curve.wrapMode}
                  onChange={(e) => onWrapModeChange(curve.id, Number(e.target.value) as WrapModeType)}
                  onClick={(e) => e.stopPropagation()}
                >
                  {Object.entries(WrapModeLabels).map(([value, label]) => (
                    <option key={value} value={value}>{label}</option>
                  ))}
                </select>
              )}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};
