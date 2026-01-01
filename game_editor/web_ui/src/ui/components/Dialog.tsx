'use client';

import React, { forwardRef } from 'react';
import { Dialog as DialogPrimitive } from '@base-ui/react/dialog';
import styles from './Dialog.module.css';

export interface DialogProps {
  /** Whether the dialog is open */
  open: boolean;
  /** Callback when the dialog should close */
  onClose: () => void;
  /** Dialog title */
  title?: string;
  /** Maximum width of the dialog */
  maxWidth?: 'xs' | 'sm' | 'md' | 'lg' | 'xl' | false;
  /** Whether the dialog takes full width */
  fullWidth?: boolean;
  /** Children */
  children: React.ReactNode;
}

export const Dialog: React.FC<DialogProps> & {
  Title: typeof DialogTitle;
  Content: typeof DialogContent;
  Actions: typeof DialogActions;
} = ({ open, onClose, title, maxWidth = 'sm', fullWidth = false, children }) => {
  const maxWidthClass = maxWidth ? styles[`maxWidth${maxWidth.charAt(0).toUpperCase()}${maxWidth.slice(1)}`] : '';

  return (
    <DialogPrimitive.Root open={open} onOpenChange={(isOpen) => !isOpen && onClose()}>
      <DialogPrimitive.Portal>
        <DialogPrimitive.Backdrop className={styles.backdrop} />
        <DialogPrimitive.Popup className={`${styles.dialog} ${maxWidthClass} ${fullWidth ? styles.fullWidth : ''}`}>
          {title && (
            <DialogPrimitive.Title className={styles.title}>
              {title}
            </DialogPrimitive.Title>
          )}
          {children}
        </DialogPrimitive.Popup>
      </DialogPrimitive.Portal>
    </DialogPrimitive.Root>
  );
};

// Dialog Title
interface DialogTitleProps {
  children: React.ReactNode;
  className?: string;
}

const DialogTitle = forwardRef<HTMLHeadingElement, DialogTitleProps>(
  ({ children, className }, ref) => {
    return (
      <DialogPrimitive.Title ref={ref} className={`${styles.title} ${className || ''}`}>
        {children}
      </DialogPrimitive.Title>
    );
  }
);
DialogTitle.displayName = 'DialogTitle';

// Dialog Content
interface DialogContentProps {
  children: React.ReactNode;
  className?: string;
  dividers?: boolean;
}

const DialogContent = forwardRef<HTMLDivElement, DialogContentProps>(
  ({ children, className, dividers = false }, ref) => {
    return (
      <DialogPrimitive.Description
        ref={ref}
        className={`${styles.content} ${dividers ? styles.dividers : ''} ${className || ''}`}
      >
        {children}
      </DialogPrimitive.Description>
    );
  }
);
DialogContent.displayName = 'DialogContent';

// Dialog Actions
interface DialogActionsProps {
  children: React.ReactNode;
  className?: string;
}

const DialogActions = forwardRef<HTMLDivElement, DialogActionsProps>(
  ({ children, className }, ref) => {
    return (
      <div ref={ref} className={`${styles.actions} ${className || ''}`}>
        {children}
      </div>
    );
  }
);
DialogActions.displayName = 'DialogActions';

Dialog.Title = DialogTitle;
Dialog.Content = DialogContent;
Dialog.Actions = DialogActions;

export { DialogTitle, DialogContent, DialogActions };
