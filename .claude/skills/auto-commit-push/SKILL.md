---
name: auto-commit-push
description: Automatically commit and push changes after completing significant work
---

# Auto Commit and Push Skill

Use this skill when you have completed a significant amount of work and want to automatically commit and push the changes to the repository.

## When to Use

Use this skill when ALL of the following conditions are met:
1. You have completed a coherent unit of work (e.g., implemented a feature, fixed a bug, added tests)
2. All changes have been made and verified
3. The user has not explicitly asked you to NOT commit changes
4. You are ready to finalize the current work session

## Pre-Commit Checklist

Before committing, ensure:
- [ ] Run any relevant lint/typecheck commands (check AGENTS.md for project-specific commands)
- [ ] Verify no secrets or sensitive data are included in changes
- [ ] Check that all changes are relevant to the completed work

## Commit Process

1. **Check git status**: Run `git status` to see staged/unstaged changes
2. **Stage changes**: Add relevant files with `git add [files]` or `git add .` for all changes
3. **Generate commit message**:
   - Analyze changes to determine type (feature, fix, refactor, docs, test, etc.)
   - Write concise commit message following pattern: `<type>: <description>`
   - Common types: `feat`, `fix`, `refactor`, `docs`, `test`, `chore`
   - Add brief description of changes (1-2 sentences)
4. **Create commit**: `git commit -m "<message>"`
5. **Push to remote**: `git push origin <current-branch>`
6. **Verify**: Check `git status` and `git log` to confirm success

## Commit Message Guidelines

- **Feature additions**: "feat: Add [feature] to [component]"
- **Bug fixes**: "fix: Resolve [issue] in [component]" 
- **Refactoring**: "refactor: Improve [aspect] of [component]"
- **Documentation**: "docs: Update [documentation] for [component]"
- **Tests**: "test: Add tests for [component]"
- **Maintenance**: "chore: Update [dependencies/tools/config]"

## Error Handling

If commit fails:
- Check for pre-commit hooks or validation errors
- Fix issues and retry
- Do NOT use `--amend` unless you created the HEAD commit in this session

If push fails:
- Check if branch exists on remote: `git push -u origin <branch-name>`
- Resolve any conflicts before retrying

## Safety Rules

1. **NEVER** commit if user explicitly said not to
2. **ALWAYS** verify changes before committing
3. **NEVER** commit secrets or sensitive data
4. **ALWAYS** run verification commands if specified in AGENTS.md
5. **NEVER** force push (`--force`) unless explicitly instructed
6. **ALWAYS** check git status before and after operations