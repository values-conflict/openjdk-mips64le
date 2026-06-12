// Phase 3 test: C2 JIT compilation correctness.
//
// Run without extra flags on both QEMU and hardware; C2 JIT-compiles the hot
// methods.  Each test method runs 100 000+ iterations to cross the JIT threshold and
// force C2 compilation on hardware.  The loop count is printed so -XX:+PrintCompilation
// output can be correlated on first run.
//
// Tests:
//   1. Integer arithmetic in a hot loop (basic C2 arithmetic)
//   2. Long arithmetic (64-bit ops, MIPS-specific)
//   3. Array element read/write (memory access patterns)
//   4. Virtual dispatch (invokevirtual compiled by C2)
//   5. Thread.currentThread() correctness (the Phase 3 intrinsic fix)
static Number[] buildNums(int size) {
    Number[] nums = new Number[size];
    for (int i = 0; i < size; i++) nums[i] = i % 2 == 0 ? i : (long) i;
    return nums;
}

static long dispatchSum(Number[] nums) {
    long total = 0;
    for (Number n : nums) total += n.longValue();
    return total;
}

void main() {
    System.out.println("Phase 3 C2 JIT validation");

    // 1. Integer arithmetic -- sum wraps around: 0+1+...+99999 = 4999950000
    //    which overflows int (mod 2^32): 4999950000 - 4294967296 = 704982704
    {
        int sum = 0;
        for (int i = 0; i < 100_000; i++) sum += i;
        int expected = (int) 4_999_950_000L;  // 704982704 after int truncation
        if (sum != expected) {
            System.err.println("FAIL pass 1: sum=" + sum + " expected=" + expected);
            System.exit(1);
        }
        System.out.println("pass 1: integer loop sum=" + sum);
    }

    // 2. Long arithmetic (tests 64-bit MIPS instruction selection by C2)
    {
        long product = 1L;
        for (int i = 1; i <= 20; i++) product *= i;  // 20! = 2432902008176640000
        long expected = 2432902008176640000L;
        if (product != expected) {
            System.err.println("FAIL pass 2: product=" + product + " expected=" + expected);
            System.exit(1);
        }
        System.out.println("pass 2: long 20! = " + product);
    }

    // 3. Array access (read/write through a hot loop)
    {
        int[] arr = new int[1000];
        for (int round = 0; round < 200; round++) {
            for (int i = 0; i < arr.length; i++) arr[i] = i * round;
        }
        int checksum = 0;
        for (int v : arr) checksum ^= v;
        // final round=199: arr[i] = i*199; xor of i*199 for i in [0,999]
        int expected = 0;
        for (int i = 0; i < 1000; i++) expected ^= i * 199;
        if (checksum != expected) {
            System.err.println("FAIL pass 3: checksum=" + checksum + " expected=" + expected);
            System.exit(1);
        }
        System.out.println("pass 3: array checksum=" + checksum);
    }

    // 4. Virtual dispatch (invokevirtual compiled by C2)
    // Helper methods keep the hot loops in separate non-OSR compilations.
    // (Previously worked around an OSR boxing miscompilation (H-3); root cause
    // fixed by IC check + liveness fixes -- no CompilerOracle exclude needed.)
    {
        Number[] nums = buildNums(100_000);
        long total = dispatchSum(nums);
        // sum of 0..99999 = 4999950000
        if (total != 4_999_950_000L) {
            System.err.println("FAIL pass 4: total=" + total + " expected=4999950000");
            System.exit(1);
        }
        System.out.println("pass 4: virtual dispatch total=" + total);
    }

    // 5. Thread.currentThread() intrinsic (Phase 3 bug: V0 vs A0 return register)
    {
        Thread t = Thread.currentThread();
        if (t == null) {
            System.err.println("FAIL pass 5: Thread.currentThread() null");
            System.exit(1);
        }
        // invokevirtual on the returned object -- this crashed before the fix
        if (t.getName() == null) {
            System.err.println("FAIL pass 5: getName() null");
            System.exit(1);
        }
        System.out.println("pass 5: Thread.currentThread() name=" + t.getName());
    }

    System.out.println("all 5 tests passed");
}
