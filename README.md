# CadenceArc Sandbox

CadenceArc Sandbox is a minimal Unreal Engine 5.7 C++ host project used to develop, integrate, and validate the **CadenceArc** combo framework.

CadenceArc is intended to resolve semantic input tags through a configurable branching combo graph and emit action tags for an external execution system. The framework core is designed to remain independent of Gameplay Ability System, animation montages, collision handling, and damage calculation.

## Purpose

This repository provides an isolated environment for:

- validating that CadenceArc can run without the WarriorRPG project;
- configuring test combo graphs with Unreal Engine data assets;
- exercising deterministic `InputTag -> graph traversal -> ActionTag` behavior;
- running automated and in-editor integration tests;
- demonstrating framework behavior without unrelated game systems.

The reusable framework itself lives in the separate `CadenceArc` repository and will be referenced from `Plugins/CadenceArc` as a Git submodule.

## Current Milestone

Phase 1 focuses on the smallest complete resolution pipeline:

```text
InputTag -> configurable combo graph -> ActionTag
```

Phase 1 deliberately excludes:

- Gameplay Ability System integration;
- animation montage playback;
- collision and damage processing;
- input buffering and timing windows;
- cancellation, interruption, and networking;
- custom graph editor tooling.

## Planned Repository Layout

```text
CadenceArcSandbox/
|-- CadenceArcSandbox.uproject
|-- Config/
|-- Content/
|-- Source/
`-- Plugins/
    `-- CadenceArc/    # Git submodule
```

## Requirements

- Unreal Engine 5.7
- A supported C++ toolchain for Unreal Engine 5.7
- Git
- Git LFS for Unreal binary assets

## Getting Started

Clone the project together with its plugin submodule:

```bash
git clone --recurse-submodules https://github.com/1zumiii/CadenceArcSandbox.git
```

If the repository was cloned without submodules:

```bash
git submodule update --init --recursive
```

Generate project files, build the `CadenceArcSandboxEditor` target in the Development Editor configuration, and open `CadenceArcSandbox.uproject` with Unreal Engine 5.7.

## Repository Boundary

Sandbox-specific maps, test input tags, and test combo data belong in this repository. Reusable runtime code, framework-owned data types, validation logic, and framework tests belong in the CadenceArc plugin repository.

