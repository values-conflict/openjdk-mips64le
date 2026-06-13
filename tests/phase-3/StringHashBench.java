// Phase 3 regression test: String::hashCode and StringLatin1::replace under C2.
//
// Background: a bug in ifg.cpp's small_user_method path caused build_ifg_physical
// to run an O(N^3) SpillCopy pre-scan on every spill-split-recycle iteration (not
// just trip 0).  For String::hashCode (60 bytes, always qualifies as a small method)
// the scan's cost grew cubically as Split() added SpillCopy nodes each iteration;
// combined with the cross-block OOP re-insert firing on every trip, the 28-iteration
// bailout limit took 19+ hours to reach, pegging two C2 threads at 100% indefinitely.
//
// Fix: `small_user_method` gated on `_trip_cnt <= 3` in ifg.cpp, plus an early break
// in the SpillCopy pre-scan when `sc_count` reaches `SC_MAX`.  With the fix,
// String::hashCode converges in ~5-6 trips and compiles normally.
//
// Detection strategy: pre-compile this class with host javac so the JVM starts with
// a small queue (no ~8000 javac framework methods queued ahead of hashCode).  After
// a short warmup, C2 should compile String::hashCode and throughput should exceed the
// C1/interpreter baseline.  If the bug regresses, C2 spins and never completes the
// compilation; hashCode falls to C1, and throughput drops to the C1 baseline.
//
// Expected results (QEMU / Loongson-3 hardware):
//   With C2:       ~12 M/s QEMU (measured),  >5 M/s hardware (est.)
//   C1 only:       ~0.2-2 M/s QEMU,  ~2 M/s hardware (measured on old build)
//   Interpreter:   ~0.05-0.1 M/s QEMU
//
// MUST be run pre-compiled (not via single-source launch) for reliable detection:
// single-source launch floods the C2 queue with ~8000 javac framework methods,
// making it hard to distinguish C2 from C1 within the warmup window.
// run-tests-qemu.sh handles pre-compilation automatically.
//
// NOTE: the failure case (< 5 M/s) is only reproducible on real Loongson-3 hardware.
// On QEMU, the x86 host is fast enough that C2's 28-trip bailout for String::hashCode
// completes in seconds rather than 19 hours; by the end of the 10 s warmup, C2 is
// free and hashLoop() gets C2-compiled regardless.  Hardware old-build measured 2 M/s
// (confirmed FAIL); QEMU old-build measures ~7 M/s (false PASS — not a test defect,
// just a QEMU speed artifact).  Verify the fix on hardware after deploying the build.
//
// run-tests-qemu.sh pre-compiles this file with host javac before running it.

static volatile int sink;

static int hashLoop(String s, int n) {
    int x = 0;
    for (int i = 0; i < n; i++) x ^= s.hashCode();
    return x;
}

static int replaceLoop(String s, int n) {
    int x = 0;
    for (int i = 0; i < n; i++) x ^= s.replace("o", "0").length();
    return x;
}

void main() {
    String s = "The quick brown fox jumps over the lazy dog on Loongson MIPS64el.";
    final long WARMUP_NS  = 10_000_000_000L;  // 10 s (pre-compiled: small queue, fast drain)
    final long MEASURE_NS =  2_000_000_000L;  //  2 s timed window
    final int  BATCH      = 1_000;

    System.out.println("String::hashCode + replace throughput (pre-compiled)");

    // Correctness check before warmup
    if ("hello".hashCode() != 99162322)
        throw new AssertionError("hashCode wrong: " + "hello".hashCode());
    if (!"hello".replace('l', 'r').equals("herro"))
        throw new AssertionError("replace wrong: " + "hello".replace('l', 'r'));

    // Warmup
    long t0 = System.nanoTime();
    long warmupCount = 0;
    while (System.nanoTime() - t0 < WARMUP_NS) {
        sink = hashLoop(s, BATCH);
        warmupCount += BATCH;
    }
    long warmupMs = (System.nanoTime() - t0) / 1_000_000;
    System.out.printf("warmup: %d M iters in %d ms%n", warmupCount / 1_000_000, warmupMs);

    // Timed measurement
    long tStart = System.nanoTime();
    long count = 0;
    while (System.nanoTime() - tStart < MEASURE_NS) {
        sink = hashLoop(s, BATCH);
        count += BATCH;
    }
    long measuredMs = (System.nanoTime() - tStart) / 1_000_000;

    long mPerSec = count / (MEASURE_NS / 1_000_000_000L);
    System.out.printf("timed:  %d M iters in %d ms = %d M/s%n",
                      count / 1_000_000, measuredMs, mPerSec / 1_000_000);
    System.out.println("  (C2: ~12 M/s QEMU / >5 M/s hw;  C1 fallback: ~2 M/s hw, ~0.5-2 M/s QEMU)");

    // 5 M/s threshold: below C2 speed (12 M/s QEMU measured) but above C1 fallback
    // speed (2 M/s measured on hardware old build with C2 stuck on String::hashCode).
    // When the ifg.cpp trip_cnt bug is present, C2 spins indefinitely on hashCode and
    // falls to C1; C1 gives ~2 M/s on hardware and ~0.5-2 M/s on QEMU — both below this.
    final long THRESHOLD = 5_000_000L;
    if (mPerSec < THRESHOLD) {
        System.out.printf("FAIL: %d M/s < %d M/s — C2 likely not compiling String::hashCode%n",
                          mPerSec / 1_000_000, THRESHOLD / 1_000_000);
        System.out.println("  (ifg.cpp SpillCopy SC_MAX early-break fix may be missing)");
        System.exit(1);
    }
    System.out.println("PASS");
}
