/* eslint-disable */
import { Stack } from "@mantine/core";
import { AdjustContrastParameters } from "@app/hooks/tools/adjustContrast/useAdjustContrastParameters";
import AdjustContrastBasicSettings from "@app/components/tools/adjustContrast/AdjustContrastBasicSettings";
import AdjustContrastColorSettings from "@app/components/tools/adjustContrast/AdjustContrastColorSettings";

import { ToolAutomationSettingsProps } from "@app/hooks/tools/shared/toolOperationTypes";

interface Props extends ToolAutomationSettingsProps<AdjustContrastParameters> {}

const AdjustContrastSingleStepSettings = ({
  parameters,
  onChange,
  disabled,
}: Props) => {
  const onParameterChange = <K extends keyof AdjustContrastParameters>(
    key: K,
    value: AdjustContrastParameters[K],
  ) => {
    onChange?.({ [key]: value } as any);
  };
  return (
    <Stack gap="lg">
      <AdjustContrastBasicSettings
        parameters={parameters}
        onParameterChange={onParameterChange}
        disabled={disabled}
      />
      <AdjustContrastColorSettings
        parameters={parameters}
        onParameterChange={onParameterChange}
        disabled={disabled}
      />
    </Stack>
  );
};

export default AdjustContrastSingleStepSettings;
