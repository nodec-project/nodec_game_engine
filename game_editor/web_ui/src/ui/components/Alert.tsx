'use client';

import React, { forwardRef } from 'react';
import { Icon } from './Icon';
import styles from './Alert.module.css';

// Icon names for each severity
const severityIconNames: Record<string, string> = {
  info: 'info',
  success: 'check_circle',
  warning: 'warning',
  error: 'error',
};

export interface AlertProps extends React.HTMLAttributes<HTMLDivElement> {
  /** Severity of the alert */
  severity?: 'info' | 'success' | 'warning' | 'error';
  /** Variant of the alert */
  variant?: 'standard' | 'filled';
  /** Title of the alert */
  title?: string;
  /** Custom icon (set to null to hide icon) */
  icon?: React.ReactNode | null;
  /** Callback when close button is clicked */
  onClose?: () => void;
  /** Action buttons */
  action?: React.ReactNode;
  /** Alert content */
  children: React.ReactNode;
}

/**
 * Alert component
 *
 * Displays a message with severity level.
 */
export const Alert = forwardRef<HTMLDivElement, AlertProps>(
  (
    {
      severity = 'info',
      variant = 'standard',
      title,
      icon,
      onClose,
      action,
      className,
      children,
      ...props
    },
    ref
  ) => {
    const iconName = severityIconNames[severity];
    const showIcon = icon !== null;

    const classNames = [
      styles.alert,
      styles[severity],
      variant === 'filled' && styles.filled,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <div ref={ref} role="alert" className={classNames} {...props}>
        {showIcon && (
          <span className={styles.icon}>
            {icon !== undefined ? icon : <Icon name={iconName} />}
          </span>
        )}
        <div className={styles.content}>
          {title && <div className={styles.title}>{title}</div>}
          <div className={styles.message}>{children}</div>
          {action && <div className={styles.actions}>{action}</div>}
        </div>
        {onClose && (
          <button
            type="button"
            className={styles.closeButton}
            onClick={onClose}
            aria-label="Close alert"
          >
            <Icon name="close" size={18} />
          </button>
        )}
      </div>
    );
  }
);

Alert.displayName = 'Alert';
