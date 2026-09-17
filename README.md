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

## What the Sandbox Provides

- a playable demo that drives CadenceArc with real Enhanced Input and a temporary Timer-based executor;
- test graphs, Gameplay Tags, and Blueprint test assets for the plugin's public API;
- a PowerShell test runner for the plugin's automation suite.

Framework features, current status, and API contracts are documented in the [plugin README](Plugins/CadenceArc/README.md).

## Demo and Time Contract

The startup and game map is `Content/Demo/L_CadenceArcDemo`. `ACadenceArcDemoCharacter` maps Enhanced Input actions to semantic Gameplay Tags through `UCadenceArcInputConfig` and forwards them to `UCadenceArcDemoExecutorComponent`.

The Demo Executor stamps each `FCadenceArcInputEvent` with `GetWorld()->GetTimeSeconds()` and passes the same World game time to `NotifyActionCompleted`. Time is measured in seconds and follows pause and time dilation. Timers and all World access stay in the Sandbox; the plugin runtime never reads engine time.

`DA_TestComboGraph` supplies the demo graph. `MaxBufferedInputAgeSeconds = 0` disables expiry; a positive value limits the age of the buffered input at completion.

`LogCadenceArcDemo` writes to the Output Log and `Saved/Logs/CadenceArcSandbox.log` alongside on-screen messages. Completion messages report the handshake result and, only when it succeeds, the buffer-consumption result.

`Content/Demo/BP_CadenceArcBPTest` and `L_CadenceArcBPTest` exercise the plugin's Blueprint API with their own resolver, independently of the C++ demo executor.

## Manual Checks

- **Asset validation:** run Unreal's data validation on a valid combo graph and on a separate, intentionally broken copy. Errors should identify the invalid configuration; the valid graph should pass.
- **PIE smoke test:** confirm real input, window handling, combo continuation, and readable log output.

Exact expiry boundaries, invalid time, and Last Input Wins are covered by automated tests; manual subsecond timing is not required. For an easy visual expiry demo, set action duration to 6 s, the buffer window to 1-5 s, and MaxAge to 2 s: an input early in the window expires, one near its end does not.

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
