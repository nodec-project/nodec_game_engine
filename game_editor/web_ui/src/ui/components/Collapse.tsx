'use client';

import React from 'react';
import { Collapsible as CollapsiblePrimitive } from '@base-ui/react/collapsible';
import styles from './Collapse.module.css';

export interface CollapseProps {
  /** Whether the content is visible */
  in: boolean;
  /** Unmount children when collapsed */
  unmountOnExit?: boolean;
  /** Children to render */
  children: React.ReactNode;
  /** Additional class name */
  className?: string;
}

/**
 * Collapse component using Base UI Collapsible
 *
 * Animates showing/hiding content with a height transition.
 */
export const Collapse: React.FC<CollapseProps> = ({
  in: isOpen,
  unmountOnExit = false,
  children,
  className,
}) => {
  return (
    <CollapsiblePrimitive.Root open={isOpen}>
      <CollapsiblePrimitive.Panel
        className={`${styles.panel} ${className || ''}`}
        keepMounted={!unmountOnExit}
      >
        {children}
      </CollapsiblePrimitive.Panel>
    </CollapsiblePrimitive.Root>
  );
};
