'use client';

import React, { forwardRef } from 'react';
import styles from './Typography.module.css';

export type TypographyVariant =
  | 'displayLarge'
  | 'displayMedium'
  | 'displaySmall'
  | 'headlineLarge'
  | 'headlineMedium'
  | 'headlineSmall'
  | 'titleLarge'
  | 'titleMedium'
  | 'titleSmall'
  | 'labelLarge'
  | 'labelMedium'
  | 'labelSmall'
  | 'bodyLarge'
  | 'bodyMedium'
  | 'bodySmall';

// MUI compatibility aliases
export type TypographyVariantAlias =
  | 'h1'
  | 'h2'
  | 'h3'
  | 'h4'
  | 'h5'
  | 'h6'
  | 'subtitle1'
  | 'subtitle2'
  | 'body1'
  | 'body2'
  | 'caption'
  | 'overline'
  // Kebab-case aliases
  | 'display-large'
  | 'display-medium'
  | 'display-small'
  | 'headline-large'
  | 'headline-medium'
  | 'headline-small'
  | 'title-large'
  | 'title-medium'
  | 'title-small'
  | 'label-large'
  | 'label-medium'
  | 'label-small'
  | 'body-large'
  | 'body-medium'
  | 'body-small';

const variantAliasMap: Record<TypographyVariantAlias, TypographyVariant> = {
  h1: 'displayLarge',
  h2: 'displayMedium',
  h3: 'displaySmall',
  h4: 'headlineLarge',
  h5: 'headlineMedium',
  h6: 'headlineSmall',
  subtitle1: 'titleMedium',
  subtitle2: 'titleSmall',
  body1: 'bodyLarge',
  body2: 'bodyMedium',
  caption: 'bodySmall',
  overline: 'labelSmall',
  // Kebab-case aliases
  'display-large': 'displayLarge',
  'display-medium': 'displayMedium',
  'display-small': 'displaySmall',
  'headline-large': 'headlineLarge',
  'headline-medium': 'headlineMedium',
  'headline-small': 'headlineSmall',
  'title-large': 'titleLarge',
  'title-medium': 'titleMedium',
  'title-small': 'titleSmall',
  'label-large': 'labelLarge',
  'label-medium': 'labelMedium',
  'label-small': 'labelSmall',
  'body-large': 'bodyLarge',
  'body-medium': 'bodyMedium',
  'body-small': 'bodySmall',
};

const variantElementMap: Record<TypographyVariant, keyof JSX.IntrinsicElements> = {
  displayLarge: 'h1',
  displayMedium: 'h2',
  displaySmall: 'h3',
  headlineLarge: 'h4',
  headlineMedium: 'h5',
  headlineSmall: 'h6',
  titleLarge: 'h6',
  titleMedium: 'p',
  titleSmall: 'p',
  labelLarge: 'span',
  labelMedium: 'span',
  labelSmall: 'span',
  bodyLarge: 'p',
  bodyMedium: 'p',
  bodySmall: 'p',
};

export interface TypographyProps extends React.HTMLAttributes<HTMLElement> {
  /** Typography variant */
  variant?: TypographyVariant | TypographyVariantAlias;
  /** Color variant */
  color?: 'primary' | 'secondary' | 'tertiary' | 'error' | 'onSurface' | 'onSurfaceVariant';
  /** Text alignment */
  align?: 'left' | 'center' | 'right' | 'justify';
  /** Prevent text wrapping and show ellipsis */
  noWrap?: boolean;
  /** Add margin-bottom */
  gutterBottom?: boolean;
  /** Add paragraph margin-bottom */
  paragraph?: boolean;
  /** Override the element type */
  as?: React.ElementType;
  /** Children */
  children: React.ReactNode;
}

/**
 * Typography component
 *
 * Displays text with Material Design 3 typography styles.
 */
export const Typography = forwardRef<HTMLElement, TypographyProps>(
  (
    {
      variant = 'bodyMedium',
      color,
      align,
      noWrap = false,
      gutterBottom = false,
      paragraph = false,
      as,
      className,
      children,
      ...props
    },
    ref
  ) => {
    // Resolve variant alias
    const resolvedVariant = (variant in variantAliasMap
      ? variantAliasMap[variant as TypographyVariantAlias]
      : variant) as TypographyVariant;

    // Determine element type
    const Component = as || variantElementMap[resolvedVariant] || 'span';

    const alignClass = align ? styles[`align${align.charAt(0).toUpperCase() + align.slice(1)}`] : '';

    const classNames = [
      styles.typography,
      styles[resolvedVariant],
      color && styles[color],
      alignClass,
      noWrap && styles.noWrap,
      gutterBottom && styles.gutterBottom,
      paragraph && styles.paragraph,
      className,
    ]
      .filter(Boolean)
      .join(' ');

    return (
      <Component ref={ref} className={classNames} {...props}>
        {children}
      </Component>
    );
  }
);

Typography.displayName = 'Typography';
