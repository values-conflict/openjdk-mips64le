# porting-notes.md

Living notes for any LLM doing porting or archaeology work on these repos.
Companion to `mips64le-archaeology.md`, which has the high-level synthesis and history.

This file answers the questions you hit while actually doing the work: which file maps to
which, what changed between JDK 17 and JDK 25, what needs to be written from scratch,
and what commands to run.

---

## File Mapping: mips64le → LoongArch64

The mips and loongarch backends are structurally parallel. Every mips file has a loongarch
counterpart with the same role. The table below covers all 77 cpu files and 15 os_cpu files.

Notation: `→` means direct rename with the same structure; special cases are called out.

### `src/hotspot/cpu/mips/` → `src/hotspot/cpu/loongarch/`

**Core JIT and interpreter (largest files):**

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `mips.ad` | `loongarch.ad` | |
| `mips_64.ad` | `loongarch_64.ad` | 334 KB → 525 KB; 57% growth from intrinsics/vectors |
| `assembler_mips.{cpp,hpp,inline.hpp}` | `assembler_loongarch.{cpp,hpp,inline.hpp}` | assembler_mips.hpp is 85 KB; assembler_loongarch.hpp is 286 KB |
| `macroAssembler_mips.{cpp,hpp,inline.hpp}` | `macroAssembler_loongarch.{cpp,hpp,inline.hpp}` | |
| `interp_masm_mips.hpp` | `interp_masm_loongarch.hpp` | |
| `interp_masm_mips_64.cpp` | `interp_masm_loongarch_64.cpp` | |
| `templateTable_mips.hpp` | `templateTable_loongarch.hpp` | |
| `templateTable_mips_64.cpp` | `templateTable_loongarch_64.cpp` | |
| `templateInterpreterGenerator_mips.cpp` | `templateInterpreterGenerator_loongarch.cpp` | |
| `sharedRuntime_mips_64.cpp` | `sharedRuntime_loongarch_64.cpp` | |
| `stubGenerator_mips_64.cpp` | `stubGenerator_loongarch_64.cpp` | |
| `c2_MacroAssembler_mips.{cpp,hpp}` | `c2_MacroAssembler_loongarch.{cpp,hpp}` | |

**Runtime:**

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `nativeInst_mips.{cpp,hpp}` | `nativeInst_loongarch.{cpp,hpp}` | |
| `methodHandles_mips.{cpp,hpp}` | `methodHandles_loongarch.{cpp,hpp}` | |
| `frame_mips.{cpp,hpp,inline.hpp}` | `frame_loongarch.{cpp,hpp,inline.hpp}` | |
| `interpreterRT_mips.hpp` | `interpreterRT_loongarch.hpp` | |
| `interpreterRT_mips_64.cpp` | `interpreterRT_loongarch_64.cpp` | |
| `jniFastGetField_mips_64.cpp` | `jniFastGetField_loongarch_64.cpp` | |
| `runtime_mips_64.cpp` | `runtime_loongarch_64.cpp` | |
| `vtableStubs_mips_64.cpp` | `vtableStubs_loongarch_64.cpp` | |
| `compiledIC_mips.cpp` | `compiledIC_loongarch.cpp` | |
| `relocInfo_mips.{cpp,hpp}` | `relocInfo_loongarch.{cpp,hpp}` | |
| `abstractInterpreter_mips.cpp` | `abstractInterpreter_loongarch.cpp` | |
| `icache_mips.{cpp,hpp}` | `icache_loongarch.{cpp,hpp}` | |

**Registers and infrastructure:**

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `register_mips.{cpp,hpp}` + `register_definitions_mips.cpp` | `register_loongarch.{cpp,hpp}` | **`register_definitions` merged in** |
| `vmreg_mips.{cpp,hpp,inline.hpp}` | `vmreg_loongarch.{cpp,hpp,inline.hpp}` | |
| `registerMap_mips.hpp` | `registerMap_loongarch.hpp` | |
| `vm_version_mips.{cpp,hpp}` | `vm_version_loongarch.{cpp,hpp}` | |
| `vmStructs_mips.hpp` | `vmStructs_loongarch.hpp` | |

**C2 JIT support:**

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `c2_globals_mips.hpp` | `c2_globals_loongarch.hpp` | |
| `c2_init_mips.cpp` | `c2_init_loongarch.cpp` | |
| `matcher_mips.hpp` | `matcher_loongarch.hpp` | |

**Small headers:**

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `globalDefinitions_mips.hpp` | `globalDefinitions_loongarch.hpp` | |
| `globals_mips.hpp` | `globals_loongarch.hpp` | |
| `bytes_mips.hpp` | `bytes_loongarch.hpp` | |
| `javaFrameAnchor_mips.hpp` | `javaFrameAnchor_loongarch.hpp` | |
| `jniTypes_mips.hpp` | `jniTypes_loongarch.hpp` | |
| `disassembler_mips.hpp` | `disassembler_loongarch.hpp` | |

**Files upgraded from header-only to having a `.cpp`:**

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `codeBuffer_mips.hpp` | `codeBuffer_loongarch.{cpp,hpp}` | **added `.cpp`** |
| `copy_mips.hpp` | `copy_loongarch.{cpp,hpp}` | **added `.cpp`** |
| `stubRoutines_mips.hpp` | `stubRoutines_loongarch.{cpp,hpp}` | **added `.cpp`** |

**Renamed:**

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `foreign_globals_mips.{cpp,hpp}` | `foreignGlobals_loongarch.{cpp,hpp}` | **snake_case → camelCase** |

**Removed upstream (no jdk25u equivalent):**

| jdk17u mips file | Notes |
| --- | --- |
| `icBuffer_mips.cpp` | removed in JDK 21+ (inline cache buffer infrastructure removed) |
| `depChecker_mips.{cpp,hpp}` | removed upstream; no loongarch equivalent in any repo |

**GC:**

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `gc/g1/g1BarrierSetAssembler_mips.{cpp,hpp}` | `gc/g1/g1BarrierSetAssembler_loongarch.{cpp,hpp}` | |
| `gc/g1/g1Globals_mips.hpp` | `gc/g1/g1Globals_loongarch.hpp` | |
| `gc/shared/barrierSetAssembler_mips.{cpp,hpp}` | `gc/shared/barrierSetAssembler_loongarch.{cpp,hpp}` | |
| `gc/shared/barrierSetNMethod_mips.cpp` | `gc/shared/barrierSetNMethod_loongarch.cpp` | |
| `gc/shared/cardTableBarrierSetAssembler_mips.{cpp,hpp}` | `gc/shared/cardTableBarrierSetAssembler_loongarch.{cpp,hpp}` | |
| `gc/shared/modRefBarrierSetAssembler_mips.{cpp,hpp}` | `gc/shared/modRefBarrierSetAssembler_loongarch.{cpp,hpp}` | |

### `src/hotspot/os_cpu/linux_mips/` → `src/hotspot/os_cpu/linux_loongarch/`

| jdk17u mips file | jdk25u loongarch equivalent | Notes |
| --- | --- | --- |
| `assembler_linux_mips.cpp` | `assembler_linux_loongarch.cpp` | |
| `atomic_linux_mips.hpp` | `atomic_linux_loongarch.hpp` | |
| `copy_linux_mips.inline.hpp` | `copy_linux_loongarch.inline.hpp` | |
| `globals_linux_mips.hpp` | `globals_linux_loongarch.hpp` | |
| `orderAccess_linux_mips.hpp` | `orderAccess_linux_loongarch.hpp` | |
| `os_linux_mips.cpp` + `os_linux_mips.hpp` | `os_linux_loongarch.cpp` + `os_linux_loongarch.inline.hpp` | **`.hpp` became `.inline.hpp`** |
| `prefetch_linux_mips.inline.hpp` | `prefetch_linux_loongarch.inline.hpp` | |
| `safefetch_linux_mips64.S` | `safefetch_linux_loongarch64.S` | |
| `thread_linux_mips.{cpp,hpp}` | `javaThread_linux_loongarch.{cpp,hpp}` | **renamed `thread` → `javaThread`** |
| `vm_version_linux_mips.cpp` | `vm_version_linux_loongarch.cpp` | |
| `bytes_linux_mips.inline.hpp` | *(removed)* | bytes moved to cpu layer or shared |
| `linux_mips.s` | *(removed)* | signal trampoline absorbed into os layer |
| `vmStructs_linux_mips.hpp` | *(removed)* | vmStructs moved to shared in JDK 21+ |

**New in jdk25u os_cpu that mips needs:**

- `gc/x/xSyscall_linux_loongarch.hpp` -- Generational ZGC syscall stubs
- `gc/z/zSyscall_linux_loongarch.hpp` -- ZGC syscall stubs

---

## Critical API Changes (JDK 17 → JDK 25)

### CP cache API replacement -- the most impactful single change

The constant-pool cache API was redesigned between JDK 17 and JDK 25 (`JDK-8302708` and related). Every bytecode that touches the CP cache must be rewritten.

**Old API (jdk17u):**

```
get_cache_and_index_at_bcp
get_cache_and_index_and_bytecode_at_bcp
get_cache_entry_pointer_at_bcp
```

**New API (jdk25u):**

```
load_resolved_indy_entry   ← for invokedynamic
load_field_entry           ← for getfield/putfield/getstatic/putstatic
load_method_entry          ← for invokevirtual/invokespecial/invokestatic/invokeinterface
```

The affected file is primarily `templateTable_mips_64.cpp`. Look at `templateTable_loongarch_64.cpp` in jdk25u for the pattern to follow.

### `get_4_byte_integer_at_bcp` signature change

- jdk17u mips: `get_4_byte_integer_at_bcp(Register reg, Register tmp, int offset)`
- jdk25u loongarch: `get_4_byte_integer_at_bcp(Register reg, int offset)` -- `tmp` dropped

### Loom entry points (JDK 21+)

New methods added to `InterpreterMacroAssembler` between jdk17u and jdk25u loongarch:
- `call_VM_preemptable` -- preemptable VM call for virtual threads
- `restore_after_resume` -- resume interpreter after continuation yield

New method added to `TemplateInterpreterGenerator`:
- `generate_cont_resume_interpreter_adapter`

### How to produce the full delta

Diff jdk17u loongarch against jdk25u loongarch to get the exact change list. Repeat for each file pair:

```bash
git -C /home/user/loongson-java/jdk17u show HEAD:src/hotspot/cpu/loongarch/templateTable_loongarch_64.cpp \
  > /tmp/tt_17.cpp
git -C /home/user/loongson-java/jdk25u show HEAD:src/hotspot/cpu/loongarch/templateTable_loongarch_64.cpp \
  > /tmp/tt_25.cpp
diff /tmp/tt_17.cpp /tmp/tt_25.cpp
```

Files to prioritize for the delta: `templateInterpreterGenerator_loongarch.cpp`,
`templateTable_loongarch_64.cpp`, `interp_masm_loongarch_64.cpp`,
`sharedRuntime_loongarch_64.cpp`, `macroAssembler_loongarch.cpp`.

---

## New Files Needed for a jdk25u mips64 Port

Files in jdk25u loongarch with no mips equivalent -- need to be written for any complete
mips64 jdk25u port. Phase 1 (interpreter + C2) can disable most of these; they become
relevant in later phases.

| Category | Phase | Key files |
| --- | --- | --- |
| Loom/continuations | 1 (API glue) | `continuationEntry_mips.hpp/.inline.hpp`, `continuationFreezeThaw_mips.inline.hpp`, `continuationHelper_mips.inline.hpp` |
| Stack chunks | 1 (API glue) | `smallRegisterMap_mips.inline.hpp`, `stackChunkFrameStream_mips.inline.hpp`, `stackChunkOop_mips.inline.hpp` |
| `c2_CodeStubs` | 1 | `c2_CodeStubs_mips.cpp` |
| `stubDeclarations` | 1 | `stubDeclarations_mips.hpp` |
| `vmstorage` | 1 | `vmstorage_mips.hpp` |
| C1 JIT | 2 (13 files) | Adapt from jdk17u loongarch C1 (see below) |
| Shenandoah | 3 | 3 files in `gc/shenandoah/` |
| ZGC | 3 | 7 files in `gc/z/` + `gc/x/` |
| G1 JIT rules | 3 | `gc/g1/g1_mips.ad` |
| Panama FFI | 3 | `downcallLinker_mips_64.cpp`, `upcallLinker_mips_64.cpp` |
| JVMCI | optional | `jvmciCodeInstaller_mips.cpp` |
| Crypto intrinsics | optional | `macroAssembler_mips_chacha.cpp`, `macroAssembler_mips_trig.cpp` |

---

## Phase 0 -- Build jdk17u mips64el and Establish Baseline

**Status: build complete 2026-05-30.  QEMU user-mode test reached JVM initialization but
did not complete `java -version` (see below).**

### Environment

- Host: x86-64 Debian 13 "Trixie" (in Docker container inside WSL2)
- Cross-compiler: `gcc-mips64el-linux-gnuabi64` / `g++-mips64el-linux-gnuabi64` 14.2.0 (from Trixie `main`)
- Boot JDK: Eclipse Temurin 17.0.19+10 (downloaded separately; JDK 17 is not in Trixie)
- Machine: Intel Core Ultra 7 165H, 22 cores, 32 GB RAM

### Obstacles and solutions

**WSL detection.**  `config.guess` returns `x86_64-pc-wsl`; OpenJDK maps that to
`windows`, activating Windows-specific path handling that immediately fails.  `--build`
cannot be combined with `--openjdk-target`.  Fix: put a `uname` shim early in `PATH`
that strips `-microsoft-` from `uname -r` output, causing `config.guess` to return
`x86_64-unknown-linux-gnu` instead.

```bash
mkdir -p /tmp/fake-bin
cat > /tmp/fake-bin/uname << 'EOF'
#!/bin/sh
case "$1" in
    -r) /bin/uname -r | sed 's/-microsoft-/-/g' ;;
    *)  exec /bin/uname "$@" ;;
esac
EOF
chmod +x /tmp/fake-bin/uname
# then prepend /tmp/fake-bin to PATH when running configure and make
```

**Cross-compiler triplet mismatch.**  `--openjdk-target=mips64el-linux-gnu` makes configure
look for `mips64el-linux-gnu-gcc`, but Debian's package installs `mips64el-linux-gnuabi64-gcc`.
Fix: use `--openjdk-target=mips64el-linux-gnuabi64`.

**Boot JDK version.**  JDK 17 is absent from Trixie apt; JDK 21 is available but jdk17u
configure rejects it (requires 16 or 17).  Fix: download Temurin 17 JDK tarball and extract
to `~/bin/temurin17`; add `~/bin/temurin17/bin` to PATH so configure auto-detects it.

**mips64el system libraries absent from Trixie.**  Trixie dropped mips64el from main repos
and Debian ports no longer carries it either.  cups, fontconfig, and ALSA headers are all
required by jdk17u even for `--enable-headless-only` builds.  X11 is skipped by headless
mode, but `libawt` still includes X11 headers unconditionally.  Fix for headers: install the
x86-64 dev packages (`libcups2-dev`, `libfontconfig1-dev`, `libasound2-dev`, `libx11-dev`,
`libxrender-dev`, `libxext-dev`, `libxi-dev`, `libxrandr-dev`, `libxtst-dev`) and pass
`--with-cups-include=/usr/include`, `--with-fontconfig-include=/usr/include`,
`--with-alsa-include=/usr/include`, plus `--with-extra-cflags='-I/usr/include'` and
`--with-extra-cxxflags='-I/usr/include'` for X11.  These are header-only uses
(all three libraries are `dlopen`'d at runtime) so mixing x86-64 headers into a mips64el
build is safe.

**ALSA is directly linked.**  Unlike cups and fontconfig, `libjsound.so` links directly
against `-lasound` (not via `dlopen`).  The mips64el `libasound.so` stub must export all
symbols or the linker fails.  Fix: generate a stub from the x86-64 symbol table and compile
it with the cross-compiler:

```bash
nm --dynamic /usr/lib/x86_64-linux-gnu/libasound.so.2.0.0 \
  | awk '$2 == "T" {sub(/@.*/,"",$3); print $3}' \
  | sort --unique \
  > /tmp/alsa-syms.txt
{
  printf '/* mips64el libasound stub -- real library used at runtime */\n'
  while IFS= read -r sym; do
    printf 'void __attribute__((visibility("default"))) %s(void){}\n' "$sym"
  done < /tmp/alsa-syms.txt
} > /tmp/alsa-stub.c
mkdir -p /tmp/alsa-stub-lib
mips64el-linux-gnuabi64-gcc -shared -fPIC \
  -o /tmp/alsa-stub-lib/libasound.so /tmp/alsa-stub.c
# then pass --with-alsa-lib=/tmp/alsa-stub-lib to configure
```

**GCC 14 `-Werror=address`.**  `mips_64.ad` contains patterns like `if (&var == NULL)`
which GCC 14 rejects as always-false address comparisons.  Fix: `--disable-warnings-as-errors`.

### Working configure command

`--with-jvm-variants=server` is the default and can be omitted.  `--disable-hotspot-gtest`
is deprecated/removed in jdk25u and is a no-op in jdk17u; omit it.

```bash
PATH=/tmp/fake-bin:$PATH bash ./configure \
  --openjdk-target=mips64el-linux-gnuabi64 \
  --with-boot-jdk=/opt/java/jdk17 \
  --with-debug-level=release \
  --enable-headless-only \
  --with-freetype=bundled \
  --with-harfbuzz=bundled \
  --with-cups-include=/usr/include \
  --with-fontconfig-include=/usr/include \
  --with-alsa-include=/usr/include \
  --with-alsa-lib=/opt/alsa-stub \
  --disable-warnings-as-errors \
  --with-extra-cflags='-I/usr/include' \
  --with-extra-cxxflags='-I/usr/include' \
  --with-vendor-version-string=Tianon \
  --with-vendor-url='https://github.com/values-conflict/openjdk-mips64le' \
  --with-vendor-bug-url='https://github.com/values-conflict/openjdk-mips64le'
```

`images` is the right make target.  `legacy-jre-image` was evaluated but build time is
identical (both compile every module; the jlink assembly step is seconds); both produce
`jmods/`; and `images` is the standard target that also gives you `jlink` and other JDK
tools in the output in case you need them on the target.

```bash
{ time PATH=/tmp/fake-bin:$HOME/bin/temurin17/bin:$PATH gmake CONF=release images; } \
  2>&1 | tee build/linux-mips64el-server-release/build.log
```

### Build results

- Configuration: `linux-mips64el-server-release`
- JVM features: `cds compiler2 epsilongc g1gc jfr jni-check jvmti management nmt parallelgc serialgc services vm-structs`
- No C1 (expected -- mips has no C1), no ZGC, no Shenandoah, no JVMCI
- Clean build time: **3m57s wall / 58m12s user** on a 22-core Intel Core Ultra 7 165H
- Output: `build/linux-mips64el-server-release/images/jre/bin/java`
  - confirmed `ELF 64-bit LSB pie executable, MIPS, MIPS64 rel2`

### `custom-spec.gmk` -- documented but not recommended

The wall-clock savings are ~40s and the ALSA stub is stable in practice, so this is not
the primary workflow.  Documented here in case the ALSA stub ever breaks (e.g., libasound
drops symbols between Debian versions).

Excluding `java.desktop` via `MODULES_FILTER` eliminates the ALSA stub, the X11 header
injection, and 6 X11 dev packages.  The build auto-includes
`build/<config>/custom-spec.gmk` if it exists with no configure change required.
The file does not survive `make dist-clean` or a config directory change and must be
recreated manually -- that operational overhead is the main downside.

`java.desktop` has 7 transitive dependents in the jdk17u module graph (all confirmed
via grep of `requires java.desktop` in `module-info.java` files):

```makefile
MODULES_FILTER += java.desktop
MODULES_FILTER += java.se
MODULES_FILTER += jdk.accessibility
MODULES_FILTER += jdk.editpad
MODULES_FILTER += jdk.hotspot.agent
MODULES_FILTER += jdk.jconsole
MODULES_FILTER += jdk.jpackage
MODULES_FILTER += jdk.unsupported.desktop
```

Place this file at `build/linux-mips64el-server-release/custom-spec.gmk` (or whatever the
config directory is) after configure runs.  The file survives `make clean` but not `make
dist-clean` or a reconfigure into a new directory -- recreate it from the snippet above.

With this file in place, the configure command simplifies to:

```bash
PATH=/tmp/fake-bin:$HOME/bin/temurin17/bin:$PATH bash ./configure \
  --openjdk-target=mips64el-linux-gnuabi64 \
  --with-debug-level=release \
  --enable-headless-only \
  --with-freetype=bundled \
  --with-harfbuzz=bundled \
  --with-cups-include=/usr/include \
  --with-fontconfig-include=/usr/include \
  --with-alsa-include=/usr/include \
  --disable-warnings-as-errors
```

Eliminated vs the full configure: `--with-alsa-lib` (and the entire ALSA stub generation
script), `--with-extra-cflags='-I/usr/include'`, `--with-extra-cxxflags='-I/usr/include'`,
and 6 X11 dev packages (`libx11-dev`, `libxrender-dev`, `libxext-dev`, `libxi-dev`,
`libxrandr-dev`, `libxtst-dev`).  The three remaining `--with-*-include` flags survive
because configure's `NEEDS_LIB_CUPS/ALSA/FONTCONFIG` checks are unconditional for Linux
regardless of which modules get built.

Build timing with `custom-spec.gmk` (clean): **3m15s wall / 50m22s user** -- slightly
faster than the full build (fewer modules compiled).

The `custom-spec.gmk` module list should be re-verified for jdk25u since the dependency
graph may differ.

### QEMU user-mode test results

Both `QEMU_CPU` and `QEMU_LD_PREFIX` are environment variables recognised by QEMU
user-mode (see `qemu-mips64el --help`).  They work whether `qemu-mips64el` is invoked
directly or whether a binfmt_misc handler on the host runs it transparently:

```bash
QEMU_CPU=Loongson-3A1000 \
  QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64 \
  build/linux-mips64el-server-release/images/jdk/bin/java -version
```

Result:

```
openjdk version "17.0.19-internal" 2026-04-21
OpenJDK Runtime Environment Tianon (build 17.0.19-internal+0-adhoc..jdk17u)
OpenJDK 64-Bit Server VM Tianon (build 17.0.19-internal+0-adhoc..jdk17u, mixed mode)
```

**`QEMU_CPU=Loongson-3A1000` is required.**  Without it QEMU uses the default MIPS CPU
model (`MIPS64R2-generic`), which enforces strict alignment and delivers `SIGBUS` on any
misaligned access.  The Loongson CPU models emulate the Loongson-3 kernel's
unaligned-access trap handler (`arch/mips/kernel/unaligned.c`), matching real hardware
behaviour.  `Loongson-3A1000` is the closest match to the target hardware (Loongson-3
V0.13, first-gen Loongson-3).  Without this env var the binary will crash:

```
# SIGBUS (BUS_ADRALN) at pc=...
# Problematic frame:
# j  java.lang.Thread.<init>(...)V+21 java.base
```

This crash is identical in `-Xint` mode and with all compressed-oop variants; no JVM flag
can avoid it.  The faulting instruction (decoded from the hs_err disassembly) is
`lhu T1, 315(T3)` where T3 holds the `java.lang.Object` Klass -- offset 315 is odd and
therefore inherently misaligned for a halfword load.  It is a genuine alignment quirk in
the MIPS template interpreter that real hardware silently tolerates via the kernel trap
handler.

### Real hardware test result -- glibc mismatch

Tested on target (Loongson-3 V0.13, Debian Bookworm, glibc 2.36, kernel 4.19.0-12-loongson-3):

```
./test-jdk17/bin/java: /lib/mips64el-linux-gnuabi64/libc.so.6:
  version `GLIBC_2.38' not found (required by .../lib/libjli.so)
```

**Root cause.**  The build host (Debian Trixie, glibc 2.40) has a cross-compiler sysroot
with glibc 2.40 headers.  In glibc 2.38+, defining `_GNU_SOURCE` (which OpenJDK does)
implicitly sets `_ISOC23_SOURCE=1`, which causes `sscanf` to be redirected to
`__isoc23_sscanf@GLIBC_2.38` via `__REDIRECT_NTH`.  The target glibc 2.36 predates this
change and does not have `__isoc23_sscanf`.  The full chain in glibc 2.40's `features.h`:
`_GNU_SOURCE → _ISOC23_SOURCE=1 → __GLIBC_USE_ISOC23=1 → __GLIBC_USE_C23_STRTOL=1 →
sscanf redirected to __isoc23_sscanf@GLIBC_2.38`.

**Fix.**  Build inside a `debian:bookworm-slim` container (glibc 2.36 sysroot), which
predates the `_GNU_SOURCE → _ISOC23_SOURCE` chain entirely.  The Dockerfile has been
updated accordingly.  Note: `apt-get dist-clean` is Trixie-only (APT 2.7.8+); Bookworm
requires the traditional `rm -rf /var/lib/apt/lists/*` instead.

### Phase 0 summary

| Objective | Result |
| --- | --- |
| Cross-compiler produces MIPS64 ELF | yes |
| jdk17u mips port compiles with GCC 12 (Bookworm) | yes (with `--disable-warnings-as-errors`) |
| Build time, full (22-core x86-64) | 3m56s wall / 57m51s user |
| Make target | `images` (`legacy-jre-image` saves no time -- identical compilation) |
| JVM loads under QEMU (default CPU) | yes -- reaches `Thread.<init>`, then SIGBUS |
| `java -version` under QEMU (`QEMU_CPU=Loongson-3A1000`) | **yes** -- `openjdk 17.0.19-internal` |
| `java -version` on real hardware (Bookworm build) | **yes** -- `openjdk 17.0.19-internal` |
| Build host must be | `debian:bookworm-slim` (glibc 2.36, matches target) |

---

## Build System Wiring

### `make/autoconf/platform.m4` -- architecture detection

```m4
mips64el)
  VAR_CPU=mips64el
  VAR_CPU_ARCH=mips64el
  VAR_CPU_BITS=64
  VAR_CPU_ENDIAN=little
  ;;
```

Both `mips64` and `mips64el` map to the same hotspot backend:

```m4
elif test "x$OPENJDK_$1_CPU" = xmips64el; then
  HOTSPOT_$1_CPU=mips_64
```

The critical override that selects `cpu/mips/` vs `cpu/loongarch/`:

```m4
if test "x$OPENJDK_$1_CPU" = xmips64el; then
  HOTSPOT_TARGET_CPU_ARCH=mips
elif test "x$OPENJDK_$1_CPU" = xloongarch64; then
  HOTSPOT_TARGET_CPU_ARCH=loongarch
fi
```

### `make/autoconf/jvm-features.m4` -- C1 disable for mips

```m4
JVM_FEATURES_CHECK_AVAILABILITY(compiler1, [
  AC_MSG_CHECKING([if platform is supported by COMPILER1])
  if test "x$HOTSPOT_TARGET_CPU_ARCH" != "xmips"; then
    # Disable compiler1 on mips
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no, $OPENJDK_TARGET_OS])
    AVAILABLE=false
  fi
])
```

### Build command for a Phase 1 port

```bash
bash ./configure \
  --openjdk-target=mips64el-linux-gnu \
  --with-jvm-variants=server \
  --with-jvm-features=-c1,-zgc,-shenandoahgc \
  --with-boot-jdk=/path/to/x86-jdk25 \
  --disable-hotspot-gtest \
  --with-debug-level=fastdebug
make images
```

`--with-jvm-features=-c1,-zgc,-shenandoahgc` disables C1, ZGC, and Shenandoah, matching
jdk17u's current mips scope. Remove `-c1` when starting Phase 2.

---

## C1 Files Reference

jdk17u loongarch C1 files (the best starting point for a mips C1 port):

```
c1_CodeStubs_loongarch_64.cpp
c1_Defs_loongarch.hpp
c1_FpuStackSim_loongarch.hpp         ← removed in jdk25u (x87-specific no-op)
c1_FpuStackSim_loongarch_64.cpp      ← removed in jdk25u
c1_FrameMap_loongarch.hpp
c1_FrameMap_loongarch_64.cpp
c1_LIRAssembler_loongarch.hpp
c1_LIRAssembler_loongarch_64.cpp     ← 112 KB; instruction-level translation needed
c1_LIRGenerator_loongarch_64.cpp
c1_LIR_loongarch_64.cpp
c1_LinearScan_loongarch.hpp
c1_LinearScan_loongarch_64.cpp       ← present in jdk17u; removed in jdk25u (moved to shared)
c1_MacroAssembler_loongarch.hpp
c1_MacroAssembler_loongarch_64.cpp
c1_Runtime1_loongarch_64.cpp
c1_globals_loongarch.hpp
```

For a mips C1 port, start from **jdk17u's loongarch C1** (not jdk25u's -- jdk17u is closer
in API surface to what the mips port needs), then apply the jdk17u→jdk25u loongarch C1 delta.

---

## Practical Commands

### Find all Loongson-specific commits

Standard upstream commits: `8NNNNN: description`. Loongson commits: "Initial commit",
"Update (YYYY.MM.DD)", "Merge".

```bash
# all Loongson authors in jdk17u
git -C /home/user/loongson-java/jdk17u log --oneline \
  --author='loongson\|aoqi' master-ls

# commits touching mips OR loongarch
git -C /home/user/loongson-java/jdk17u log --oneline master-ls \
  -- src/hotspot/cpu/mips/ src/hotspot/cpu/loongarch/ \
     src/hotspot/os_cpu/linux_mips/ src/hotspot/os_cpu/linux_loongarch/
```

### Diff a file between the two eras

```bash
git -C /home/user/loongson-java/jdk17u \
  show HEAD:src/hotspot/cpu/loongarch/FILE.cpp > /tmp/f_17.cpp
git -C /home/user/loongson-java/jdk25u \
  show HEAD:src/hotspot/cpu/loongarch/FILE.cpp > /tmp/f_25.cpp
diff /tmp/f_17.cpp /tmp/f_25.cpp
```

### Verify open questions

```bash
# confirm SA is gone from jdk25u
git -C /home/user/loongson-java/jdk25u ls-tree --name-only -r HEAD \
  | grep hotspot.agent

# check for icBuffer or depChecker in loongarch (should be empty)
git -C /home/user/loongson-java/jdk25u ls-tree --name-only -r HEAD \
  -- src/hotspot/cpu/loongarch/ | grep -i 'icbuf\|depcheck'
```

---

## Background

### Tag `jdk17_35` -- why it's missing

The tag `jdk17_35` (version `17.1.0-jdk17_35`) is absent from all three repos. Its origin
is unclear -- it may have been from a private build system or a misremembered reference.
Loongson's public tags follow `jdk-17.0.X+Y-ls-Z` (49 tags in jdk17u, oldest
`jdk-17.0.5+8-ls-4`). Use jdk17u `master-ls` HEAD as the mips64le reference.

### Other reference architectures in jdk25u

All CPU backends available in jdk25u: `aarch64`, `arm`, `loongarch`, `ppc`, `riscv`, `s390`, `x86`, `zero`.

**LoongArch** (`cpu/loongarch/`, 112 files) -- primary reference. Derived from the mips port
by the same team; identical function names and structure throughout. Use this first.

**RISC-V** (`cpu/riscv/`, 115 files) -- useful secondary reference. Upstream-contributed and
reviewed by OpenJDK maintainers; follows all JDK 25 conventions cleanly. Good cross-check for
"how should a RISC ISA implement this JDK 25 API?" independently of Loongson's choices. Less
structurally similar to MIPS64 than LoongArch (no delay slots, different calling convention,
different instruction encoding), but any case where the LoongArch approach looks unusual is
worth cross-checking against RISC-V.

**Zero** (`cpu/zero/`) -- interpreter-only, no JIT. The simplest complete port. Useful if you
want to understand what the non-ISA-specific parts of a port look like in isolation.

### Author notes

**loongson-jvm** (`jvm@loongson.cn`) -- author of jdk17u and jdk11u Loongson commits.
Squash/rebase workflow: each "Update (YYYY.MM.DD)" is a rebase onto a new upstream
OpenJDK security release.

**aoqi** (`aoqi@loongson.cn`, Ao Qi) -- author of jdk25u LoongArch port and merge commits
in jdk17u. Primary Loongson OpenJDK contributor since ~2018; fixed MIPS-specific bugs
(`8200213`, `8204091`, `8256829`, `8256831`, `8310019`) well before LoongArch existed.
Strong evidence the LoongArch code was derived from his earlier MIPS work.

### jdk11u vs jdk17u -- the 9-file difference

Files in jdk17u mips that don't exist in jdk11u (added between JDK 11 and JDK 17):

- `c2_MacroAssembler_mips.{cpp,hpp}` -- split out from `macroAssembler_mips.cpp`
- `foreign_globals_mips.{cpp,hpp}` -- Panama FFI stub-only stubs (very thin)
- `depChecker_mips.{cpp,hpp}` -- dependency checker (now removed upstream)
- `gc/shared/barrierSetNMethod_mips.cpp` -- NMethod code cache barrier
- `gc/shared/modRefBarrierSetAssembler_mips.{cpp,hpp}` -- modref barrier refactor

Use jdk17u as the porting base, not jdk11u.

---

## Phase 1 -- Port jdk25u mips64el (interpreter-only, no C2)

### Phase 1 summary

| Objective | Result |
| --- | --- |
| jdk25u mips port compiles with GCC 12 (Bookworm) | **yes** (with `--disable-warnings-as-errors`) |
| All shared runtime stubs generate without crash | **yes** |
| JVM starts and loads Java classes | **yes** |
| `java --version` on real Loongson-3 hardware | **yes** |
| `java /tmp/T.java` (source launcher, invokedynamic) | **yes** -- "hello 42 world: mips64el" |
| `java /tmp/M.java` (HashMap, lambdas) | **yes** -- "3" |
| `java /tmp/S.java` (Thread, synchronized) | **yes** -- "1" |
| `java -jar jenkins-agent.jar --help` | **yes** -- exit 0 |
| Build script | `build-jdk.sh tianon-jdk25u-mips64` |
| QEMU local testing | `QEMU_CPU=Loongson-3A1000 QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64 ./bin/java ...` |

### Key bugs fixed during Phase 1 porting

**1. `internal_pc_type` → `internal_word_type` in `generate_resolve_blob` and deopt stubs**

jdk17u used `relocate(relocInfo::internal_pc_type)` before `patchable_set48(AT, save_pc)` calls
in several functions in `sharedRuntime_mips_64.cpp`. In jdk25u, `internal_pc_type` was removed;
the replacement is `internal_word_type`. Our initial port mistakenly used `runtime_call_type`,
which caused `pd_call_destination` to be called on a `lui AT` (metadata store) instruction and
crash with `ShouldNotReachHere`. Five instances in: `generate_native_wrapper`, `generate_deopt_blob`
(×2), `generate_uncommon_trap_blob`, and `generate_resolve_blob`.

Affected lines: 1875, 2472, 2645, 2726, 3050 of `sharedRuntime_mips_64.cpp`.

**2. `OPT_THREAD` missing**

jdk17u defines `#define OPT_THREAD 1` in `register_mips.hpp`, telling the JVM that the
Java thread pointer is always in TREG (S6) so `get_thread()` is a no-op (just reads TREG).
Our jdk25u port omitted this. Without `OPT_THREAD`, every `get_thread()` call saved all
registers, called `Thread::current()`, and restored all registers -- ~70 instructions per call.
With 3 calls in `generate_resolve_blob`, the instruction section overflowed the CodeBuffer
(2196 bytes generated vs 2080-byte capacity). Fix: add `#define OPT_THREAD 1` to
`register_mips.hpp` adjacent to the `constexpr Register TREG = S6` declaration.

**3. `StubGenerator_generate` blob ID mapping**

Our initial port called `generate_all()` for both `compiler_id` and `final_id` blob phases,
generating arraycopy stubs (each containing `UnsafeMemoryAccessMark`) twice. The
`UnsafeMemoryAccess` global table had capacity 2 but received 4 insertions. Fix: map only
`final_id` to `generate_all()` and increase table capacity to 4 in `StubGenerator_generate`.

**4. `remove_activation` return address (T3 → RA)**

jdk17u's `remove_activation(state, Register ret_addr, ...)` loaded the frame's return address
into the `ret_addr` parameter register. Our jdk25u port dropped that parameter, instead loading
the return address into RA at the end of `remove_activation` (line 858 of
`interp_masm_mips_64.cpp`). But the caller in `templateInterpreterGenerator_mips.cpp` still
used T3 (which at that point holds the frame's monitor-block boundary address, NOT the return
address). Fix: change `push2(T0, T3)` and `move(A1, T3)` to `push2(T0, RA)` and `move(A1, RA)`
at the `exception_handler_for_return_address` call site in `generate_remove_activation_entry`.

Symptom: `SharedRuntime::raw_exception_handler_for_return_address` received a stack address
instead of a code address → `ShouldNotReachHere` at `relocInfo_mips.cpp:596`.

**5. `ThreadStackSize` too small for jdk25u initialization**

jdk17u uses ~700KB of the 1MB default thread stack during JVM initialization. jdk25u uses
~916KB (due to deeper initialization: Loom, JFR, new CP cache infrastructure), which exceeds
the ~896KB effective limit (1MB minus 80KB shadow zone plus guards). This caused
`StackOverflowError` during `Throwable` class initialization, producing an infinite NPE
recursion in `Throwable.<init>` → `NullPointerException.<init>` → ... Fix: increase
`ThreadStackSize` and `VMThreadStackSize` from 1024KB to 2048KB in
`src/hotspot/os_cpu/linux_mips/globals_linux_mips.hpp`. This matches what RISC-V uses.

### Additional bugs fixed during hardware testing (2026-06-02)

**6. TOS state extracted from wrong field in `load_invoke_cp_cache_entry`**

JDK-8302708 split `_flags` and `_tos_state` into separate bytes in `ResolvedMethodEntry`.  The
MIPS port loaded `flags_offset()` into `flags` and then did `dsrl(flags, flags, tos_state_shift=28)`
to extract the TOS state.  Since `_flags` is only 8 bits, shifting right by 28 always gives 0, so
every invoke returned through the itos entry.  Fix: after loading `flags`, also load `type_offset()`
(`_tos_state`) into AT, shift it left by 28, and OR it into `flags`.

**7. `invokevirtual` vtable index vs Method***

`load_invoke_cp_cache_entry` with `is_invokevirtual=true` always loaded `method_offset()` into
`method`.  Non-final virtuals need the vtable index from `table_index_offset()`; only vfinal calls
use the Method*.  Fix: branch on `is_vfinal` at runtime; load `table_index_offset()` for non-final.

**8. `invokeinterface` klass/method registers swapped**

For `invokeinterface`, T2 (`method`) should receive the interface klass from `klass_offset()` and
Rmethod (`itable_index`) should receive the Method* from `method_offset()` -- not the other way
around as the original code had it.  For forced-virtual (Object methods via interface), Rmethod
instead gets the vtable index or Method* based on `is_vfinal`.

**9. `invokehandle` caught by `invokeinterface` branch**

Both `invokeinterface` and `invokehandle` pass `itable_index != NOREG`, so the `else if
(itable_index != NOREG)` branch in `load_invoke_cp_cache_entry` was applying the invokeinterface
klass/method logic to `invokehandle`, which is wrong.  Fix: gate the invokeinterface branch on
`bytecode() == Bytecodes::_invokeinterface`; the else branch then handles invokehandle by loading
Method* into `method` and `resolved_references_index` into `itable_index`.

**10. Three `invokedynamic` bugs**

*a. Wrong `has_appendix` bit.*  `ResolvedIndyEntry::has_appendix_shift = 1` (bit 1), but
`prepare_invoke` was checking `(1 << ResolvedMethodEntry::has_appendix_shift)` = bit 3.  The
appendix (MethodType or CallSite) was never pushed; lambdas returned null; NPE at boot layer init.

*b. `resolved_references_index` not loaded.*  `itable_index` (T2_callsite) was not loaded from
`ResolvedIndyEntry::resolved_references_index_offset()` before `load_resolved_reference_at_index`
used it, so the appendix lookup used a stale register.

*c. Wrong return entry for invokedynamic.*  `generate_return_entry_for` used `load_method_entry`
(reads a 2-byte method index) to advance SP for invokedynamic returns.  invokedynamic uses a
4-byte operand and `load_resolved_indy_entry` instead.  Wrong SP advance left the int argument on
the stack; GC later found the integer as a garbage oop.

**11. `invokedynamic` resolution check missing**

When first executed, the `ResolvedIndyEntry._method` is null.  The port jumped directly to
`jump_from_interpreted(null)` without calling `InterpreterRuntime::resolve_from_cache`.  Fix: add
`bne(method, R0, resolved); call_VM(resolve_from_cache, _invokedynamic)` before the dispatch, with
a reload after the call (call_VM clobbers T1-T9).

**12. `get_cache_index_at_bcp` for sizeof(u4) decoded with old JDK 17 `~index` formula**

In JDK 17, the Rewriter stored `~indy_index` in the `invokedynamic` bytecode operand.  JDK 25
stores the index directly (`Bytes::put_native_u4(p, (u2)_invokedynamic_index)`).  The MIPS port
still applied `nor(index, index, R0); sll(index, index, 0)` to "decode" the value, turning index 0
into -1, then `dsll(-1, 4) = -16`, then `daddu(array+8, -16) = array-8`, pointing the entry
pointer 16 bytes before the first actual entry.  Fix: remove the nor/sll decode.

**13. `_flags` field in `ResolvedIndyEntry` at odd byte offset**

`ResolvedIndyEntry` layout: `Method*(8), u2(8), u2(10), u2(12), u1(14), u1 _flags(15)`.
Using `lhu` at offset 15 is a 2-byte access to an odd address → SIGBUS/SEGV on strict-alignment
MIPS.  Fix: use `lbu` (single-byte load).

**14. LM_LIGHTWEIGHT locking fast path used JDK 17 stack-locking code**

`lock_object` and `unlock_object` had an inline fast path for `LM_LIGHTWEIGHT` that implemented
the JDK 17 "displaced header" protocol, which is invalid in JDK 25.  Fix: always call the
runtime (`InterpreterRuntime::monitorenter` / `monitorexit`), which correctly handles the current
protocol.

**15. Interpreter locals pointer stored as absolute address; JDK 25 expects word-offset from FP**

`frame::interpreter_frame_locals()` reads the frame slot at `interpreter_frame_locals_offset` as
a signed integer `n` and returns `&fp()[n]` = `FP + n*8`.  The MIPS port stored `LVP` (an absolute
stack address) there; the correct value is `(LVP - FP) / wordSize`.  The GC computed garbage locals
addresses, called `do_oop` on them, and crashed.  Fix: store `(LVP - FP) / wordSize` in
`generate_fixed_frame`; decode back in `restore_locals()` as `FP + n*wordSize`.

### QEMU user-mode test status (2026-06-02)

Use `QEMU_CPU=Loongson-3A1000` and `QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64` for all
QEMU testing -- see the Phase 0 QEMU section for the full explanation.  With the Loongson
CPU model, QEMU emulates the unaligned-access trap handler, matching real hardware behaviour.
Any crash with `QEMU_CPU=Loongson-3A1000` reflects a genuine interpreter or runtime bug.

All Phase 1 target tests pass locally under QEMU and on real Loongson-3 hardware:

```bash
QEMU_CPU=Loongson-3A1000 QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64 \
  build/linux-mips64el-server-release/images/jdk/bin/java --version
QEMU_CPU=Loongson-3A1000 QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64 \
  build/linux-mips64el-server-release/images/jdk/bin/java /tmp/T.java
```

### Remaining work

- Phase 2: implement Loom continuation stubs (`gen_continuation_enter`,
  `gen_continuation_yield`) in `sharedRuntime_mips_64.cpp`; re-enable `VMContinuations`.
  Currently disabled via `globals_mips.hpp`: `define_pd_global(bool, VMContinuations, false)`.
- Phase 3: implement `mips.ad` / `mips_64.ad` for C2 JIT (requires AD file authoring)
- Phase 4: Panama FFI (`ForeignGlobals`, `DowncallLinker`, `UpcallLinker`)
