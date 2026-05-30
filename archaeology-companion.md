# Archaeology Companion — LLM Reference Notes

Companion to `mips64le-archaeology.md`. Contains raw data, full file lists, exact
hashes, build commands, and verification steps — details too verbose for the main
report but valuable for any LLM continuing this analysis.

---

## Tag `jdk17_35` — Why It's Missing

The context that spawned this investigation mentioned a tag `jdk17_35` (version
`17.1.0-jdk17_35`) as a known-good mips64le build. This tag does not exist in any
of the three Loongson repos. Likely explanations:

- It belongs to **Huawei's Bisheng JDK** (`gitee.com/openeuler/bishengjdk-17`), which
  uses a `jdk17_N` tagging scheme and also has mips64 support.
- It belongs to an **internal Loongson build system** not yet published to GitHub.

To check Bisheng JDK: `git ls-remote https://gitee.com/openeuler/bishengjdk-17 'refs/tags/jdk17_35'`

Loongson's public repos use `jdk-17.0.X+Y-ls-Z` tags. The 49 `-ls` tags in jdk17u
range from `jdk-17.0.5+8-ls-4` (oldest) to `jdk-17.0.19+10-ls-0` (newest as of
unshallow date).

---

## Author Identity

**loongson-jvm** (`jvm@loongson.cn`) — commits in jdk17u and jdk11u. All their
commits use opaque messages: "Initial commit by Loongson", "Update (YYYY.MM.DD)",
"Update (YYYY.MM.DD, 2nd)", "Merge". Squash-based workflow; each "Update" is a
rebase onto new upstream OpenJDK security/patch releases.

**aoqi** (`aoqi@loongson.cn`) / **Ao Qi** — commits in jdk25u as author; appears
as merge author in jdk17u (the merge commits that pull in loongson-jvm's work). His
upstream OpenJDK contributions:
- `8200213` (2018-03-26): Configure broken on MIPS
- `8204091` (2018-05-30): Configure broken on MIPS when uname returns mipsel or mips64el
- `8219814` (2019-04): Help-info for pns(...) on Linux/mips lost
- `8256829` (2020): GNU hash style not available on MIPS
- `8256831` (2020): MIPS Zero builds fail with undefined `__atomic_compare_exchange_8`
- `8310019` (2023): MIPS builds broken after JDK-8304913

He has been the primary Loongson OpenJDK contributor since ~2018.

The jdk25u initial LoongArch commit (`526ad75a343f`, 2022-08-11) lists co-authors:
- Ao Qi `aoqi@loongson.cn`
- Wang Rui `wangrui@loongson.cn`
- Zhai Xiang `zhaixiang@loongson.cn`
- Wang Haomin `wanghaomin@loongson.cn`
- Sun Guoyun `sunguoyun@loongson.cn`
- Pan Xuefeng `panxuefeng@loongson.cn`
- Sun Xu `sunxu01@loongson.cn`

---

## Complete File Lists

### jdk17u `src/hotspot/cpu/mips/` (77 files)

```
abstractInterpreter_mips.cpp
assembler_mips.cpp
assembler_mips.hpp
assembler_mips.inline.hpp
bytes_mips.hpp
c2_MacroAssembler_mips.cpp
c2_MacroAssembler_mips.hpp
c2_globals_mips.hpp
c2_init_mips.cpp
codeBuffer_mips.hpp
compiledIC_mips.cpp
copy_mips.hpp
depChecker_mips.cpp
depChecker_mips.hpp
disassembler_mips.hpp
foreign_globals_mips.cpp
foreign_globals_mips.hpp
frame_mips.cpp
frame_mips.hpp
frame_mips.inline.hpp
gc/g1/g1BarrierSetAssembler_mips.cpp
gc/g1/g1BarrierSetAssembler_mips.hpp
gc/g1/g1Globals_mips.hpp
gc/shared/barrierSetAssembler_mips.cpp
gc/shared/barrierSetAssembler_mips.hpp
gc/shared/barrierSetNMethod_mips.cpp
gc/shared/cardTableBarrierSetAssembler_mips.cpp
gc/shared/cardTableBarrierSetAssembler_mips.hpp
gc/shared/modRefBarrierSetAssembler_mips.cpp
gc/shared/modRefBarrierSetAssembler_mips.hpp
globalDefinitions_mips.hpp
globals_mips.hpp
icBuffer_mips.cpp
icache_mips.cpp
icache_mips.hpp
interp_masm_mips.hpp
interp_masm_mips_64.cpp
interpreterRT_mips.hpp
interpreterRT_mips_64.cpp
javaFrameAnchor_mips.hpp
jniFastGetField_mips_64.cpp
jniTypes_mips.hpp
macroAssembler_mips.cpp
macroAssembler_mips.hpp
macroAssembler_mips.inline.hpp
matcher_mips.hpp
methodHandles_mips.cpp
methodHandles_mips.hpp
mips.ad
mips_64.ad
nativeInst_mips.cpp
nativeInst_mips.hpp
registerMap_mips.hpp
register_definitions_mips.cpp
register_mips.cpp
register_mips.hpp
relocInfo_mips.cpp
relocInfo_mips.hpp
runtime_mips_64.cpp
sharedRuntime_mips_64.cpp
stubGenerator_mips_64.cpp
stubRoutines_mips.hpp
templateInterpreterGenerator_mips.cpp
templateTable_mips.hpp
templateTable_mips_64.cpp
vmStructs_mips.hpp
vm_version_mips.cpp
vm_version_mips.hpp
vmreg_mips.cpp
vmreg_mips.hpp
vmreg_mips.inline.hpp
vtableStubs_mips_64.cpp
icBuffer_mips.cpp
icache_mips.cpp
icache_mips.hpp
```

### jdk17u `src/hotspot/os_cpu/linux_mips/` (15 files)

```
assembler_linux_mips.cpp
atomic_linux_mips.hpp
bytes_linux_mips.inline.hpp
copy_linux_mips.inline.hpp
globals_linux_mips.hpp
linux_mips.s
orderAccess_linux_mips.hpp
os_linux_mips.cpp
os_linux_mips.hpp
prefetch_linux_mips.inline.hpp
safefetch_linux_mips64.S
thread_linux_mips.cpp
thread_linux_mips.hpp
vmStructs_linux_mips.hpp
vm_version_linux_mips.cpp
```

### jdk17u SA agent mips64 files (15 Java files)

All under `src/jdk.hotspot.agent/share/classes/sun/jvm/hotspot/`:
```
debugger/MachineDescriptionMIPS64.java
debugger/linux/mips64/LinuxMIPS64CFrame.java
debugger/linux/mips64/LinuxMIPS64ThreadContext.java
debugger/mips64/MIPS64ThreadContext.java
debugger/proc/mips64/ProcMIPS64Thread.java
debugger/proc/mips64/ProcMIPS64ThreadContext.java
debugger/proc/mips64/ProcMIPS64ThreadFactory.java
debugger/remote/mips64/RemoteMIPS64Thread.java
debugger/remote/mips64/RemoteMIPS64ThreadContext.java
debugger/remote/mips64/RemoteMIPS64ThreadFactory.java
runtime/linux_mips64/LinuxMIPS64JavaThreadPDAccess.java
runtime/mips64/MIPS64CurrentFrameGuess.java
runtime/mips64/MIPS64Frame.java
runtime/mips64/MIPS64JavaCallWrapper.java
runtime/mips64/MIPS64RegisterMap.java
```

Note: the SA was removed from OpenJDK in JDK 21+. jdk25u has no SA for any architecture.
jdk17u retains it; jdk11u retains an identical copy.

---

## Complete Loongson Commit Timeline (jdk17u master-ls)

From `git log --format="%H %ad %an %s" --date=short master-ls -- src/hotspot/cpu/mips/ src/hotspot/cpu/loongarch/ src/hotspot/os_cpu/linux_mips/ src/hotspot/os_cpu/linux_loongarch/`:

```
f1d5298a733 2026-05-07 aoqi            Update (2026.05.07)
a66f78c6520 2026-01-29 loongson-jvm   Update (2026.01.29, 2nd)
a671c4f393a 2026-01-29 loongson-jvm   Update (2026.01.29)
c362ea9e7e9 2025-10-30 loongson-jvm   Update (2025.10.29, 2nd)
15f5afd71be 2024-10-17 loongson-jvm   Update (2024.10.17)
c6583c8c5cb 2024-10-16 loongson-jvm   Update (2024.10.16)
4cc0b3e9764 2024-07-26 loongson-jvm   Update (2024.07.26)
c53840e66c9 2024-04-26 loongson-jvm   Update (2024.04.26, 2nd)
38b11e8e016 2024-04-26 loongson-jvm   Update (2024.04.26)
5f77562ba03 2024-01-26 loongson-jvm   Update (2024.01.26, 2nd)
fe193bcbc1c 2024-01-26 loongson-jvm   Update (2024.01.26)
99147f78245 2023-12-07 loongson-jvm   Update (2023.12.07, 2nd)
e7efed5c455 2023-12-07 loongson-jvm   Update (2023.12.07)
0ca05554a06 2023-08-14 loongson-jvm   Update (2023.08.14, 2nd)
d833eebe238 2023-08-14 loongson-jvm   Update (2023.08.14)
cece1917fcf 2023-07-03 loongson-jvm   Update (2023.07.03)
f3b0a23f659 2023-05-17 loongson-jvm   Update (2023.05.17)
4d654249359 2023-02-24 loongson-jvm   Update (2023.02.23)
d4dcc0bf7b8 2023-01-31 loongson-jvm   Initial commit by Loongson
```

---

## Autoconf Build System Details

### `make/autoconf/platform.m4` — relevant excerpts

Architecture detection and HOTSPOT_CPU_ARCH mapping:

```m4
mips64)
  VAR_CPU=mips64
  VAR_CPU_ARCH=mips64
  VAR_CPU_BITS=64
  VAR_CPU_ENDIAN=big
  ;;
mips64el)
  VAR_CPU=mips64el
  VAR_CPU_ARCH=mips64el
  VAR_CPU_BITS=64
  VAR_CPU_ENDIAN=little
  ;;
```

Both map to the same hotspot backend:

```m4
elif test "x$OPENJDK_$1_CPU" = xmips64; then
  HOTSPOT_$1_CPU=mips_64
elif test "x$OPENJDK_$1_CPU" = xmips64el; then
  HOTSPOT_$1_CPU=mips_64
```

The `_arch` override (critical — this selects `cpu/mips/` vs `cpu/loongarch/`):

```m4
# Override hotspot cpu definitions for MIPS platforms
if test "x$OPENJDK_$1_CPU" = xmips64el; then
  HOTSPOT_TARGET_CPU_ARCH=mips
elif test "x$OPENJDK_$1_CPU" = xloongarch64; then
  HOTSPOT_TARGET_CPU_ARCH=loongarch
fi
```

### `make/autoconf/jvm-features.m4` — C1 disable for mips

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

---

## C1 Presence in Each Repo

jdk17u loongarch has **16** C1 files (includes `c1_FpuStackSim_loongarch.hpp` and
`c1_FpuStackSim_loongarch_64.cpp`). jdk25u loongarch has **13** C1 files — the
`FpuStackSim` pair was removed. FPU stack simulation is x87-specific; on RISC ISAs
it was always a stub/no-op and was eventually cleaned up.

jdk17u loongarch C1 files (all present in jdk11u too):
```
c1_CodeStubs_loongarch_64.cpp
c1_Defs_loongarch.hpp
c1_FpuStackSim_loongarch.hpp         ← removed in jdk25u
c1_FpuStackSim_loongarch_64.cpp      ← removed in jdk25u
c1_FrameMap_loongarch.hpp
c1_FrameMap_loongarch_64.cpp
c1_LIRAssembler_loongarch.hpp
c1_LIRAssembler_loongarch_64.cpp
c1_LIRGenerator_loongarch_64.cpp
c1_LIR_loongarch_64.cpp
c1_LinearScan_loongarch.hpp
c1_LinearScan_loongarch_64.cpp
c1_MacroAssembler_loongarch.hpp
c1_MacroAssembler_loongarch_64.cpp
c1_Runtime1_loongarch_64.cpp
c1_globals_loongarch.hpp
```

When forward-porting C1 to mips for jdk25u, start from jdk17u's loongarch C1 files
(not jdk25u's), then apply jdk17u→jdk25u loongarch delta to understand what changed.

---

## Function-Level Structural Comparison

### `InterpreterMacroAssembler` methods (ordering comparison)

Both `interp_masm_mips_64.cpp` (jdk17u) and `interp_masm_loongarch_64.cpp` (jdk25u)
open with the same functions in the same order:

```
get_2_byte_integer_at_bcp
get_4_byte_integer_at_bcp     ← signature differs: mips takes extra tmp Register
jump_to_entry
call_VM_leaf_base
call_VM_base
check_and_handle_popframe
load_earlyret_value
check_and_handle_earlyret
get_unsigned_2_byte_index_at_bcp
```

jdk25u loongarch adds between `call_VM_base` and `check_and_handle_popframe`:
- `call_VM_preemptable` (Loom)
- `restore_after_resume` (Loom)

And replaces the old CP cache API:
- old: `get_cache_and_index_at_bcp`, `get_cache_and_index_and_bytecode_at_bcp`, `get_cache_entry_pointer_at_bcp`
- new: `load_resolved_indy_entry`, `load_field_entry`, `load_method_entry`

This CP cache API replacement (`JDK-8302708` and related) is the most impactful single
change to the interpreter between JDK 17 and JDK 25. Every bytecode that accesses the
constant pool cache needs to be rewritten.

### `MacroAssembler` shared patterns (same in both ISAs)

These function names appear identically in `macroAssembler_mips.cpp` and
`macroAssembler_loongarch.cpp`:

```
patchable_jump
patchable_call
trampoline_call
emit_trampoline_stub
beq_far / bne_far / b_far   ← Loongson-specific far-branch helpers
atomic_inc32
bang_stack_size
reserved_stack_check
```

`beq_far`/`bne_far`/`b_far` are worth special attention: they are not part of upstream
OpenJDK's shared infrastructure. They exist because both MIPS64 and LoongArch have
limited branch-offset fields (16-bit in MIPS, 26-bit in LoongArch for unconditional
branches), requiring trampolines for long-range branches. The fact that both ISAs share
the same helper names and signatures is strong evidence of common authorship.

---

## `jdk17u` vs `jdk25u` Loongarch — Delta for API Changes

To enumerate every JVM API change affecting the mips port between JDK 17 and JDK 25,
run this in your working tree:

```bash
# Get a list of all changed loongarch files between jdk17u and jdk25u
git -C /home/user/loongson-java/jdk17u show HEAD:src/hotspot/cpu/loongarch/macroAssembler_loongarch.cpp \
  > /tmp/ma_17.cpp
git -C /home/user/loongson-java/jdk25u show HEAD:src/hotspot/cpu/loongarch/macroAssembler_loongarch.cpp \
  > /tmp/ma_25.cpp
diff /tmp/ma_17.cpp /tmp/ma_25.cpp
```

Repeat for each shared file (templateInterpreterGenerator, templateTable, sharedRuntime,
etc.). The diff will show you exactly which upstream API changed and what the adaptation
looks like. Then apply the equivalent change to the mips file with MIPS64 instructions.

---

## Build Commands for a Minimal mips64le Port

Cross-compiling from an x86-64 host (once the port files are in place):

```bash
bash ./configure \
  --openjdk-target=mips64el-linux-gnu \
  --with-jvm-variants=server \
  --with-jvm-features=-c1,-zgc,-shenandoahgc \
  --with-boot-jdk=/path/to/x86-jdk17 \
  --disable-hotspot-gtest \
  --with-debug-level=fastdebug
make images
```

The `--with-jvm-features=-c1,-zgc,-shenandoahgc` flag disables C1, ZGC, and Shenandoah,
matching jdk17u's current mips scope. Once Phase 1 builds cleanly, add `-c1` back and
start Phase 2.

---

## SA Agent Absence in jdk25u

The Serviceability Agent (`jdk.hotspot.agent`, Java-based JVM debugger accessible via
`jhsdb`) was removed from OpenJDK in JDK 21 (deprecated in 17, removed in 21). Any
forward port to jdk25 therefore does not need SA agent Java files. The 15 mips64 SA
files in jdk17u/jdk11u are jdk17-only artifacts.

---

## `c1_LinearScan` Removal in jdk25u

jdk17u loongarch has `c1_LinearScan_loongarch.hpp` and `c1_LinearScan_loongarch_64.cpp`.
jdk25u loongarch has `c1_LinearScan_loongarch.hpp` but the `_64.cpp` may have been
removed (not confirmed in this investigation — verify with `git -C jdk25u ls-tree HEAD
src/hotspot/cpu/loongarch/ | grep LinearScan`). The `.hpp` is the interface; the `.cpp`
held arch-specific implementations of linear scan register allocation hints. If removed,
it likely means the implementation moved into shared code.

---

## How to Find Loongson-Specific Commits

Standard upstream commits have issue-number subjects like `8NNNNNn: description`.
Loongson-specific commits have terse subjects: "Initial commit by Loongson", "Update
(YYYY.MM.DD)", "Merge".

Useful filters:

```bash
# All Loongson update commits in jdk17u
git -C /home/user/loongson-java/jdk17u log --oneline \
  --author='loongson\|aoqi' master-ls

# Commits that touched mips OR loongarch source
git -C /home/user/loongson-java/jdk17u log --oneline master-ls \
  -- src/hotspot/cpu/mips/ src/hotspot/cpu/loongarch/ \
     src/hotspot/os_cpu/linux_mips/ src/hotspot/os_cpu/linux_loongarch/
```

---

## jdk11u vs jdk17u mips — What's Different

Files in jdk17u mips that are absent in jdk11u (9 files added between JDK 11 and 17):

- `c2_MacroAssembler_mips.cpp` / `c2_MacroAssembler_mips.hpp` — split from `macroAssembler`
- `foreign_globals_mips.cpp` / `foreign_globals_mips.hpp` — Panama FFI stubs (stub-only)
- `depChecker_mips.cpp` / `depChecker_mips.hpp` — dependency checker
- `gc/shared/barrierSetNMethod_mips.cpp` — NMethod code cache barrier
- `gc/shared/modRefBarrierSetAssembler_mips.cpp` / `.hpp` — modref barrier refactor

These additions reflect upstream JDK 17 architectural changes that the mips port had to
track. When forward-porting to jdk25u, use jdk17u as the base (more complete).
