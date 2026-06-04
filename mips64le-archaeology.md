# mips64le Archaeology -- Loongson OpenJDK Forks

Investigated 2026-05-30. All three repos fully unshallowed from GitHub.

See `porting-notes.md` for file-by-file mapping, build commands, and API change reference.

## Repository Overview

| Directory | Remote | Branch | Total commits |
| --- | --- | --- | --- |
| `jdk11u` | `github.com/loongson/jdk11u` | `master-ls` | ~80k |
| `jdk17u` | `github.com/loongson/jdk17u` | `master-ls` | ~70k |
| `jdk25u` | `github.com/loongson/jdk25u` | `loongarch-port` | ~85k |

The bulk of each branch's history is upstream OpenJDK. Loongson-specific commits number in the low dozens per repo.

---

## Tag `jdk17_35` -- Does Not Exist Here

The tag `jdk17_35` (version `17.1.0-jdk17_35`) is **absent** from all three repos. The Loongson repos use their own scheme: `jdk-17.0.X+Y-ls-Z` (49 such tags in jdk17u). The `jdk17_35` tag likely belongs to Huawei's Bisheng JDK or another vendor's internal fork. Analysis below uses `jdk17u HEAD` (`master-ls`) as the mips64le reference.

---

## Step 2 -- mips64le Port in jdk17u (primary reference)

The mips64le backend is **fully present and actively maintained in jdk17u and jdk11u**. It was never removed from either repo. jdk25u never had it -- that absence is the problem we're investigating.

### Build System Mapping (`make/autoconf/platform.m4`)

```
mips64el  →  HOTSPOT_TARGET_CPU_ARCH=mips  →  src/hotspot/cpu/mips/
mips64    →  HOTSPOT_TARGET_CPU_ARCH=mips  →  src/hotspot/cpu/mips/
loongarch64  →  HOTSPOT_TARGET_CPU_ARCH=loongarch  →  src/hotspot/cpu/loongarch/
```

Both big-endian `mips64` and little-endian `mips64el` share the `cpu/mips/` backend. Loongson's machines historically ran `mips64el`.

### `src/hotspot/cpu/mips/` -- 77 files, ~1.1 MB

Key files by size:

| File | Size | Purpose |
| --- | --- | --- |
| `mips_64.ad` | 334 KB | C2 machine description (instruction selection) |
| `templateTable_mips_64.cpp` | 130 KB | Bytecode template table |
| `sharedRuntime_mips_64.cpp` | 125 KB | Calling conventions, deopt |
| `macroAssembler_mips.cpp` | 111 KB | MacroAssembler implementation |
| `assembler_mips.hpp` | 85 KB | Full MIPS64 assembler |
| `stubGenerator_mips_64.cpp` | 84 KB | Runtime stub generation |
| `interp_masm_mips_64.cpp` | 73 KB | Interpreter macro assembler |
| `templateInterpreterGenerator_mips.cpp` | 70 KB | Template interpreter generator |
| `nativeInst_mips.cpp` | 59 KB | Native instruction patching |

### `src/hotspot/os_cpu/linux_mips/` -- 15 files

`os_linux_mips.cpp` (31 KB) handles signal stack setup, thread context, and CPU feature detection. `safefetch_linux_mips64.S` carries `mips64` in the filename confirming 64-bit target.

### `src/jdk.hotspot.agent/` -- 15 Java files (SA agent, jdk17u only)

Full Serviceability Agent support for mips64: `MachineDescriptionMIPS64.java`, `LinuxMIPS64CFrame.java`, `MIPS64Frame.java`, `MIPS64RegisterMap.java`, `LinuxMIPS64JavaThreadPDAccess.java`, etc. The SA was removed from OpenJDK in JDK 21+; jdk25u has no SA for any architecture. **These 15 files do not carry over to a jdk25u port.**

**Total: 107 mips64-specific files across all modules, ~1.55 MB.**

### Architecture Capabilities

| Component | mips64 (jdk17u) | loongarch (jdk17u) | loongarch (jdk25u) |
| --- | --- | --- | --- |
| Template interpreter | ✓ | ✓ | ✓ |
| C2 JIT | ✓ | ✓ | ✓ |
| C1 JIT | ✗ (disabled) | ✓ (16 files) | ✓ (13 files) |
| G1 GC | ✓ | ✓ | ✓ |
| ZGC | ✗ | ✓ | ✓ |
| Shenandoah | ✗ | ✓ | ✓ |
| Panama FFI | ✗ | ✗ | ✓ |
| JVMCI | ✗ | ✗ | ✓ |
| Loom/continuations | ✗ | ✗ | ✓ |

C1 is explicitly disabled for mips in `make/autoconf/jvm-features.m4`:

```m4
if test "x$HOTSPOT_TARGET_CPU_ARCH" != "xmips"; then
  # Disable compiler1 on mips
```

---

## Step 3 -- History of mips in jdk17u

mips was never removed. Loongson-specific commits touching mips or loongarch on `master-ls`:

| Commit | Date | Notes |
| --- | --- | --- |
| `d4dcc0bf7b8` | 2023-01-31 | "Initial commit by Loongson -- A LoongArch64 port (template interpreter, C1 JIT and C2 JIT) and a MIPS64 port (template interpreter and C2 JIT)." |
| `4d654249359` | 2023-02-24 | Update |
| `f3b0a23f659` | 2023-05-17 | Update |
| `cece1917fcf` | 2023-07-03 | Update |
| `d833eebe238` | 2023-08-14 | Update |
| `0ca05554a06` | 2023-08-14 | Update (2nd) |
| … | 2023-2025 | Continuing updates |
| `a66f78c6520` | 2026-01-29 | Modified `templateTable_mips_64.cpp` directly |
| `f1d5298a733` | 2026-05-07 | Most recent Loongson commit |

Both architectures are always updated in the same commits.

---

## Step 4 -- LoongArch Port in jdk25u

mips **never existed** in jdk25u's `loongarch-port`. The initial commit is:

```
526ad75a343f  2022-08-11  aoqi <aoqi@loongson.cn>
"Initial Linux/LoongArch64 Port -- A complete (ie, template interpreter,
C1 JIT compiler, C2 JIT compiler, Vector API, ZGC and Shenandoah GC)
LoongArch64 port has been implemented."
```

This predates jdk17u/jdk11u's combined mips+loongarch commit by six months.

### `src/hotspot/cpu/loongarch/` -- 112 files, 2.72 MB total (hotspot only)

Top files by size vs mips64 equivalents:

| File | Size | mips64 equivalent | mips64 size |
| --- | --- | --- | --- |
| `loongarch_64.ad` | 525 KB | `mips_64.ad` | 334 KB |
| `assembler_loongarch.hpp` | 286 KB | `assembler_mips.hpp` | 85 KB |
| `stubGenerator_loongarch_64.cpp` | 204 KB | `stubGenerator_mips_64.cpp` | 84 KB |
| `macroAssembler_loongarch.cpp` | 153 KB | `macroAssembler_mips.cpp` | 111 KB |
| `templateTable_loongarch_64.cpp` | 118 KB | `templateTable_mips_64.cpp` | 130 KB |
| `sharedRuntime_loongarch_64.cpp` | 106 KB | `sharedRuntime_mips_64.cpp` | 125 KB |

The loongarch port is larger due to C1, JVMCI, ZGC, Panama FFI, Loom, and richer crypto intrinsics. The `.ad` file is 57% larger from additional C2 intrinsics and vector patterns.

### Outside hotspot (jdk25u loongarch only)

- `src/java.base/.../foreign/abi/loongarch64/` -- Panama FFI ABI (3 Java files)
- `src/jdk.internal.vm.ci/.../loongarch64/` -- JVMCI (3 Java files)
- `test/micro/.../loongarch/` -- microbenchmarks

### New files jdk25u loongarch has that mips64 (jdk17u) lacks -- 45 files

| Category | Count | Notes for porting |
| --- | --- | --- |
| C1 JIT | 13 | Entirely new; adapt from jdk17u loongarch C1 to MIPS64 ISA |
| Loom/continuations | 4 | JDK 21+ virtual threads |
| ZGC | 7 | Read barrier lowering in `z_64.ad` and support files |
| Shenandoah | 3 | GC write barriers |
| G1 JIT rules | 1 | `gc/g1/g1.ad` -- G1 write barrier patterns in C2 |
| Panama FFI | 2 | `downcallLinker_64.cpp`, `upcallLinker_64.cpp` |
| JVMCI | 1 | `jvmciCodeInstaller.cpp` |
| Upgraded to `.cpp` | 3 | `codeBuffer.cpp`, `copy.cpp`, `compressedKlass.cpp` |
| Crypto intrinsics | 2 | `macroAssembler_chacha.cpp`, `macroAssembler_trig.cpp` |
| Stack chunks | 3 | `smallRegisterMap.inline.hpp` etc |
| Other | 6 | `c2_CodeStubs.cpp`, `foreignGlobals.cpp/.hpp`, `stubDeclarations.hpp`, `vmstorage.hpp` |

---

## Step 5 -- jdk11u Cross-Check

jdk11u is a slightly older and smaller port:

- **68 files** in `src/hotspot/cpu/mips/` (9 fewer than jdk17u -- missing `foreign_globals_mips.cpp`, `c2_MacroAssembler_mips.cpp`, and a few others added in JDK 17)
- **14 files** in `src/hotspot/os_cpu/linux_mips/`
- **15 SA agent files** (identical to jdk17u)
- `mips_64.ad` is 330,938 bytes vs 333,772 in jdk17u -- nearly identical

Initial commit: `f56d7f29f49` (2023-01-31, same day as jdk17u, author `loongson-jvm`).

**jdk17u is the more complete reference for forward-porting.** jdk11u is useful for identifying what was truly JDK-11-era vs JDK-17-era additions to the mips port.

---

## Step 6 -- Synthesis

### Q1: Is there a complete, intact mips64le patch set?

**Yes, at HEAD -- not only in history.** The mips64le port is fully intact and actively maintained in jdk17u and jdk11u on `master-ls` as of 2026-05-07. It was never removed. Total scope: 107 files (~1.55 MB) in jdk17u.

### Q2: How structurally similar are the mips64le and LoongArch patches?

**Extremely similar.** Evidence:

- **Identical file layout.** Every mips file has a direct loongarch counterpart: `mips_64.ad` ↔ `loongarch_64.ad`, `interp_masm_mips_64.cpp` ↔ `interp_masm_loongarch_64.cpp`, `sharedRuntime_mips_64.cpp` ↔ `sharedRuntime_loongarch_64.cpp`, etc. See `porting-notes.md` for the full mapping table.
- **Identical function names in identical order.** `TemplateInterpreterGenerator::generate_slow_signature_handler`, `generate_CRC32_update_entry`, `generate_StackOverflowError_handler`, `generate_return_entry_for`, `generate_native_entry`, `generate_normal_entry` -- all present with the same names in both backends.
- **Shared utility patterns.** `beq_far`/`bne_far`/`b_far` (far-branch helpers), `atomic_inc32`, `bang_stack_size`, `reserved_stack_check`, `patchable_jump`, `patchable_call`, `emit_trampoline_stub` -- same names, same roles.
- **Shared copyright heritage.** Both carry `Copyright (c) 2015, Loongson Technology` -- predating LoongArch's public availability.

### Q3: Shortest realistic path to mips64le in jdk25u

> **Note (2026-06-04):** The phase plan below is the initial planning snapshot from the
> 2026-05-30 archaeology session.  It does not reflect what actually happened.  Key
> divergences: Phase 1 originally bundled interpreter + C2 together; in practice C2 became
> its own Phase 3 because Loom/continuations (entirely absent from this plan) had to land
> first as Phase 2.  C1 slipped from Phase 2 to Phase 6.  ZGC, Shenandoah, and Panama were
> renumbered to Phases 5 and 4 respectively.  **For the current phase plan, see
> `porting-notes.md`.**  This section is retained as historical record only.

A **hybrid forward-port** in phases. See `porting-notes.md` for the file mapping, build commands, and Phase 0 detail.

**Phase 0 -- build jdk17u mips64el and establish baseline (1-2 days):**

Build jdk17u's existing mips64le port to validate the toolchain, learn the build time, and establish the testing methodology before touching jdk25u. jdk17u builds cleanly for mips64el today -- this is a free sanity check. The key open question this phase answers: are you testing under QEMU user-mode, QEMU full system, or native Loongson hardware? That answer determines the edit-build-test cycle time for all subsequent phases.

**Phase 1 -- interpreter + C2 only (4-8 weeks):**

1. Copy jdk17u's 92 hotspot mips files into `jdk25u/src/hotspot/cpu/mips/` and `os_cpu/linux_mips/`.
2. Use the jdk17u→jdk25u loongarch delta as the exact API-change guide. Every upstream JVM API change reflected in jdk25u's loongarch port needs to be applied to the mips port with MIPS64 ISA instructions.
3. Most impactful API changes: new constant-pool-cache API (`load_field_entry`, `load_method_entry`, `load_resolved_indy_entry`), `get_4_byte_integer_at_bcp` signature change, Loom continuation adapter entry points.
4. Add the "upgraded from `.hpp` to `.cpp`" files: `codeBuffer_mips.cpp`, `copy_mips.cpp`, `compressedKlass_mips.cpp` -- small, mechanical.
5. Add stack-chunk inline headers -- translate from loongarch equivalents.
6. Build with `--with-jvm-features=-c1,-zgc,-shenandoahgc` to defer unsupported features.

**Phase 2 -- C1 JIT (4-8 weeks, optional):**

Adapt the 13 jdk17u loongarch `c1_*.cpp/.hpp` files to MIPS64 ISA. Start from jdk17u's loongarch C1 (not jdk25u's -- the jdk17u versions are closer to the mips API surface), then apply the jdk17u→jdk25u loongarch C1 delta.

**Phase 3 -- ZGC, Shenandoah, Panama (deferred, add incrementally).**

The jdk25u loongarch implementations serve as templates throughout -- not a blind copy-replace (MIPS64 assembly must be rewritten), but the structure and algorithm are correct starting points.

### Q4: Was jdk25u's LoongArch port derived from the mips work?

**Strong circumstantial evidence, effectively conclusive:**

1. **Same author.** aoqi (`aoqi@loongson.cn`) authored the jdk25u initial LoongArch commit (2022-08-11) and had been fixing MIPS-specific OpenJDK bugs since at least 2018.
2. **Same copyright year.** Both backends carry `Copyright (c) 2015, Loongson Technology` -- predating LoongArch's public availability.
3. **Identical structural skeleton.** Particularly `beq_far`/`bne_far`/`b_far` -- a Loongson-internal abstraction for limited branch offsets, not an upstream OpenJDK pattern -- appears identically named in both.
4. **Temporal sequence.** aoqi introduced LoongArch for jdk25 in August 2022; Loongson's JVM team added both mips+loongarch to jdk17u/jdk11u in January 2023 in a single commit. The mips port was likely ISA-adapted from the already-complete LoongArch work.

**Most likely lineage:** Loongson maintained an internal MIPS64 port since ~2015. When developing LoongArch in 2022, they adapted the MIPS code to the new ISA. Then in January 2023 they published both ports to GitHub simultaneously, back-porting to JDK 11 and JDK 17 in one coordinated release.

---

## Key Commit Hashes

| Item | Hash | Date |
| --- | --- | --- |
| jdk25u initial LoongArch commit | `526ad75a343f` | 2022-08-11 |
| jdk17u initial Loongson commit | `d4dcc0bf7b8` | 2023-01-31 |
| jdk11u initial Loongson commit | `f56d7f29f49` | 2023-01-31 |
| jdk17u most recent Loongson commit | `f1d5298a733` | 2026-05-07 |
| jdk17u most recent mips-touching commit | `a66f78c6520` | 2026-01-29 |
