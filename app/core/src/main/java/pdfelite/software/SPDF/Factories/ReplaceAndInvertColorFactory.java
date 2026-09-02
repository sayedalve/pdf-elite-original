package pdfelite.software.SPDF.Factories;

import org.springframework.stereotype.Component;
import org.springframework.web.multipart.MultipartFile;

import lombok.RequiredArgsConstructor;

import pdfelite.software.SPDF.config.EndpointConfiguration;
import pdfelite.software.common.model.api.misc.HighContrastColorCombination;
import pdfelite.software.common.model.api.misc.ReplaceAndInvert;
import pdfelite.software.common.util.TempFileManager;
import pdfelite.software.common.util.misc.ColorSpaceConversionStrategy;
import pdfelite.software.common.util.misc.CustomColorReplaceStrategy;
import pdfelite.software.common.util.misc.InvertFullColorStrategy;
import pdfelite.software.common.util.misc.ReplaceAndInvertColorStrategy;

@Component
@RequiredArgsConstructor
public class ReplaceAndInvertColorFactory {

    private final TempFileManager tempFileManager;
    private final EndpointConfiguration endpointConfiguration;

    public ReplaceAndInvertColorStrategy replaceAndInvert(
            MultipartFile file,
            ReplaceAndInvert replaceAndInvertOption,
            HighContrastColorCombination highContrastColorCombination,
            String backGroundColor,
            String textColor) {

        if (replaceAndInvertOption == null) {
            return null;
        }

        // Check Ghostscript availability for CMYK conversion
        if (replaceAndInvertOption == ReplaceAndInvert.COLOR_SPACE_CONVERSION
                && !endpointConfiguration.isGroupEnabled("Ghostscript")) {
            throw new IllegalStateException(
                    "CMYK color space conversion requires Ghostscript, which is not available on this system");
        }

        return switch (replaceAndInvertOption) {
            case CUSTOM_COLOR, HIGH_CONTRAST_COLOR ->
                    new CustomColorReplaceStrategy(
                            file,
                            replaceAndInvertOption,
                            textColor,
                            backGroundColor,
                            highContrastColorCombination);
            case FULL_INVERSION -> new InvertFullColorStrategy(file, replaceAndInvertOption);
            case COLOR_SPACE_CONVERSION ->
                    new ColorSpaceConversionStrategy(file, replaceAndInvertOption, tempFileManager);
        };
    }
}
