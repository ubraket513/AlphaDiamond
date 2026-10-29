## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

When the user types `/graphify`, use the installed graphify skill or instructions before doing anything else.

Rules:
- Use Serena by default for code navigation and symbol/reference analysis, as required by the user’s global instructions. When graphify-out/graph.json exists, use graphify query, path, or explain as a complementary source for architectural relationships; verify findings against source with Serena.
- Dirty graphify-out/ files are expected after hooks or incremental updates; dirty graph files are not a reason to skip graphify. Only skip graphify if the task is about stale or incorrect graph output, or the user explicitly says not to use it.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying supported code, run `graphify update .` to refresh its AST graph. QML and document relationships in this graph were extracted separately; this command does not re-extract QML or document semantics, so refresh those separately after changes.
- A committed graphify-out/graph.json is rewritten whole on every build, so a large diff to it in a pull request is expected. Do not flag it as churn or suggest skipping the graph rebuild or its hooks.
