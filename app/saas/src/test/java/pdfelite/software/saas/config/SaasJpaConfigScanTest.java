package pdfelite.software.saas.config;

import static org.assertj.core.api.Assertions.assertThat;

import java.util.Arrays;
import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.Test;
import org.springframework.boot.persistence.autoconfigure.EntityScan;
import org.springframework.data.jpa.repository.config.EnableJpaRepositories;

/**
 * Guards {@link SaasJpaConfig}'s scan paths from drifting out of sync with the actual entity and
 * repository packages — without this, a missing package goes undetected until a runtime "No
 * qualifying bean of type" startup failure that Mockito-based tests can't catch.
 *
 * <p>Reflection-based rather than a real Spring boot because the production schema uses
 * Postgres-specific features H2 doesn't fully support.
 */
class SaasJpaConfigScanTest {

    private static final List<String> EXPECTED_REPO_PACKAGES =
            List.of(
                    "pdfelite.software.saas.repository",
                    "pdfelite.software.saas.billing.repository",
                    "pdfelite.software.saas.ai.repository",
                    "pdfelite.software.saas.payg.repository",
                    // PrepaidBundleRepository lives in payg.bundle, not payg.repository — the repo
                    // scan is leaf-level, not recursive.
                    "pdfelite.software.saas.payg.bundle");

    private static final List<String> EXPECTED_ENTITY_PACKAGES =
            List.of(
                    "pdfelite.software.saas.model",
                    "pdfelite.software.saas.billing.model",
                    "pdfelite.software.saas.ai.model",
                    // Recursive — covers all payg.* sub-packages.
                    "pdfelite.software.saas.payg");

    @Test
    void enableJpaRepositoriesIncludesAllExpectedPackages() {
        EnableJpaRepositories annotation =
                SaasJpaConfig.class.getAnnotation(EnableJpaRepositories.class);
        assertThat(annotation).as("SaasJpaConfig must carry @EnableJpaRepositories").isNotNull();

        Set<String> actual = Set.copyOf(Arrays.asList(annotation.basePackages()));
        assertThat(actual)
                .as("Every package holding @Repository interfaces must be listed")
                .containsAll(EXPECTED_REPO_PACKAGES);
    }

    @Test
    void entityScanIncludesAllExpectedPackages() {
        EntityScan annotation = SaasJpaConfig.class.getAnnotation(EntityScan.class);
        assertThat(annotation).as("SaasJpaConfig must carry @EntityScan").isNotNull();

        Set<String> actual = Set.copyOf(Arrays.asList(annotation.value()));
        assertThat(actual)
                .as("Every package holding @Entity classes must be listed")
                .containsAll(EXPECTED_ENTITY_PACKAGES);
    }
}
