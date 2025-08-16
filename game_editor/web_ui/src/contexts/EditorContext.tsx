'use client';

import React, { createContext, useContext, useState, ReactNode } from 'react';

interface EditorContextType {
  selectedEntityId: string | null;
  setSelectedEntityId: (id: string | null) => void;
}

const EditorContext = createContext<EditorContextType | undefined>(undefined);

export const EditorProvider: React.FC<{ children: ReactNode }> = ({ children }) => {
  const [selectedEntityId, setSelectedEntityId] = useState<string | null>(null);

  return (
    <EditorContext.Provider value={{ selectedEntityId, setSelectedEntityId }}>
      {children}
    </EditorContext.Provider>
  );
};

export const useEditor = () => {
  const context = useContext(EditorContext);
  if (!context) {
    throw new Error('useEditor must be used within an EditorProvider');
  }
  return context;
};