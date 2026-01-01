'use client';

import React, { forwardRef, createContext, useContext, useState } from 'react';
import { Tabs as TabsPrimitive } from '@base-ui/react/tabs';
import styles from './Tabs.module.css';

// Tabs Root
export interface TabsProps {
  /** Currently selected tab value */
  value?: string | number;
  /** Default selected tab (uncontrolled) */
  defaultValue?: string | number;
  /** Callback when tab changes */
  onChange?: (event: React.SyntheticEvent, value: string | number) => void;
  /** Orientation */
  orientation?: 'horizontal' | 'vertical';
  /** Children */
  children: React.ReactNode;
  /** Additional class name */
  className?: string;
}

export const Tabs = forwardRef<HTMLDivElement, TabsProps>(
  ({ value, defaultValue, onChange, orientation = 'horizontal', className, children }, ref) => {
    const handleValueChange = (newValue: string | number | null) => {
      if (newValue !== null && onChange) {
        onChange({} as React.SyntheticEvent, newValue);
      }
    };

    return (
      <TabsPrimitive.Root
        ref={ref}
        value={value}
        defaultValue={defaultValue}
        onValueChange={handleValueChange}
        orientation={orientation}
        className={`${styles.tabs} ${styles[orientation]} ${className || ''}`}
      >
        {children}
      </TabsPrimitive.Root>
    );
  }
);
Tabs.displayName = 'Tabs';

// Tab List
export interface TabListProps {
  children: React.ReactNode;
  className?: string;
}

export const TabList = forwardRef<HTMLDivElement, TabListProps>(
  ({ className, children }, ref) => {
    return (
      <TabsPrimitive.List ref={ref} className={`${styles.tabList} ${className || ''}`}>
        {children}
      </TabsPrimitive.List>
    );
  }
);
TabList.displayName = 'TabList';

// Tab
export interface TabProps {
  /** Tab value */
  value: string | number;
  /** Tab label */
  label?: string;
  /** Icon to display */
  icon?: React.ReactNode;
  /** Icon position */
  iconPosition?: 'start' | 'end' | 'top' | 'bottom';
  /** Whether the tab is disabled */
  disabled?: boolean;
  /** Children (alternative to label) */
  children?: React.ReactNode;
  /** Additional class name */
  className?: string;
}

export const Tab = forwardRef<HTMLButtonElement, TabProps>(
  ({ value, label, icon, iconPosition = 'start', disabled = false, children, className }, ref) => {
    const content = children ?? label;

    return (
      <TabsPrimitive.Tab
        ref={ref}
        value={value}
        disabled={disabled}
        className={`${styles.tab} ${icon ? styles[`icon${iconPosition.charAt(0).toUpperCase()}${iconPosition.slice(1)}`] : ''} ${className || ''}`}
      >
        {icon && (iconPosition === 'start' || iconPosition === 'top') && (
          <span className={styles.tabIcon}>{icon}</span>
        )}
        {content && <span className={styles.tabLabel}>{content}</span>}
        {icon && (iconPosition === 'end' || iconPosition === 'bottom') && (
          <span className={styles.tabIcon}>{icon}</span>
        )}
      </TabsPrimitive.Tab>
    );
  }
);
Tab.displayName = 'Tab';

// Tab Panel
export interface TabPanelProps {
  /** Panel value (must match corresponding Tab value) */
  value: string | number;
  /** Keep mounted when not active */
  keepMounted?: boolean;
  /** Children */
  children: React.ReactNode;
  /** Additional class name */
  className?: string;
}

export const TabPanel = forwardRef<HTMLDivElement, TabPanelProps>(
  ({ value, keepMounted = false, className, children }, ref) => {
    return (
      <TabsPrimitive.Panel
        ref={ref}
        value={value}
        keepMounted={keepMounted}
        className={`${styles.tabPanel} ${className || ''}`}
      >
        {children}
      </TabsPrimitive.Panel>
    );
  }
);
TabPanel.displayName = 'TabPanel';
