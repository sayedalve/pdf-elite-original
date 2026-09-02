package pdfelite.software.SPDF.controller.api.misc;

import pdfelite.software.common.service.LoginAgreementService;

/** Restores LoginDisclaimerController expected by tests. */
public class LoginDisclaimerController {

    public static class LoginDisclaimerResponse {
        private final boolean enabled;
        private final String content;
        private final boolean showInAnonymousMode;
        private final String format;

        public LoginDisclaimerResponse(
                boolean enabled, String content, boolean showInAnonymousMode, String format) {
            this.enabled = enabled;
            this.content = content;
            this.showInAnonymousMode = showInAnonymousMode;
            this.format = format;
        }

        public boolean enabled() {
            return enabled;
        }

        public String content() {
            return content;
        }

        public boolean showInAnonymousMode() {
            return showInAnonymousMode;
        }

        public String format() {
            return format;
        }
    }

    private final LoginAgreementService loginAgreementService;

    public LoginDisclaimerController(LoginAgreementService loginAgreementService) {
        this.loginAgreementService = loginAgreementService;
    }

    public LoginDisclaimerResponse getLoginDisclaimer(String locale) {
        boolean enabled = loginAgreementService.isEnabled();
        boolean showInAnonymous = loginAgreementService.isShowInAnonymousMode();
        if (!enabled) {
            return new LoginDisclaimerResponse(false, "", showInAnonymous, "markdown");
        }
        String content = loginAgreementService.resolveContent(locale);
        if (content == null || content.trim().isEmpty()) {
            return new LoginDisclaimerResponse(false, "", showInAnonymous, "markdown");
        }
        return new LoginDisclaimerResponse(true, content, showInAnonymous, "markdown");
    }
}
