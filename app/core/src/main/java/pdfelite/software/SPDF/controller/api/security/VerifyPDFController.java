package pdfelite.software.SPDF.controller.api.security;

import java.io.IOException;
import java.io.InputStream;
import java.util.List;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.RequestBody;

import pdfelite.software.SPDF.model.api.security.PDFVerificationRequest;
import pdfelite.software.SPDF.model.api.security.PDFVerificationResult;
import pdfelite.software.SPDF.service.VeraPDFService;

/** Restores VerifyPDFController expected by tests. Exposes verifyPDF(PDFVerificationRequest). */
public class VerifyPDFController {

    private final VeraPDFService veraPDFService;

    public VerifyPDFController(VeraPDFService veraPDFService) {
        this.veraPDFService = veraPDFService;
    }

    public ResponseEntity<List<PDFVerificationResult>> verifyPDF(
            @RequestBody PDFVerificationRequest request) {
        if (request == null || request.getFileInput() == null) {
            throw new RuntimeException("fileInput is required");
        }
        try {
            if (request.getFileInput().getSize() == 0) {
                throw new RuntimeException("empty file");
            }
            try (InputStream is = request.getFileInput().getInputStream()) {
                List<PDFVerificationResult> results = veraPDFService.validatePDF(is);
                return ResponseEntity.ok(results);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        } catch (RuntimeException re) {
            throw re;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
