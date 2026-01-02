'use client';

import React, { forwardRef } from 'react';
import { Checkbox as CheckboxPrimitive } from '@base-ui/react/checkbox';
import { Icon } from './Icon';
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
          {indeterminate ? <Icon name="remove" size={18} /> : <Icon name="check" size={18} />}
        </CheckboxPrimitive.Indicator>
      </CheckboxPrimitive.Root>
    );
  }
);

Checkbox.displayName = 'Checkbox';
