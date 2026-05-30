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

- mips64le is **fully intact** in jdk17u and jdk11u HEAD -- it was never removed. Last
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

The primary investigation report. Mostly static -- documents what was found in the
2026-05-30 investigation. Covers:
- Tag landscape and what `jdk17_35` means (or doesn't)
- Inventory of mips64le files across all modules in jdk17u
- Architecture capabilities table (which GCs, which JIT tiers, Panama, Loom, JVMCI)
- Loongson commit timeline for both architectures
- jdk25u LoongArch inventory with size comparisons vs mips64
- Table of 45 new files a mips64 jdk25u port would need to add
- jdk11u cross-check (68 files, older/smaller than jdk17u)
- Synthesis: is the patch set intact? how similar are the ports? what's the porting path?
  was loongarch derived from mips?

### `porting-notes.md`

Living document -- update this as porting or archaeology work progresses. The reference
file for any LLM doing hands-on work. Covers:
- Complete file mapping: every jdk17u mips file → its jdk25u loongarch equivalent,
  including renamed files, files upgraded from header-only, and files removed upstream
- Critical API changes between JDK 17 and JDK 25 (CP cache API replacement, signature
  changes, Loom entry points)
- New files needed for a jdk25u mips port, organized by phase
- Build system wiring (platform.m4, jvm-features.m4 excerpts, configure command)
- C1 file reference list with which files were removed in jdk25u
- Bash recipes for finding Loongson commits, diffing file pairs, and verifying open questions
- Background: tag mystery, author notes, jdk11u vs jdk17u differences

## Suggested Next Steps

1. **Produce the full loongarch delta** -- diff jdk17u loongarch vs jdk25u loongarch for
   each key file to get the exact API change guide for the forward port.
   See the "Diff a file between the two eras" command in `porting-notes.md`.

2. **Verify icBuffer / depChecker removal** -- confirm neither has a jdk25u equivalent:
   `git -C jdk25u ls-tree -r HEAD src/hotspot/cpu/loongarch/ | grep -i 'icbuf\|depcheck'`

3. **Verify SA removal** -- confirm `jdk.hotspot.agent` is gone in jdk25u:
   `git -C jdk25u ls-tree --name-only -r HEAD | grep hotspot.agent`

4. **Note on `jdk17_35`** -- the origin of this version string is unclear; it does not
   correspond to any tag in these repos or any known public fork. Treat it as unresolvable
   and use jdk17u `master-ls` HEAD as the mips64le reference instead.
