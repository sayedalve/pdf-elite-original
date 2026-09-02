package pdfelite.software.saas.config;

import org.springframework.boot.persistence.autoconfigure.EntityScan;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;
import org.springframework.data.jpa.repository.config.EnableJpaRepositories;

/**
 * Registers the {@code :saas} module's entities and repositories with Spring Data JPA. Any new
 * package holding {@code @Repository} or {@code @Entity} classes must be added here, or the beans
 * won't wire at startup.
 */
@Configuration
@Profile("saas")
@EnableJpaRepositories(
        basePackages = {
            "pdfelite.software.saas.accountlink",
            "pdfelite.software.saas.repository",
            "pdfelite.software.saas.billing.repository",
            "pdfelite.software.saas.ai.repository",
            "pdfelite.software.saas.payg.repository",
            "pdfelite.software.saas.payg.bundle",
            "pdfelite.software.saas.procurement.repository"
        })
@EntityScan({
    "pdfelite.software.saas.accountlink",
    "pdfelite.software.saas.model",
    "pdfelite.software.saas.billing.model",
    "pdfelite.software.saas.ai.model",
    "pdfelite.software.saas.payg",
    "pdfelite.software.saas.procurement.model"
})
public class SaasJpaConfig {}
