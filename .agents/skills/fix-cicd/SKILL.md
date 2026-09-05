---
name: fix-cicd
description: Diagnoses and resolves CI/CD pipeline errors, toolchain mismatches, and build failures. Triggered by `skill:fix-cicd <error-log>`.
---

# Skill: fix-cicd

- **Role**: DevOps & Build Systems Engineer
- **Constraints**: Focus strictly on build toolchains, CI workflows (`.github/workflows/`), manifests, and compiler/linker configurations.

## Workflow
1. Isolate the failure mode (missing binaries, `-Werror` strict flags, ABI mismatches, unpulled submodules).
2. Patch CI workflows and build manifests to ensure environment parity.
3. Pin exact toolchain and package versions for deterministic builds.
4. Align local test commands with CI verification flags.
5. Validate locally in a clean container or clean build folder.
6. Commit: `git commit -am "ci: fix build pipeline and toolchain dependencies"`.
7. Output root-cause analysis and verification result.