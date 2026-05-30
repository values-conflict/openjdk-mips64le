# loongson-java workspace

Working directory for archaeology of Loongson's OpenJDK forks, focused on understanding
mips64le support and what it would take to forward-port it to JDK 25.

## Conventions

This workspace is Tianon-authored -- apply his formatting and style conventions to files
written directly here (`.md` docs, scripts, etc.).

OpenJDK source files inside any of the JDK repo directories (including
`tianon-jdk25u-mips64/`) follow OpenJDK conventions, not Tianon's -- preserve their
existing style when editing files there.

## Repos

Read-only reference forks of OpenJDK from `github.com/loongson/` (gitignored):

| Directory | Remote | Branch | Architecture coverage |
| --- | --- | --- | --- |
| `jdk17u/` | `https://github.com/loongson/jdk17u.git` | `master-ls` | mips64le + LoongArch64 (primary reference) |
| `jdk25u/` | `https://github.com/loongson/jdk25u.git` | `loongarch-port` | LoongArch64 only |
| `jdk11u/` | `https://github.com/loongson/jdk11u.git` | `master-ls` | mips64le + LoongArch64 -- *not checked out* |

`jdk17u/` and `jdk25u/` were fully unshallowed on 2026-05-30. Both use this workspace's
object store (`../../../.git/objects` in each repo's `.git/objects/info/alternates`) --
all upstream JDK history lives in the workspace `.git`, fetched from the `jdk25u` branch.
No uncommitted changes in the reference repos; do not create commits there.

`jdk11u/` was unshallowed on 2026-05-30 but has since been removed -- it covered the same
mips64le port as jdk17u but was older (68 files vs jdk17u's larger set) and is not needed
for porting work. Rehydrate the same way as the other reference repos if a cross-check is
ever needed.

`tianon-jdk25u-mips64/` is a tracked git submodule on the `jdk25u` branch of
`https://github.com/values-conflict/openjdk-mips64le.git` (the same repo as this
workspace's `main` branch; push via SSH). It also uses `../../../.git/objects` as its
alternate.

To rehydrate: clone this repo first, then run `git fetch --no-tags origin jdk25u` to
populate the workspace object store with all JDK history. Initialize the submodule via
`git submodule update --init --reference .`, overwrite the absolute path git writes in
`tianon-jdk25u-mips64/.git/objects/info/alternates` with `../../../.git/objects`, and
run `git repack -a -d -l` in it. For each reference repo, clone with `--reference .`,
apply the same alternates fix (`../../../.git/objects`), and repack.

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
- tag landscape and what `jdk17_35` means (or doesn't)
- inventory of mips64le files across all modules in jdk17u
- architecture capabilities table (which GCs, which JIT tiers, Panama, Loom, JVMCI)
- Loongson commit timeline for both architectures
- jdk25u LoongArch inventory with size comparisons vs mips64
- table of 45 new files a mips64 jdk25u port would need to add
- jdk11u cross-check (68 files, older/smaller than jdk17u)
- synthesis: is the patch set intact? how similar are the ports? what's the porting path?
  was loongarch derived from mips?

### `porting-notes.md`

Living document -- update this as porting or archaeology work progresses. The reference
file for any LLM doing hands-on work. Covers:
- complete file mapping: every jdk17u mips file → its jdk25u loongarch equivalent,
  including renamed files, files upgraded from header-only, and files removed upstream
- critical API changes between JDK 17 and JDK 25 (CP cache API replacement, signature
  changes, Loom entry points)
- new files needed for a jdk25u mips port, organized by phase
- build system wiring (platform.m4, jvm-features.m4 excerpts, configure command)
- C1 file reference list with which files were removed in jdk25u
- Bash recipes for finding Loongson commits, diffing file pairs, and verifying open questions
- background: tag mystery, author notes, jdk11u vs jdk17u differences

## Suggested Next Steps

1. **produce the full loongarch delta** -- diff jdk17u loongarch vs jdk25u loongarch for
   each key file to get the exact API change guide for the forward port.
   See the "Diff a file between the two eras" command in `porting-notes.md`.

2. **verify icBuffer / depChecker removal** -- confirm neither has a jdk25u equivalent:
   `git -C jdk25u ls-tree -r HEAD src/hotspot/cpu/loongarch/ | grep -i 'icbuf\|depcheck'`

3. **verify SA removal** -- confirm `jdk.hotspot.agent` is gone in jdk25u:
   `git -C jdk25u ls-tree --name-only -r HEAD | grep hotspot.agent`

4. **note on `jdk17_35`** -- the origin of this version string is unclear; it does not
   correspond to any tag in these repos or any known public fork. Treat it as unresolvable
   and use jdk17u `master-ls` HEAD as the mips64le reference instead.
