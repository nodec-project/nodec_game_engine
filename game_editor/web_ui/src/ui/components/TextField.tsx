'use client';

import React, { forwardRef } from 'react';
import { Field as FieldPrimitive } from '@base-ui/react/field';
import { Input as InputPrimitive } from '@base-ui/react/input';
import styles from './TextField.module.css';

// Common props shared between input and textarea
interface BaseTextFieldProps {
  /** Label for the text field */
  label?: string;
  /** Visual variant */
  variant?: 'outlined' | 'filled';
  /** Size of the text field */
  size?: 'small' | 'medium' | 'large';
  /** Whether the field has an error */
  error?: boolean;
  /** Helper or error text */
  helperText?: string;
  /** Whether the field should take full width */
  fullWidth?: boolean;
  /** Leading icon */
  leadingIcon?: React.ReactNode;
  /** Trailing icon */
  trailingIcon?: React.ReactNode;
  /** Whether the field has unsaved changes */
  dirty?: boolean;
  /** Additional class name */
  className?: string;
  /** Whether the field is disabled */
  disabled?: boolean;
}

// Single-line input props
interface SingleLineTextFieldProps extends BaseTextFieldProps, Omit<React.InputHTMLAttributes<HTMLInputElement>, 'size'> {
  /** Whether to render as textarea */
  multiline?: false;
  /** Number of rows (not applicable for single-line) */
  rows?: never;
}

// Multi-line textarea props
interface MultiLineTextFieldProps extends BaseTextFieldProps, Omit<React.TextareaHTMLAttributes<HTMLTextAreaElement>, 'size'> {
  /** Whether to render as textarea */
  multiline: true;
  /** Number of rows for multiline */
  rows?: number;
}

export type TextFieldProps = SingleLineTextFieldProps | MultiLineTextFieldProps;

/**
 * TextField component using Base UI Field + Input
 *
 * Text input field following Material Design 3 specifications.
 */
export const TextField = forwardRef<HTMLInputElement | HTMLTextAreaElement, TextFieldProps>(
  (props, ref) => {
    const {
      label,
      variant = 'outlined',
      size = 'medium',
      error = false,
      helperText,
      fullWidth = false,
      leadingIcon,
      trailingIcon,
      dirty = false,
      className,
      disabled,
      multiline,
      ...restProps
    } = props;

    const containerClassNames = [
      styles.textField,
      styles[variant],
      styles[size],
      error && styles.error,
      disabled && styles.disabled,
      fullWidth && styles.fullWidth,
      leadingIcon && styles.hasLeadingIcon,
      trailingIcon && styles.hasTrailingIcon,
      dirty && styles.dirty,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <FieldPrimitive.Root className={containerClassNames} disabled={disabled} invalid={error}>
        {label && (
          <FieldPrimitive.Label className={styles.label}>
            {label}
          </FieldPrimitive.Label>
        )}
        <div className={styles.inputContainer}>
          {leadingIcon && <span className={styles.leadingIcon}>{leadingIcon}</span>}
          {multiline ? (
            <textarea
              ref={ref as React.Ref<HTMLTextAreaElement>}
              className={`${styles.input} ${styles.textarea}`}
              rows={(restProps as MultiLineTextFieldProps).rows ?? 4}
              disabled={disabled}
              {...(restProps as Omit<MultiLineTextFieldProps, keyof BaseTextFieldProps | 'multiline' | 'rows'>)}
            />
          ) : (
            <InputPrimitive
              ref={ref as React.Ref<HTMLInputElement>}
              className={styles.input}
              {...(restProps as Omit<SingleLineTextFieldProps, keyof BaseTextFieldProps | 'multiline' | 'rows'>)}
            />
          )}
          {trailingIcon && <span className={styles.trailingIcon}>{trailingIcon}</span>}
        </div>
        {helperText && !error && (
          <FieldPrimitive.Description className={styles.helperText}>
            {helperText}
          </FieldPrimitive.Description>
        )}
        {error && helperText && (
          <FieldPrimitive.Error className={styles.errorText} match>
            {helperText}
          </FieldPrimitive.Error>
        )}
      </FieldPrimitive.Root>
    );
  }
);

TextField.displayName = 'TextField';
