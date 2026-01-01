'use client';

import React, { forwardRef } from 'react';
import styles from './Alert.module.css';

// Default icons for each severity
const InfoIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z" />
  </svg>
);

const SuccessIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z" />
  </svg>
);

const WarningIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z" />
  </svg>
);

const ErrorIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z" />
  </svg>
);

const CloseIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor" width="18" height="18">
    <path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" />
  </svg>
);

const severityIcons = {
  info: InfoIcon,
  success: SuccessIcon,
  warning: WarningIcon,
  error: ErrorIcon,
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
    const IconComponent = severityIcons[severity];
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
            {icon !== undefined ? icon : <IconComponent />}
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
            <CloseIcon />
          </button>
        )}
      </div>
    );
  }
);

Alert.displayName = 'Alert';
