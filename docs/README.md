# Documentation Index

This directory contains comprehensive documentation for the Goblin Instrumentality Project.

## 📋 Quick Links

### Getting Started
- **[QUICKSTART_TASKS.md](QUICKSTART_TASKS.md)** - Quick guide to create GitHub Issues from task documentation

### Workflow Documentation  
- **[GITHUB_WORKFLOWS.md](GITHUB_WORKFLOWS.md)** - Complete GitHub Actions workflow documentation
- **[REMAINING_WORKFLOW_TASKS.md](REMAINING_WORKFLOW_TASKS.md)** - Overview of remaining workflow tasks from PR #2

### Task Management
- **[tasks/](tasks/)** - Detailed task specifications for planned features
  - [task-001-format-check-workflow.md](tasks/task-001-format-check-workflow.md) - Format validation workflow
  - [task-002-code-quality-workflow.md](tasks/task-002-code-quality-workflow.md) - Code quality checks workflow
  - [task-003-metrics-generation-workflow.md](tasks/task-003-metrics-generation-workflow.md) - Metrics generation workflow
  - [README.md](tasks/README.md) - Task documentation guide

### Troubleshooting
- **[copilot-known-errors.md](copilot-known-errors.md)** - Known errors and solutions

## 📂 Documentation Structure

```
docs/
├── README.md                          (this file)
├── QUICKSTART_TASKS.md               Quick guide to creating issues from tasks
├── REMAINING_WORKFLOW_TASKS.md       Overview of remaining work from PR #2
├── GITHUB_WORKFLOWS.md               Complete workflow documentation
├── copilot-known-errors.md           Troubleshooting guide
└── tasks/                            Detailed task specifications
    ├── README.md                     How to use task files
    ├── task-001-format-check-workflow.md
    ├── task-002-code-quality-workflow.md
    └── task-003-metrics-generation-workflow.md
```

## 🎯 What Should I Read?

### If you want to...

**Create GitHub Issues for remaining work:**
→ Read [QUICKSTART_TASKS.md](QUICKSTART_TASKS.md)

**Understand the existing GitHub workflows:**
→ Read [GITHUB_WORKFLOWS.md](GITHUB_WORKFLOWS.md)

**Learn about planned workflow enhancements:**
→ Read [REMAINING_WORKFLOW_TASKS.md](REMAINING_WORKFLOW_TASKS.md)

**Implement a specific task:**
→ Read the corresponding file in [tasks/](tasks/)

**Troubleshoot build/workflow errors:**
→ Read [copilot-known-errors.md](copilot-known-errors.md) and [GITHUB_WORKFLOWS.md](GITHUB_WORKFLOWS.md#troubleshooting)

**Understand task documentation format:**
→ Read [tasks/README.md](tasks/README.md)

## 🔄 Recent Updates

### 2026-02-14: Task Documentation Added
- Created comprehensive task files for workflows removed from PR #2
- Added quick start guide for creating GitHub Issues
- Added overview document for remaining workflow tasks
- Organized tasks in dedicated directory with README

### 2026-02-14: PR #2 Merged
- Added build-and-validate.yml workflow
- Created GitHub workflows documentation
- Added CI automation to copilot instructions

## 📝 Contributing to Documentation

When adding new documentation:

1. **Create descriptive filenames** using SCREAMING_SNAKE_CASE for overview docs
2. **Use kebab-case** for detailed task/feature docs
3. **Update this README** with links to new files
4. **Follow existing structure** for consistency
5. **Include examples** where applicable
6. **Link related documents** for easy navigation

### Documentation Guidelines

- Use clear, concise language
- Include code examples where helpful
- Link to related files and commits
- Keep table of contents up to date
- Use markdown formatting consistently
- Add troubleshooting sections where needed

## 🔗 Related Documentation

### Project Root
- **[README.md](../README.md)** - Project overview and architecture
- **[.github/copilot-instructions.md](../.github/copilot-instructions.md)** - Development guidelines and standards

### GitHub Workflows
- **[.github/workflows/](../.github/workflows/)** - Actual workflow files
  - [build-and-validate.yml](../.github/workflows/build-and-validate.yml) - Build automation

### Source Documentation
- **[src/](../src/)** - Source code (self-documenting)
- **[include/](../include/)** - Header files

## ❓ Questions?

If you can't find what you're looking for:

1. Check the main [README.md](../README.md)
2. Review [copilot-instructions.md](../.github/copilot-instructions.md)
3. Search documentation with `grep -r "search term" docs/`
4. Check related PRs and issues on GitHub
5. Review commit history for context

## 📚 External Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [CMake Documentation](https://cmake.org/documentation/)
- [DirectX 12 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [NVENC Video Encoder API](https://docs.nvidia.com/video-technologies/video-codec-sdk/)
