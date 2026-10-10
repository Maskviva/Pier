# Verifying that LegacyMoney really became optional

After editing `xmake.lua`, do not stop at the build succeeding. With `/DELAYLOAD` written
into the wrong channel the build succeeds as usual, the artifact still carries a hard
dependency, and the symptom is identical to not having changed anything. This document
gives the ways to see the answer directly, ordered from most to least reliable.

---

## 1. Read the import table (the most direct, one command)

In a VS developer command prompt:

```
dumpbin /imports pier.dll | findstr /i legacymoney
```

Done right, `LegacyMoney.dll` appears in the *delay load imports* section:

```
    Section contains the following delay load imports:

      LegacyMoney.dll
                40C0A0 Import Address Table
                ...
```

Done wrong, it appears in the ordinary imports section, with no "delay load" before it.

For a yes-or-no answer only:

```
dumpbin /dependents pier.dll | findstr /i legacymoney
```

Done right, this command should print nothing, since `/dependents` lists hard dependencies
only. Any output means the flag did not take effect.

---

## 2. Read the link command line (confirming xmake really passed the flag down)

```
xmake f -c -y
xmake -v 2>&1 | findstr /i DELAYLOAD
```

`-v` prints the real link.exe command line. Not seeing `/DELAYLOAD:LegacyMoney.dll` means
xmake did not take that line, and the problem is then in the build script and not in the
linker.

This step separates the flag not being passed down from the flag being passed down and not
working. Treating those two as one leads to guessing between them.

---

## 3. Final acceptance (move LegacyMoney away and start)

1. Move the whole `plugins/LegacyMoney/` directory elsewhere
2. Start BDS

Expected: pier loads normally and the startup log carries a line saying that no usable
LLMoney backend was found, that every money entry point is inert for this session with
reads returning 0 and writes failing, and asking whether LegacyMoney is installed and
enabled.

Afterwards every economy entry point returns its failure value, with `get_money` returning
-1 and a write returning false, so the features of a consumer that depend on the economy
close themselves and everything else is unaffected.

What must not appear: `0x7E The specified module could not be found`.

---

## What to check if step 1 already looks wrong

In order of likelihood:

1. `xmake f -c` was not run again. xmake caches its configuration and a bare `xmake` does
   not regenerate the link command line. Run `xmake f -c -y` and then `xmake`.

2. `@levibuildscript/linkrule` overrode the flags. That rule takes over the link, and it
   may assemble the command line itself rather than reading the shflags of the target. The
   `-v` of step 2 shows this: present in shflags and absent from the command line means the
   rule consumed it. The fix is then adding the flag at the entry point the rule reads, or
   editing it inside `after_link`.

3. The dependency came in from another target. `pier-api` also carries
   `add_packages("legacymoney")`. It is of object kind and does not link itself, but if
   levibuildscript aggregates the package dependencies of subtargets into the final link,
   the flag belongs at that aggregating layer.

4. `delayimp` was not linked. The linker then reports `__delayLoadHelper2` as undefined,
   which is an explicit error rather than a silent one, so a successful build rules this
   out.

Item 2 is the most likely one, and verifying it needs xmake and levibuildscript. The
`xmake -v` of step 2 rules it in or out in one run.
