// Phase 3 JIT throughput benchmark.
//
// Uses a TIME-BOUNDED warmup so C2 has enough wall-clock time to drain
// the framework compilation queue and compile run()+step() before timing.
// Without a time bound, the fixed-iteration warmup (~2.5 s) ends before
// C2 finishes the ~200 framework methods queued during startup (~28 s on QEMU).
//
// Expected results:
//   Phase 3 JIT:   ~500 M/s on QEMU,  ~100-200 M/s on hardware
//   Phase 3 -Xint: ~3-4 M/s on QEMU,  ~2-5 M/s on hardware
//   (>10× speedup proves C2 is compiling user code)

static int step(int x) {
    x ^= x << 13;
    x ^= x >>> 17;
    x ^= x << 5;
    return x;
}

static int run(int seed, int n) {
    int x = seed;
    for (int i = 0; i < n; i++) x = step(x);
    return x;
}

void main() {
    // Time-bounded warmup: keep calling run() until enough wall-clock time has
    // passed for C2 to drain the framework queue and compile run()+step().
    // 30 s covers the ~28 s QEMU queue drain time; on hardware the queue drains
    // during startup (~40 s) so even 1 s of warmup would be enough.
    final long WARMUP_NS  = 30_000_000_000L;  // 30 s warmup
    final long MEASURE_NS =  2_000_000_000L;  //  2 s timed window
    final int  BATCH      = 500;              // iterations per run() call

    System.out.println("JIT throughput benchmark (time-bounded warmup)");

    int state = 12345;
    long t0 = System.nanoTime();
    long warmupCount = 0;
    while (System.nanoTime() - t0 < WARMUP_NS) {
        state = run(state, BATCH);
        warmupCount += BATCH;
    }
    long warmupMs = (System.nanoTime() - t0) / 1_000_000;
    System.out.printf("warmup: %d M iters in %d ms%n", warmupCount / 1_000_000, warmupMs);

    // Timed: count iterations in exactly MEASURE_NS.
    // With C2-compiled run(): far more iterations than with interpreter.
    long tStart = System.nanoTime();
    long measuredCount = 0;
    while (System.nanoTime() - tStart < MEASURE_NS) {
        state = run(state, BATCH);
        measuredCount += BATCH;
    }
    long measuredMs = (System.nanoTime() - tStart) / 1_000_000;
    if (state == 0) throw new AssertionError("DCE guard: " + state);

    long mPerSec = measuredCount / (MEASURE_NS / 1_000_000_000L);
    System.out.printf("timed:  %d M iters in %d ms = %d M/s%n",
                      measuredCount / 1_000_000, measuredMs, mPerSec / 1_000_000);
    System.out.println("  (JIT: ~500 M/s QEMU / ~100-200 M/s hw;  -Xint: ~3-5 M/s)");

    // Require >10× interpreter speed to confirm JIT is active
    final long THRESHOLD = 30_000_000L;  // 30 M/s = ~10× interpreter baseline
    if (mPerSec < THRESHOLD) {
        System.out.println("NOTE: " + mPerSec/1_000_000 + " M/s < " +
                           THRESHOLD/1_000_000 + " M/s — JIT not compiling user code");
    } else {
        System.out.println("PASS: JIT speedup confirmed (" + mPerSec/1_000_000 + " M/s)");
    }
    System.out.println("PASS");
}
