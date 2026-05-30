# loongson-java workspace

Working directory for archaeology of Loongson's OpenJDK forks, focused on understanding
mips64le support and what it would take to forward-port it to JDK 25.

## Repos

Three shallow-cloned-then-unshallowed forks of OpenJDK from `github.com/loongson/`:

| Directory | Branch | Architecture coverage |
| --- | --- | --- |
| `jdk11u/` | `master-ls` | mips64le + LoongArch64 |
| `jdk17u/` | `master-ls` | mips64le + LoongArch64 (primary reference) |
| `jdk25u/` | `loongarch-port` | LoongArch64 only |

All three were fully unshallowed on 2026-05-30. History is complete. No uncommitted
changes; do not create commits here.

## Key Facts

- mips64le is **fully intact** in jdk17u and jdk11u HEAD — it was never removed. Last
  touched 2026-01-29 in jdk17u.
- jdk25u never had mips64le. Its initial LoongArch commit (`526ad75a343f`, 2022-08-11)
  predates jdk17u/jdk11u by six months.
- The mips64le and LoongArch backends share the same structural skeleton, function names,
  and Loongson-internal helper patterns (`beq_far`, `bne_far`, `b_far`, `atomic_inc32`).
- mips64le has no C1 JIT (only template interpreter + C2). LoongArch has full C1.
- The tag `jdk17_35` mentioned in early context does not exist in any of these repos.
  It likely belongs to Huawei's Bisheng JDK.

## Files in This Directory

### `mips64le-archaeology.md`

The primary investigation report. Covers:
- Tag landscape and what `jdk17_35` means (or doesn't)
- Complete inventory of mips64le files across all modules in jdk17u
- Architecture capabilities table (which GCs, which JIT tiers, Panama, Loom, JVMCI)
- Full Loongson commit timeline for both architectures
- jdk25u LoongArch inventory with size comparisons vs mips64
- Table of 45 new files that a mips64 jdk25u port would need to add
- jdk11u cross-check (68 files, older/smaller than jdk17u)
- Synthesis answers: is the patch set intact? how similar are the two ports? what's
  the shortest porting path? was loongarch derived from mips?

### `archaeology-companion.md`

LLM-oriented companion with raw data and verification commands. Covers:
- Why `jdk17_35` is missing (and how to check Bisheng JDK)
- Author identity (aoqi / loongson-jvm, their upstream commit history)
- Complete file lists for jdk17u mips64 (cpu + os_cpu + SA agent)
- Full Loongson commit log with hashes and dates
- Autoconf code excerpts (platform.m4, jvm-features.m4)
- C1 file list for jdk17u loongarch (including which files were removed in jdk25u)
- Function-level structural comparison (`InterpreterMacroAssembler`, `MacroAssembler`)
- The CP cache API replacement (`load_field_entry` etc.) as the biggest single API change
- Build commands for a minimal cross-compiled mips64el port
- How to produce the jdk17u→jdk25u loongarch delta as a porting guide
- jdk11u vs jdk17u diff (9 files added between JDK 11 and JDK 17 in the mips port)

## Suggested Next Steps

1. **Verify SA removal assumption** — confirm `jdk.hotspot.agent` is truly gone in jdk25u:
   `git -C jdk25u ls-tree --name-only -r HEAD | grep hotspot.agent`

2. **Produce the loongarch delta** — diff jdk17u loongarch vs jdk25u loongarch to get the
   exact API change guide for the forward port:
   `git -C jdk17u show HEAD:src/hotspot/cpu/loongarch/templateInterpreterGenerator_loongarch.cpp > /tmp/tig_17.cpp`
   `git -C jdk25u show HEAD:src/hotspot/cpu/loongarch/templateInterpreterGenerator_loongarch.cpp > /tmp/tig_25.cpp`
   `diff /tmp/tig_17.cpp /tmp/tig_25.cpp | head -100`

3. **Confirm `c1_LinearScan` status** — verify whether `c1_LinearScan_loongarch_64.cpp`
   still exists in jdk25u:
   `git -C jdk25u ls-tree HEAD src/hotspot/cpu/loongarch/ | grep LinearScan`

4. **Explore Bisheng JDK** — if you need the `jdk17_35` reference build:
   `git ls-remote https://gitee.com/openeuler/bishengjdk-17 'refs/tags/jdk17_35'`
