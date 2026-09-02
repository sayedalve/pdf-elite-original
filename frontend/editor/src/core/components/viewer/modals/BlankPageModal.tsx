import { useState } from "react";
import {
  Modal,
  Button,
  Stack,
  Group,
  NumberInput,
  Select,
  TextInput,
} from "@mantine/core";
import { useTranslation } from "react-i18next";

export type BlankPageLocation =
  | "before_selected"
  | "after_selected"
  | "beginning"
  | "end"
  | "after_page";
export type BlankPageSize = "match" | "A4" | "Letter" | "custom";

interface BlankPageModalProps {
  opened: boolean;
  onClose: () => void;
  onInsert: (
    count: number,
    location: BlankPageLocation,
    pageRef: number,
    size: BlankPageSize,
    customDims: [number, number],
  ) => void;
  hasSelection: boolean;
  totalPages: number;
}

export function BlankPageModal({
  opened,
  onClose,
  onInsert,
  hasSelection,
  totalPages,
}: BlankPageModalProps) {
  const { t } = useTranslation();

  const [count, setCount] = useState<number>(1);
  const [location, setLocation] = useState<BlankPageLocation>(
    hasSelection ? "after_selected" : "end",
  );
  const [pageRef, setPageRef] = useState<number>(totalPages);
  const [size, setSize] = useState<BlankPageSize>("match");

  const [customWidth, setCustomWidth] = useState<number>(595);
  const [customHeight, setCustomHeight] = useState<number>(842);

  const handleInsert = () => {
    onInsert(count, location, pageRef, size, [customWidth, customHeight]);
    onClose();
  };

  return (
    <Modal
      opened={opened}
      onClose={onClose}
      title={t("blankPage.title", "Insert Blank Page")}
      size="sm"
      centered
    >
      <Stack gap="md">
        <NumberInput
          label={t("blankPage.count", "Number of pages")}
          min={1}
          max={100}
          value={count}
          onChange={(val) => setCount(typeof val === "number" ? val : 1)}
        />

        <Select
          label={t("blankPage.location", "Location")}
          value={location}
          onChange={(val) => setLocation((val as BlankPageLocation) || "end")}
          data={[
            {
              value: "before_selected",
              label: t("blankPage.locBeforeSel", "Before selected page"),
              disabled: !hasSelection,
            },
            {
              value: "after_selected",
              label: t("blankPage.locAfterSel", "After selected page"),
              disabled: !hasSelection,
            },
            {
              value: "beginning",
              label: t("blankPage.locBeginning", "At the beginning"),
            },
            { value: "end", label: t("blankPage.locEnd", "At the end") },
            {
              value: "after_page",
              label: t("blankPage.locAfterPage", "After specific page..."),
            },
          ]}
        />

        {location === "after_page" && (
          <NumberInput
            label={t("blankPage.pageRef", "Page Number")}
            min={1}
            max={totalPages}
            value={pageRef}
            onChange={(val) => setPageRef(typeof val === "number" ? val : 1)}
          />
        )}

        <Select
          label={t("blankPage.size", "Page Size")}
          value={size}
          onChange={(val) => setSize((val as BlankPageSize) || "match")}
          data={[
            {
              value: "match",
              label: t("blankPage.sizeMatch", "Match neighboring page"),
            },
            { value: "A4", label: t("blankPage.sizeA4", "A4 (210 × 297 mm)") },
            {
              value: "Letter",
              label: t("blankPage.sizeLetter", "Letter (8.5 × 11 in)"),
            },
            {
              value: "custom",
              label: t("blankPage.sizeCustom", "Custom Size"),
            },
          ]}
        />

        {size === "custom" && (
          <Group grow>
            <NumberInput
              label="Width (pt)"
              value={customWidth}
              onChange={(val) =>
                setCustomWidth(typeof val === "number" ? val : 595)
              }
            />
            <NumberInput
              label="Height (pt)"
              value={customHeight}
              onChange={(val) =>
                setCustomHeight(typeof val === "number" ? val : 842)
              }
            />
          </Group>
        )}

        <Group justify="flex-end" mt="md">
          <Button variant="default" onClick={onClose}>
            {t("common.cancel", "Cancel")}
          </Button>
          <Button onClick={handleInsert}>
            {t("blankPage.insert", "Insert")}
          </Button>
        </Group>
      </Stack>
    </Modal>
  );
}
