package pdfelite.software.proprietary.mcp;

import java.util.Set;

/** Per-call context: resolved pdfelite identity and granted scopes for an {@link McpTool#call}. */
public record McpCallContext(
        String pdfeliteUserId, Set<String> grantedScopes, boolean scopesEnabled) {

    public boolean hasScope(String required) {
        if (!scopesEnabled) {
            return true;
        }
        return required == null || required.isBlank() || grantedScopes.contains(required);
    }
}
