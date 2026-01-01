/**
 * @editor/ui - Editor UI Component Library
 *
 * Material Design 3 components built with Base UI and CSS Modules.
 * Designed for plugin system compatibility via Import Maps.
 *
 * Usage:
 * 1. Import styles in your app root:
 *    import '@/ui/styles/tokens.css';
 *    import '@/ui/styles/base.css';
 *
 * 2. Import components:
 *    import { Button, IconButton, Tooltip } from '@/ui';
 */

// Components
export { IconButton, type IconButtonProps } from './components/IconButton';
export { Button, type ButtonProps } from './components/Button';
export { Tooltip, type TooltipProps } from './components/Tooltip';
export { Surface, type SurfaceProps } from './components/Surface';
export { Chip, type ChipProps } from './components/Chip';
export { Alert, type AlertProps } from './components/Alert';
export { Spinner, type SpinnerProps } from './components/Spinner';
export { TextField, type TextFieldProps } from './components/TextField';
export { Menu, MenuItem, MenuDivider, MenuGroupLabel, type MenuProps, type MenuItemProps } from './components/Menu';
export {
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  ListDivider,
  ListSubheader,
  type ListProps,
  type ListItemProps,
  type ListItemButtonProps,
  type ListItemIconProps,
  type ListItemTextProps,
  type ListDividerProps,
  type ListSubheaderProps,
} from './components/List';
export { Collapse, type CollapseProps } from './components/Collapse';
export {
  Accordion,
  AccordionItem,
  AccordionHeader,
  AccordionPanel,
  AccordionSummary,
  AccordionDetails,
  type AccordionProps,
  type AccordionItemProps,
  type AccordionHeaderProps,
  type AccordionPanelProps,
} from './components/Accordion';
export { Divider, type DividerProps } from './components/Divider';
export { Typography, type TypographyProps, type TypographyVariant } from './components/Typography';
export {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  type DialogProps,
} from './components/Dialog';
export {
  Tabs,
  TabList,
  Tab,
  TabPanel,
  type TabsProps,
  type TabListProps,
  type TabProps,
  type TabPanelProps,
} from './components/Tabs';
export { Checkbox, type CheckboxProps } from './components/Checkbox';

// Icons
export * from './icons';
