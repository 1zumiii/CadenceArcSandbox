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

- a playable demo that drives CadenceArc with real Enhanced Input and a temporary Timer-based executor, including press/release Hold input;
- test graphs, Gameplay Tags, and Blueprint test assets for the plugin's public API;
- a PowerShell test runner for the plugin's automation suite.

Framework features, current status, and API contracts are documented in the [plugin README](Plugins/CadenceArc/README.en.md).

## Demo and Time Contract

The startup and game map is `Content/Demo/L_CadenceArcDemo`. `ACadenceArcDemoCharacter` maps Enhanced Input actions to semantic Gameplay Tags through `UCadenceArcInputConfig`. Each action binds `Started`, `Completed`, and `Canceled`, carrying its tag and `InputMode`, and forwards them to the character's `UCadenceArcComponent` as press, release, and cancel, together with a `Dir.Forward` context tag while W is held. A tag mapped by more than one action is skipped with a warning, because the component pairs presses by tag.

`InputMode` selects how a key reaches the resolver: `PressOnly` submits on press, and `HoldRelease` requests a hold qualification on press and settles on release or automatic release. The node you are on must have `Released` transitions for every `HoldRelease` tag; otherwise the press is rejected with `NoMatchingTransition` and nothing happens.

The character's `UCadenceArcComponent` (from the plugin) owns the resolver and holds the combo graph. It stamps every call with World game time, advances hold time every tick, pairs presses and releases, and broadcasts every action request through `OnActionRequested`. `UCadenceArcDemoExecutorComponent` only executes: it subscribes to `OnActionRequested`, simulates each action with timers, and reports start, buffer window, and completion back to the component. Timers stay in the Sandbox; the resolver itself never reads engine time. `EndPlay` cancels tracked inputs without synthesizing releases.

Demo assets:

- `DA_CadenceArcInputConfig` -- both keys `PressOnly`;
- `DA_CadenceArcInputConfigHeavyHoldRelease` -- Light `PressOnly`, Heavy `HoldRelease`;
- `DA_TestComboGraph` -- the original press-only graph;
- `DA_TestComboGraphAutoRelease` and `DA_TestComboGraphMaxRelease` -- Hold graphs with a charge start of 0.2 s, a full charge of 0.8 s, and zero or positive maximum hold;
- `DA_TestComboGraphCombo` -- a realistic Light/Heavy combo with shared finishers, charge tiers, and a loop back to the first skill;
- `DA_TestComboGraphTree` -- a wide branching tree (27 nodes, five levels, no merges) for checking the debugger layout;
- `DA_TestComboGraphStress` -- a dense graph for layout stress testing.

The character blueprint selects the input config and graph. `MaxBufferedInputAgeSeconds = 0` disables expiry; a positive value limits the age of the buffered input at completion.

Every resolver call and its outcome is visible in the editor's **Arc Debugger** and **Arc History** tabs (**Tools > Debug**); see the [plugin's debugger guide](https://github.com/1zumiii/CadenceArc/blob/master/Docs/Debugger.md) for screenshots. The demo therefore prints only problems the resolver cannot see, in red on screen: an invalid executor timing configuration, a missing resolver or World, and a failed resolver initialization (such a resolver never appears in the debugger). Handshake failures, debug-scenario rejections, and failed time advances are written to `LogCadenceArcDemo` as warnings only (Output Log and `Saved/Logs/CadenceArcSandbox.log`).

`Content/Demo/BP_CadenceArcBPTest` and `L_CadenceArcBPTest` exercise the plugin's Blueprint API with their own resolver, independently of the C++ demo executor.

## Manual Checks

- **Asset validation:** run Unreal's data validation on a valid combo graph and on a separate, intentionally broken copy. Errors should identify the invalid configuration; the valid graph should pass.
- **PIE smoke test:** confirm real input, window handling, combo continuation, and readable log output.
- **Hold smoke test:** with the Heavy `HoldRelease` config, a short press gives the tap tier and a full charge gives the charged tier; with zero maximum hold the charged attack fires on its own and the later physical release does nothing.
- **Debugger smoke test:** open Arc Debugger and Arc History, select the PIE resolver, and play a combo. The committed node, candidate, preparatory edges, and history rows should follow the input; a rejected input should appear as a red history row with its reason.

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
