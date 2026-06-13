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

package jdk.internal.foreign.abi.mips64.linux;

import java.lang.foreign.GroupLayout;
import java.lang.foreign.MemoryLayout;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.PaddingLayout;
import java.lang.foreign.SequenceLayout;
import java.lang.foreign.ValueLayout;

/**
 * MIPS N64 ABI type classification.
 *
 * MIPS N64 rules:
 *  - INTEGER: integer/pointer scalar types
 *  - FLOAT: float/double scalar types
 *  - POINTER: address types
 *  - STRUCT_REGISTER_X: aggregates ≤ 16 bytes, passed in up to 2 integer registers
 *  - STRUCT_REFERENCE: aggregates > 16 bytes, passed by reference (pointer in an integer register)
 *
 * Note: unlike RISC-V, MIPS N64 does NOT pass struct float fields in float registers.
 */
public enum TypeClass {
    INTEGER,
    FLOAT,
    POINTER,
    STRUCT_REGISTER_X,
    STRUCT_REFERENCE;

    private static final int MAX_AGGREGATE_REGS_SIZE = 2; // MIPS GRLEN=64, max 2 * 8 = 16 bytes in registers

    private static TypeClass classifyValueType(ValueLayout type) {
        Class<?> carrier = type.carrier();
        if (carrier == boolean.class || carrier == byte.class || carrier == char.class ||
            carrier == short.class || carrier == int.class || carrier == long.class) {
            return INTEGER;
        } else if (carrier == float.class || carrier == double.class) {
            return FLOAT;
        } else if (carrier == MemorySegment.class) {
            return POINTER;
        } else {
            throw new IllegalStateException("Cannot get here: " + carrier.getName());
        }
    }

    private static boolean isRegisterAggregate(MemoryLayout type) {
        return type.byteSize() <= MAX_AGGREGATE_REGS_SIZE * 8;
    }

    private static TypeClass classifyStructType(GroupLayout layout) {
        // MIPS N64: all structs ≤ 16 bytes go in integer registers; larger go by reference
        return isRegisterAggregate(layout) ? STRUCT_REGISTER_X : STRUCT_REFERENCE;
    }

    public static TypeClass classifyLayout(MemoryLayout type) {
        if (type instanceof ValueLayout vt) {
            return classifyValueType(vt);
        } else if (type instanceof GroupLayout gt) {
            return classifyStructType(gt);
        } else {
            throw new IllegalArgumentException("Unsupported layout: " + type);
        }
    }
}
