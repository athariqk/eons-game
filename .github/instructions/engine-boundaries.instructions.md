---
name: Engine Boundaries
description: "Use when refactoring or modifying engine code in src/engine. Enforces boundary rules and prevents edits to vendors and build outputs for engine tasks."
applyTo: "src/engine/**"
---
# Engine Boundaries

For engine tasks, follow these non-negotiable boundaries:

- Never edit anything under vendors/**.
- Never edit anything under build/**.
- Keep engine changes generic and reusable; do not introduce game-specific logic into src/engine.
- If an engine API change requires game updates, limit those updates to minimal adapter changes in src/game.

Before making changes:

1. Confirm the target files are not inside vendors/ or build/.
2. Prefer minimal, incremental edits with a clear migration path.
3. Validate with the standard CMake Debug build flow defined in AGENTS.md.
