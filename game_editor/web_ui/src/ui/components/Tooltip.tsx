'use client';

import React from 'react';
import { Tooltip as TooltipPrimitive } from '@base-ui/react/tooltip';
import styles from './Tooltip.module.css';

export interface TooltipProps {
  /** Content to display in the tooltip */
  title: React.ReactNode;
  /** The element that triggers the tooltip */
  children: React.ReactElement;
  /** Placement of the tooltip relative to the trigger */
  placement?: 'top' | 'bottom' | 'left' | 'right';
  /** Delay in ms before showing the tooltip */
  delay?: number;
  /** Whether the tooltip is disabled */
  disabled?: boolean;
}

/**
 * Tooltip component using Base UI
 *
 * Displays a tooltip on hover following Material Design 3 specifications.
 * Uses Base UI for accessibility and positioning.
 */
export const Tooltip: React.FC<TooltipProps> = ({
  title,
  children,
  placement = 'top',
  delay = 200,
  disabled = false,
}) => {
  if (disabled || !title) {
    return children;
  }

  return (
    <TooltipPrimitive.Provider delay={delay}>
      <TooltipPrimitive.Root>
        <TooltipPrimitive.Trigger render={children} />
        <TooltipPrimitive.Portal>
          <TooltipPrimitive.Positioner side={placement} sideOffset={8} className={styles.positioner}>
            <TooltipPrimitive.Popup className={styles.tooltip}>
              {title}
            </TooltipPrimitive.Popup>
          </TooltipPrimitive.Positioner>
        </TooltipPrimitive.Portal>
      </TooltipPrimitive.Root>
    </TooltipPrimitive.Provider>
  );
};
