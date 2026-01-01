'use client';

import React, { forwardRef } from 'react';
import styles from './List.module.css';

// List
export interface ListProps extends React.HTMLAttributes<HTMLUListElement> {
  /** Reduce padding for denser list */
  dense?: boolean;
  /** List items */
  children: React.ReactNode;
}

export const List = forwardRef<HTMLUListElement, ListProps>(
  ({ dense = false, className, children, ...props }, ref) => {
    const classNames = [styles.list, dense && styles.listDense, className]
      .filter(Boolean)
      .join(' ');

    return (
      <ul ref={ref} className={classNames} role="list" {...props}>
        {children}
      </ul>
    );
  }
);

List.displayName = 'List';

// ListItem
export interface ListItemProps extends React.LiHTMLAttributes<HTMLLIElement> {
  /** Reduce padding for denser item */
  dense?: boolean;
  /** Whether the item is selected */
  selected?: boolean;
  /** Whether the item is disabled */
  disabled?: boolean;
  /** Indentation level (0-5) */
  indent?: 0 | 1 | 2 | 3 | 4 | 5;
  /** Whether to remove default padding (for custom content) */
  disablePadding?: boolean;
  /** Secondary action element (e.g., icon buttons) */
  secondaryAction?: React.ReactNode;
  /** Children */
  children: React.ReactNode;
}

export const ListItem = forwardRef<HTMLLIElement, ListItemProps>(
  (
    {
      dense = false,
      selected = false,
      disabled = false,
      indent = 0,
      disablePadding = false,
      secondaryAction,
      className,
      children,
      ...props
    },
    ref
  ) => {
    const classNames = [
      styles.listItem,
      dense && styles.listItemDense,
      selected && styles.listItemSelected,
      disabled && styles.listItemDisabled,
      indent > 0 && styles[`indent${indent}`],
      disablePadding && styles.listItemNoPadding,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <li ref={ref} className={classNames} {...props}>
        {children}
        {secondaryAction && <div className={styles.listItemAction}>{secondaryAction}</div>}
      </li>
    );
  }
);

ListItem.displayName = 'ListItem';

// ListItemButton
export interface ListItemButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  /** Reduce padding for denser item */
  dense?: boolean;
  /** Whether the item is selected */
  selected?: boolean;
  /** Whether the item is disabled */
  disabled?: boolean;
  /** Indentation level (0-5) */
  indent?: 0 | 1 | 2 | 3 | 4 | 5;
  /** Children */
  children: React.ReactNode;
}

export const ListItemButton = forwardRef<HTMLButtonElement, ListItemButtonProps>(
  ({ dense = false, selected = false, disabled = false, indent = 0, className, children, ...props }, ref) => {
    const classNames = [
      styles.listItem,
      styles.listItemButton,
      dense && styles.listItemDense,
      selected && styles.listItemSelected,
      disabled && styles.listItemDisabled,
      indent > 0 && styles[`indent${indent}`],
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <button ref={ref} type="button" className={classNames} disabled={disabled} {...props}>
        {children}
      </button>
    );
  }
);

ListItemButton.displayName = 'ListItemButton';

// ListItemIcon
export interface ListItemIconProps extends React.HTMLAttributes<HTMLSpanElement> {
  /** Minimum width preset */
  minWidth?: 24 | 32 | 'auto';
  /** Children (icon) */
  children: React.ReactNode;
}

export const ListItemIcon = forwardRef<HTMLSpanElement, ListItemIconProps>(
  ({ minWidth = 32, className, children, ...props }, ref) => {
    const minWidthClass =
      minWidth === 'auto' ? styles.listItemIconAuto : styles[`minWidth${minWidth}`];

    const classNames = [styles.listItemIcon, minWidthClass, className].filter(Boolean).join(' ');

    return (
      <span ref={ref} className={classNames} {...props}>
        {children}
      </span>
    );
  }
);

ListItemIcon.displayName = 'ListItemIcon';

// ListItemText
export interface ListItemTextProps extends React.HTMLAttributes<HTMLDivElement> {
  /** Primary text */
  primary: React.ReactNode;
  /** Secondary text */
  secondary?: React.ReactNode;
  /** Use smaller font size */
  small?: boolean;
}

export const ListItemText = forwardRef<HTMLDivElement, ListItemTextProps>(
  ({ primary, secondary, small = false, className, ...props }, ref) => {
    return (
      <div ref={ref} className={`${styles.listItemText} ${className || ''}`} {...props}>
        <span className={`${styles.listItemPrimary} ${small ? styles.listItemPrimarySmall : ''}`}>
          {primary}
        </span>
        {secondary && (
          <span
            className={`${styles.listItemSecondary} ${small ? styles.listItemSecondarySmall : ''}`}
          >
            {secondary}
          </span>
        )}
      </div>
    );
  }
);

ListItemText.displayName = 'ListItemText';

// ListDivider
export interface ListDividerProps extends React.HTMLAttributes<HTMLLIElement> {
  /** Inset the divider to align with list item text */
  inset?: boolean;
}

export const ListDivider = forwardRef<HTMLLIElement, ListDividerProps>(
  ({ inset = false, className, ...props }, ref) => {
    const classNames = [styles.listDivider, inset && styles.listDividerInset, className]
      .filter(Boolean)
      .join(' ');

    return <li ref={ref} className={classNames} role="separator" {...props} />;
  }
);

ListDivider.displayName = 'ListDivider';

// ListSubheader
export interface ListSubheaderProps extends React.HTMLAttributes<HTMLLIElement> {
  children: React.ReactNode;
}

export const ListSubheader = forwardRef<HTMLLIElement, ListSubheaderProps>(
  ({ className, children, ...props }, ref) => {
    return (
      <li ref={ref} className={`${styles.listSubheader} ${className || ''}`} {...props}>
        {children}
      </li>
    );
  }
);

ListSubheader.displayName = 'ListSubheader';
