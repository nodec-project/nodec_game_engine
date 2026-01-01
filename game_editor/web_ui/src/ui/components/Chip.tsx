'use client';

import React, { forwardRef } from 'react';
import { Icon } from './Icon';
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
            <Icon name="close" size={18} />
          </button>
        )}
      </div>
    );
  }
);

Chip.displayName = 'Chip';
