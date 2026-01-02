'use client';

import React, { forwardRef } from 'react';
import { Separator } from '@base-ui/react/separator';
import styles from './Divider.module.css';

export interface DividerProps extends React.HTMLAttributes<HTMLDivElement> {
  /** Orientation of the divider */
  orientation?: 'horizontal' | 'vertical';
  /** Inset variant */
  inset?: 'none' | 'start' | 'end' | 'middle';
  /** Spacing around the divider */
  spacing?: 'none' | 'small' | 'medium' | 'large';
  /** Text to display in the middle of the divider */
  text?: string;
}

/**
 * Divider component using Base UI Separator
 *
 * A thin line that separates content.
 */
export const Divider = forwardRef<HTMLDivElement, DividerProps>(
  (
    {
      orientation = 'horizontal',
      inset = 'none',
      spacing = 'none',
      text,
      className,
      ...props
    },
    ref
  ) => {
    const insetClass = inset !== 'none' ? styles[`inset${inset.charAt(0).toUpperCase() + inset.slice(1)}`] : '';
    const spacingClass = spacing !== 'none' ? styles[`spacing${spacing.charAt(0).toUpperCase() + spacing.slice(1)}`] : '';

    const classNames = [
      styles.divider,
      styles[orientation],
      insetClass,
      spacingClass,
      text && styles.withText,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    if (text) {
      return (
        <div ref={ref} className={classNames} role="separator" aria-orientation={orientation} {...props}>
          <span className={styles.textContent}>{text}</span>
        </div>
      );
    }

    return (
      <Separator
        ref={ref}
        orientation={orientation}
        className={classNames}
        {...props}
      />
    );
  }
);

Divider.displayName = 'Divider';
