'use client';

import React, { forwardRef } from 'react';
import { Checkbox as CheckboxPrimitive } from '@base-ui/react/checkbox';
import styles from './Checkbox.module.css';

export interface CheckboxProps {
  /** Whether the checkbox is checked */
  checked?: boolean;
  /** Default checked state (uncontrolled) */
  defaultChecked?: boolean;
  /** Callback when checked state changes */
  onChange?: (event: React.ChangeEvent<HTMLInputElement>, checked: boolean) => void;
  /** Whether the checkbox is disabled */
  disabled?: boolean;
  /** Whether the checkbox is in indeterminate state */
  indeterminate?: boolean;
  /** Size of the checkbox */
  size?: 'small' | 'medium';
  /** Color variant */
  color?: 'primary' | 'secondary' | 'error';
  /** Name attribute */
  name?: string;
  /** Value attribute */
  value?: string;
  /** Additional class name */
  className?: string;
}

const CheckIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" />
  </svg>
);

const IndeterminateIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M19 13H5v-2h14v2z" />
  </svg>
);

export const Checkbox = forwardRef<HTMLButtonElement, CheckboxProps>(
  (
    {
      checked,
      defaultChecked,
      onChange,
      disabled = false,
      indeterminate = false,
      size = 'medium',
      color = 'primary',
      name,
      value,
      className,
    },
    ref
  ) => {
    const handleCheckedChange = (isChecked: boolean) => {
      if (onChange) {
        const syntheticEvent = {
          target: { checked: isChecked, name, value },
        } as React.ChangeEvent<HTMLInputElement>;
        onChange(syntheticEvent, isChecked);
      }
    };

    return (
      <CheckboxPrimitive.Root
        ref={ref}
        checked={checked}
        defaultChecked={defaultChecked}
        onCheckedChange={handleCheckedChange}
        disabled={disabled}
        indeterminate={indeterminate}
        name={name}
        className={`${styles.checkbox} ${styles[size]} ${styles[color]} ${className || ''}`}
      >
        <CheckboxPrimitive.Indicator className={styles.indicator}>
          {indeterminate ? <IndeterminateIcon /> : <CheckIcon />}
        </CheckboxPrimitive.Indicator>
      </CheckboxPrimitive.Root>
    );
  }
);

Checkbox.displayName = 'Checkbox';
