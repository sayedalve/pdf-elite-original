# Phase 32: Legacy Architecture Retirement Plan

With the native C++20 / Direct2D rewrite structurally complete, the following steps must be taken by the devops team to officially retire the legacy application.

## 1. Codebase Deprecation
- Rename the existing root `frontend` folder to `frontend_legacy`.
- Rename the existing `docker`, `backend`, and `testing` folders as they contain the Java Spring Boot backend and Stirling PDF engine which are no longer required.
- Do NOT delete these folders immediately. Keep them intact for 6 months as a functional reference for any complex PDF operations (like splitting/merging algorithms) that need to be ported to the Native C++ wrappers.

## 2. CI/CD Pipeline Migration
- Disable the old GitHub Actions workflows that build the Tauri/Java stacks.
- Ensure the new `.github/workflows/native-build.yml` is marked as a required check for pull requests.

## 3. Dependency Cleanup
- Remove `node_modules` and the `package.json` at the root.
- Remove `Taskfile.yml` as the task runner is now completely replaced by native CMake commands.
- Inform all developers that Java, Node.js, and Rust are no longer prerequisites for the project. The only prerequisites are MSVC and CMake.
