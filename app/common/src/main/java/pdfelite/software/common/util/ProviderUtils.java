package pdfelite.software.common.util;

import static pdfelite.software.common.util.ValidationUtils.isCollectionEmpty;
import static pdfelite.software.common.util.ValidationUtils.isStringEmpty;

import pdfelite.software.common.model.oauth2.Provider;

public class ProviderUtils {

    public static boolean validateProvider(Provider provider) {
        if (provider == null) {
            return false;
        }

        if (isStringEmpty(provider.getClientId())) {
            return false;
        }

        if (isStringEmpty(provider.getClientSecret())) {
            return false;
        }

        if (isCollectionEmpty(provider.getScopes())) {
            return false;
        }

        return true;
    }
}
