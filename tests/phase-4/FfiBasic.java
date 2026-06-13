import java.lang.foreign.*;
import java.lang.invoke.*;

/**
 * Phase 4 Panama FFI test: verify that the native linker works on MIPS64el.
 * Tests: linker acquisition, strlen downcall, and basic argument/return handling.
 * Run with: java tests/phase-4/FfiBasic.java
 */
public class FfiBasic {
    public static void main(String[] args) throws Throwable {
        // Test 1: verify ForeignGlobals::is_foreign_linker_supported() returns true
        Linker linker = Linker.nativeLinker();
        System.out.println("linker: " + linker.getClass().getSimpleName());

        // Test 2: downcall to strlen via Panama FFI
        SymbolLookup lookup = Linker.nativeLinker().defaultLookup();
        MethodHandle strlen = linker.downcallHandle(
            lookup.find("strlen").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.JAVA_LONG, ValueLayout.ADDRESS)
        );

        try (Arena arena = Arena.ofConfined()) {
            MemorySegment hello = arena.allocateFrom("hello");
            long len = (long) strlen.invoke(hello);
            if (len != 5) {
                throw new AssertionError("strlen(\"hello\") expected 5, got " + len);
            }
            System.out.println("strlen(\"hello\") = " + len + " (correct)");

            MemorySegment empty = arena.allocateFrom("");
            long emptyLen = (long) strlen.invoke(empty);
            if (emptyLen != 0) {
                throw new AssertionError("strlen(\"\") expected 0, got " + emptyLen);
            }
            System.out.println("strlen(\"\") = " + emptyLen + " (correct)");
        }

        // Test 3: downcall to abs (integer argument and return)
        MethodHandle abs = linker.downcallHandle(
            lookup.find("abs").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.JAVA_INT)
        );
        int absResult = (int) abs.invoke(-42);
        if (absResult != 42) {
            throw new AssertionError("abs(-42) expected 42, got " + absResult);
        }
        System.out.println("abs(-42) = " + absResult + " (correct)");

        System.out.println("Phase 4 FFI: PASS");
    }
}
