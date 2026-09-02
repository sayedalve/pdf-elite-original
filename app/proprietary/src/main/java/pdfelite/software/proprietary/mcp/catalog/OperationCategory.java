package pdfelite.software.proprietary.mcp.catalog;

/** MCP tool categories; {@link #urlPrefix} maps a {@code /api/v1/} namespace to a category. */
public enum OperationCategory {
    CONVERT("/api/v1/convert/", "pdfelite_convert"),
    PAGES("/api/v1/general/", "pdfelite_pages"),
    MISC("/api/v1/misc/", "pdfelite_misc"),
    SECURITY("/api/v1/security/", "pdfelite_security"),
    AI(null, "pdfelite_ai");

    private final String urlPrefix;
    private final String toolName;

    OperationCategory(String urlPrefix, String toolName) {
        this.urlPrefix = urlPrefix;
        this.toolName = toolName;
    }

    public String urlPrefix() {
        return urlPrefix;
    }

    public String toolName() {
        return toolName;
    }

    public static OperationCategory fromUrl(String url) {
        if (url == null) {
            return null;
        }
        for (OperationCategory c : values()) {
            if (c.urlPrefix != null && url.startsWith(c.urlPrefix)) {
                return c;
            }
        }
        return null;
    }
}
