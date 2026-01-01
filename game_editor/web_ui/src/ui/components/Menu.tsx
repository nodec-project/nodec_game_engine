'use client';

import React from 'react';
import { Menu as MenuPrimitive } from '@base-ui/react/menu';
import styles from './Menu.module.css';

// Menu Root
export interface MenuProps {
  /** The menu trigger element */
  trigger: React.ReactElement;
  /** Menu placement */
  placement?: 'bottom-start' | 'bottom-end' | 'top-start' | 'top-end' | 'bottom' | 'top';
  /** Menu content */
  children: React.ReactNode;
}

export const Menu: React.FC<MenuProps> & {
  Item: typeof MenuItem;
  Divider: typeof MenuDivider;
  GroupLabel: typeof MenuGroupLabel;
} = ({ trigger, placement = 'bottom-start', children }) => {
  // Map placement to Base UI side/alignment
  const placementMap: Record<string, { side: 'top' | 'bottom' | 'left' | 'right'; align: 'start' | 'center' | 'end' }> = {
    'bottom-start': { side: 'bottom', align: 'start' },
    'bottom-end': { side: 'bottom', align: 'end' },
    'bottom': { side: 'bottom', align: 'center' },
    'top-start': { side: 'top', align: 'start' },
    'top-end': { side: 'top', align: 'end' },
    'top': { side: 'top', align: 'center' },
  };

  const { side, align } = placementMap[placement] || placementMap['bottom-start'];

  return (
    <MenuPrimitive.Root>
      <MenuPrimitive.Trigger render={trigger} />
      <MenuPrimitive.Portal>
        <MenuPrimitive.Positioner side={side} align={align} sideOffset={4} className={styles.positioner}>
          <MenuPrimitive.Popup className={styles.menu}>
            {children}
          </MenuPrimitive.Popup>
        </MenuPrimitive.Positioner>
      </MenuPrimitive.Portal>
    </MenuPrimitive.Root>
  );
};

// Menu Item
export interface MenuItemProps {
  /** Leading icon */
  icon?: React.ReactNode;
  /** Description text */
  description?: string;
  /** Trailing content (e.g., keyboard shortcut) */
  trailing?: React.ReactNode;
  /** Whether the item is disabled */
  disabled?: boolean;
  /** Click handler */
  onClick?: (event: React.MouseEvent) => void;
  /** Children */
  children: React.ReactNode;
  /** Additional class name */
  className?: string;
}

const MenuItem: React.FC<MenuItemProps> = ({
  icon,
  description,
  trailing,
  disabled = false,
  onClick,
  children,
  className,
}) => {
  const classNames = [styles.menuItem, disabled && styles.disabled, className]
    .filter(Boolean)
    .join(' ');

  // Handle click with proper event propagation
  const handleClick = React.useCallback((event: React.MouseEvent<HTMLElement>) => {
    console.log('[MenuItem] handleClick called, disabled:', disabled);
    if (!disabled && onClick) {
      console.log('[MenuItem] calling user onClick callback');
      onClick(event);
    }
  }, [disabled, onClick]);

  return (
    <MenuPrimitive.Item
      className={classNames}
      disabled={disabled}
      onClick={handleClick}
    >
      {icon && <span className={styles.menuItemIcon}>{icon}</span>}
      <span className={styles.menuItemContent}>
        <span className={styles.menuItemLabel}>{children}</span>
        {description && <span className={styles.menuItemDescription}>{description}</span>}
      </span>
      {trailing && <span className={styles.menuItemTrailing}>{trailing}</span>}
    </MenuPrimitive.Item>
  );
};

MenuItem.displayName = 'MenuItem';

// Menu Divider
const MenuDivider: React.FC = () => {
  return <MenuPrimitive.Separator className={styles.menuDivider} />;
};

MenuDivider.displayName = 'MenuDivider';

// Menu Group Label
interface MenuGroupLabelProps {
  children: React.ReactNode;
}

const MenuGroupLabel: React.FC<MenuGroupLabelProps> = ({ children }) => {
  return (
    <MenuPrimitive.Group>
      <MenuPrimitive.GroupLabel className={styles.menuGroupLabel}>
        {children}
      </MenuPrimitive.GroupLabel>
    </MenuPrimitive.Group>
  );
};

MenuGroupLabel.displayName = 'MenuGroupLabel';

// Attach subcomponents
Menu.Item = MenuItem;
Menu.Divider = MenuDivider;
Menu.GroupLabel = MenuGroupLabel;

export { MenuItem, MenuDivider, MenuGroupLabel };
