package pdfelite.software.SPDF.service.misc;

import java.io.IOException;

import org.springframework.core.io.InputStreamResource;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

import lombok.RequiredArgsConstructor;

import pdfelite.software.SPDF.Factories.ReplaceAndInvertColorFactory;
import pdfelite.software.common.model.api.misc.HighContrastColorCombination;
import pdfelite.software.common.model.api.misc.ReplaceAndInvert;
import pdfelite.software.common.util.misc.ReplaceAndInvertColorStrategy;

@Service
@RequiredArgsConstructor
public class ReplaceAndInvertColorService {
    private final ReplaceAndInvertColorFactory replaceAndInvertColorFactory;

    public InputStreamResource replaceAndInvertColor(
            MultipartFile file,
            ReplaceAndInvert replaceAndInvertOption,
            HighContrastColorCombination highContrastColorCombination,
            String backGroundColor,
            String textColor)
            throws IOException {

        ReplaceAndInvertColorStrategy replaceColorStrategy =
                replaceAndInvertColorFactory.replaceAndInvert(
                        file,
                        replaceAndInvertOption,
                        highContrastColorCombination,
                        backGroundColor,
                        textColor);

        return replaceColorStrategy.replace();
    }
}
