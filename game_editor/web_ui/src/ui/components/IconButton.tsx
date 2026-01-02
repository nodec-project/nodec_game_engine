'use client';

import React, { forwardRef } from 'react';
import styles from './IconButton.module.css';

export interface IconButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  /** Size of the button */
  size?: 'small' | 'medium' | 'large';
  /** Visual variant */
  variant?: 'standard' | 'primary' | 'filled' | 'filledTonal' | 'outlined';
  /** Color variant (MUI compatibility) */
  color?: 'default' | 'primary' | 'secondary' | 'error' | 'inherit';
  /** Edge placement for removing margin */
  edge?: 'start' | 'end' | false;
  /** Icon to display */
  children: React.ReactNode;
}

/**
 * IconButton component
 *
 * A button for icon-only actions, following Material Design 3 specifications.
 */
export const IconButton = forwardRef<HTMLButtonElement, IconButtonProps>(
  ({ size = 'medium', variant = 'standard', color = 'default', edge = false, className, children, disabled, ...props }, ref) => {
    const edgeClass = edge ? styles[`edge${edge.charAt(0).toUpperCase()}${edge.slice(1)}`] : '';
    const colorClass = color !== 'default' ? styles[`color${color.charAt(0).toUpperCase()}${color.slice(1)}`] : '';

    const classNames = [
      styles.iconButton,
      styles[size],
      styles[variant],
      colorClass,
      edgeClass,
      disabled && styles.disabled,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <button ref={ref} type="button" className={classNames} disabled={disabled} {...props}>
        {children}
      </button>
    );
  }
);

IconButton.displayName = 'IconButton';
