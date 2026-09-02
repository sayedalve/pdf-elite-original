package pdfelite.software.SPDF.model.api.converters;

import java.nio.charset.StandardCharsets;

import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ModelAttribute;
import org.springframework.web.multipart.MultipartFile;

import io.github.pixee.security.Filenames;
import io.swagger.v3.oas.annotations.Operation;

import lombok.RequiredArgsConstructor;

import stirling.software.jpdfium.PdfDocument;

import pdfelite.software.SPDF.config.swagger.MarkdownConversionResponse;
import pdfelite.software.common.annotations.AutoJobPostMapping;
import pdfelite.software.common.annotations.api.ConvertApi;
import pdfelite.software.common.enumeration.ResourceWeight;
import pdfelite.software.common.model.api.PDFFile;
import pdfelite.software.common.model.tool.ToolFormat;
import pdfelite.software.common.model.tool.ToolIO;
import pdfelite.software.common.pdf.PdfMarkdownConverter;
import pdfelite.software.common.util.TempFile;
import pdfelite.software.common.util.TempFileManager;
import pdfelite.software.common.util.WebResponseUtils;

@ConvertApi
@RequiredArgsConstructor
public class ConvertPDFToMarkdown {

    private final TempFileManager tempFileManager;

    @AutoJobPostMapping(
            consumes = MediaType.MULTIPART_FORM_DATA_VALUE,
            value = "/pdf/markdown",
            resourceWeight = ResourceWeight.MEDIUM_WEIGHT)
    @MarkdownConversionResponse
    @ToolIO(produces = ToolFormat.MARKDOWN)
    @Operation(
            summary = "Convert PDF to Markdown",
            description = "This endpoint converts a PDF file to Markdown format.")
    public ResponseEntity<byte[]> processPdfToMarkdown(@ModelAttribute PDFFile file)
            throws Exception {
        MultipartFile inputFile = file.getFileInput();

        String originalName = Filenames.toSimpleFileName(inputFile.getOriginalFilename());
        String baseName =
                originalName.contains(".")
                        ? originalName.substring(0, originalName.lastIndexOf('.'))
                        : originalName;

        String markdown;
        try (TempFile tempInput = new TempFile(tempFileManager, ".pdf")) {
            inputFile.transferTo(tempInput.getFile());
            try (PdfDocument doc = PdfDocument.open(tempInput.getPath())) {
                markdown = new PdfMarkdownConverter().convert(doc);
            }
        }

        return WebResponseUtils.bytesToWebResponse(
                markdown.getBytes(StandardCharsets.UTF_8),
                baseName + ".md",
                MediaType.valueOf("text/markdown"));
    }
}
