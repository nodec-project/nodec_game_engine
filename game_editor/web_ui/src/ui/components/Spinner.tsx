'use client';

import React, { forwardRef } from 'react';
import { Progress as ProgressPrimitive } from '@base-ui/react/progress';
import styles from './Spinner.module.css';

export interface SpinnerProps extends React.HTMLAttributes<HTMLDivElement> {
  /** Size of the spinner */
  size?: 'small' | 'medium' | 'large' | number;
  /** Color variant */
  color?: 'inherit' | 'primary' | 'secondary' | 'onSurface';
  /** Value for determinate progress (0-100) */
  value?: number;
  /** Thickness of the spinner stroke */
  thickness?: number;
}

/**
 * Spinner component using Base UI Progress
 *
 * Displays a loading indicator (circular progress).
 */
export const Spinner = forwardRef<HTMLDivElement, SpinnerProps>(
  (
    {
      size = 'medium',
      color = 'primary',
      value,
      thickness = 4,
      className,
      style,
      ...props
    },
    ref
  ) => {
    const isDeterminate = value !== undefined;
    const circumference = 44 * Math.PI; // 2 * PI * radius (radius = 22)
    const strokeDashoffset = isDeterminate
      ? circumference - (value / 100) * circumference
      : undefined;

    const sizeClass = typeof size === 'string' ? styles[size] : undefined;
    const customSize = typeof size === 'number' ? { width: size, height: size } : undefined;

    const classNames = [
      styles.spinner,
      styles.circular,
      sizeClass,
      styles[color],
      isDeterminate && styles.determinate,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <ProgressPrimitive.Root
        ref={ref}
        value={isDeterminate ? value : null}
        className={classNames}
        style={{ ...customSize, ...style }}
        {...props}
      >
        <ProgressPrimitive.Track className={styles.track}>
          <svg viewBox="22 22 44 44" className={styles.svg}>
            {isDeterminate && (
              <circle
                className={styles.trackCircle}
                cx="44"
                cy="44"
                r="20"
                fill="none"
                strokeWidth={thickness}
              />
            )}
            <ProgressPrimitive.Indicator
              render={
                <circle
                  className={styles.circularPath}
                  cx="44"
                  cy="44"
                  r="20"
                  fill="none"
                  strokeWidth={thickness}
                  strokeDasharray={isDeterminate ? circumference : undefined}
                  strokeDashoffset={strokeDashoffset}
                />
              }
            />
          </svg>
        </ProgressPrimitive.Track>
      </ProgressPrimitive.Root>
    );
  }
);

Spinner.displayName = 'Spinner';
