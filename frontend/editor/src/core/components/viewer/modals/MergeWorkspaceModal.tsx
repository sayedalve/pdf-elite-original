import { useState, useRef, useEffect } from "react";
import {
  Modal,
  Button,
  Stack,
  Text,
  Group,
  ActionIcon,
  FileButton,
  Card,
} from "@mantine/core";
import { useTranslation } from "react-i18next";
import { X, GripVertical, FileUp } from "lucide-react";

interface MergeWorkspaceModalProps {
  opened: boolean;
  onClose: () => void;
  onMerge: (files: File[]) => void;
  initialFile?: File | null;
}
export function MergeWorkspaceModal({
  opened,
  onClose,
  onMerge,
  initialFile,
}: MergeWorkspaceModalProps) {
  const { t } = useTranslation();
  const [files, setFiles] = useState<File[]>([]);

  // When modal opens, populate with the initial file (if provided)
  useEffect(() => {
    if (opened) {
      setFiles(initialFile ? [initialFile] : []);
    } else {
      setFiles([]);
    }
  }, [opened, initialFile]);

  const draggedItemIndex = useRef<number | null>(null);

  const handleAddFiles = (newFiles: File[]) => {
    // Only accept PDFs
    const pdfs = newFiles.filter(
      (f) =>
        f.type === "application/pdf" || f.name.toLowerCase().endsWith(".pdf"),
    );
    setFiles((prev) => [...prev, ...pdfs]);
  };

  const handleRemove = (index: number) => {
    setFiles((prev) => prev.filter((_, i) => i !== index));
  };

  const onDragStart = (e: React.DragEvent, index: number) => {
    draggedItemIndex.current = index;
    // For firefox
    e.dataTransfer.setData("text/html", "");
    e.dataTransfer.effectAllowed = "move";
  };

  const onDragOver = (index: number) => {
    const draggedOverItemIndex = index;

    // if the item is dragged over itself, ignore
    if (
      draggedItemIndex.current === draggedOverItemIndex ||
      draggedItemIndex.current === null
    ) {
      return;
    }

    // filter out the currently dragged item
    let newFiles = files.filter((_, i) => i !== draggedItemIndex.current);

    // add the dragged item after the dragged over item
    newFiles.splice(draggedOverItemIndex, 0, files[draggedItemIndex.current!]);

    draggedItemIndex.current = draggedOverItemIndex;
    setFiles(newFiles);
  };

  const onDragEnd = () => {
    draggedItemIndex.current = null;
  };

  const handleMerge = () => {
    if (files.length >= 2) {
      onMerge(files);
      onClose();
      // Clean up after merging
      setTimeout(() => setFiles([]), 300);
    }
  };

  return (
    <Modal
      opened={opened}
      onClose={onClose}
      title={t("merge.title", "Merge PDFs")}
      size="lg"
      centered
    >
      <Stack gap="md">
        <Text size="sm" c="dimmed">
          {t(
            "merge.description",
            "Add PDF files and drag them to set the desired order.",
          )}
        </Text>

        {files.length === 0 ? (
          <Card
            withBorder
            padding="xl"
            style={{ textAlign: "center", borderStyle: "dashed" }}
          >
            <Text c="dimmed" mb="md">
              No files added yet.
            </Text>
            <FileButton
              onChange={handleAddFiles}
              accept="application/pdf"
              multiple
            >
              {(props) => (
                <Button
                  {...props}
                  leftSection={<FileUp size={16} />}
                  variant="light"
                >
                  {t("merge.addFiles", "Add PDF Files")}
                </Button>
              )}
            </FileButton>
          </Card>
        ) : (
          <Stack
            gap="xs"
            style={{
              maxHeight: 400,
              overflowY: "auto",
              overflowX: "hidden",
              paddingRight: 4,
            }}
          >
            {files.map((file, index) => (
              <Card
                key={`${file.name}-${index}`}
                withBorder
                padding="sm"
                draggable
                onDragStart={(e) => onDragStart(e, index)}
                onDragOver={(e) => {
                  e.preventDefault();
                  onDragOver(index);
                }}
                onDragEnd={onDragEnd}
                style={{ cursor: "grab", transition: "transform 0.1s" }}
              >
                <Group justify="space-between" wrap="nowrap">
                  <Group wrap="nowrap" style={{ overflow: "hidden" }}>
                    <GripVertical size={16} style={{ color: "gray" }} />
                    <Text size="sm" truncate>
                      {file.name}
                    </Text>
                  </Group>
                  <ActionIcon
                    color="red"
                    variant="subtle"
                    onClick={() => handleRemove(index)}
                  >
                    <X size={16} />
                  </ActionIcon>
                </Group>
              </Card>
            ))}

            <FileButton
              onChange={handleAddFiles}
              accept="application/pdf"
              multiple
            >
              {(props) => (
                <Button
                  {...props}
                  leftSection={<PlusIcon />}
                  variant="light"
                  mt="xs"
                >
                  {t("merge.addMore", "Add More Files")}
                </Button>
              )}
            </FileButton>
          </Stack>
        )}

        <Group justify="flex-end" mt="md">
          <Button variant="default" onClick={onClose}>
            {t("common.cancel", "Cancel")}
          </Button>
          <Button onClick={handleMerge} disabled={files.length < 2}>
            {t("merge.button", "Merge Files")}
          </Button>
        </Group>
      </Stack>
    </Modal>
  );
}

// Temporary inline Plus component since we didn't import it at the top
function PlusIcon() {
  return (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      width="16"
      height="16"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <line x1="12" y1="5" x2="12" y2="19"></line>
      <line x1="5" y1="12" x2="19" y2="12"></line>
    </svg>
  );
}
