package pdfelite.software.proprietary.policy.config;

import java.util.List;
import java.util.Objects;

import org.springframework.stereotype.Component;

import lombok.RequiredArgsConstructor;

import pdfelite.software.common.model.ApplicationProperties;
import pdfelite.software.common.service.UserServiceInterface;
import pdfelite.software.proprietary.policy.model.Policy;
import pdfelite.software.proprietary.policy.store.PolicyStore;

/**
 * Policies are scoped to a team: a user may view, run, edit, and delete only the policies belonging
 * to their own team (the team a policy is stamped with at creation). This binds everyone — admins
 * included — so no one sees or touches another team's policies. <em>Whether</em> a user may edit
 * (vs only view/run) is a separate check gated at the controller ({@code
 * PolicyController#requirePolicyEditingAllowed} → team leader). Enforced only when login is
 * enabled; single-user deployments (login disabled) pass every check.
 */
@Component
@RequiredArgsConstructor
public class PolicyAccessGuard {

    private final UserServiceInterface userService;
    private final ApplicationProperties applicationProperties;
    private final PolicyManagementAuthority policyManagementAuthority;

    /** Owner for a new policy: the current user, or {@code null} when login is disabled. */
    public String ownerForNewPolicy() {
        return enforced() ? userService.getCurrentUsername() : null;
    }

    /** Team a new policy is stamped with — the creator's team. {@code null} when login disabled. */
    public Long teamForNewPolicy() {
        return enforced() ? policyManagementAuthority.currentUserTeamId() : null;
    }

    /** Whether the policy belongs to the current user's team (so they may view/run/edit it). */
    public boolean canAccess(Policy policy) {
        if (!enforced()) {
            return true;
        }
        return Objects.equals(policy.teamId(), policyManagementAuthority.currentUserTeamId());
    }

    /**
     * The policies visible to the caller: their whole team's, loaded scoped rather than fetched
     * globally and filtered, so on SaaS it never pulls another team's policies into memory. Login
     * disabled (single-user) returns everything.
     */
    public List<Policy> visibleFrom(PolicyStore store) {
        if (!enforced()) {
            return store.all();
        }
        return store.findByTeam(policyManagementAuthority.currentUserTeamId());
    }

    private boolean enforced() {
        return applicationProperties.getSecurity().isEnableLogin();
    }
}
