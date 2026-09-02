package pdfelite.software.proprietary.integration.dto;

import java.util.Map;

import pdfelite.software.proprietary.access.model.DefaultAccessPolicy;
import pdfelite.software.proprietary.access.model.OwnerScope;
import pdfelite.software.proprietary.integration.model.IntegrationType;

/** Create/update payload for an integration config. Sensitive config values left blank are kept. */
public record IntegrationConfigRequest(
        IntegrationType integrationType,
        String name,
        OwnerScope scope,
        Long ownerTeamId,
        Boolean enabled,
        Boolean locked,
        DefaultAccessPolicy defaultAccess,
        Map<String, Object> config) {}
