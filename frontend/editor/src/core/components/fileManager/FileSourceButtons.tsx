import React from "react";
import { Stack, Text, Group } from "@mantine/core";
import { Button } from "@app/ui/Button";
import HistoryIcon from "@mui/icons-material/History";
import { useTranslation } from "react-i18next";
import { useFileManagerContext } from "@app/contexts/FileManagerContext";
import { useFileActionTerminology } from "@app/hooks/useFileActionTerminology";
import { useFileActionIcons } from "@app/hooks/useFileActionIcons";

interface FileSourceButtonsProps {
  horizontal?: boolean;
}

const FileSourceButtons: React.FC<FileSourceButtonsProps> = ({
  horizontal = false,
}) => {
  const { activeSource, onSourceChange, onLocalFileClick } =
    useFileManagerContext();
  const { t } = useTranslation();
  const terminology = useFileActionTerminology();
  const icons = useFileActionIcons();
  const UploadIcon = icons.upload;

  // Shared Button has no `xs`; map the old horizontal `xs` to `sm`.
  const buttonSize = "sm" as const;
  const buttonJustify = horizontal ? "center" : "start";
  const buttons = (
    <>
      <Button
        variant={activeSource === "recent" ? "primary" : "tertiary"}
        accent="neutral"
        leftSection={<HistoryIcon />}
        justify={buttonJustify}
        onClick={() => onSourceChange("recent")}
        fullWidth={!horizontal}
        size={buttonSize}
      >
        {t("fileManager.recent", "Recent")}
      </Button>

      <Button
        variant="tertiary"
        accent="neutral"
        leftSection={<UploadIcon />}
        justify={buttonJustify}
        onClick={onLocalFileClick}
        fullWidth={!horizontal}
        size={buttonSize}
      >
        {horizontal ? terminology.upload : terminology.uploadFiles}
      </Button>
    </>
  );

  if (horizontal) {
    return (
      <Group gap="xs" justify="center" style={{ width: "100%" }}>
        {buttons}
      </Group>
    );
  }

  return (
    <Stack gap="xs" style={{ height: "100%" }}>
      <Text
        size="sm"
        pt="sm"
        fw={500}
        c="dimmed"
        mb="xs"
        style={{ paddingLeft: "1rem" }}
      >
        {t("fileManager.myFiles", "My Files")}
      </Text>
      {buttons}
    </Stack>
  );
};

export default FileSourceButtons;
