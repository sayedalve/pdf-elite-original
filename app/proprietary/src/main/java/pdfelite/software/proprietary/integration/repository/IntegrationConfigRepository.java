package pdfelite.software.proprietary.integration.repository;

import java.util.List;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import pdfelite.software.proprietary.access.model.OwnerScope;
import pdfelite.software.proprietary.integration.model.IntegrationConfig;
import pdfelite.software.proprietary.model.Team;
import pdfelite.software.proprietary.security.model.User;

@Repository
public interface IntegrationConfigRepository extends JpaRepository<IntegrationConfig, Long> {

    List<IntegrationConfig> findByOwnerUser(User ownerUser);

    List<IntegrationConfig> findByOwnerTeam(Team ownerTeam);

    List<IntegrationConfig> findByScope(OwnerScope scope);

    // Nested path: OwnedResource has a getOwnerTeamId() convenience getter but no such persistent
    // attribute, so the plain "...OwnerTeamId" derivation resolves to a phantom property and throws
    // UnknownPathException. The underscore forces the real ownerTeam.id association path.
    boolean existsByOwnerTeam_Id(Long teamId);

    void deleteByOwnerUser(User ownerUser);

    void deleteByOwnerTeam_Id(Long teamId);
}
