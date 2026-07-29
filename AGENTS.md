# ScyllaDB — AI Agent Instructions

This file routes you to the relevant instruction files.
Do NOT load all files at once — read only what applies to your current task.

## Instruction Files

- `.github/copilot-instructions.md` — build system, test runner, code philosophy, test philosophy
- `.github/instructions/cpp.instructions.md` — C++ style, Seastar patterns, memory, error handling (for `*.cc`, `*.hh`)
- `.github/instructions/python.instructions.md` — Python style, testing conventions (for `*.py`)
- `.github/instructions/review.instructions.md` — code review process and checks

## Which files to read

- **Always read** `.github/copilot-instructions.md` for build/test commands and project values
- **If editing C++ files** (`*.cc`, `*.hh`): also read `.github/instructions/cpp.instructions.md`
- **If editing Python files** (`*.py`): also read `.github/instructions/python.instructions.md`
- **Before starting a code review**: read `.github/instructions/review.instructions.md`

Claude Code loads `.github/instructions/` automatically through the `.claude/rules`
symlink, scoped by each file's `paths:` frontmatter; Copilot and CodeRabbit read it
natively. Keep shared agent guidance there rather than in agent-specific skills.
