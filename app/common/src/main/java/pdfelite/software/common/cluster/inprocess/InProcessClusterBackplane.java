package pdfelite.software.common.cluster.inprocess;

import lombok.extern.slf4j.Slf4j;

import pdfelite.software.common.cluster.ClusterBackplane;
import pdfelite.software.common.model.ApplicationProperties;

@Slf4j
public class InProcessClusterBackplane implements ClusterBackplane {

    private final ApplicationProperties applicationProperties;

    public InProcessClusterBackplane(ApplicationProperties applicationProperties) {
        this.applicationProperties = applicationProperties;
    }

    @Override
    public boolean isHealthy() {
        return true;
    }

    @Override
    public String backplaneType() {
        return "inprocess";
    }

    @Override
    public String localNodeId() {
        return applicationProperties.getCluster().resolvedNodeId();
    }
}
