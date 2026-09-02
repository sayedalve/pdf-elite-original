/* eslint-disable */
import React, { useState, useCallback } from "react";
import { useTranslation } from "react-i18next";
import { createToolFlow } from "@app/components/tools/shared/createToolFlow";
import ReplaceImageSettings from "@app/components/tools/replaceImage/ReplaceImageSettings";
import { useReplaceImageParameters } from "@app/hooks/tools/replaceImage/useReplaceImageParameters";
import { useBaseTool } from "@app/hooks/tools/shared/useBaseTool";
import { BaseToolProps, ToolComponent } from "@app/types/tool";
import { buildReplaceImageFormData } from "@app/hooks/tools/replaceImage/useReplaceImageOperation";

const ReplaceImage = (props: BaseToolProps) => {
  const { t } = useTranslation();

  const base = useBaseTool(
    "replaceImage",
    useReplaceImageParameters,
    // Custom operation hook that handles the replacement image
    () => {
      const operation =
        require("@app/hooks/tools/replaceImage/useReplaceImageOperation").useReplaceImageOperation();
      return operation;
    },
    props,
  );

  const [replacementFile, setReplacementFile] = useState<File | null>(null);

  React.useEffect(() => {
    try {
      const stored = sessionStorage.getItem("pdf-elite-selected-image");
      if (stored) {
        const parsed = JSON.parse(stored);
        base.params.updateParameter("pageNumber", parsed.pageNumber);
        base.params.updateParameter("imageIndex", parsed.imageIndex);
        sessionStorage.removeItem("pdf-elite-selected-image");
      }
      
      const file = (window as any).__pdfEliteSelectedImageFile;
      if (file) {
        setReplacementFile(file);
        delete (window as any).__pdfEliteSelectedImageFile;
      }
    } catch(e) {}
  }, []);

  const handleExecute = useCallback(async () => {
    if (!replacementFile || base.selectedFiles.length === 0) {
      return;
    }

    await base.operation.executeOperation(base.params.parameters, [
      base.selectedFiles[0],
      replacementFile,
    ]);
  }, [replacementFile, base]);

  return createToolFlow({
    files: {
      selectedFiles: base.selectedFiles,
      isCollapsed: base.hasResults,
    },
    steps: [
      {
        title: t("replaceImage.settings.title", "Replace Image Settings"),
        isCollapsed: base.settingsCollapsed,
        onCollapsedClick: base.settingsCollapsed
          ? base.handleSettingsReset
          : undefined,
        content: (
          <ReplaceImageSettings
            parameters={base.params.parameters}
            onParameterChange={base.params.updateParameter}
            disabled={base.endpointLoading}
            onReplaceImageSelect={setReplacementFile}
            selectedReplacementFile={replacementFile}
          />
        ),
      },
    ],
    executeButton: {
      text: t("replaceImage.submit", "Replace Image"),
      isVisible: !base.hasResults,
      loadingText: t("loading"),
      onClick: async () => await handleExecute(),
      endpointEnabled: base.endpointEnabled && !!replacementFile,
      paramsValid: base.params.validateParameters() && !!replacementFile,
    },
    review: {
      isVisible: base.hasResults,
      operation: base.operation,
      title: t("replaceImage.results.title", "Replace Image Results"),
      onFileClick: base.handleThumbnailClick,
      onUndo: base.handleUndo,
    },
  });
};

export default ReplaceImage as ToolComponent;
