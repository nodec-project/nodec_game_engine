'use client';

import React from 'react';

export interface IconProps {
  /** Icon name from Material Symbols (e.g., "play_arrow", "delete", "settings") */
  name: string;
  /** Icon size in pixels */
  size?: number;
  /** Whether to use filled variant */
  fill?: boolean;
  /** Icon weight (100-700) */
  weight?: number;
  /** Additional CSS class */
  className?: string;
  /** Additional inline styles */
  style?: React.CSSProperties;
}

/**
 * Material Symbols Icon component
 *
 * Uses Google's Material Symbols Outlined font.
 * Browse available icons at: https://fonts.google.com/icons
 *
 * @example
 * <Icon name="play_arrow" />
 * <Icon name="delete" size={16} />
 * <Icon name="favorite" fill />
 */
export const Icon: React.FC<IconProps> = ({
  name,
  size = 24,
  fill = false,
  weight = 400,
  className,
  style,
}) => {
  const fontVariationSettings = `'FILL' ${fill ? 1 : 0}, 'wght' ${weight}`;

  return (
    <span
      className={`material-symbols-outlined ${className ?? ''}`}
      style={{
        fontSize: size,
        fontVariationSettings,
        ...style,
      }}
      aria-hidden="true"
    >
      {name}
    </span>
  );
};

Icon.displayName = 'Icon';
