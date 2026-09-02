import { Stack, TextInput, Group, Text } from "@mantine/core";
import { Button } from "@app/ui/Button";
import { useTranslation } from "react-i18next";
import { ChangeMetadataParameters } from "@app/hooks/tools/changeMetadata/useChangeMetadataParameters";

interface CustomMetadataStepProps {
  parameters: ChangeMetadataParameters;
  onParameterChange?: <K extends keyof ChangeMetadataParameters>(
    key: K,
    value: ChangeMetadataParameters[K],
  ) => void;
  disabled?: boolean;
  addCustomMetadata: (key?: string, value?: string) => void;
  removeCustomMetadata: (index: number) => void;
  updateCustomMetadata: (
    index: number,
    key: keyof ChangeMetadataParameters["customMetadata"][0],
    value: string,
  ) => void;
}

const CustomMetadataStep = ({
  parameters,
  disabled = false,
  addCustomMetadata,
  removeCustomMetadata,
  updateCustomMetadata,
}: CustomMetadataStepProps) => {
  const { t } = useTranslation();

  return (
    <Stack gap="sm">
      <Group justify="space-between" align="center">
        <Text size="sm" fw={500}>
          {t("changeMetadata.customFields.title", "Custom Metadata")}
        </Text>
        <Button
          variant="secondary"
          size="sm"
          onClick={() => addCustomMetadata()}
          disabled={disabled}
        >
          {t("changeMetadata.customFields.add", "Add Field")}
        </Button>
      </Group>

      {parameters.customMetadata.length > 0 && (
        <Text size="xs" c="dimmed">
          {t(
            "changeMetadata.customFields.description",
            "Add custom metadata fields to the document",
          )}
        </Text>
      )}

      {parameters.customMetadata.map((entry, index) => (
        <Stack key={index} gap="xs">
          <TextInput
            placeholder={t(
              "changeMetadata.customFields.keyPlaceholder",
              "Custom key",
            )}
            value={entry.key}
            onChange={(e) => updateCustomMetadata(index, "key", e.target.value)}
            disabled={disabled}
          />
          <TextInput
            placeholder={t(
              "changeMetadata.customFields.valuePlaceholder",
              "Custom value",
            )}
            value={entry.value}
            onChange={(e) =>
              updateCustomMetadata(index, "value", e.target.value)
            }
            disabled={disabled}
          />
          <Button
            variant="secondary"
            accent="danger"
            size="sm"
            onClick={() => removeCustomMetadata(index)}
            disabled={disabled}
          >
            {t("changeMetadata.customFields.remove", "Remove")}
          </Button>
        </Stack>
      ))}
    </Stack>
  );
};

export default CustomMetadataStep;
