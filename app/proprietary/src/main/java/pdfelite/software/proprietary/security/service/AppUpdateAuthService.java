package pdfelite.software.proprietary.security.service;

import java.util.Optional;

import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.stereotype.Service;

import lombok.RequiredArgsConstructor;

import pdfelite.software.common.configuration.interfaces.ShowAdminInterface;
import pdfelite.software.common.model.ApplicationProperties;
import pdfelite.software.proprietary.security.database.repository.UserRepository;
import pdfelite.software.proprietary.security.model.User;

@Service
@RequiredArgsConstructor
class AppUpdateAuthService implements ShowAdminInterface {

    private final UserRepository userRepository;

    private final ApplicationProperties applicationProperties;

    @Override
    public boolean getShowUpdateOnlyAdmins() {
        boolean showUpdate = applicationProperties.getSystem().isShowUpdate();
        if (!showUpdate) {
            return showUpdate;
        }
        boolean showUpdateOnlyAdmin = applicationProperties.getSystem().isShowUpdateOnlyAdmin();
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        if (authentication == null || !authentication.isAuthenticated()) {
            return !showUpdateOnlyAdmin;
        }
        if ("anonymousUser".equalsIgnoreCase(authentication.getName())) {
            return !showUpdateOnlyAdmin;
        }
        Optional<User> user = userRepository.findByUsername(authentication.getName());
        if (user.isPresent() && showUpdateOnlyAdmin) {
            return "ROLE_ADMIN".equals(user.get().getRolesAsString());
        }
        return showUpdate;
    }
}
