'use client';

import React, { useRef, useEffect, useState, useCallback } from 'react';
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
  const [zoom, setZoom] = useState(1);
  const [offset, setOffset] = useState({ x: 0, y: 0 });
  const [selectedCurveIndex, setSelectedCurveIndex] = useState<number | null>(null);
  const [selectedKeyframe, setSelectedKeyframe] = useState<SelectedKeyframe | null>(null);
  const [isDragging, setIsDragging] = useState(false);
  const [isAddMode, setIsAddMode] = useState(false);
  const [scrubberTime, setScrubberTime] = useState(currentTime);

  // Colors for different curves (stable reference)
  const curveColors = React.useMemo(() => [
    '#ff6b6b', '#4ecdc4', '#45b7d1', '#f9ca24',
    '#f0932b', '#eb4d4b', '#6ab04c', '#130f40'
  ], []);

  // Convert canvas coordinates to time/value
  const canvasToTimeValue = useCallback((canvasX: number, canvasY: number, canvas: HTMLCanvasElement) => {
    const timeScale = (canvas.width - 100) / duration * zoom;
    const valueScale = 100 * zoom;
    const centerY = canvas.height / 2;

    const time = (canvasX - 50 - offset.x) / timeScale;
    const value = (centerY - canvasY + offset.y) / valueScale;

    return { time: Math.max(0, time), value };
  }, [duration, zoom, offset]);

  // Convert time/value to canvas coordinates
  const timeValueToCanvas = useCallback((time: number, value: number, canvas: HTMLCanvasElement) => {
    const timeScale = (canvas.width - 100) / duration * zoom;
    const valueScale = 100 * zoom;
    const centerY = canvas.height / 2;

    const x = 50 + time * timeScale + offset.x;
    const y = centerY - value * valueScale + offset.y;

    return { x, y };
  }, [duration, zoom, offset]);

  // Find keyframe at position
  const findKeyframeAtPosition = useCallback((canvasX: number, canvasY: number): SelectedKeyframe | null => {
    const canvas = canvasRef.current;
    if (!canvas) return null;

    const hitRadius = 8;

    for (let curveIdx = 0; curveIdx < curves.length; curveIdx++) {
      const curve = curves[curveIdx];
      for (let kfIdx = 0; kfIdx < curve.keyframes.length; kfIdx++) {
        const kf = curve.keyframes[kfIdx];
        const { x, y } = timeValueToCanvas(kf.time, kf.value, canvas);

        const dx = canvasX - x;
        const dy = canvasY - y;
        if (Math.sqrt(dx * dx + dy * dy) < hitRadius) {
          return { curveId: curve.id, keyframeIndex: kfIdx };
        }
      }
    }
    return null;
  }, [curves, timeValueToCanvas]);

  // Get CSS variable colors from container
  const getCanvasColors = useCallback(() => {
    const container = containerRef.current?.parentElement;
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
    const styles = getComputedStyle(container);
    return {
      bg: styles.getPropertyValue('--curve-bg').trim() || '#fafafa',
      grid: styles.getPropertyValue('--curve-grid').trim() || '#e0e0e0',
      axes: styles.getPropertyValue('--curve-axes').trim() || '#9e9e9e',
      label: styles.getPropertyValue('--curve-label').trim() || '#616161',
      scrubber: styles.getPropertyValue('--curve-scrubber').trim() || '#ffd700',
      addOverlay: styles.getPropertyValue('--curve-add-overlay').trim() || 'rgba(76, 175, 80, 0.1)',
      addText: styles.getPropertyValue('--curve-add-text').trim() || '#4caf50',
    };
  }, []);

  // Draw the curves on canvas
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
    ctx.strokeStyle = colors.grid;
    ctx.lineWidth = 0.5;
    const gridSize = 50 * zoom;

    for (let x = 0; x < canvas.width; x += gridSize) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
    }

    for (let y = 0; y < canvas.height; y += gridSize) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(canvas.width, y);
      ctx.stroke();
    }

    // Draw axes
    ctx.strokeStyle = colors.axes;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(50, 0);
    ctx.lineTo(50, canvas.height);
    ctx.moveTo(0, canvas.height / 2);
    ctx.lineTo(canvas.width, canvas.height / 2);
    ctx.stroke();

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
          const { x, y } = timeValueToCanvas(keyframe.time, keyframe.value, canvas);
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
        const { x, y } = timeValueToCanvas(keyframe.time, keyframe.value, canvas);
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
    if (currentTime >= 0 && currentTime <= duration) {
      const timeScale = (canvas.width - 100) / duration * zoom;
      const x = 50 + currentTime * timeScale + offset.x;

      ctx.strokeStyle = colors.scrubber;
      ctx.lineWidth = 2;
      ctx.setLineDash([5, 5]);
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
      ctx.setLineDash([]);
    }

    // Draw time labels
    ctx.fillStyle = colors.label;
    ctx.font = '12px monospace';
    const timeStep = Math.max(duration / 10, 0.1);
    for (let t = 0; t <= duration; t += timeStep) {
      const x = 50 + t * (canvas.width - 100) / duration * zoom + offset.x;
      ctx.fillText(t.toFixed(1) + 's', x - 15, canvas.height - 5);
    }

    // Draw add mode indicator
    if (isAddMode) {
      ctx.fillStyle = colors.addOverlay;
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.fillStyle = colors.addText;
      ctx.font = '14px sans-serif';
      ctx.fillText('Click to add keyframe', 10, 20);
    }
  }, [curves, zoom, offset, selectedCurveIndex, selectedKeyframe, currentTime, duration, timeValueToCanvas, isAddMode, curveColors, getCanvasColors]);

  useEffect(() => {
    drawCurves();
  }, [drawCurves]);

  // Handle canvas resize - canvas width expands with zoom for horizontal scroll
  useEffect(() => {
    const handleResize = () => {
      if (canvasRef.current && containerRef.current) {
        const containerWidth = containerRef.current.clientWidth;
        const containerHeight = containerRef.current.clientHeight;
        // Canvas width grows with zoom to enable horizontal scrolling
        canvasRef.current.width = Math.max(containerWidth, containerWidth * zoom);
        canvasRef.current.height = containerHeight;
        drawCurves();
      }
    };

    handleResize();
    window.addEventListener('resize', handleResize);
    return () => window.removeEventListener('resize', handleResize);
  }, [drawCurves, zoom]);

  // Sync scrubber time with external currentTime
  useEffect(() => {
    setScrubberTime(currentTime);
  }, [currentTime]);

  // Handle keyboard events
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
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
  }, [editable, selectedKeyframe, onKeyframeDelete]);

  // Mouse event handlers
  const handleMouseDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const canvasX = e.clientX - rect.left;
    const canvasY = e.clientY - rect.top;

    if (editable && isAddMode && selectedCurveIndex !== null) {
      const { time, value } = canvasToTimeValue(canvasX, canvasY, canvas);
      const curve = curves[selectedCurveIndex];
      if (curve && onKeyframeAdd) {
        onKeyframeAdd(curve.id, { time, value });
      }
      setIsAddMode(false);
      return;
    }

    const hit = findKeyframeAtPosition(canvasX, canvasY);
    if (hit) {
      setSelectedKeyframe(hit);
      if (editable) {
        setIsDragging(true);
      }
    } else {
      setSelectedKeyframe(null);
      if (onTimeChange) {
        const { time } = canvasToTimeValue(canvasX, canvasY, canvas);
        const clampedTime = Math.max(0, Math.min(duration, time));
        onTimeChange(clampedTime);
        setScrubberTime(clampedTime);
      }
    }
  };

  const handleMouseMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (!isDragging || !selectedKeyframe || !editable) return;

    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const canvasX = e.clientX - rect.left;
    const canvasY = e.clientY - rect.top;

    const { time, value } = canvasToTimeValue(canvasX, canvasY, canvas);

    if (onKeyframeUpdate) {
      onKeyframeUpdate(selectedKeyframe.curveId, selectedKeyframe.keyframeIndex, {
        time: Math.max(0, time),
        value
      });
    }
  };

  const handleMouseUp = () => {
    setIsDragging(false);
  };

  const handleDoubleClick = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (!editable || selectedCurveIndex === null) return;

    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const canvasX = e.clientX - rect.left;
    const canvasY = e.clientY - rect.top;

    const { time, value } = canvasToTimeValue(canvasX, canvasY, canvas);
    const curve = curves[selectedCurveIndex];
    if (curve && onKeyframeAdd) {
      onKeyframeAdd(curve.id, { time: Math.max(0, time), value });
    }
  };

  const handleZoomIn = () => setZoom(prev => Math.min(prev * 1.2, 5));
  const handleZoomOut = () => setZoom(prev => Math.max(prev / 1.2, 0.2));
  const handleFitScreen = () => {
    setZoom(1);
    setOffset({ x: 0, y: 0 });
  };

  const handleDeleteKeyframe = () => {
    if (selectedKeyframe && onKeyframeDelete) {
      onKeyframeDelete(selectedKeyframe.curveId, selectedKeyframe.keyframeIndex);
      setSelectedKeyframe(null);
    }
  };

  const getCursorClass = () => {
    if (isDragging) return styles.cursorGrabbing;
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

        <Tooltip title="Fit to Screen">
          <IconButton size="small" onClick={handleFitScreen}>
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
          onMouseLeave={handleMouseUp}
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
