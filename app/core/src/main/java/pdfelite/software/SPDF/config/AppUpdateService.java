package pdfelite.software.SPDF.config;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Scope;

import pdfelite.software.common.configuration.interfaces.ShowAdminInterface;
import pdfelite.software.common.model.ApplicationProperties;

@Configuration
class AppUpdateService {

    private final ApplicationProperties applicationProperties;

    private final ShowAdminInterface showAdmin;

    public AppUpdateService(
            ApplicationProperties applicationProperties,
            @Autowired(required = false) ShowAdminInterface showAdmin) {
        this.applicationProperties = applicationProperties;
        this.showAdmin = showAdmin;
    }

    @Bean(name = "shouldShow")
    @Scope("request")
    public boolean shouldShow() {
        boolean showUpdate = applicationProperties.getSystem().isShowUpdate();
        boolean showAdminResult = showAdmin == null || showAdmin.getShowUpdateOnlyAdmins();
        return showUpdate && showAdminResult;
    }
}
