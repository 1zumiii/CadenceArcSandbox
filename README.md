# CadenceArc Sandbox

CadenceArc Sandbox is a minimal Unreal Engine 5.7 C++ host project used to develop, integrate, and validate the [CadenceArc](https://github.com/1zumiii/CadenceArc) branching action framework.

CadenceArc resolves semantic input tags through configurable action graphs and emits action requests for an external execution system. Its core remains independent of Gameplay Ability System, animation montages, collision handling, and damage calculation.

## Repository Relationship

```text
CadenceArcSandbox
`-- Plugins/
    `-- CadenceArc/    # Git submodule
```

The repositories have separate histories:

- `CadenceArc` contains reusable runtime types, resolver logic, and framework-owned tests.
- `CadenceArcSandbox` contains the host project, test assets, test Gameplay Tags, and local development tooling.

The Sandbox records a specific CadenceArc commit through its submodule pointer.

## Current Status

Phase 1 established deterministic graph resolution:

```text
InputTag -> graph transition -> ActionTag
```

Phase 2 replaced immediate state mutation with an asynchronous action handshake:

```text
InputTag
  -> ActionRequest
  -> external acceptance or rejection
  -> Started commits the target action
  -> Completed / Cancelled / Interrupted
```

Phase 3 added an externally controlled, RequestId-protected input window and a Last Input Wins single-slot buffer:

```text
Executing action
  -> open buffer window
  -> cache the latest semantic input
  -> action completes
  -> resolve the cached input
  -> emit the next ActionRequest
```

The current `0.3.0-alpha` implementation has 15 passing Unreal Automation Tests. Buffer consumption remains part of the execution handshake, while animation, GAS, collision, and damage stay outside the framework core.

## Getting Started

Clone the Sandbox together with the plugin:

```powershell
git clone --recurse-submodules https://github.com/1zumiii/CadenceArcSandbox.git
```

If the repository was cloned without submodules:

```powershell
git submodule update --init --recursive
```

Generate project files if required, build the `CadenceArcSandboxEditor` target in the Development Editor configuration, and open `CadenceArcSandbox.uproject` with Unreal Engine 5.7.

Before modifying the plugin, confirm that its submodule is on a branch rather than a detached commit:

```powershell
git -C Plugins/CadenceArc switch master
git -C Plugins/CadenceArc pull --ff-only
```

## Running Tests

Close Unreal Editor and run this command from the Sandbox root:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCadenceArcTests.ps1
```

The script:

- reads the project's `EngineAssociation`;
- locates the corresponding Unreal Engine installation from the Windows registry;
- performs a cold `CadenceArcSandboxEditor` build;
- runs every test matching `CadenceArc` with `-culture=en`;
- prints a concise result summary;
- returns the underlying build or test exit code.

Useful optional arguments:

```powershell
# Run a narrower test filter.
.\Scripts\RunCadenceArcTests.ps1 -Filter "CadenceArc.Resolver.Handshake"

# Reuse an already-built editor binary.
.\Scripts\RunCadenceArcTests.ps1 -SkipBuild

# Override engine discovery.
.\Scripts\RunCadenceArcTests.ps1 -EngineRoot "E:\Games\UE_5.7"
```

The full Unreal log is written to:

```text
Saved/Logs/CadenceArcSandbox.log
```

### Rider External Tool

Rider's Unreal test runner may mark successful localized test output as aborted. A reliable local shortcut can be configured under `Settings -> Tools -> External Tools`:

```text
Name:              Run CadenceArc Tests
Program:           C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe
Arguments:         -NoLogo -NoProfile -ExecutionPolicy Bypass -File "<absolute sandbox path>\Scripts\RunCadenceArcTests.ps1"
Working directory: <absolute sandbox path>
```

The External Tool definition is machine-specific and should not be committed. The PowerShell runner itself is part of this repository.

## Development Workflow

When both repositories change, use this order:

1. Commit and push `Plugins/CadenceArc`.
2. Return to the Sandbox root.
3. Commit the updated `Plugins/CadenceArc` submodule pointer and any Sandbox changes.
4. Push `CadenceArcSandbox`.

This prevents the Sandbox from referencing a plugin commit that other clones cannot fetch.

## Repository Boundary

Sandbox-specific maps, input tags, and demonstration assets belong here. Reusable graph types, resolver behavior, validation logic, and framework-owned tests belong in the CadenceArc plugin repository.

The following remain outside the core framework:

- Gameplay Ability and montage execution;
- character, weapon, collision, and damage systems;
- WarriorRPG-specific integration code.

## Requirements

- Unreal Engine 5.7
- A supported Unreal Engine C++ toolchain
- Git
- Git LFS
