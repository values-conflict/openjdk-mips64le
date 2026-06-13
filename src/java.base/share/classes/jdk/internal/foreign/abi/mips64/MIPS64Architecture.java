/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2021, 2025, Loongson Technology. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package jdk.internal.foreign.abi.mips64;

import jdk.internal.foreign.abi.ABIDescriptor;
import jdk.internal.foreign.abi.Architecture;
import jdk.internal.foreign.abi.StubLocations;
import jdk.internal.foreign.abi.VMStorage;

/**
 * MIPS64 N64 ABI register definitions.
 *
 * Register file (32 GPRs + 32 FPRs):
 *  - GPR indices 0-31 match the MIPS register numbers ($0-$31)
 *  - FPR indices 0-31 match the MIPS FP register numbers ($f0-$f31)
 */
public final class MIPS64Architecture implements Architecture {
    public static final Architecture INSTANCE = new MIPS64Architecture();

    private static final short REG64_MASK   = 0b0000_0000_0000_0001;
    private static final short FLOAT64_MASK = 0b0000_0000_0000_0001;

    private static final int INTEGER_REG_SIZE = 8; // bytes
    private static final int FLOAT_REG_SIZE   = 8;

    private MIPS64Architecture() {}

    @Override
    public boolean isStackType(int cls) {
        return cls == StorageType.STACK;
    }

    @Override
    public int typeSize(int cls) {
        return switch (cls) {
            case StorageType.INTEGER -> INTEGER_REG_SIZE;
            case StorageType.FLOAT   -> FLOAT_REG_SIZE;
            default -> throw new IllegalArgumentException("Invalid Storage Class: " + cls);
        };
    }

    public interface StorageType {
        byte INTEGER     = 0;
        byte FLOAT       = 1;
        byte STACK       = 2;
        byte PLACEHOLDER = 3;
    }

    public static class Regs {
        // Integer registers ($0-$31)
        public static final VMStorage r0  = integerRegister( 0, "zero");
        public static final VMStorage r1  = integerRegister( 1, "at");
        public static final VMStorage r2  = integerRegister( 2, "v0");
        public static final VMStorage r3  = integerRegister( 3, "v1");
        public static final VMStorage r4  = integerRegister( 4, "a0");
        public static final VMStorage r5  = integerRegister( 5, "a1");
        public static final VMStorage r6  = integerRegister( 6, "a2");
        public static final VMStorage r7  = integerRegister( 7, "a3");
        public static final VMStorage r8  = integerRegister( 8, "a4");
        public static final VMStorage r9  = integerRegister( 9, "a5");
        public static final VMStorage r10 = integerRegister(10, "a6");
        public static final VMStorage r11 = integerRegister(11, "a7");
        public static final VMStorage r12 = integerRegister(12, "t0");
        public static final VMStorage r13 = integerRegister(13, "t1");
        public static final VMStorage r14 = integerRegister(14, "t2");
        public static final VMStorage r15 = integerRegister(15, "t3");
        public static final VMStorage r16 = integerRegister(16, "s0");
        public static final VMStorage r17 = integerRegister(17, "s1");
        public static final VMStorage r18 = integerRegister(18, "s2");
        public static final VMStorage r19 = integerRegister(19, "s3");
        public static final VMStorage r20 = integerRegister(20, "s4");
        public static final VMStorage r21 = integerRegister(21, "s5");
        public static final VMStorage r22 = integerRegister(22, "s6");
        public static final VMStorage r23 = integerRegister(23, "s7");
        public static final VMStorage r24 = integerRegister(24, "t8");
        public static final VMStorage r25 = integerRegister(25, "t9");
        public static final VMStorage r28 = integerRegister(28, "gp");
        public static final VMStorage r29 = integerRegister(29, "sp");
        public static final VMStorage r30 = integerRegister(30, "fp");
        public static final VMStorage r31 = integerRegister(31, "ra");

        // Float registers ($f0-$f31)
        public static final VMStorage f0  = floatRegister( 0, "f0");
        public static final VMStorage f1  = floatRegister( 1, "f1");
        public static final VMStorage f2  = floatRegister( 2, "f2");
        public static final VMStorage f3  = floatRegister( 3, "f3");
        public static final VMStorage f4  = floatRegister( 4, "f4");
        public static final VMStorage f5  = floatRegister( 5, "f5");
        public static final VMStorage f6  = floatRegister( 6, "f6");
        public static final VMStorage f7  = floatRegister( 7, "f7");
        public static final VMStorage f8  = floatRegister( 8, "f8");
        public static final VMStorage f9  = floatRegister( 9, "f9");
        public static final VMStorage f10 = floatRegister(10, "f10");
        public static final VMStorage f11 = floatRegister(11, "f11");
        public static final VMStorage f12 = floatRegister(12, "f12");
        public static final VMStorage f13 = floatRegister(13, "f13");
        public static final VMStorage f14 = floatRegister(14, "f14");
        public static final VMStorage f15 = floatRegister(15, "f15");
        public static final VMStorage f16 = floatRegister(16, "f16");
        public static final VMStorage f17 = floatRegister(17, "f17");
        public static final VMStorage f18 = floatRegister(18, "f18");
        public static final VMStorage f19 = floatRegister(19, "f19");
        public static final VMStorage f20 = floatRegister(20, "f20");
        public static final VMStorage f21 = floatRegister(21, "f21");
        public static final VMStorage f22 = floatRegister(22, "f22");
        public static final VMStorage f23 = floatRegister(23, "f23");
        public static final VMStorage f24 = floatRegister(24, "f24");
        public static final VMStorage f25 = floatRegister(25, "f25");
        public static final VMStorage f26 = floatRegister(26, "f26");
        public static final VMStorage f27 = floatRegister(27, "f27");
        public static final VMStorage f28 = floatRegister(28, "f28");
        public static final VMStorage f29 = floatRegister(29, "f29");
        public static final VMStorage f30 = floatRegister(30, "f30");
        public static final VMStorage f31 = floatRegister(31, "f31");
    }

    private static VMStorage integerRegister(int index, String debugName) {
        return new VMStorage(StorageType.INTEGER, REG64_MASK, index, debugName);
    }

    private static VMStorage floatRegister(int index, String debugName) {
        return new VMStorage(StorageType.FLOAT, FLOAT64_MASK, index, debugName);
    }

    public static VMStorage stackStorage(short size, int byteOffset) {
        return new VMStorage(StorageType.STACK, size, byteOffset);
    }

    public static ABIDescriptor abiFor(VMStorage[] inputIntRegs,
                                       VMStorage[] inputFloatRegs,
                                       VMStorage[] outputIntRegs,
                                       VMStorage[] outputFloatRegs,
                                       VMStorage[] volatileIntRegs,
                                       VMStorage[] volatileFloatRegs,
                                       int stackAlignment,
                                       int shadowSpace,
                                       VMStorage scratch1, VMStorage scratch2) {
        return new ABIDescriptor(
            INSTANCE,
            new VMStorage[][]{
                inputIntRegs,
                inputFloatRegs,
            },
            new VMStorage[][]{
                outputIntRegs,
                outputFloatRegs,
            },
            new VMStorage[][]{
                volatileIntRegs,
                volatileFloatRegs,
            },
            stackAlignment,
            shadowSpace,
            scratch1, scratch2,
            StubLocations.TARGET_ADDRESS.storage(StorageType.PLACEHOLDER),
            StubLocations.RETURN_BUFFER.storage(StorageType.PLACEHOLDER),
            StubLocations.CAPTURED_STATE_BUFFER.storage(StorageType.PLACEHOLDER));
    }
}
