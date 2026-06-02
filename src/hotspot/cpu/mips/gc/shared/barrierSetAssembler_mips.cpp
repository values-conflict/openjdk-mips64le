/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2018, 2022, Loongson Technology. All rights reserved.
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
 *
 */

#include "gc/shared/barrierSetAssembler.hpp"
#include "gc/shared/collectedHeap.hpp"
#include "interpreter/interp_masm.hpp"
#include "runtime/javaThread.hpp"
#include "runtime/jniHandles.hpp"
#ifdef COMPILER2
#include "gc/shared/c2/barrierSetC2.hpp"
#endif // COMPILER2

#define __ masm->

void BarrierSetAssembler::load_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                  Register dst, Address src, Register tmp1, Register tmp2) {
  bool in_heap = (decorators & IN_HEAP) != 0;
  bool in_native = (decorators & IN_NATIVE) != 0;
  bool is_not_null = (decorators & IS_NOT_NULL) != 0;

  switch (type) {
  case T_OBJECT:
  case T_ARRAY: {
    if (in_heap) {
      if (UseCompressedOops) {
        __ lwu(dst, src);
        if (is_not_null) {
          __ decode_heap_oop_not_null(dst);
        } else {
          __ decode_heap_oop(dst);
        }
      } else
      {
        __ ld_ptr(dst, src);
      }
    } else {
      assert(in_native, "why else?");
      __ ld_ptr(dst, src);
    }
    break;
  }
  case T_BOOLEAN: __ lbu   (dst, src);    break;
  case T_BYTE:    __ lb    (dst, src);    break;
  case T_CHAR:    __ lhu   (dst, src);    break;
  case T_SHORT:   __ lh    (dst, src);    break;
  case T_INT:     __ lw    (dst, src);    break;
  case T_LONG:    __ ld    (dst, src);    break;
  case T_ADDRESS: __ ld_ptr(dst, src);    break;
  case T_FLOAT:
    assert(dst == noreg, "only to ftos");
    __ lwc1(FSF, src);
    break;
  case T_DOUBLE:
    assert(dst == noreg, "only to dtos");
    __ ldc1(FSF, src);
    break;
  default: Unimplemented();
  }
}

void BarrierSetAssembler::store_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                   Address dst, Register val, Register tmp1, Register tmp2, Register tmp3) {
  bool in_heap = (decorators & IN_HEAP) != 0;
  bool in_native = (decorators & IN_NATIVE) != 0;
  bool is_not_null = (decorators & IS_NOT_NULL) != 0;

  switch (type) {
  case T_OBJECT:
  case T_ARRAY: {
    if (in_heap) {
      if (val == noreg) {
        assert(!is_not_null, "inconsistent access");
        if (UseCompressedOops) {
          __ sw(R0, dst);
        } else {
          __ sd(R0, dst);
        }
      } else {
        if (UseCompressedOops) {
          assert(!dst.uses(val), "not enough registers");
          if (is_not_null) {
            __ encode_heap_oop_not_null(val);
          } else {
            __ encode_heap_oop(val);
          }
          __ sw(val, dst);
        } else
        {
          __ st_ptr(val, dst);
        }
      }
    } else {
      assert(in_native, "why else?");
      assert(val != noreg, "not supported");
      __ st_ptr(val, dst);
    }
    break;
  }
  case T_BOOLEAN:
    __ andi(val, val, 0x1);  // boolean is true if LSB is 1
    __ sb(val, dst);
    break;
  case T_BYTE:
    __ sb(val, dst);
    break;
  case T_SHORT:
    __ sh(val, dst);
    break;
  case T_CHAR:
    __ sh(val, dst);
    break;
  case T_INT:
    __ sw(val, dst);
    break;
  case T_LONG:
    __ sd(val, dst);
    break;
  case T_FLOAT:
    assert(val == noreg, "only tos");
    __ swc1(FSF, dst);
    break;
  case T_DOUBLE:
    assert(val == noreg, "only tos");
    __ sdc1(FSF, dst);
    break;
  case T_ADDRESS:
    __ st_ptr(val, dst);
    break;
  default: Unimplemented();
  }
}

void BarrierSetAssembler::obj_equals(MacroAssembler* masm,
                                     Register obj1, Address obj2) {
  Unimplemented();
}

void BarrierSetAssembler::obj_equals(MacroAssembler* masm,
                                     Register obj1, Register obj2) {
  Unimplemented();
}

void BarrierSetAssembler::try_resolve_jobject_in_native(MacroAssembler* masm, Register jni_env,
                                                        Register obj, Register tmp, Label& slowpath) {
  __ clear_jweak_tag(obj);
  __ ld_ptr(obj, Address(obj, 0));
}

void BarrierSetAssembler::tlab_allocate(MacroAssembler* masm,
                                        Register obj,
                                        Register var_size_in_bytes,
                                        int con_size_in_bytes,
                                        Register t1,
                                        Register t2,
                                        Label& slow_case) {
  Unimplemented();
}

void BarrierSetAssembler::copy_load_at(MacroAssembler* masm,
                                       DecoratorSet decorators,
                                       BasicType type,
                                       size_t bytes,
                                       Register dst,
                                       Address src,
                                       Register tmp) {
  if (bytes == 1) {
    __ lbu(dst, src);
  } else if (bytes == 2) {
    __ lhu(dst, src);
  } else if (bytes == 4) {
    __ lw(dst, src);
  } else if (bytes == 8) {
    __ ld(dst, src);
  } else {
    ShouldNotReachHere();
  }
  if ((decorators & ARRAYCOPY_CHECKCAST) != 0 && UseCompressedOops) {
    __ decode_heap_oop(dst);
  }
}

void BarrierSetAssembler::copy_store_at(MacroAssembler* masm,
                                        DecoratorSet decorators,
                                        BasicType type,
                                        size_t bytes,
                                        Address dst,
                                        Register src,
                                        Register tmp1,
                                        Register tmp2,
                                        Register tmp3) {
  if ((decorators & ARRAYCOPY_CHECKCAST) != 0 && UseCompressedOops) {
    __ encode_heap_oop(src);
  }
  if (bytes == 1) {
    __ sb(src, dst);
  } else if (bytes == 2) {
    __ sh(src, dst);
  } else if (bytes == 4) {
    __ sw(src, dst);
  } else if (bytes == 8) {
    __ sd(src, dst);
  } else {
    ShouldNotReachHere();
  }
}

#ifdef COMPILER2

void SaveLiveRegisters::initialize(BarrierStubC2* stub) {
  RegSet gp_regs;
  FloatRegSet fp_regs;

  for (int i = 0; i < stub->live_count(); i++) {
    VMReg r = stub->live_at(i);
    if (r->is_Register()) {
      gp_regs += RegSet::of(r->as_Register());
    } else if (r->is_FloatRegister()) {
      fp_regs += FloatRegSet::of(r->as_FloatRegister());
    }
  }

  _gp_regs = gp_regs;
  _fp_regs = fp_regs;
}

SaveLiveRegisters::SaveLiveRegisters(MacroAssembler* masm, BarrierStubC2* stub) : _masm(masm) {
  initialize(stub);
  __ push(_gp_regs);
  // TODO: push float registers
}

SaveLiveRegisters::~SaveLiveRegisters() {
  // TODO: pop float registers
  __ pop(_gp_regs);
}

#endif // COMPILER2

void BarrierSetAssembler::nmethod_entry_barrier(MacroAssembler* masm, Label* slow_path, Label* continuation, Label* guard) {
  // Not yet implemented for MIPS
}

void BarrierSetAssembler::c2i_entry_barrier(MacroAssembler* masm) {
  // Not yet implemented for MIPS
}

void BarrierSetAssembler::check_oop(MacroAssembler* masm, Register obj, Register tmp1, Register tmp2, Label& error) {
  // Not yet implemented for MIPS
}
