# pier-probe

Walks the whole Pier API table and reports, slot by slot, whether the host provides it
and what it answers. It reads only and changes no server state, so it is safe on a live
server with players on it.

It is also the smallest honest test of the contract: it includes one header, links
nothing, and needs no build system. If that stopped being true, the ABI would have grown
a dependency it does not admit to.

The one thing that is easy to get wrong is the entry point. Write `PIER_MAIN_EXPORT` in
front of `pier_main`, not a bare `extern "C"`: a Windows DLL exports nothing unless it is
asked to, and a mod that only declares the symbol builds cleanly and is then refused at
load with "does not export pier_main". An ELF build exports it by default, so testing on
Linux does not catch this.

## Build

You need a compiler that produces an MSVC-ABI x64 DLL. Nothing else.

**MSVC.** From a Visual Studio Developer Command Prompt:

```
build-msvc.bat
```

**clang.** From the same prompt, with LLVM's `bin` on `PATH`:

```
build-clang.bat
```

Note that this is `clang-cl` and not `clang++`. A BDS mod has to be MSVC-ABI, and only
the `clang-cl` driver produces that on Windows.

**By hand**, if you would rather see the whole thing:

```
cl /std:c++20 /EHsc /utf-8 /LD /I include /I ..\..\packages\pier-abi\include ^
   src\*.cpp /Fe:pier_probe.dll
```

**xmake**, if your own mod already uses it: `xmake -P .`. The `-P .` is not optional —
without it xmake finds the Pier project above this directory and builds Pier instead.

## Install

Copy `pier_probe.dll` and `manifest.json` into `plugins/pier-probe/` on the server. Pier
itself has to be installed and enabled; this mod is a consumer of it.

## Read the output

The report goes to the server log at enable time, after the level is up. Registration
time would be the wrong moment: the world is not there yet and every world slot would
report failure for a reason that has nothing to do with the host.

Four verdicts, kept apart because each asks something different of the reader:

| | meaning | what to do |
|---|---|---|
| `ABSENT / past struct_size` | the host is older than this mod and its table ends before the slot | use a matching pair |
| `ABSENT / no such capability` | the slot is NULL: the host was built without the package that fills it | check the build target; `pier-dimensions` and `pier-lane` are droppable by contract §1 rule 4, and a client build fills a different set |
| `REFUSED` | the slot was called, returned, and reported failure | on BDS 1.26.32 this is the documented answer for a known set; see "Capabilities that now report failure" in CHANGELOG.md |
| `THREW` | the call did not return normally | a defect, in the host or here. The only verdict that is always wrong |

The census section walks every slot in the contract without calling any of them, which
answers the question a mod author has when a capability does nothing: is the slot
missing, or present and refusing? Those two have different fixes and the log line for
each says which one it is.

## What it does not cover

Only the slots that change nothing. Writing slots need a player, a loaded world and a
place to put the damage, so they belong behind an explicit command rather than in a
report that runs at startup.
