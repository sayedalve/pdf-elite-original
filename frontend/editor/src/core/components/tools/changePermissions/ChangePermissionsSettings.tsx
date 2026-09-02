/* eslint-disable */
import { Stack, Checkbox } from "@mantine/core";
import { useTranslation } from "react-i18next";
import { ChangePermissionsParameters } from "@app/hooks/tools/changePermissions/useChangePermissionsParameters";

interface ChangePermissionsSettingsProps {
  parameters: ChangePermissionsParameters;
  onParameterChange?: <K extends keyof ChangePermissionsParameters>(
    key: K,
    value: ChangePermissionsParameters[K],
  ) => void;
  disabled?: boolean;
}

const ChangePermissionsSettings = ({
  parameters,
  onParameterChange,
  disabled = false,
}: ChangePermissionsSettingsProps) => {
  const { t } = useTranslation();

  return (
    <Stack gap="sm">
      <Stack gap="xs">
        {(
          Object.keys(parameters) as Array<keyof ChangePermissionsParameters>
        ).map((key) => (
          <Checkbox
            key={key}
            label={
              t(`changePermissions.permissions.${key}.label` as any, key) as any
            }
            checked={parameters[key]}
            onChange={(e) =>
              (onParameterChange as any)?.(key, e.target.checked)
            }
            disabled={disabled}
          />
        ))}
      </Stack>
    </Stack>
  );
};

export default ChangePermissionsSettings;
