---
name: 🐛 Bug Report
about: Create a bug report
title: "[BUG]: "
labels: ["status: unverified"]
assignees: []
---

## 🔍 Subsystem Scope

Select the exact architectural layer this issue impacts (delete inapplicable items):

- [ ] **Engine** --- Kahn scheduling, priority queue calculations, resource graph bugs, memory allocations.
- [ ] **CLI** --- Cobra commands, token flags, boundary linking exceptions, manifest loading.
- [ ] **Dashboard** --- Node rendering bugs, `hypha browse` UI glitches, JSON graph payloads.
- [ ] **Scripting Layer** --- Sandbox restrictions, custom controller processing.
- [ ] **Data Layer** --- Schema errors
- [ ] **Unsure** --- Elaborate in comments

## 📝 Problem Description

Provide a clear, concise description of the unexpected behavior.

## ⚡ Reproduction Steps

1. Your manifest file definitions (YAML/JSON/Jsonnet/Lua snippet):
2. Exact CLI command executed (e.g., `hypha apply --verbose`):
3. Actual output vs. expected target output state:

## 💻 Environment Diagnostics

- **Operating System / Distro**:
- **Hypha Version (`hypha info`)**:
- **Execution Target**: Native Host / Sandbox Container

## 🛠️ Additional Context / Logs

Paste your diagnostic logs or traceback sequences here.
