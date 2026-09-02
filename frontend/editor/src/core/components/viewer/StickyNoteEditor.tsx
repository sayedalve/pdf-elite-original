// eslint-disable-next-line no-restricted-syntax
import {
  Stack,
  Group,
  Textarea,
  ActionIcon,
  ColorSwatch,
  Text,
  Tooltip,
} from "@mantine/core";
import { useTranslation } from "react-i18next";
import DeleteIcon from "@mui/icons-material/Delete";
import type { TrackedAnnotation } from "@embedpdf/plugin-annotation";
import type { PdfAnnotationObject } from "@embedpdf/models";
import { useEffect, useRef, useState } from "react";
import type { AnnotationPatch } from "@app/components/viewer/viewerTypes";
import { Button } from "@app/ui/Button";

interface StickyNoteEditorProps {
  annotation: TrackedAnnotation<PdfAnnotationObject>;
  onUpdate: (patch: AnnotationPatch) => void;
  onDelete: () => void;
  onSave: () => void;
}

const PRESET_COLORS = [
  "#ffa000", // Yellow (Default)
  "#f44336", // Red
  "#4caf50", // Green
  "#2196f3", // Blue
  "#9c27b0", // Purple
];

export function StickyNoteEditor({
  annotation,
  onUpdate,
  onDelete,
  onSave,
}: StickyNoteEditorProps) {
  const { t } = useTranslation();

  // Local state for the textarea to avoid layout jumps on every keystroke
  const [content, setContent] = useState(annotation.object.contents || "");
  const textareaRef = useRef<HTMLTextAreaElement>(null);

  // Focus the textarea when opened (removed to allow keyboard Delete on selection)
  useEffect(() => {
    // We intentionally do NOT auto-focus the textarea.
    // This allows the user to press the 'Delete' key on their keyboard
    // to delete the annotation immediately after clicking it.
  }, []);

  const handleSave = () => {
    if (content !== annotation.object.contents) {
      onUpdate({ contents: content });
    }
    onSave(); // Closes the editor (deselects)
  };

  const obj = annotation.object as any;
  const currentColor = obj.strokeColor || obj.color || "#ffa000";

  return (
    <Stack gap="sm" style={{ minWidth: 280 }}>
      <Text size="sm" fw={600}>
        {t("annotation.stickyNote", "Sticky Note")}
      </Text>

      <Textarea
        ref={textareaRef}
        placeholder={t("annotation.notePlaceholder", "Type your note here...")}
        value={content}
        onChange={(e) => setContent(e.currentTarget.value)}
        minRows={3}
        maxRows={8}
        autosize
        data-no-interaction="true"
        onKeyDown={(e) => {
          // Stop propagation so ViewerShell doesn't intercept keystrokes
          e.stopPropagation();
          if (e.key === "Enter" && (e.ctrlKey || e.metaKey)) {
            handleSave();
          }
        }}
        styles={{
          input: {
            backgroundColor: "var(--mantine-color-body)",
            fontSize: "14px",
          },
        }}
      />

      <Group justify="space-between" mt="xs">
        <Group gap="xs">
          {PRESET_COLORS.map((color) => (
            <ColorSwatch
              key={color}
              color={color}
              size={18}
              withShadow
              style={{
                cursor: "pointer",
                border:
                  currentColor === color
                    ? "2px solid var(--mantine-color-blue-filled)"
                    : "1px solid rgba(255, 255, 255, 0.4)",
              }}
              onClick={() => onUpdate({ strokeColor: color, color: color })}
            />
          ))}
        </Group>

        <Group gap="sm">
          <Tooltip label={t("common.delete", "Delete")}>
            <ActionIcon variant="subtle" color="red" onClick={onDelete}>
              <DeleteIcon fontSize="small" />
            </ActionIcon>
          </Tooltip>
          <Button variant="primary" onClick={handleSave}>
            {t("common.save", "Save")}
          </Button>
        </Group>
      </Group>
    </Stack>
  );
}
