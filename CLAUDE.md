# loongson-java workspace

Working directory for archaeology of Loongson's OpenJDK forks, focused on understanding
mips64le support and what it would take to forward-port it to JDK 25.

## End Goal

The target workload is the Jenkins remoting agent -- a pure-Java process that runs headless.  This means:

- `--enable-headless-only` is correct for all build phases; no AWT, Swing, sound, X11, or fontconfig needed
- QEMU user-mode is sufficient for testing (no GUI or hardware I/O required)
- the final deliverable is a server JVM, not a desktop JDK

## Target Hardware

The Loongson MIPS64el machine that will run the Jenkins agent:

- OS: Debian GNU/Linux 12 (Bookworm)
- glibc: 2.36 (Debian GLIBC 2.36-9+deb12u14)
- Kernel: `4.19.0-12-loongson-3` -- Loongson-provided, SMP, 2021-06-19
- CPU: `Loongson-3 V0.13  FPU V0.1`

**Build implication:** the Dockerfile and cross-compilation environment must target glibc 2.36.
Trixie's cross-compiler sysroot (glibc 2.40) causes `_GNU_SOURCE` to activate
`_ISOC23_SOURCE`, which redirects `sscanf` to `__isoc23_sscanf@GLIBC_2.38` -- absent on
the target.  The Dockerfile base image must be `debian:bookworm-slim` to match.

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

### `build-jdk.sh`

**Use this script to build any JDK source tree for mips64el -- do not reconstruct the
configure command manually.**  It auto-detects the correct boot JDK by reading
`make/conf/version-numbers.conf` from the source tree and matching against the JDKs
installed at `/opt/java/jdk{N}`.

The script lives in the workspace at `/home/user/loongson-java/build-jdk.sh`.  The build
environment (cross-compiler, `/opt/java/jdk17`, `/opt/java/jdk25`, `/opt/alsa-stub/`,
uname shim) is already present in this container.

```bash
./build-jdk.sh jdk17u
./build-jdk.sh tianon-jdk25u-mips64
```

**Outside the container** (e.g. on the Trixie host or a fresh machine), `/opt/java/` and
`/opt/alsa-stub/` won't exist -- use `--with-boot-jdk=` and `--with-alsa-lib=` explicitly
per the configure command in `porting-notes.md`, and prepend `/tmp/fake-bin` to PATH for
the uname shim.

Output: `$src/build/linux-mips64el-server-release/images/jdk/`
Build log: `$src/build/linux-mips64el-server-release/build.log`

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

**Phase 0 is complete (2026-05-30).**  jdk17u mips64el builds successfully with GCC 12 on
Debian Bookworm and `java --version` runs correctly on the real Loongson-3 target machine.
Build host must be `debian:bookworm-slim` -- Trixie's glibc 2.40 cross-compiler sysroot
produces `__isoc23_sscanf@GLIBC_2.38` references that are absent on the target's glibc 2.36.
See `porting-notes.md` Phase 0 section for the full configure command and all workarounds.

1. **obtain QEMU full system or real hardware** -- QEMU user-mode cannot complete `java
   -version` due to missing kernel unaligned-access emulation.  A Debian mips64el QEMU
   system image or a real Loongson 3A/3B machine is the next testing blocker.

2. **produce the full loongarch delta** -- diff jdk17u loongarch vs jdk25u loongarch for
   each key file to get the exact API change guide for the forward port.
   See the "Diff a file between the two eras" command in `porting-notes.md`.

3. **verify icBuffer / depChecker removal** -- confirm neither has a jdk25u equivalent:
   `git -C jdk25u ls-tree -r HEAD src/hotspot/cpu/loongarch/ | grep -i 'icbuf\|depcheck'`

4. **verify SA removal** -- confirm `jdk.hotspot.agent` is gone in jdk25u:
   `git -C jdk25u ls-tree --name-only -r HEAD | grep hotspot.agent`

5. **note on `jdk17_35`** -- the origin of this version string is unclear; it does not
   correspond to any tag in these repos or any known public fork. Treat it as unresolvable
   and use jdk17u `master-ls` HEAD as the mips64le reference instead.
