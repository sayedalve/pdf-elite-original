import { useState, useMemo, useEffect } from "react";
import { Modal, Button, TextInput, Stack, Text, Group } from "@mantine/core";
import { useTranslation } from "react-i18next";
import { parsePageSequence } from "@app/services/offlinePageOps";

interface ExtractPagesModalProps {
  opened: boolean;
  onClose: () => void;
  onExtract: (pageNumbers: string) => void;
  totalPages: number;
  initialSelection: number[];
}

export function ExtractPagesModal({
  opened,
  onClose,
  onExtract,
  totalPages,
  initialSelection,
}: ExtractPagesModalProps) {
  const { t } = useTranslation();

  const [pageNumbers, setPageNumbers] = useState("");

  // Update initial selection when modal opens
  useEffect(() => {
    if (opened) {
      if (initialSelection.length > 0) {
        setPageNumbers(initialSelection.join(", "));
      } else {
        setPageNumbers("");
      }
    }
  }, [opened, initialSelection]);

  // Validate and calculate preview
  const extractedCount = useMemo(() => {
    if (!pageNumbers.trim()) return 0;
    try {
      const parsed = parsePageSequence(pageNumbers, totalPages);
      return parsed.length;
    } catch {
      return 0;
    }
  }, [pageNumbers, totalPages]);

  const handleExtract = () => {
    if (extractedCount > 0) {
      onExtract(pageNumbers);
      onClose();
    }
  };

  return (
    <Modal
      opened={opened}
      onClose={onClose}
      title={t("extract.title", "Extract Pages")}
      size="sm"
      centered
    >
      <Stack gap="md">
        <Text size="sm">
          {t(
            "extract.description",
            "Enter page numbers and/or ranges separated by commas (e.g. 1, 3, 5-8). Pages will be extracted in the exact order specified.",
          )}
        </Text>

        <TextInput
          placeholder="e.g. 1, 3, 5-8"
          value={pageNumbers}
          onChange={(e) => setPageNumbers(e.currentTarget.value)}
          data-autofocus
          description={
            extractedCount > 0
              ? `${extractedCount} ${extractedCount === 1 ? "page" : "pages"} will be extracted`
              : "No valid pages specified"
          }
          error={pageNumbers.trim() !== "" && extractedCount === 0}
        />

        <Group justify="flex-end" mt="md">
          <Button variant="default" onClick={onClose}>
            {t("common.cancel", "Cancel")}
          </Button>
          <Button onClick={handleExtract} disabled={extractedCount === 0}>
            {t("extract.button", "Extract")}
          </Button>
        </Group>
      </Stack>
    </Modal>
  );
}
