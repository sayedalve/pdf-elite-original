import { useTranslation } from "react-i18next";
import { createToolFlow } from "@app/components/tools/shared/createToolFlow";
import { BaseToolProps, ToolComponent } from "@app/types/tool";
import { useBaseTool } from "@app/hooks/tools/shared/useBaseTool";
import ReorganizePagesSettings from "@app/components/tools/reorganizePages/ReorganizePagesSettings";
import { useReorganizePagesParameters } from "@app/hooks/tools/reorganizePages/useReorganizePagesParameters";
import { useReorganizePagesOperation } from "@app/hooks/tools/reorganizePages/useReorganizePagesOperation";

const ReorganizePages = (props: BaseToolProps) => {
  const { t } = useTranslation();

  const base = useBaseTool(
    "rearrange-pages",
    useReorganizePagesParameters,
    useReorganizePagesOperation,
    props,
  );

  const settingsContent = (
    <ReorganizePagesSettings
      parameters={base.params.parameters}
      onParameterChange={base.params.updateParameter}
      disabled={base.endpointLoading}
    />
  );

  return createToolFlow({
    files: {
      selectedFiles: base.selectedFiles,
      isCollapsed: base.hasResults,
    },
    steps: [
      {
        title: t("reorganizePages.settings.title", "Settings"),
        isCollapsed: base.settingsCollapsed,
        onCollapsedClick: base.settingsCollapsed
          ? base.handleSettingsReset
          : undefined,
        content: settingsContent,
      },
    ],
    executeButton: {
      text: t("reorganizePages.submit", "Reorganize Pages"),
      loadingText: t("loading"),
      onClick: base.handleExecute,
      isVisible: !base.hasResults,
      endpointEnabled: base.endpointEnabled,
      paramsValid: base.params.validateParameters(),
    },
    review: {
      isVisible: base.hasResults,
      operation: base.operation,
      title: t("reorganizePages.results.title", "Pages Reorganized"),
      onFileClick: base.handleThumbnailClick,
      onUndo: base.handleUndo,
    },
  });
};

(ReorganizePages as ToolComponent).tool = () => useReorganizePagesOperation;

export default ReorganizePages as ToolComponent;
