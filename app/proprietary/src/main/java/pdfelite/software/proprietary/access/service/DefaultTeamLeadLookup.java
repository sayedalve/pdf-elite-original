package pdfelite.software.proprietary.access.service;

import pdfelite.software.proprietary.security.model.User;

/** No-op {@link TeamLeadLookup}: always false. */
public class DefaultTeamLeadLookup implements TeamLeadLookup {

    @Override
    public boolean isAnyTeamLeader(User user) {
        return false;
    }

    @Override
    public boolean isLeaderOfTeam(User user, Long teamId) {
        return false;
    }
}
