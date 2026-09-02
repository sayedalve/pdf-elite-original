package pdfelite.software.SPDF.config;

import ch.qos.logback.core.PropertyDefinerBase;
import pdfelite.software.common.configuration.InstallationPathConfig;

public class LogbackPropertyLoader extends PropertyDefinerBase {
    @Override
    public String getPropertyValue() {
        return InstallationPathConfig.getLogPath();
    }
}
