# Agent Instructions

## Build and Testing

**DO NOT attempt to test compile (cmake --build) automatically.**  
The build environment (cmake, ninja) is not guaranteed to be installed or configured. Instead:

1. **Verify changes compile** by checking syntax/semantic errors via LSP diagnostics (already shown in editor)
2. **If compilation is required**, ask the user to run the build manually or provide the appropriate build command
3. **Default assumption:** The project builds successfully when no LSP errors are introduced

## Code Verification

- Rely on LSP diagnostics for C++/Qt errors
- Run any existing unit tests if `make test` or `ctest` is known to work
- For visual/UI changes, manual testing by the user is expected

## Git Workflow

- Follow CLAUDE.md conventions for commit messages
- Create feature branches when appropriate
- Use `git status` and `git diff` to verify changes before committing