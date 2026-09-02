package pdfelite.software.proprietary.mcp.tools;

import org.springframework.beans.factory.ObjectProvider;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.stereotype.Component;

import pdfelite.software.proprietary.mcp.catalog.McpToolCatalog;
import pdfelite.software.proprietary.mcp.catalog.OperationCategory;
import tools.jackson.databind.ObjectMapper;

/** Exposes the {@code /api/v1/convert/*} namespace as a single MCP tool. */
@Component
@ConditionalOnProperty(name = "mcp.enabled", havingValue = "true")
public class pdfeliteConvertTool extends AbstractCategoryTool {

    public pdfeliteConvertTool(
            ObjectMapper mapper,
            ObjectProvider<McpToolCatalog> catalog,
            ObjectProvider<McpOperationExecutor> executor) {
        super(mapper, catalog, executor);
    }

    @Override
    public String name() {
        return "pdfelite_convert";
    }

    @Override
    public String description() {
        return "Convert files between PDF and other formats (PDF<->Word, PDF<->image, HTML->PDF,"
                + " etc.). Inspect the `operation` enum, then call pdfelite_describe_operation"
                + " with the chosen op to get its parameters JSON Schema before calling this"
                + " tool.";
    }

    @Override
    protected OperationCategory category() {
        return OperationCategory.CONVERT;
    }
}
