/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates. All rights reserved.
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

import jdk.internal.foreign.Utils;
import jdk.internal.foreign.abi.ABIDescriptor;
import jdk.internal.foreign.abi.AbstractLinker.UpcallStubFactory;
import jdk.internal.foreign.abi.Binding;
import jdk.internal.foreign.abi.CallingSequence;
import jdk.internal.foreign.abi.CallingSequenceBuilder;
import jdk.internal.foreign.abi.DowncallLinker;
import jdk.internal.foreign.abi.LinkerOptions;
import jdk.internal.foreign.abi.SharedUtils;
import jdk.internal.foreign.abi.VMStorage;

import java.lang.foreign.AddressLayout;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.GroupLayout;
import java.lang.foreign.MemoryLayout;
import java.lang.foreign.MemorySegment;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodType;
import java.util.List;
import java.util.Optional;

import static jdk.internal.foreign.abi.mips64.MIPS64Architecture.Regs.*;
import static jdk.internal.foreign.abi.mips64.MIPS64Architecture.*;
import static jdk.internal.foreign.abi.mips64.linux.TypeClass.*;

/**
 * MIPS64 N64 ABI calling convention implementation.
 *
 * N64 summary:
 *  - 8 integer argument registers: a0-a7 (r4-r11)
 *  - 8 float argument registers: f12-f19
 *  - Integer return: v0 (r2), v1 (r3) for large integers
 *  - Float return: f0
 *  - Stack alignment: 16 bytes
 *  - No shadow space
 */
public class LinuxMIPS64CallArranger {
    private static final int STACK_SLOT_SIZE = 8;
    public static final int MAX_REGISTER_ARGUMENTS = 8;

    private static final ABIDescriptor CLinux = abiFor(
            // input integer: a0-a7 (r4-r11)
            new VMStorage[]{r4, r5, r6, r7, r8, r9, r10, r11},
            // input float: f12-f19
            new VMStorage[]{f12, f13, f14, f15, f16, f17, f18, f19},
            // output integer: v0 (r2), v1 (r3)
            new VMStorage[]{r2, r3},
            // output float: f0
            new VMStorage[]{f0},
            // volatile integer (caller-save, not input/output): at(r1), t0-t3(r12-r15), t8(r24), t9(r25)
            new VMStorage[]{r1, r12, r13, r14, r15, r24, r25},
            // volatile float (caller-save, not input): f1-f11, f20-f23
            new VMStorage[]{f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11,
                            f20, f21, f22, f23},
            16, // stackAlignment
            0,  // no shadow space
            r25, r24 // scratch1=t9, scratch2=t8
    );

    public record Bindings(CallingSequence callingSequence,
                           boolean isInMemoryReturn) {
    }

    public static Bindings getBindings(MethodType mt, FunctionDescriptor cDesc, boolean forUpcall) {
        return getBindings(mt, cDesc, forUpcall, LinkerOptions.empty());
    }

    public static Bindings getBindings(MethodType mt, FunctionDescriptor cDesc, boolean forUpcall, LinkerOptions options) {
        CallingSequenceBuilder csb = new CallingSequenceBuilder(CLinux, forUpcall, options);
        BindingCalculator argCalc = forUpcall ? new BoxBindingCalculator(true) : new UnboxBindingCalculator(true, options.allowsHeapAccess());
        BindingCalculator retCalc = forUpcall ? new UnboxBindingCalculator(false, false) : new BoxBindingCalculator(false);

        boolean returnInMemory = isInMemoryReturn(cDesc.returnLayout());
        if (returnInMemory) {
            Class<?> carrier = MemorySegment.class;
            MemoryLayout layout = SharedUtils.C_POINTER;
            csb.addArgumentBindings(carrier, layout, argCalc.getBindings(carrier, layout, false));
        } else if (cDesc.returnLayout().isPresent()) {
            Class<?> carrier = mt.returnType();
            MemoryLayout layout = cDesc.returnLayout().get();
            csb.setReturnBindings(carrier, layout, retCalc.getBindings(carrier, layout, false));
        }

        for (int i = 0; i < mt.parameterCount(); i++) {
            Class<?> carrier = mt.parameterType(i);
            MemoryLayout layout = cDesc.argumentLayouts().get(i);
            boolean isVar = options.isVarargsIndex(i);
            csb.addArgumentBindings(carrier, layout, argCalc.getBindings(carrier, layout, isVar));
        }

        return new Bindings(csb.build(), returnInMemory);
    }

    public static MethodHandle arrangeDowncall(MethodType mt, FunctionDescriptor cDesc, LinkerOptions options) {
        Bindings bindings = getBindings(mt, cDesc, false, options);

        MethodHandle handle = new DowncallLinker(CLinux, bindings.callingSequence).getBoundMethodHandle();

        if (bindings.isInMemoryReturn) {
            handle = SharedUtils.adaptDowncallForIMR(handle, cDesc, bindings.callingSequence);
        }

        return handle;
    }

    public static UpcallStubFactory arrangeUpcall(MethodType mt, FunctionDescriptor cDesc, LinkerOptions options) {
        Bindings bindings = getBindings(mt, cDesc, true, options);
        final boolean dropReturn = true;
        return SharedUtils.arrangeUpcallHelper(mt, bindings.isInMemoryReturn, dropReturn, CLinux,
                bindings.callingSequence);
    }

    private static boolean isInMemoryReturn(Optional<MemoryLayout> returnLayout) {
        return returnLayout
                .filter(GroupLayout.class::isInstance)
                .filter(g -> TypeClass.classifyLayout(g) == TypeClass.STRUCT_REFERENCE)
                .isPresent();
    }

    static class StorageCalculator {
        private final boolean forArguments;
        private final int IntegerRegIdx = 0;
        private final int FloatRegIdx = 1;
        private final int[] nRegs = {0, 0};

        private long stackOffset = 0;

        public StorageCalculator(boolean forArguments) {
            this.forArguments = forArguments;
        }

        void alignStack(long alignment) {
            alignment = Utils.alignUp(Math.clamp(alignment, STACK_SLOT_SIZE, 16), STACK_SLOT_SIZE);
            stackOffset = Utils.alignUp(stackOffset, alignment);
        }

        VMStorage stackAlloc() {
            assert forArguments : "no stack returns";
            VMStorage storage = stackStorage((short) STACK_SLOT_SIZE, (int) stackOffset);
            stackOffset += STACK_SLOT_SIZE;
            return storage;
        }

        Optional<VMStorage> regAlloc(int storageClass) {
            if (nRegs[storageClass] < MAX_REGISTER_ARGUMENTS) {
                VMStorage[] source = (forArguments ? CLinux.inputStorage : CLinux.outputStorage)[storageClass];
                Optional<VMStorage> result = Optional.of(source[nRegs[storageClass]]);
                nRegs[storageClass] += 1;
                return result;
            }
            return Optional.empty();
        }

        VMStorage getStorage(int storageClass) {
            Optional<VMStorage> storage = regAlloc(storageClass);
            if (storage.isPresent()) {
                return storage.get();
            }
            // MIPS N64: no float-to-integer fallback; go straight to stack
            return stackAlloc();
        }

        VMStorage[] getStorages(MemoryLayout layout) {
            int regCnt = (int) SharedUtils.alignUp(layout.byteSize(), 8) / 8;
            VMStorage[] storages = new VMStorage[regCnt];
            for (int i = 0; i < regCnt; i++) {
                storages[i] = getStorage(StorageType.INTEGER);
            }
            return storages;
        }

        boolean regsAvailable(int integerRegs) {
            return nRegs[IntegerRegIdx] + integerRegs <= MAX_REGISTER_ARGUMENTS;
        }
    }

    abstract static class BindingCalculator {
        protected final StorageCalculator storageCalculator;

        protected BindingCalculator(boolean forArguments) {
            this.storageCalculator = new StorageCalculator(forArguments);
        }

        abstract List<Binding> getBindings(Class<?> carrier, MemoryLayout layout, boolean isVariadicArg);
    }

    static final class UnboxBindingCalculator extends BindingCalculator {
        protected final boolean forArguments;
        private final boolean useAddressPairs;

        UnboxBindingCalculator(boolean forArguments, boolean useAddressPairs) {
            super(forArguments);
            this.forArguments = forArguments;
            this.useAddressPairs = useAddressPairs;
        }

        @Override
        List<Binding> getBindings(Class<?> carrier, MemoryLayout layout, boolean isVariadicArg) {
            TypeClass typeClass = TypeClass.classifyLayout(layout);
            return getBindings(carrier, layout, typeClass);
        }

        List<Binding> getBindings(Class<?> carrier, MemoryLayout layout, TypeClass argumentClass) {
            Binding.Builder bindings = Binding.builder();
            switch (argumentClass) {
                case INTEGER -> {
                    VMStorage storage = storageCalculator.getStorage(StorageType.INTEGER);
                    bindings.vmStore(storage, carrier);
                }
                case FLOAT -> {
                    VMStorage storage = storageCalculator.getStorage(StorageType.FLOAT);
                    bindings.vmStore(storage, carrier);
                }
                case POINTER -> {
                    VMStorage storage = storageCalculator.getStorage(StorageType.INTEGER);
                    if (useAddressPairs) {
                        bindings.dup()
                                .segmentBase()
                                .vmStore(storage, Object.class)
                                .segmentOffsetAllowHeap()
                                .vmStore(null, long.class);
                    } else {
                        bindings.unboxAddress();
                        bindings.vmStore(storage, long.class);
                    }
                }
                case STRUCT_REGISTER_X -> {
                    assert carrier == MemorySegment.class;
                    if (!storageCalculator.regsAvailable(1)) {
                        storageCalculator.alignStack(layout.byteAlignment());
                    }
                    VMStorage[] locations = storageCalculator.getStorages(layout);
                    int locIndex = 0;
                    long offset = 0;
                    while (offset < layout.byteSize()) {
                        final long copy = Math.min(layout.byteSize() - offset, 8);
                        VMStorage storage = locations[locIndex++];
                        Class<?> type = SharedUtils.primitiveCarrierForSize(copy, false);
                        if (offset + copy < layout.byteSize()) {
                            bindings.dup();
                        }
                        bindings.bufferLoad(offset, type, (int) copy)
                                .vmStore(storage, type);
                        offset += copy;
                    }
                }
                case STRUCT_REFERENCE -> {
                    assert carrier == MemorySegment.class;
                    bindings.copy(layout)
                            .unboxAddress();
                    VMStorage storage = storageCalculator.getStorage(StorageType.INTEGER);
                    bindings.vmStore(storage, long.class);
                }
                default -> throw new UnsupportedOperationException("Unhandled class " + argumentClass);
            }
            return bindings.build();
        }
    }

    static class BoxBindingCalculator extends BindingCalculator {

        BoxBindingCalculator(boolean forArguments) {
            super(forArguments);
        }

        @Override
        List<Binding> getBindings(Class<?> carrier, MemoryLayout layout, boolean isVariadicArg) {
            TypeClass typeClass = TypeClass.classifyLayout(layout);
            return getBindings(carrier, layout, typeClass);
        }

        List<Binding> getBindings(Class<?> carrier, MemoryLayout layout, TypeClass argumentClass) {
            Binding.Builder bindings = Binding.builder();
            switch (argumentClass) {
                case INTEGER -> {
                    VMStorage storage = storageCalculator.getStorage(StorageType.INTEGER);
                    bindings.vmLoad(storage, carrier);
                }
                case FLOAT -> {
                    VMStorage storage = storageCalculator.getStorage(StorageType.FLOAT);
                    bindings.vmLoad(storage, carrier);
                }
                case POINTER -> {
                    AddressLayout addressLayout = (AddressLayout) layout;
                    VMStorage storage = storageCalculator.getStorage(StorageType.INTEGER);
                    bindings.vmLoad(storage, long.class)
                            .boxAddressRaw(Utils.pointeeByteSize(addressLayout), Utils.pointeeByteAlign(addressLayout));
                }
                case STRUCT_REGISTER_X -> {
                    assert carrier == MemorySegment.class;
                    if (!storageCalculator.regsAvailable(1)) {
                        storageCalculator.alignStack(layout.byteAlignment());
                    }
                    bindings.allocate(layout);
                    VMStorage[] locations = storageCalculator.getStorages(layout);
                    int locIndex = 0;
                    long offset = 0;
                    while (offset < layout.byteSize()) {
                        final long copy = Math.min(layout.byteSize() - offset, 8);
                        VMStorage storage = locations[locIndex++];
                        Class<?> type = SharedUtils.primitiveCarrierForSize(copy, false);
                        bindings.dup().vmLoad(storage, type)
                                .bufferStore(offset, type, (int) copy);
                        offset += copy;
                    }
                }
                case STRUCT_REFERENCE -> {
                    assert carrier == MemorySegment.class;
                    VMStorage storage = storageCalculator.getStorage(StorageType.INTEGER);
                    bindings.vmLoad(storage, long.class)
                            .boxAddress(layout);
                }
                default -> throw new UnsupportedOperationException("Unhandled class " + argumentClass);
            }
            return bindings.build();
        }
    }
}
