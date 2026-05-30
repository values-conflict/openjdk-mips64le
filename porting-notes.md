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
`--with-alsa-include=/usr/include`, plus `--with-extra-cflags="-I/usr/include"` and
`--with-extra-cxxflags="-I/usr/include"` for X11.  These are header-only uses
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

```bash
PATH=/tmp/fake-bin:$HOME/bin/temurin17/bin:$PATH bash ./configure \
  --openjdk-target=mips64el-linux-gnuabi64 \
  --with-jvm-variants=server \
  --with-debug-level=release \
  --disable-hotspot-gtest \
  --enable-headless-only \
  --with-freetype=bundled \
  --with-harfbuzz=bundled \
  --with-cups-include=/usr/include \
  --with-fontconfig-include=/usr/include \
  --with-alsa-include=/usr/include \
  --with-alsa-lib=/tmp/alsa-stub-lib \
  --disable-warnings-as-errors \
  --with-extra-cflags="-I/usr/include" \
  --with-extra-cxxflags="-I/usr/include"
```

```bash
{ time PATH=/tmp/fake-bin:$HOME/bin/temurin17/bin:$PATH gmake CONF=release images; } \
  2>&1 | tee build/linux-mips64el-server-release/build.log
```

### Build results

- Configuration: `linux-mips64el-server-release`
- JVM features: `cds compiler2 epsilongc g1gc jfr jni-check jvmti management nmt parallelgc serialgc services vm-structs`
- No C1 (expected -- mips has no C1), no ZGC, no Shenandoah, no JVMCI
- Build time: **3m52s wall / 58m18s user** on a 22-core Intel Core Ultra 7 165H
- Output binary: `build/linux-mips64el-server-release/images/jdk/bin/java`
  - confirmed `ELF 64-bit LSB pie executable, MIPS, MIPS64 rel2`

### QEMU user-mode test results

binfmt_misc is configured system-wide; mips64el ELFs execute transparently.  The cross-compiler
sysroot provides the dynamic linker at `/usr/mips64el-linux-gnuabi64/lib64/ld.so.1`; set
`QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64` to point QEMU at it.

```bash
QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64 \
  build/linux-mips64el-server-release/images/jdk/bin/java -version
```

Result: **JVM starts and reaches Java-level initialization, then crashes** with
`SIGBUS (BUS_ADRALN)` in `Thread.<init>` at bytecode offset +21.  Faulting address:
`0x00000008000410fb` (clearly misaligned -- low 3 bits set).  The crash is identical in
`-Xint` interpreter mode and with compressed oops disabled, ruling out JIT and compressed
oops as the proximate cause.

**Root cause hypothesis.**  The faulting address pattern (`0x00000008_000410fb`) suggests a
narrow oop value being used as a raw pointer without the heap base being added, or a
shift-and-add decode that produces a misaligned result.  This is a bug in the MIPS template
interpreter's object reference handling that would be papered over on real hardware: the
Linux/MIPS kernel's unaligned-access handler (`arch/mips/kernel/unaligned.c`) emulates
misaligned loads/stores transparently.  QEMU user-mode does not replicate this kernel-level
handler and instead delivers `SIGBUS` directly to the process.

**Conclusion for QEMU user-mode testing:**
- Insufficient for JVM boot testing -- QEMU does not emulate the MIPS unaligned-access kernel handler
- QEMU full system emulation (with a proper MIPS kernel) or real Loongson hardware is required for `java -version`

### Phase 0 summary

| Objective | Result |
| --- | --- |
| Cross-compiler produces MIPS64 ELF | yes |
| jdk17u mips port compiles with GCC 14 | yes (with `--disable-warnings-as-errors`) |
| Build time on 22-core x86-64 | 3m52s wall / 58m18s user |
| JVM loads under QEMU user-mode | yes -- reaches Java-level `Thread.<init>` |
| `java -version` completes under QEMU | no -- SIGBUS in interpreter (unaligned access) |
| QEMU full system / real hardware needed | yes |

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
