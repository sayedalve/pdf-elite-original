package pdfelite.software.proprietary.mcp.tools;

import org.springframework.beans.factory.ObjectProvider;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.stereotype.Component;

import pdfelite.software.proprietary.mcp.catalog.McpToolCatalog;
import pdfelite.software.proprietary.mcp.catalog.OperationCategory;
import tools.jackson.databind.ObjectMapper;

/** Exposes the {@code /api/v1/general/*} (page operations) namespace as a single MCP tool. */
@Component
@ConditionalOnProperty(name = "mcp.enabled", havingValue = "true")
public class pdfelitePagesTool extends AbstractCategoryTool {

    public pdfelitePagesTool(
            ObjectMapper mapper,
            ObjectProvider<McpToolCatalog> catalog,
            ObjectProvider<McpOperationExecutor> executor) {
        super(mapper, catalog, executor);
    }

    @Override
    public String name() {
        return "pdfelite_pages";
    }

    @Override
    public String description() {
        return "Manipulate PDF pages: merge, split, rotate, rearrange, crop, delete, overlay,"
                + " add blank pages. Call pdfelite_describe_operation with the chosen op to get"
                + " its parameters schema before invoking this tool.";
    }

    @Override
    protected OperationCategory category() {
        return OperationCategory.PAGES;
    }
}
