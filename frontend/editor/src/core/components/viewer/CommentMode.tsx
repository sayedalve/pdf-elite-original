import React from "react";

export const CommentMode: React.FC<{ highlightColor: string }> = ({
  highlightColor,
}) => {
  return (
    <div className="comment-mode-integrated">
      <div className="cm-header">
        <h4>Comments</h4>
        <span>Fixed: no separate error window, stays inside viewer</span>
      </div>
      <div className="cm-list">
        <div className="cm-item">
          <div className="cm-color" style={{ background: highlightColor }} />
          <div className="cm-text">Highlight persists across selections</div>
        </div>
      </div>
      <style>{`
        .comment-mode-integrated { padding: 16px; background: var(--surface-elevated); border-radius: 12px; border: 1px solid var(--border); }
        .cm-header h4 { margin: 0; font-size: 13px; }
        .cm-header span { font-size: 11px; color: var(--success); }
        .cm-item { display: flex; gap: 8px; align-items: center; margin-top: 12px; padding: 8px; background: var(--surface-card); border-radius: 8px; font-size: 12px; }
        .cm-color { width: 12px; height: 12px; border-radius: 3px; }
      `}</style>
    </div>
  );
};
