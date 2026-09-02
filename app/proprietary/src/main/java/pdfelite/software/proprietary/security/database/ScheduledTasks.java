package pdfelite.software.proprietary.security.database;

import java.sql.SQLException;

import org.springframework.context.annotation.Conditional;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import lombok.RequiredArgsConstructor;

import pdfelite.software.common.model.exception.UnsupportedProviderException;
import pdfelite.software.proprietary.security.service.DatabaseServiceInterface;

@Component
@Conditional(H2SQLCondition.class)
@RequiredArgsConstructor
public class ScheduledTasks {

    private final DatabaseServiceInterface databaseService;

    @Scheduled(cron = "#{applicationProperties.system.databaseBackup.cron}")
    public void performBackup() throws SQLException, UnsupportedProviderException {
        databaseService.exportDatabase();
    }
}
