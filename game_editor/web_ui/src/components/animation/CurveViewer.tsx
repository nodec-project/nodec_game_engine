'use client';

import React, { useRef, useEffect, useState } from 'react';
import { Box, Typography, Paper, Chip, IconButton, Tooltip } from '@mui/material';
import { ZoomIn, ZoomOut, FitScreen, PlayArrow, Pause } from '@mui/icons-material';
import { AnimationCurve, Keyframe } from '../../api/gameEngine';

interface CurveViewerProps {
  curves: AnimationCurve[];
  duration: number;
  currentTime?: number;
  onTimeChange?: (time: number) => void;
}

export const CurveViewer: React.FC<CurveViewerProps> = ({ 
  curves, 
  duration, 
  currentTime = 0,
  onTimeChange 
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [zoom, setZoom] = useState(1);
  const [offset, setOffset] = useState({ x: 0, y: 0 });
  const [selectedCurve, setSelectedCurve] = useState<number | null>(null);
  const [isPlaying, setIsPlaying] = useState(false);
  const [playbackTime, setPlaybackTime] = useState(0);
  const animationRef = useRef<number>();

  // Colors for different curves
  const curveColors = [
    '#ff6b6b', '#4ecdc4', '#45b7d1', '#f9ca24', 
    '#f0932b', '#eb4d4b', '#6ab04c', '#130f40'
  ];

  // Draw the curves on canvas
  const drawCurves = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // Fill background
    ctx.fillStyle = '#fafafa';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // Draw grid
    ctx.strokeStyle = '#e0e0e0';
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
    ctx.strokeStyle = '#9e9e9e';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(50, 0);
    ctx.lineTo(50, canvas.height);
    ctx.moveTo(0, canvas.height / 2);
    ctx.lineTo(canvas.width, canvas.height / 2);
    ctx.stroke();

    // Draw curves
    curves.forEach((curve, index) => {
      if (curve.keyframes.length < 2) return;

      ctx.strokeStyle = curveColors[index % curveColors.length];
      ctx.lineWidth = selectedCurve === index ? 3 : 2;
      ctx.globalAlpha = selectedCurve === null || selectedCurve === index ? 1 : 0.3;

      ctx.beginPath();
      
      // Convert keyframes to canvas coordinates
      const timeScale = (canvas.width - 100) / duration * zoom;
      const valueScale = 100 * zoom;
      const centerY = canvas.height / 2;

      curve.keyframes.forEach((keyframe, i) => {
        const x = 50 + keyframe.time * timeScale + offset.x;
        const y = centerY - keyframe.value * valueScale + offset.y;

        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          // Linear interpolation for now
          ctx.lineTo(x, y);
        }

        // Draw keyframe point
        ctx.fillStyle = ctx.strokeStyle;
        ctx.fillRect(x - 3, y - 3, 6, 6);
      });

      ctx.stroke();
      ctx.globalAlpha = 1;
    });

    // Draw current time indicator
    if (currentTime >= 0 && currentTime <= duration) {
      const timeScale = (canvas.width - 100) / duration * zoom;
      const x = 50 + currentTime * timeScale + offset.x;
      
      ctx.strokeStyle = '#ffd700';
      ctx.lineWidth = 2;
      ctx.setLineDash([5, 5]);
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
      ctx.setLineDash([]);
    }

    // Draw time labels
    ctx.fillStyle = '#616161';
    ctx.font = '12px monospace';
    const timeStep = duration / 10;
    for (let t = 0; t <= duration; t += timeStep) {
      const x = 50 + t * (canvas.width - 100) / duration * zoom + offset.x;
      ctx.fillText(t.toFixed(1) + 's', x - 15, canvas.height - 5);
    }
  };

  useEffect(() => {
    drawCurves();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [curves, zoom, offset, selectedCurve, currentTime]);

  // Handle canvas resize
  useEffect(() => {
    const handleResize = () => {
      if (canvasRef.current) {
        const container = canvasRef.current.parentElement;
        if (container) {
          canvasRef.current.width = container.clientWidth;
          canvasRef.current.height = container.clientHeight;
          drawCurves();
        }
      }
    };

    handleResize();
    window.addEventListener('resize', handleResize);
    return () => window.removeEventListener('resize', handleResize);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Animation playback
  useEffect(() => {
    if (isPlaying) {
      let lastTime = performance.now();
      const animate = (currentTime: number) => {
        const deltaTime = (currentTime - lastTime) / 1000;
        lastTime = currentTime;
        
        setPlaybackTime(prev => {
          const newTime = prev + deltaTime;
          if (newTime >= duration) {
            setIsPlaying(false);
            return 0;
          }
          onTimeChange?.(newTime);
          return newTime;
        });
        
        if (isPlaying) {
          animationRef.current = requestAnimationFrame(animate);
        }
      };
      
      animationRef.current = requestAnimationFrame(animate);
    } else {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    }
    
    return () => {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, [isPlaying, duration, onTimeChange]);

  const handleZoomIn = () => setZoom(prev => Math.min(prev * 1.2, 5));
  const handleZoomOut = () => setZoom(prev => Math.max(prev / 1.2, 0.2));
  const handleFitScreen = () => {
    setZoom(1);
    setOffset({ x: 0, y: 0 });
  };

  return (
    <Box sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
      {/* Toolbar */}
      <Box sx={{ 
        display: 'flex', 
        alignItems: 'center', 
        gap: 1, 
        p: 1, 
        borderBottom: 1,
        borderColor: 'divider'
      }}>
        <Typography variant="caption" sx={{ mr: 2 }}>
          Curves: {curves.length}
        </Typography>
        
        <Tooltip title="Play">
          <IconButton size="small" onClick={() => setIsPlaying(!isPlaying)}>
            {isPlaying ? <Pause /> : <PlayArrow />}
          </IconButton>
        </Tooltip>
        
        <Tooltip title="Zoom In">
          <IconButton size="small" onClick={handleZoomIn}>
            <ZoomIn />
          </IconButton>
        </Tooltip>
        
        <Tooltip title="Zoom Out">
          <IconButton size="small" onClick={handleZoomOut}>
            <ZoomOut />
          </IconButton>
        </Tooltip>
        
        <Tooltip title="Fit to Screen">
          <IconButton size="small" onClick={handleFitScreen}>
            <FitScreen />
          </IconButton>
        </Tooltip>
        
        <Typography variant="caption" sx={{ ml: 2 }}>
          Time: {playbackTime.toFixed(2)}s / {duration.toFixed(2)}s
        </Typography>
      </Box>

      {/* Canvas */}
      <Box sx={{ flex: 1, position: 'relative', overflow: 'hidden' }}>
        <canvas
          ref={canvasRef}
          style={{
            width: '100%',
            height: '100%',
            cursor: 'crosshair'
          }}
          onClick={(e) => {
            const rect = canvasRef.current?.getBoundingClientRect();
            if (rect && onTimeChange) {
              const x = e.clientX - rect.left - 50;
              const timeScale = (rect.width - 100) / duration * zoom;
              const time = Math.max(0, Math.min(duration, x / timeScale));
              onTimeChange(time);
              setPlaybackTime(time);
            }
          }}
        />
      </Box>

      {/* Curve List */}
      <Box sx={{ 
        p: 1, 
        borderTop: 1,
        borderColor: 'divider',
        maxHeight: 150,
        overflow: 'auto'
      }}>
        <Typography variant="caption" sx={{ display: 'block', mb: 1 }}>
          Property Curves:
        </Typography>
        <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 0.5 }}>
          {curves.map((curve, index) => (
            <Chip
              key={index}
              label={curve.propertyPath}
              size="small"
              sx={{
                backgroundColor: selectedCurve === index ? 
                  curveColors[index % curveColors.length] : 'transparent',
                color: selectedCurve === index ? '#fff' : 'text.primary',
                borderColor: curveColors[index % curveColors.length],
                cursor: 'pointer',
                '&:hover': {
                  backgroundColor: curveColors[index % curveColors.length],
                  color: '#fff',
                  opacity: 0.8
                }
              }}
              variant="outlined"
              onClick={() => setSelectedCurve(selectedCurve === index ? null : index)}
            />
          ))}
        </Box>
      </Box>
    </Box>
  );
};