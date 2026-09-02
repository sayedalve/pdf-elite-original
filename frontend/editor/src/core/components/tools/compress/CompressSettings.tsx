/* eslint-disable */
import {
  Stack,
  Text,
  NumberInput,
  Select,
  Divider,
  Checkbox,
  Slider,
  Tooltip,
  Box,
} from "@mantine/core";
import { SegmentedControl } from "@app/ui/SegmentedControl";
import SliderWithInput from "@app/components/shared/sliderWithInput/SliderWithInput";
import { useTranslation } from "react-i18next";
import { CompressParameters } from "@app/hooks/tools/compress/useCompressParameters";
import ButtonSelector from "@app/components/shared/ButtonSelector";
import { useGroupEnabled } from "@app/hooks/useGroupEnabled";
import { ToolAutomationSettingsProps } from "@app/hooks/tools/shared/toolOperationTypes";
import { Z_INDEX_AUTOMATE_DROPDOWN } from "@app/styles/zIndex";

interface CompressSettingsProps extends ToolAutomationSettingsProps<CompressParameters> {}

const CompressSettings = ({
  parameters,
  onChange,
  disabled = false,
}: CompressSettingsProps) => {
  const { t } = useTranslation();
  const {
    enabled: imageMagickAvailable,
    unavailableReason: imageMagickReason,
  } = useGroupEnabled("ImageMagick");

  return (
    <Stack gap="md">
      <Divider ml="-md"></Divider>
      {/* Compression Method */}
      <ButtonSelector
        label={t("compress.method.title", "Compression Method")}
        value={parameters.compressionMethod}
        onChange={(value) => onChange?.({ compressionMethod: value as any })}
        options={[
          { value: "quality", label: t("compress.method.quality", "Quality") },
          {
            value: "filesize",
            label: t("compress.method.filesize", "File Size"),
          },
        ]}
        disabled={disabled}
      />

      {/* Quality Adjustment */}
      {parameters.compressionMethod === "quality" && (
        <Stack gap="md">
          <Divider />
          <SliderWithInput
            label={t(
              "compress.tooltip.qualityAdjustment.title",
              "Compression Level",
            )}
            value={parameters.compressionLevel || 5}
            onChange={(value) =>
              onChange?.({ compressionLevel: Number(value) })
            }
            disabled={disabled}
            min={1}
            max={9}
            step={1}
            suffix=""
          />
          <Text size="xs" c="dimmed" mt={-4}>
            {(parameters.compressionLevel || 5) <= 3 &&
              t(
                "compress.compressionLevel.range1to3",
                "Lower values preserve quality but result in larger files",
              )}
            {(parameters.compressionLevel || 5) >= 4 &&
              (parameters.compressionLevel || 5) <= 6 &&
              t(
                "compress.compressionLevel.range4to6",
                "Medium compression with moderate quality reduction",
              )}
            {(parameters.compressionLevel || 5) >= 7 &&
              t(
                "compress.compressionLevel.range7to9",
                "Higher values reduce file size significantly but may reduce image clarity",
              )}
          </Text>
        </Stack>
      )}

      <Divider />

      {/* File Size Input */}
      {parameters.compressionMethod === "filesize" && (
        <Stack gap="sm">
          <Text size="sm" fw={500}>
            {t("compress.settings.desiredSize", "Desired File Size")}
          </Text>
          <div style={{ display: "flex", gap: "8px", alignItems: "flex-end" }}>
            <NumberInput
              placeholder={t(
                "compress.settings.desiredSizePlaceholder",
                "Enter size",
              )}
              value={parameters.fileSizeValue}
              onChange={(value) =>
                onChange?.({ fileSizeValue: value?.toString() || "" })
              }
              min={0}
              disabled={disabled}
              style={{ flex: 1 }}
            />
            <Select
              value={parameters.fileSizeUnit}
              onChange={(value) => {
                // Prevent deselection - if value is null/undefined, keep the current value
                if (value) {
                  onChange?.({ fileSizeUnit: value as "KB" | "MB" });
                }
              }}
              disabled={disabled}
              data={[
                { value: "KB", label: "KB" },
                { value: "MB", label: "MB" },
              ]}
              style={{ width: "80px" }}
              comboboxProps={{
                withinPortal: true,
                zIndex: Z_INDEX_AUTOMATE_DROPDOWN,
              }}
            />
          </div>
        </Stack>
      )}

      {/* Compression Options */}
      <Stack gap="sm">
        <Checkbox
          checked={parameters.grayscale}
          onChange={(event) =>
            onChange?.({ grayscale: event.currentTarget.checked })
          }
          disabled={disabled}
          label={t(
            "compress.grayscale.label",
            "Apply Grayscale for compression",
          )}
        />

        {/* Linearize Option */}
        <Stack gap="sm">
          <Checkbox
            checked={parameters.linearize}
            onChange={(event) =>
              onChange?.({ linearize: event.currentTarget.checked })
            }
            disabled={disabled}
            label={t(
              "compress.linearize.label",
              "Linearize PDF for fast web viewing",
            )}
          />
        </Stack>

        <Tooltip
          label={
            imageMagickReason ??
            t(
              "compress.lineArt.unavailable",
              "ImageMagick is not installed or enabled on this server",
            )
          }
          disabled={imageMagickAvailable !== false}
          multiline
          maw={280}
        >
          <Box
            style={{
              cursor:
                imageMagickAvailable === false ? "not-allowed" : undefined,
            }}
          >
            <Checkbox
              checked={parameters.lineArt}
              onChange={(event) =>
                onChange?.({ lineArt: event.currentTarget.checked })
              }
              disabled={disabled || imageMagickAvailable === false}
              label={t(
                "compress.lineArt.label",
                "Convert images to line art (bilevel)",
              )}
              description={
                imageMagickAvailable !== false
                  ? t(
                      "compress.lineArt.description",
                      "Uses ImageMagick to reduce pages to high-contrast black and white for maximum size reduction.",
                    )
                  : undefined
              }
              style={{
                pointerEvents:
                  imageMagickAvailable === false ? "none" : undefined,
              }}
            />
          </Box>
        </Tooltip>
        {parameters.lineArt && (
          <Stack
            gap="xs"
            style={{
              opacity: disabled || imageMagickAvailable === false ? 0.6 : 1,
            }}
          >
            <Text size="sm" fw={600}>
              {t("compress.lineArt.detailLevel", "Detail level")}
            </Text>
            <Slider
              min={1}
              max={5}
              step={1}
              value={(() => {
                // Map threshold to slider position
                const thresholdMap = [20, 35, 50, 65, 80];
                const closest = thresholdMap.reduce(
                  (prev, curr, idx) =>
                    Math.abs(curr - (parameters.lineArtThreshold ?? 50)) <
                    Math.abs(
                      thresholdMap[prev] - (parameters.lineArtThreshold ?? 50),
                    )
                      ? idx
                      : prev,
                  0,
                );
                return closest + 1;
              })()}
              onChange={(value) => {
                // Map slider position to threshold: 1=20%, 2=35%, 3=50%, 4=65%, 5=80%
                const thresholdMap = [20, 35, 50, 65, 80];
                onChange?.({ lineArtThreshold: thresholdMap[value - 1] });
              }}
              disabled={disabled || imageMagickAvailable === false}
              label={null}
              marks={[
                { value: 1 },
                { value: 2 },
                { value: 3 },
                { value: 4 },
                { value: 5 },
              ]}
            />

            <Text size="sm" fw={600}>
              {t("compress.lineArt.edgeEmphasis", "Edge emphasis")}
            </Text>
            <SegmentedControl
              fullWidth
              options={[
                {
                  value: "1",
                  label: t("compress.lineArt.edgeLow", "Gentle"),
                  disabled: disabled || imageMagickAvailable === false,
                },
                {
                  value: "2",
                  label: t("compress.lineArt.edgeMedium", "Balanced"),
                  disabled: disabled || imageMagickAvailable === false,
                },
                {
                  value: "3",
                  label: t("compress.lineArt.edgeHigh", "Strong"),
                  disabled: disabled || imageMagickAvailable === false,
                },
              ]}
              value={(parameters.lineArtEdgeLevel ?? 2).toString()}
              onChange={(value) =>
                onChange?.({ lineArtEdgeLevel: parseInt(value) as 1 | 2 | 3 })
              }
            />
          </Stack>
        )}
      </Stack>
    </Stack>
  );
};

export default CompressSettings;
