package pdfelite.software.SPDF.controller.api.security;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.GeneralSecurityException;
import java.security.cert.CertPathBuilderException;
import java.security.cert.X509Certificate;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

import org.apache.pdfbox.pdmodel.PDDocument;
import org.apache.pdfbox.pdmodel.interactive.digitalsignature.PDSignature;
import org.bouncycastle.cert.X509CertificateHolder;
import org.bouncycastle.cert.jcajce.JcaX509CertificateConverter;
import org.bouncycastle.cms.CMSSignedData;
import org.bouncycastle.cms.SignerInformation;
import org.bouncycastle.cms.SignerInformationStore;
import org.bouncycastle.cms.jcajce.JcaSimpleSignerInfoVerifierBuilder;
import org.bouncycastle.util.Store;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.WebDataBinder;
import org.springframework.web.bind.annotation.InitBinder;

import lombok.extern.slf4j.Slf4j;

import pdfelite.software.SPDF.model.api.security.SignatureValidationRequest;
import pdfelite.software.SPDF.model.api.security.SignatureValidationResult;
import pdfelite.software.SPDF.service.CertificateValidationService;
import pdfelite.software.common.service.CustomPDFDocumentFactory;

/**
 * Controller that validates digital signatures embedded in PDF documents.
 *
 * <p>Each signature is validated in three steps:
 *
 * <ol>
 *   <li>CMS cryptographic verification – the signed bytes have not changed.
 *   <li>Certificate-chain building – the signer's certificate chains to a trusted anchor.
 *   <li>Metadata extraction – signer name, reason, location, validity dates, etc.
 * </ol>
 */
@Slf4j
public class ValidateSignatureController {

    private static final String DATE_FORMAT = "yyyy-MM-dd'T'HH:mm:ss'Z'";

    private final CustomPDFDocumentFactory pdfDocumentFactory;
    private final CertificateValidationService certValidationService;

    public ValidateSignatureController(
            CustomPDFDocumentFactory pdfDocumentFactory,
            CertificateValidationService certValidationService) {
        this.pdfDocumentFactory = pdfDocumentFactory;
        this.certValidationService = certValidationService;
    }

    /** Prevents arbitrary class binding from request parameters – standard Spring MVC hardening. */
    @InitBinder
    public void initBinder(WebDataBinder binder) {
        binder.setAllowedFields();
    }

    /**
     * Validates all digital signatures present in the supplied PDF.
     *
     * @param request contains the PDF file and an optional custom trust-anchor DER certificate
     * @return list of per-signature validation results; empty when the PDF is unsigned
     * @throws IOException when the PDF cannot be loaded
     */
    public ResponseEntity<List<SignatureValidationResult>> validateSignature(
            SignatureValidationRequest request) throws IOException {
        if (request == null || request.getFileInput() == null) {
            throw new RuntimeException("fileInput is required");
        }

        // Resolve optional custom trust anchor supplied by the caller
        X509Certificate customAnchor = loadCustomAnchor(request);

        InputStream in = null;
        PDDocument doc = null;
        try {
            in = request.getFileInput().getInputStream();
            doc = pdfDocumentFactory.load(in);

            List<PDSignature> sigDicts = doc.getSignatureDictionaries();
            if (sigDicts == null || sigDicts.isEmpty()) {
                return ResponseEntity.ok(new ArrayList<>());
            }

            // We need the raw PDF bytes for signature byte-range extraction
            byte[] pdfBytes = request.getFileInput().getBytes();

            List<SignatureValidationResult> results = new ArrayList<>();
            for (PDSignature sig : sigDicts) {
                SignatureValidationResult result = validateOneSig(sig, pdfBytes, customAnchor);
                results.add(result);
            }
            return ResponseEntity.ok(results);

        } finally {
            if (doc != null) {
                try {
                    doc.close();
                } catch (Exception ignored) {
                }
            }
            if (in != null) {
                try {
                    in.close();
                } catch (Exception ignored) {
                }
            }
        }
    }

    // ─── private helpers ─────────────────────────────────────────────────────

    /** Load the optional DER or PEM certificate from the request. */
    private X509Certificate loadCustomAnchor(SignatureValidationRequest request) {
        if (request.getCertFile() == null || request.getCertFile().isEmpty()) {
            return null;
        }
        try {
            byte[] certBytes = request.getCertFile().getBytes();
            if (certBytes.length == 0) {
                return null;
            }
            java.security.cert.CertificateFactory cf =
                    java.security.cert.CertificateFactory.getInstance("X.509");

            // Try DER first, then PEM-wrapped stream
            try {
                return (X509Certificate)
                        cf.generateCertificate(new ByteArrayInputStream(certBytes));
            } catch (Exception e) {
                // might be PEM – CertificateFactory handles PEM too via the same method
                throw e;
            }
        } catch (Exception e) {
            throw new RuntimeException("Failed to parse custom certificate: " + e.getMessage(), e);
        }
    }

    /** Validate a single signature dictionary and return a populated result. */
    private SignatureValidationResult validateOneSig(
            PDSignature sig, byte[] pdfBytes, X509Certificate customAnchor) {

        SignatureValidationResult result = new SignatureValidationResult();

        // ── signature dictionary metadata ─────────────────────────────────
        result.setSignerName(sig.getName());
        result.setReason(sig.getReason());
        result.setLocation(sig.getLocation());
        Calendar sigDate = sig.getSignDate();
        if (sigDate != null) {
            result.setSignatureDate(formatDate(sigDate.getTime()));
        }

        // ── extract CMS /Contents bytes from the PDF byte-range ──────────
        byte[] cmsBytes;
        try {
            cmsBytes = sig.getContents(new ByteArrayInputStream(pdfBytes));
        } catch (Exception e) {
            log.warn("Could not extract /Contents from signature: {}", e.getMessage());
            result.setValid(false);
            result.setErrorMessage("Cannot extract CMS contents: " + e.getMessage());
            return result;
        }

        // ── parse CMSSignedData ───────────────────────────────────────────
        CMSSignedData cmsSignedData;
        try {
            // /Contents may be zero-padded to its reserved length; use stream variant
            cmsSignedData = new CMSSignedData(new ByteArrayInputStream(cmsBytes));
        } catch (Exception e) {
            log.warn("Failed to parse CMS structure: {}", e.getMessage());
            result.setValid(false);
            result.setErrorMessage("Invalid CMS structure: " + e.getMessage());
            return result;
        }

        // ── iterate signers (typically one per signature field) ───────────
        SignerInformationStore signerInfos = cmsSignedData.getSignerInfos();
        Iterator<SignerInformation> signerIt = signerInfos.getSigners().iterator();
        if (!signerIt.hasNext()) {
            result.setValid(false);
            result.setErrorMessage("No signer information found in CMS");
            return result;
        }

        SignerInformation signerInfo = signerIt.next();

        @SuppressWarnings("unchecked")
        Store<X509CertificateHolder> certStore = cmsSignedData.getCertificates();

        // ── find the signer's certificate ─────────────────────────────────
        X509Certificate signerCert = resolveSignerCert(signerInfo, certStore);
        if (signerCert == null) {
            result.setValid(false);
            result.setErrorMessage("Signer certificate not found in CMS bag");
            return result;
        }

        // ── cryptographic verification ────────────────────────────────────
        boolean cmsValid = false;
        try {
            cmsValid =
                    signerInfo.verify(new JcaSimpleSignerInfoVerifierBuilder().build(signerCert));
        } catch (Exception e) {
            log.debug("CMS verification failed: {}", e.getMessage());
        }
        result.setValid(cmsValid);

        // ── validation time ───────────────────────────────────────────────
        CertificateValidationService.ValidationTime valTime =
                certValidationService.extractValidationTime(signerInfo);
        if (valTime != null) {
            result.setValidationTimeSource(valTime.source);
        } else {
            result.setValidationTimeSource("current");
            valTime = new CertificateValidationService.ValidationTime(new Date(), "current");
        }

        // ── certificate expiry ────────────────────────────────────────────
        boolean notExpired =
                !certValidationService.isOutsideValidityPeriod(signerCert, valTime.date);
        result.setNotExpired(notExpired);

        // ── certificate chain validation ──────────────────────────────────
        Collection<X509Certificate> intermediates =
                certValidationService.extractIntermediateCertificates(certStore, signerCert);
        try {
            var pathResult =
                    certValidationService.buildAndValidatePath(
                            signerCert, intermediates, customAnchor, valTime.date);
            result.setChainValid(true);
            result.setTrustValid(true);
            result.setCertPathLength(pathResult.getCertPath().getCertificates().size());
        } catch (CertPathBuilderException e) {
            result.setChainValid(false);
            result.setTrustValid(false);
            result.setChainValidationError(e.getMessage());
            log.debug("Certificate chain validation failed: {}", e.getMessage());
        } catch (GeneralSecurityException e) {
            result.setChainValid(false);
            result.setTrustValid(false);
            result.setChainValidationError(e.getMessage());
            log.debug("Certificate validation error: {}", e.getMessage());
        }

        // ── revocation status ─────────────────────────────────────────────
        boolean revEnabled = certValidationService.isRevocationEnabled();
        result.setRevocationChecked(revEnabled);
        result.setRevocationStatus(revEnabled ? "unknown" : "not-checked");

        // ── certificate metadata ──────────────────────────────────────────
        populateCertMetadata(result, signerCert);

        return result;
    }

    /** Resolve the signing certificate from the CMS certificate bag. */
    private X509Certificate resolveSignerCert(
            SignerInformation signerInfo, Store<X509CertificateHolder> certStore) {
        try {
            Collection<X509CertificateHolder> matches = certStore.getMatches(signerInfo.getSID());
            if (matches == null || matches.isEmpty()) {
                return null;
            }
            JcaX509CertificateConverter conv = new JcaX509CertificateConverter();
            return conv.getCertificate(matches.iterator().next());
        } catch (Exception e) {
            log.debug("Could not resolve signer certificate: {}", e.getMessage());
            return null;
        }
    }

    /** Populate certificate-level metadata fields on the result. */
    private void populateCertMetadata(SignatureValidationResult result, X509Certificate cert) {
        result.setSubjectDN(cert.getSubjectX500Principal().getName());
        result.setIssuerDN(cert.getIssuerX500Principal().getName());
        result.setSerialNumber(cert.getSerialNumber().toString(16).toUpperCase());
        result.setValidFrom(formatDate(cert.getNotBefore()));
        result.setValidUntil(formatDate(cert.getNotAfter()));
        result.setSignatureAlgorithm(cert.getSigAlgName());
        result.setVersion(String.valueOf(cert.getVersion()));
        result.setSelfSigned(certValidationService.isSelfSigned(cert));

        // Key size
        try {
            var publicKey = cert.getPublicKey();
            if (publicKey instanceof java.security.interfaces.RSAKey rsa) {
                result.setKeySize(rsa.getModulus().bitLength());
            } else if (publicKey instanceof java.security.interfaces.ECKey ec) {
                result.setKeySize(ec.getParams().getCurve().getField().getFieldSize());
            }
        } catch (Exception e) {
            log.debug("Could not determine key size: {}", e.getMessage());
        }
    }

    private String formatDate(Date date) {
        if (date == null) return null;
        return new SimpleDateFormat(DATE_FORMAT).format(date);
    }
}
