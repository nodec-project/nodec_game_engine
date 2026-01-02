'use client';

import React, { forwardRef } from 'react';
import styles from './Button.module.css';

export interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  /** Visual variant */
  variant?: 'filled' | 'filledTonal' | 'outlined' | 'text' | 'elevated';
  /** Size of the button */
  size?: 'small' | 'medium' | 'large';
  /** Icon to display at the start */
  startIcon?: React.ReactNode;
  /** Icon to display at the end */
  endIcon?: React.ReactNode;
  /** Whether the button should take full width */
  fullWidth?: boolean;
  /** Button content */
  children: React.ReactNode;
}

/**
 * Button component
 *
 * A button for actions, following Material Design 3 specifications.
 */
export const Button = forwardRef<HTMLButtonElement, ButtonProps>(
  (
    {
      variant = 'filled',
      size = 'medium',
      startIcon,
      endIcon,
      fullWidth = false,
      className,
      children,
      ...props
    },
    ref
  ) => {
    const classNames = [
      styles.button,
      styles[variant],
      styles[size],
      startIcon && styles.iconStart,
      endIcon && styles.iconEnd,
      fullWidth && styles.fullWidth,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <button ref={ref} type="button" className={classNames} {...props}>
        {startIcon}
        {children}
        {endIcon}
      </button>
    );
  }
);

Button.displayName = 'Button';
