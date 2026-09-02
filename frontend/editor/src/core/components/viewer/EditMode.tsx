/* eslint-disable */
import React from "react";
import { Type, Image, Link2 } from "lucide-react";

export const EditMode: React.FC = () => {
  return (
    <div className="edit-mode-integrated">
      <div className="em-hint">
        Text editing stays inside main viewer. Select text to edit. Same UI
        system, no separate window.
      </div>
      <div className="em-tools">
        <button>
          <Type size={16} /> Edit Text
        </button>
        <button>
          <Image size={16} /> Replace Image
        </button>
        <button>
          <Link2 size={16} /> Link
        </button>
      </div>
      <style>{`
        .edit-mode-integrated { padding: 12px; }
        .em-hint { font-size: 11px; color: var(--text-tertiary); margin-bottom: 8px; }
        .em-tools { display: flex; gap: 8px; }
        .em-tools button { background: var(--surface-card); border: 1px solid var(--border); border-radius: 8px; padding: 6px 12px; font-size: 12px; color: var(--text-primary); cursor: pointer; display: flex; gap: 6px; align-items: center; }
      `}</style>
    </div>
  );
};
