import { useMemo } from "react";
import { useTranslation } from "react-i18next";
import {
  useWorkbenchBarButtons,
  WorkbenchBarButtonWithAction,
} from "@app/hooks/useWorkbenchBarButtons";
import { PdfTextEditorViewData } from "@app/tools/pdfTextEditor/pdfTextEditorTypes";
import LocalIcon from "@app/components/shared/LocalIcon";
import { SegmentedControl } from "@app/ui/SegmentedControl";

export function usePdfTextEditorWorkbenchBarButtons(
  data: PdfTextEditorViewData,
) {
  const { t } = useTranslation();

  const {
    hasDocument,
    isGeneratingPdf,
    groupingMode,
    autoScaleText,
    onAutoScaleTextChange,
    onGroupingModeChange,
    onGeneratePdf,
    onReset,
  } = data;

  const buttons = useMemo<WorkbenchBarButtonWithAction[]>(() => {
    return [
      {
        id: "pdf-text-editor-grouping",
        section: "edit",
        order: 10,
        visible: hasDocument,
        render: ({ disabled }) => (
          <SegmentedControl
            size="xs"
            value={groupingMode}
            onChange={(value: string) => onGroupingModeChange(value as any)}
            disabled={disabled}
            options={[
              {
                label: t("pdfTextEditor.options.groupingMode.auto", "Auto"),
                value: "auto",
              },
              {
                label: t(
                  "pdfTextEditor.options.groupingMode.paragraph",
                  "Paragraph",
                ),
                value: "paragraph",
              },
              {
                label: t(
                  "pdfTextEditor.options.groupingMode.singleLine",
                  "Line",
                ),
                value: "singleLine",
              },
            ]}
          />
        ),
      },
      {
        id: "pdf-text-editor-autoscale",
        tooltip: t(
          "pdfTextEditor.options.autoScaleText.title",
          "Auto-scale Text",
        ),
        ariaLabel: t(
          "pdfTextEditor.options.autoScaleText.title",
          "Auto-scale Text",
        ),
        section: "edit",
        order: 20,
        visible: hasDocument,
        active: autoScaleText,
        onClick: () => onAutoScaleTextChange(!autoScaleText),
        icon: <LocalIcon icon="match-case" width="1.25rem" height="1.25rem" />,
      },
      {
        id: "pdf-text-editor-reset",
        tooltip: t("pdfTextEditor.options.reset.title", "Reset Edits"),
        ariaLabel: t("pdfTextEditor.options.reset.title", "Reset Edits"),
        section: "edit",
        order: 30,
        visible: hasDocument,
        onClick: onReset,
        icon: <LocalIcon icon="undo" width="1.25rem" height="1.25rem" />,
      },
      {
        id: "pdf-text-editor-generate",
        tooltip: t("pdfTextEditor.options.generatePdf.title", "Apply Changes"),
        ariaLabel: t(
          "pdfTextEditor.options.generatePdf.title",
          "Apply Changes",
        ),
        section: "edit",
        order: 40,
        visible: hasDocument,
        disabled: isGeneratingPdf,
        onClick: () => onGeneratePdf(),
        icon: <LocalIcon icon="save" width="1.25rem" height="1.25rem" />,
      },
    ];
  }, [
    t,
    hasDocument,
    groupingMode,
    onGroupingModeChange,
    autoScaleText,
    onAutoScaleTextChange,
    onReset,
    isGeneratingPdf,
    onGeneratePdf,
  ]);

  useWorkbenchBarButtons(buttons);
}
