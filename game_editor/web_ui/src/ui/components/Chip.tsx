'use client';

import React, { forwardRef } from 'react';
import styles from './Chip.module.css';

export interface ChipProps extends Omit<React.HTMLAttributes<HTMLDivElement>, 'onClick'> {
  /** Label text (alternative to children) */
  label?: string;
  /** Children (alternative to label) */
  children?: React.ReactNode;
  /** Chip variant */
  variant?: 'assist' | 'filter' | 'input' | 'suggestion' | 'outlined';
  /** Size of the chip */
  size?: 'small' | 'medium';
  /** Color variant for status indicators */
  color?: 'default' | 'success' | 'warning' | 'error' | 'info';
  /** Whether to use elevated style */
  elevated?: boolean;
  /** Whether the chip is selected (for filter chips) */
  selected?: boolean;
  /** Whether the chip is disabled */
  disabled?: boolean;
  /** Icon to display at the start */
  icon?: React.ReactNode;
  /** Click handler */
  onClick?: (event: React.MouseEvent<HTMLDivElement>) => void;
  /** Delete handler (shows delete button when provided) */
  onDelete?: (event: React.MouseEvent<HTMLButtonElement>) => void;
}

/**
 * Chip component
 *
 * A compact element for input, attributes, or actions.
 */
export const Chip = forwardRef<HTMLDivElement, ChipProps>(
  (
    {
      label,
      children,
      variant = 'assist',
      size = 'medium',
      color = 'default',
      elevated = false,
      selected = false,
      disabled = false,
      icon,
      onClick,
      onDelete,
      className,
      ...props
    },
    ref
  ) => {
    const content = children ?? label;
    const classNames = [
      styles.chip,
      styles[variant],
      styles[size],
      color !== 'default' && styles[color],
      elevated && styles.elevated,
      selected && styles.selected,
      disabled && styles.disabled,
      onClick && styles.clickable,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <div
        ref={ref}
        className={classNames}
        onClick={disabled ? undefined : onClick}
        role={onClick ? 'button' : undefined}
        tabIndex={onClick && !disabled ? 0 : undefined}
        {...props}
      >
        {icon && <span className={styles.leadingIcon}>{icon}</span>}
        <span>{content}</span>
        {onDelete && (
          <button
            type="button"
            className={styles.deleteButton}
            onClick={(e) => {
              e.stopPropagation();
              onDelete(e);
            }}
            disabled={disabled}
            aria-label={label ? `Remove ${label}` : 'Remove'}
          >
            <svg viewBox="0 0 24 24" fill="currentColor">
              <path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" />
            </svg>
          </button>
        )}
      </div>
    );
  }
);

Chip.displayName = 'Chip';
