/**
 * Phase 2 hardware validation test: virtual thread freeze/thaw.
 *
 * Run: java Phase2Test.java
 * Expected: all lines print, exit 0.
 *
 * Tests the continuation freeze/thaw path that crashes under QEMU
 * but may work on real Loongson-3 hardware.
 */
public class Phase2Test {
    static int passed = 0;

    // Test 1: single VT with explicit yield
    static void test1() throws Exception {
        Thread t = Thread.ofVirtual().start(() -> {
            Thread.yield();
        });
        t.join();
        System.out.println("pass 1: single VT yield");
        passed++;
    }

    // Test 2: yield with work before and after
    static void test2() throws Exception {
        int[] result = {0};
        Thread t = Thread.ofVirtual().start(() -> {
            result[0] = 1;
            Thread.yield();
            result[0] = 2;
        });
        t.join();
        if (result[0] != 2) throw new AssertionError("expected 2 got " + result[0]);
        System.out.println("pass 2: yield resumes correctly, result=" + result[0]);
        passed++;
    }

    // Test 3: multiple yields in sequence
    static void test3() throws Exception {
        int[] counter = {0};
        Thread t = Thread.ofVirtual().start(() -> {
            for (int i = 0; i < 3; i++) {
                counter[0]++;
                Thread.yield();
            }
        });
        t.join();
        if (counter[0] != 3) throw new AssertionError("expected 3 got " + counter[0]);
        System.out.println("pass 3: multiple yields, counter=" + counter[0]);
        passed++;
    }

    // Test 4: LockSupport.parkNanos (timed park, triggers real unmount)
    static void test4() throws Exception {
        long[] elapsed = {0};
        Thread t = Thread.ofVirtual().start(() -> {
            long start = System.nanoTime();
            java.util.concurrent.locks.LockSupport.parkNanos(1_000_000L); // 1ms
            elapsed[0] = System.nanoTime() - start;
        });
        t.join();
        System.out.println("pass 4: parkNanos completed, elapsed=" + elapsed[0] + "ns");
        passed++;
    }

    // Test 5: concurrent VTs each yielding
    static void test5() throws Exception {
        int N = 5;
        Thread[] threads = new Thread[N];
        int[] results = new int[N];
        for (int i = 0; i < N; i++) {
            final int id = i;
            threads[i] = Thread.ofVirtual().start(() -> {
                Thread.yield();
                results[id] = id + 1;
            });
        }
        for (Thread t : threads) t.join();
        int sum = 0;
        for (int r : results) sum += r;
        if (sum != N * (N + 1) / 2) throw new AssertionError("wrong sum: " + sum);
        System.out.println("pass 5: " + N + " concurrent yielding VTs, sum=" + sum);
        passed++;
    }

    public static void main(String[] args) throws Exception {
        System.out.println("Phase 2 freeze/thaw validation");
        test1();
        test2();
        test3();
        test4();
        test5();
        System.out.println("all " + passed + " tests passed");
    }
}
