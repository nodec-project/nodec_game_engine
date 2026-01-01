'use client';

import React, { forwardRef } from 'react';
import { Accordion as AccordionPrimitive } from '@base-ui/react/accordion';
import styles from './Accordion.module.css';

// Default expand icon
const ExpandMoreIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor" width="24" height="24">
    <path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" />
  </svg>
);

// Accordion Root
export interface AccordionProps {
  /** Array of values of expanded items (controlled) */
  value?: string[];
  /** Default expanded items (uncontrolled) */
  defaultValue?: string[];
  /** Callback when expanded items change */
  onValueChange?: (value: string[]) => void;
  /** Whether only one item can be expanded at a time */
  single?: boolean;
  /** Whether the accordion is disabled */
  disabled?: boolean;
  /** Use outlined variant */
  outlined?: boolean;
  /** Children (AccordionItem components) */
  children: React.ReactNode;
  /** Additional class name */
  className?: string;
}

export const Accordion = forwardRef<HTMLDivElement, AccordionProps>(
  (
    {
      value,
      defaultValue,
      onValueChange,
      single = false,
      disabled = false,
      outlined = false,
      className,
      children,
    },
    ref
  ) => {
    const classNames = [
      styles.accordion,
      outlined && styles.outlined,
      disabled && styles.accordionDisabled,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <AccordionPrimitive.Root
        ref={ref}
        className={classNames}
        value={value}
        defaultValue={defaultValue}
        onValueChange={onValueChange}
        disabled={disabled}
      >
        {children}
      </AccordionPrimitive.Root>
    );
  }
);

Accordion.displayName = 'Accordion';

// Accordion Item
export interface AccordionItemProps {
  /** Unique value for this item */
  value: string;
  /** Whether this item is disabled */
  disabled?: boolean;
  /** Children (AccordionHeader and AccordionPanel) */
  children: React.ReactNode;
  /** Additional class name */
  className?: string;
}

export const AccordionItem = forwardRef<HTMLDivElement, AccordionItemProps>(
  ({ value, disabled = false, className, children }, ref) => {
    return (
      <AccordionPrimitive.Item
        ref={ref}
        value={value}
        disabled={disabled}
        className={`${styles.accordionItem} ${className || ''}`}
      >
        {children}
      </AccordionPrimitive.Item>
    );
  }
);

AccordionItem.displayName = 'AccordionItem';

// Accordion Header (Trigger/Summary)
export interface AccordionHeaderProps {
  /** Custom expand icon */
  expandIcon?: React.ReactNode;
  /** Children */
  children: React.ReactNode;
  /** Additional class name */
  className?: string;
}

export const AccordionHeader = forwardRef<HTMLButtonElement, AccordionHeaderProps>(
  ({ expandIcon, className, children }, ref) => {
    return (
      <AccordionPrimitive.Header className={styles.accordionHeader}>
        <AccordionPrimitive.Trigger
          ref={ref}
          className={`${styles.accordionTrigger} ${className || ''}`}
        >
          <span className={styles.accordionTriggerContent}>{children}</span>
          <span className={styles.expandIcon}>
            {expandIcon || <ExpandMoreIcon />}
          </span>
        </AccordionPrimitive.Trigger>
      </AccordionPrimitive.Header>
    );
  }
);

AccordionHeader.displayName = 'AccordionHeader';

// Accordion Panel (Details/Content)
export interface AccordionPanelProps {
  /** Whether to keep mounted when collapsed */
  keepMounted?: boolean;
  /** Children */
  children: React.ReactNode;
  /** Additional class name */
  className?: string;
}

export const AccordionPanel = forwardRef<HTMLDivElement, AccordionPanelProps>(
  ({ keepMounted = false, className, children }, ref) => {
    return (
      <AccordionPrimitive.Panel
        ref={ref}
        className={`${styles.accordionPanel} ${className || ''}`}
        keepMounted={keepMounted}
      >
        {children}
      </AccordionPrimitive.Panel>
    );
  }
);

AccordionPanel.displayName = 'AccordionPanel';

// Legacy API - AccordionSummary (alias for AccordionHeader)
export const AccordionSummary = AccordionHeader;

// Legacy API - AccordionDetails (alias for AccordionPanel)
export const AccordionDetails = AccordionPanel;
