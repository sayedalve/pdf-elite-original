import { Stack, TextInput } from "@mantine/core";
import { useTranslation } from "react-i18next";
import { ExtractPagesParameters } from "@app/hooks/tools/extractPages/useExtractPagesParameters";
import PageSelectionSyntaxHint from "@app/components/shared/PageSelectionSyntaxHint";
import { ToolAutomationSettingsProps } from "@app/hooks/tools/shared/toolOperationTypes";

const ExtractPagesSettings = ({
  parameters,
  onChange,
  disabled = false,
}: ToolAutomationSettingsProps<ExtractPagesParameters>) => {
  const { t } = useTranslation();

  const handleChange = (value: string) => {
    onChange?.({ pageNumbers: value });
  };

  return (
    <Stack gap="md">
      <TextInput
        label={t("extractPages.pageNumbers.label", "Pages to Extract")}
        value={parameters.pageNumbers || ""}
        onChange={(event) => handleChange(event.currentTarget.value)}
        placeholder={t(
          "extractPages.pageNumbers.placeholder",
          "e.g., 1,3,5-8 or odd & 1-10",
        )}
        disabled={disabled}
        required
      />
      <PageSelectionSyntaxHint
        input={parameters.pageNumbers || ""}
        variant="compact"
      />
    </Stack>
  );
};

export default ExtractPagesSettings;
