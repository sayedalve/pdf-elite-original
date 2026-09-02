import React from "react";

export const EditMode: React.FC = () => {
  return (
    <div className="edit-mode-integrated">
      <div className="em-hint">
        Edit tools are available in the toolbar below.
        <br />
        <br />
        Use "Add Text" to overlay text, "Add Image" to place images, or
        "Advanced Edit" for grouped text reflow.
      </div>
      <style>{`
        .edit-mode-integrated { padding: 12px; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%; text-align: center; }
        .em-hint { font-size: 13px; color: var(--text-tertiary); max-width: 250px; line-height: 1.5; }
      `}</style>
    </div>
  );
};
