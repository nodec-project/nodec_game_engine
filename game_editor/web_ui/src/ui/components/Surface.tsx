'use client';

import React, { forwardRef } from 'react';
import styles from './Surface.module.css';

export interface SurfaceProps extends React.HTMLAttributes<HTMLDivElement> {
  /** Elevation level (0-5) */
  elevation?: 0 | 1 | 2 | 3 | 4 | 5;
  /** Whether to use outlined variant instead of elevation */
  outlined?: boolean;
  /** Corner radius */
  radius?: 'none' | 'extraSmall' | 'small' | 'medium' | 'large' | 'extraLarge';
  /** The HTML element or React component to render */
  as?: React.ElementType;
  /** Children */
  children?: React.ReactNode;
}

/**
 * Surface component (Paper replacement)
 *
 * A container surface following Material Design 3 specifications.
 */
export const Surface = forwardRef<HTMLDivElement, SurfaceProps>(
  (
    {
      elevation = 1,
      outlined = false,
      radius = 'medium',
      as: Component = 'div',
      className,
      children,
      ...props
    },
    ref
  ) => {
    const radiusClass = `radius${radius.charAt(0).toUpperCase() + radius.slice(1)}` as keyof typeof styles;

    const classNames = [
      styles.surface,
      outlined ? styles.outlined : styles[`elevation${elevation}`],
      styles[radiusClass],
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <Component ref={ref} className={classNames} {...props}>
        {children}
      </Component>
    );
  }
);

Surface.displayName = 'Surface';
