/*
 * Copyright (c) 2020, Red Hat, Inc. All rights reserved.
 * Copyright (c) 2021, Loongson Technology. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
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

#include "prims/foreignGlobals.hpp"
#include "prims/foreignGlobals.inline.hpp"
#include "utilities/debug.hpp"

// Panama FFI not yet implemented for MIPS64el (Phase 3).

bool ForeignGlobals::is_foreign_linker_supported() {
  return false;
}

bool ABIDescriptor::is_volatile_reg(Register reg) const {
  return false;
}

bool ABIDescriptor::is_volatile_reg(FloatRegister reg) const {
  return false;
}

const ABIDescriptor ForeignGlobals::parse_abi_descriptor(jobject jabi) {
  Unimplemented();
  return {};
}

// parse_call_regs is now in shared foreignGlobals.cpp (jdk25u)

#include "prims/upcallLinker.hpp"

// UpcallLinker not yet implemented for MIPS64 (Phase 3 - Panama FFI)
address UpcallLinker::make_upcall_stub(jobject receiver, Symbol* signature,
                                       BasicType* out_sig_bt, int total_out_args,
                                       BasicType ret_type,
                                       jobject jabi, jobject jconv,
                                       bool needs_return_buffer, int ret_buf_size) {
  ShouldNotCallThis();
  return nullptr;
}

#include "prims/downcallLinker.hpp"

// DowncallLinker not yet implemented for MIPS64 (Phase 3 - Panama FFI)
RuntimeStub* DowncallLinker::make_downcall_stub(BasicType* sig_bt,
                                                int num_args,
                                                BasicType ret_bt,
                                                const ABIDescriptor& abi,
                                                const GrowableArray<VMStorage>& input_registers,
                                                const GrowableArray<VMStorage>& output_registers,
                                                bool needs_return_buffer,
                                                int captured_state_mask,
                                                bool needs_transition) {
  ShouldNotCallThis();
  return nullptr;
}

#include "code/codeBlob.hpp"

// UpcallStub support - not yet fully implemented for MIPS
UpcallStub::FrameData* UpcallStub::frame_data_for_frame(const frame& f) const {
  assert(f.is_upcall_stub_frame(), "wrong frame");
  return reinterpret_cast<UpcallStub::FrameData*>(
    reinterpret_cast<address>(f.unextended_sp()) + in_bytes(_frame_data_offset));
}

// Panama FFI RegSpiller and DowncallLinker::StubGenerator - not yet implemented for MIPS

#include "prims/foreignGlobals.hpp"
#include "prims/downcallLinker.hpp"

int RegSpiller::pd_reg_size(VMStorage reg) {
  ShouldNotCallThis();
  return 0;
}

void RegSpiller::pd_store_reg(MacroAssembler* masm, int offset, VMStorage reg) {
  ShouldNotCallThis();
}

void RegSpiller::pd_load_reg(MacroAssembler* masm, int offset, VMStorage reg) {
  ShouldNotCallThis();
}

void DowncallLinker::StubGenerator::pd_add_offset_to_oop(VMStorage reg_oop, VMStorage reg_buf,
                                                          VMStorage tmp1, VMStorage tmp2) const {
  ShouldNotCallThis();
}
