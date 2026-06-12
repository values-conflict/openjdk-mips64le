// Phase 3 test: Thread.currentThread() interpreter intrinsic.
//
// This exercises the generate_currentThread() entry added in Phase 3.  The
// bug was that the result was stored in A0 (LoongArch convention) rather than
// V0 (MIPS atos return register = FSR), causing Thread.<init> to call
// invokevirtual on a garbage receiver (V0 = 2 from previous junk) and crash
// with klass decode → 0x10.
//
// The fix: load the vthread oop into V0, not A0, before returning.
//
// Validates: generate_currentThread() returns the correct Thread object.
void main() {
    Thread t = Thread.currentThread();
    if (t == null) {
        System.err.println("FAIL: Thread.currentThread() returned null");
        System.exit(1);
    }
    // Calling a method on t exercises invokevirtual on the returned object --
    // this is exactly the call that crashed before the fix.
    String name = t.getName();
    if (name == null) {
        System.err.println("FAIL: currentThread().getName() returned null");
        System.exit(1);
    }
    System.out.println("ok: currentThread name=" + name);
}
