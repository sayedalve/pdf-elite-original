import { useState, useMemo } from "react";
import {
  Modal,
  Button,
  Stack,
  Text,
  Group,
  Radio,
  NumberInput,
  Textarea,
  ScrollArea,
} from "@mantine/core";
import { useTranslation } from "react-i18next";
import { parsePageSequence } from "@app/services/offlinePageOps";

interface SplitModalProps {
  opened: boolean;
  onClose: () => void;
  onSplit: (splits: number[][]) => void;
  totalPages: number;
  selectedIndices: number[]; // 0-indexed
}

type SplitMode = "after_selected" | "every_n" | "ranges" | "individual";

export function SplitModal({
  opened,
  onClose,
  onSplit,
  totalPages,
  selectedIndices,
}: SplitModalProps) {
  const { t } = useTranslation();

  const [mode, setMode] = useState<SplitMode>(
    selectedIndices.length > 0 ? "after_selected" : "every_n",
  );

  const [everyN, setEveryN] = useState<number>(2);
  const [rangesInput, setRangesInput] = useState<string>("");

  const splits = useMemo<number[][]>(() => {
    const result: number[][] = [];

    if (mode === "after_selected") {
      if (selectedIndices.length === 0) return [];

      const sortedSplitPoints = [...selectedIndices].sort((a, b) => a - b);
      let currentPart: number[] = [];
      let currentSplitPointIndex = 0;

      for (let i = 0; i < totalPages; i++) {
        currentPart.push(i);
        if (
          currentSplitPointIndex < sortedSplitPoints.length &&
          i === sortedSplitPoints[currentSplitPointIndex]
        ) {
          result.push([...currentPart]);
          currentPart = [];
          currentSplitPointIndex++;
        }
      }
      if (currentPart.length > 0) {
        result.push(currentPart);
      }
    } else if (mode === "every_n") {
      const n = Math.max(1, everyN || 1);
      for (let i = 0; i < totalPages; i += n) {
        const part = [];
        for (let j = 0; j < n && i + j < totalPages; j++) {
          part.push(i + j);
        }
        result.push(part);
      }
    } else if (mode === "ranges") {
      const lines = rangesInput
        .split(/\r?\n/)
        .map((line) => line.trim())
        .filter(Boolean);
      for (const line of lines) {
        try {
          const parsed = parsePageSequence(line, totalPages);
          if (parsed.length > 0) {
            result.push(parsed);
          }
        } catch {
          // Ignore invalid lines
        }
      }
    } else if (mode === "individual") {
      for (let i = 0; i < totalPages; i++) {
        result.push([i]);
      }
    }

    return result.filter((part) => part.length > 0);
  }, [mode, everyN, rangesInput, selectedIndices, totalPages]);

  const handleSplit = () => {
    if (splits.length > 0) {
      onSplit(splits);
      onClose();
    }
  };

  return (
    <Modal
      opened={opened}
      onClose={onClose}
      title={t("split.title", "Split PDF")}
      size="md"
      centered
    >
      <Stack gap="md">
        <Radio.Group
          value={mode}
          onChange={(val) => setMode(val as SplitMode)}
          name="splitMode"
          label={t("split.modeLabel", "Split mode")}
        >
          <Stack mt="xs" gap="xs">
            <Radio
              value="after_selected"
              label={t("split.afterSelected", "Split after selected pages")}
              disabled={selectedIndices.length === 0}
            />
            <Radio
              value="every_n"
              label={t("split.everyN", "Split every N pages")}
            />
            <Radio
              value="ranges"
              label={t("split.ranges", "Split by specific page ranges")}
            />
            <Radio
              value="individual"
              label={t("split.individual", "Split into individual pages")}
            />
          </Stack>
        </Radio.Group>

        {mode === "every_n" && (
          <NumberInput
            label={t("split.pagesPerFile", "Pages per file")}
            min={1}
            max={totalPages}
            value={everyN}
            onChange={(val) => setEveryN(typeof val === "number" ? val : 1)}
          />
        )}

        {mode === "ranges" && (
          <Textarea
            label={t("split.rangesInput", "Ranges (e.g. 1-5, 6-10)")}
            description={t(
              "split.rangesHelp",
              "Separate parts by commas or new lines",
            )}
            placeholder="1-5, 6-10"
            value={rangesInput}
            onChange={(e) => {
              // Convert commas to new lines for easier multiline parsing
              const val = e.currentTarget.value.replace(/,/g, "\n");
              setRangesInput(val);
            }}
            rows={3}
          />
        )}

        <Text size="sm" fw={500} mt="md">
          {t("split.preview", "Output Preview")}
        </Text>
        <ScrollArea h={120} type="always" offsetScrollbars>
          {splits.length === 0 ? (
            <Text c="dimmed" size="sm">
              No valid splits
            </Text>
          ) : (
            <Stack gap={4}>
              {splits.map((part, index) => (
                <Text key={index} size="sm">
                  Part {index + 1}: {part.length}{" "}
                  {part.length === 1 ? "page" : "pages"} (Pages:{" "}
                  {part.map((p) => p + 1).join(", ")})
                </Text>
              ))}
            </Stack>
          )}
        </ScrollArea>

        <Group justify="flex-end" mt="md">
          <Button variant="default" onClick={onClose}>
            {t("common.cancel", "Cancel")}
          </Button>
          <Button
            onClick={handleSplit}
            disabled={
              splits.length === 0 ||
              (splits.length === 1 && splits[0].length === totalPages)
            }
          >
            {t("split.button", "Split")} ({splits.length} parts)
          </Button>
        </Group>
      </Stack>
    </Modal>
  );
}
