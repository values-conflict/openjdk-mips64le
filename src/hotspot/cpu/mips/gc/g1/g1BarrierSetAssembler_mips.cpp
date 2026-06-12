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

#include "asm/macroAssembler.inline.hpp"
#include "gc/g1/g1BarrierSet.hpp"
#include "gc/g1/g1BarrierSetAssembler.hpp"
#include "gc/g1/g1BarrierSetRuntime.hpp"
#include "gc/g1/g1CardTable.hpp"
#include "gc/g1/g1ThreadLocalData.hpp"
#include "gc/g1/g1HeapRegion.hpp"
#include "interpreter/interp_masm.hpp"
#include "runtime/sharedRuntime.hpp"
#include "utilities/macros.hpp"
#ifdef COMPILER1
#include "c1/c1_LIRAssembler.hpp"
#include "c1/c1_MacroAssembler.hpp"
#include "gc/g1/c1/g1BarrierSetC1.hpp"
#endif // COMPILER1
#ifdef COMPILER2
#include "gc/g1/c2/g1BarrierSetC2.hpp"
#endif // COMPILER2

#define __ masm->

#define A0 RA0
#define A1 RA1
#define A2 RA2
#define A3 RA3
#define A4 RA4
#define A5 RA5
#define A6 RA6
#define A7 RA7

void G1BarrierSetAssembler::gen_write_ref_array_pre_barrier(MacroAssembler* masm, DecoratorSet decorators,
                                                            Register addr, Register count, RegSet saved_regs) {
  bool dest_uninitialized = (decorators & IS_DEST_UNINITIALIZED) != 0;

  if (!dest_uninitialized) {
#ifndef OPT_THREAD
    Register thread = T9;
    __ get_thread(thread);
#else
    Register thread = TREG;
#endif

    Label filtered;
    Address in_progress(thread, in_bytes(G1ThreadLocalData::satb_mark_queue_active_offset()));
    // Is marking active?
    if (in_bytes(SATBMarkQueue::byte_width_of_active()) == 4) {
      __ lw(AT, in_progress);
    } else {
      assert(in_bytes(SATBMarkQueue::byte_width_of_active()) == 1, "Assumption");
      __ lb(AT, in_progress);
    }

    __ beq(AT, R0, filtered);
    __ delayed()->nop();

    __ pushad();                      // push registers
    if (count == A0) {
      if (addr == A1) {
        __ move(AT, A0);
        __ move(A0, A1);
        __ move(A1, AT);
      } else {
        __ move(A1, count);
        __ move(A0, addr);
      }
    } else {
      __ move(A0, addr);
      __ move(A1, count);
    }
    if (UseCompressedOops) {
      __ call_VM_leaf(CAST_FROM_FN_PTR(address, G1BarrierSetRuntime::write_ref_array_pre_narrow_oop_entry), 2);
    } else {
      __ call_VM_leaf(CAST_FROM_FN_PTR(address, G1BarrierSetRuntime::write_ref_array_pre_oop_entry), 2);
    }
    __ popad();

    __ bind(filtered);
  }
}

void G1BarrierSetAssembler::gen_write_ref_array_post_barrier(MacroAssembler* masm, DecoratorSet decorators,
                                                             Register addr, Register count, Register tmp, RegSet saved_regs) {
  __ pushad();             // push registers (overkill)
  if (count == A0) {
    assert_different_registers(A1, addr);
    __ move(A1, count);
    __ move(A0, addr);
  } else {
    assert_different_registers(A0, count);
    __ move(A0, addr);
    __ move(A1, count);
  }
  __ call_VM_leaf(CAST_FROM_FN_PTR(address, G1BarrierSetRuntime::write_ref_array_post_entry), 2);
  __ popad();
}

void G1BarrierSetAssembler::load_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                    Register dst, Address src, Register tmp1, Register tmp2) {
  bool on_oop = type == T_OBJECT || type == T_ARRAY;
  bool on_weak = (decorators & ON_WEAK_OOP_REF) != 0;
  bool on_phantom = (decorators & ON_PHANTOM_OOP_REF) != 0;
  bool on_reference = on_weak || on_phantom;
  ModRefBarrierSetAssembler::load_at(masm, decorators, type, dst, src, tmp1, tmp2);
  if (on_oop && on_reference) {
    const Register thread = TREG;
#ifndef OPT_THREAD
    __ get_thread(thread);
#endif
    // Generate the G1 pre-barrier code to log the value of
    // the referent field in an SATB buffer.
    g1_write_barrier_pre(masm /* masm */,
                         noreg /* obj */,
                         dst /* pre_val */,
                         thread /* thread */,
                         tmp1 /* tmp1 */,
                         tmp2 /* tmp2 */,
                         true /* tosca_live */,
                         true /* expand_call */);
  }
}

void G1BarrierSetAssembler::g1_write_barrier_pre(MacroAssembler* masm,
                                                 Register obj,
                                                 Register pre_val,
                                                 Register thread,
                                                 Register tmp1,
                                                 Register tmp2,
                                                 bool tosca_live,
                                                 bool expand_call) {
  // If expand_call is true then we expand the call_VM_leaf macro
  // directly to skip generating the check by
  // InterpreterMacroAssembler::call_VM_leaf_base that checks _last_sp.

  assert(thread == TREG, "must be");

  Label done;
  Label runtime;

  assert(pre_val != noreg, "check this code");

  if (obj != noreg) {
    assert_different_registers(obj, pre_val, tmp1);
    assert(pre_val != V0, "check this code");
  }

  Address in_progress(thread, in_bytes(G1ThreadLocalData::satb_mark_queue_active_offset()));
  Address index(thread, in_bytes(G1ThreadLocalData::satb_mark_queue_index_offset()));
  Address buffer(thread, in_bytes(G1ThreadLocalData::satb_mark_queue_buffer_offset()));

  // Is marking active?
  if (in_bytes(SATBMarkQueue::byte_width_of_active()) == 4) {
    __ lw(AT, in_progress);
  } else {
    assert(in_bytes(SATBMarkQueue::byte_width_of_active()) == 1, "Assumption");
    __ lb(AT, in_progress);
  }
  __ beq(AT, R0, done);
  __ delayed()->nop();

  // Do we need to load the previous value?
  if (obj != noreg) {
    __ load_heap_oop(pre_val, Address(obj, 0));
  }

  // Is the previous value null?
  __ beq(pre_val, R0, done);
  __ delayed()->nop();

  // Can we store original value in the thread's buffer?
  // Is index == 0?
  // (The index field is typed as size_t.)

  __ ld(tmp1, index);
  __ beq(tmp1, R0, runtime);
  __ delayed()->nop();

  __ daddiu(tmp1, tmp1, -1 * wordSize);
  __ sd(tmp1, index);
  __ ld(AT, buffer);
  __ daddu(tmp1, tmp1, AT);

  // Record the previous value
  __ sd(pre_val, tmp1, 0);
  __ beq(R0, R0, done);
  __ delayed()->nop();

  __ bind(runtime);
  // save the live input values
  if (tosca_live) __ push(V0);

  if (obj != noreg && obj != V0) __ push(obj);

  if (pre_val != V0) __ push(pre_val);

  // Calling the runtime using the regular call_VM_leaf mechanism generates
  // code (generated by InterpreterMacroAssember::call_VM_leaf_base)
  // that checks that the *(ebp+frame::interpreter_frame_last_sp) == NULL.
  //
  // If we care generating the pre-barrier without a frame (e.g. in the
  // intrinsified Reference.get() routine) then ebp might be pointing to
  // the caller frame and so this check will most likely fail at runtime.
  //
  // Expanding the call directly bypasses the generation of the check.
  // So when we do not have have a full interpreter frame on the stack
  // expand_call should be passed true.

  if (expand_call) {
    assert(pre_val != A1, "smashed arg");
    if (thread != A1) __ move(A1, thread);
    if (pre_val != A0) __ move(A0, pre_val);
    __ super_call_VM_leaf(CAST_FROM_FN_PTR(address, G1BarrierSetRuntime::write_ref_field_pre_entry), pre_val, thread);
  } else {
    __ call_VM_leaf(CAST_FROM_FN_PTR(address, G1BarrierSetRuntime::write_ref_field_pre_entry), pre_val, thread);
  }

  // save the live input values
  if (pre_val != V0)
    __ pop(pre_val);

  if (obj != noreg && obj != V0)
    __ pop(obj);

  if (tosca_live) __ pop(V0);

  __ bind(done);
}

void G1BarrierSetAssembler::g1_write_barrier_post(MacroAssembler* masm,
                                                  Register store_addr,
                                                  Register new_val,
                                                  Register thread,
                                                  Register tmp1,
                                                  Register tmp2) {
  assert_different_registers(tmp1, tmp2, AT);
  assert(thread == TREG, "must be");

  Address queue_index(thread, in_bytes(G1ThreadLocalData::dirty_card_queue_index_offset()));
  Address buffer(thread, in_bytes(G1ThreadLocalData::dirty_card_queue_buffer_offset()));

  CardTableBarrierSet* ct = barrier_set_cast<CardTableBarrierSet>(BarrierSet::barrier_set());
  assert(sizeof(*ct->card_table()->byte_map_base()) == sizeof(jbyte), "adjust this code");

  Label done;
  Label runtime;

  // Does store cross heap regions?
  __ xorr(AT, store_addr, new_val);
  // G1HeapRegion::LogOfHRGrainBytes is a runtime value; load into register for dsrlv
  __ li64(tmp2, (jlong)(address)&G1HeapRegion::LogOfHRGrainBytes);
  __ lw(tmp2, tmp2, 0);
  __ dsrlv(AT, AT, tmp2);
  __ beq(AT, R0, done);
  __ delayed()->nop();

  // crosses regions, storing NULL?
  __ beq(new_val, R0, done);
  __ delayed()->nop();

  // storing region crossing non-NULL, is card already dirty?
  const Register card_addr = tmp1;
  const Register cardtable = tmp2;

  __ move(card_addr, store_addr);
  __ dsrl(card_addr, card_addr, CardTable::card_shift());
  // Do not use ExternalAddress to load 'byte_map_base', since 'byte_map_base' is NOT
  // a valid address and therefore is not properly handled by the relocation code.
  __ set64(cardtable, (intptr_t)ct->card_table()->byte_map_base());
  __ daddu(card_addr, card_addr, cardtable);

  __ lb(AT, card_addr, 0);
  __ daddiu(AT, AT, -1 * (int)G1CardTable::g1_young_card_val());
  __ beq(AT, R0, done);
  __ delayed()->nop();

  __ sync();
  __ lb(AT, card_addr, 0);
  __ daddiu(AT, AT, -1 * (int)G1CardTable::dirty_card_val());
  __ beq(AT, R0, done);
  __ delayed()->nop();

  // storing a region crossing, non-NULL oop, card is clean.
  // dirty card and log.
  __ move(AT, (int)G1CardTable::dirty_card_val());
  __ sb(AT, card_addr, 0);
  // MIPS store ordering: card-dirty must be visible before queue log.
  __ sync();

  __ lw(AT, queue_index);
  __ beq(AT, R0, runtime);
  __ delayed()->nop();
  __ daddiu(AT, AT, -1 * wordSize);
  __ sw(AT, queue_index);
  __ ld(tmp2, buffer);
  __ ld(AT, queue_index);
  __ daddu(tmp2, tmp2, AT);
  __ sd(card_addr, tmp2, 0);
  __ beq(R0, R0, done);
  __ delayed()->nop();

  __ bind(runtime);
  // save the live input values
  __ push(store_addr);
  __ push(new_val);
  __ call_VM_leaf(CAST_FROM_FN_PTR(address, G1BarrierSetRuntime::write_ref_field_post_entry), card_addr, TREG);
  __ pop(new_val);
  __ pop(store_addr);

  __ bind(done);
}

void G1BarrierSetAssembler::oop_store_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                         Address dst, Register val, Register tmp1, Register tmp2, Register tmp3) {
  bool in_heap = (decorators & IN_HEAP) != 0;
  bool as_normal = (decorators & AS_NORMAL) != 0;
  assert((decorators & IS_DEST_UNINITIALIZED) == 0, "unsupported");

  bool needs_pre_barrier = as_normal;
  bool needs_post_barrier = val != noreg && in_heap;

  Register obj_addr = RT3;  // scratch reg for object address (renamed from tmp3 to avoid shadowing)
  Register rthread = TREG;
  // flatten object address if needed
  // We do it regardless of precise because we need the registers
  if (dst.index() == noreg && dst.disp() == 0) {
    if (dst.base() != obj_addr) {
      __ move(obj_addr, dst.base());
    }
  } else {
    __ lea(obj_addr, dst);
  }

  if (needs_pre_barrier) {
    g1_write_barrier_pre(masm /*masm*/,
                         obj_addr /* obj */,
                         tmp2 /* pre_val */,
                         rthread /* thread */,
                         tmp1  /* tmp1 */,
                         AT   /* tmp2 */,
                         val != noreg /* tosca_live */,
                         false /* expand_call */);
  }
  if (val == noreg) {
    BarrierSetAssembler::store_at(masm, decorators, type, Address(obj_addr, 0), val, noreg, noreg, noreg);
  } else {
    Register new_val = val;
    if (needs_post_barrier) {
      // G1 barrier needs uncompressed oop for region cross check.
      if (UseCompressedOops) {
        new_val = tmp2;
        __ move(new_val, val);
      }
    }
    BarrierSetAssembler::store_at(masm, decorators, type, Address(obj_addr, 0), val, noreg, noreg, noreg);
    if (needs_post_barrier) {
      g1_write_barrier_post(masm /*masm*/,
                            obj_addr /* store_adr */,
                            new_val /* new_val */,
                            rthread /* thread */,
                            tmp1 /* tmp */,
                            tmp2 /* tmp2 */);
    }
  }
}

#ifdef COMPILER2

// --- C2 G1 barrier helper statics ---

static void generate_queue_test_and_insertion(MacroAssembler* masm,
                                              ByteSize index_offset,
                                              ByteSize buffer_offset,
                                              Label& runtime,
                                              const Register thread,
                                              const Register value,
                                              const Register tmp1,
                                              const Register tmp2) {
  __ ld(tmp1, Address(thread, in_bytes(index_offset)));    // tmp1 = index
  __ beq(tmp1, R0, runtime); __ delayed()->nop();          // index == 0 → full, use runtime
  __ daddiu(tmp1, tmp1, -wordSize);                        // tmp1 = next index
  __ sd(tmp1, Address(thread, in_bytes(index_offset)));    // store next index
  __ ld(tmp2, Address(thread, in_bytes(buffer_offset)));   // tmp2 = buffer base
  __ daddu(tmp1, tmp2, tmp1);                              // tmp1 = buffer + next index
  __ sd(value, Address(tmp1));                             // *(buffer + next index) = value
}

static void generate_pre_barrier_fast_path(MacroAssembler* masm,
                                           const Register thread) {
  // Read the SATB-active flag into AT (the MIPS assembler temporary, r1).
  // AT is NOT in C2's allocatable register set, so writing 0 or 1 here cannot
  // corrupt any C2-managed variable.  The caller branches on AT via bne_far;
  // bne_far's internal b_far expansion also uses AT but only AFTER the
  // comparison value has already been consumed by the preceding beq instruction.
  Address in_progress(thread, in_bytes(G1ThreadLocalData::satb_mark_queue_active_offset()));
  if (in_bytes(SATBMarkQueue::byte_width_of_active()) == 4) {
    __ lwu(AT, in_progress);
  } else {
    assert(in_bytes(SATBMarkQueue::byte_width_of_active()) == 1, "Assumption");
    __ lbu(AT, in_progress);
  }
}

static void generate_pre_barrier_slow_path(MacroAssembler* masm,
                                           const Register obj,
                                           const Register pre_val,
                                           const Register thread,
                                           const Register tmp1,
                                           const Register tmp2,
                                           Label& done,
                                           Label& runtime) {
  if (obj != noreg) {
    __ load_heap_oop(pre_val, Address(obj, 0), noreg, noreg, AS_RAW);
  }
  __ beq(pre_val, R0, done); __ delayed()->nop();   // pre_val == null? done
  generate_queue_test_and_insertion(masm,
                                    G1ThreadLocalData::satb_mark_queue_index_offset(),
                                    G1ThreadLocalData::satb_mark_queue_buffer_offset(),
                                    runtime,
                                    thread, pre_val, tmp1, tmp2);
  __ beq(R0, R0, done); __ delayed()->nop();
}

static void generate_post_barrier_fast_path(MacroAssembler* masm,
                                            const Register store_addr,
                                            const Register new_val,
                                            Label& done,
                                            bool new_val_may_be_null) {
  // K0, K1, and AT are the C2-invisible MIPS barrier scratch registers.
  // store_addr may be AT (IndOffset8/BasePosIndex effective addresses).
  // new_val may be K0 (decoded OOP placed there by IndOffset8 caller).
  // In both cases the register usage below is safe:
  //   dsrl(K1, store_addr, ...) reads store_addr first, writes K1 — no clobber.
  //   xorr(K0, store_addr, new_val) reads both, writes K0 — safe even if K0==new_val.
  if (new_val_may_be_null) {
    __ beq(new_val, R0, done); __ delayed()->nop();
  }
  __ dsrl(K1, store_addr, CardTable::card_shift());    // K1 = card page
  __ xorr(K0, store_addr, new_val);                   // K0 = addr ^ val
  __ li64(AT, (jlong)(address)&G1HeapRegion::LogOfHRGrainBytes);
  __ lw(AT, AT, 0);                                   // AT = LogOfHRGrainBytes
  __ dsrlv(K0, K0, AT);                               // K0 = region_diff
  __ beq(K0, R0, done); __ delayed()->nop();          // same region? done
  // Different region: compute card address into K1.
  CardTableBarrierSet* ctbs = barrier_set_cast<CardTableBarrierSet>(BarrierSet::barrier_set());
  __ li64(AT, (intptr_t)ctbs->card_table()->byte_map_base());
  __ daddu(K1, K1, AT);                               // K1 = card address
  __ lbu(AT, Address(K1));                            // AT = card value
  // Outputs: K1 = card address (stub->tmp1()), AT = card value.
}

static void generate_post_barrier_slow_path(MacroAssembler* masm,
                                            const Register thread,
                                            const Register card_addr,   // K1
                                            const Register scratch,     // K0
                                            Label& done,
                                            Label& runtime) {
  __ sync();                                         // StoreLoad barrier
  __ lbu(scratch, Address(card_addr));               // scratch = card value
  __ beq(scratch, R0, done); __ delayed()->nop();   // already dirty? done
  STATIC_ASSERT(CardTable::dirty_card_val() == 0);
  __ sb(R0, Address(card_addr));                    // dirty the card
  // MIPS store ordering: ensure card-dirty store is visible to G1
  // refinement threads before logging the card address in the queue.
  __ sync();
  generate_queue_test_and_insertion(masm,
                                    G1ThreadLocalData::dirty_card_queue_index_offset(),
                                    G1ThreadLocalData::dirty_card_queue_buffer_offset(),
                                    runtime,
                                    thread, card_addr, scratch, AT);
  __ beq(R0, R0, done); __ delayed()->nop();
}

static void generate_c2_barrier_runtime_call(MacroAssembler* masm,
                                             G1BarrierStubC2* stub,
                                             const Register arg,
                                             const address runtime_path) {
  SaveLiveRegisters save_registers(masm, stub);
  if (A0 != arg) {
    __ move(A0, arg);
  }
  __ move(A1, TREG);
  // Use T9 so the callee's daddu $gp,$gp,$t9 prologue sets up GP correctly.
  __ set64(T9, (intptr_t)runtime_path);
  __ jalr(T9); __ delayed()->nop();
}

// --- C2 G1 barrier implementations ---

void G1BarrierSetAssembler::g1_write_barrier_pre_c2(MacroAssembler* masm,
                                                    Register obj,
                                                    Register pre_val,
                                                    Register thread,
                                                    Register /* tmp1 */,  // ignored; AT used
                                                    Register /* tmp2 */,  // ignored; K1 used
                                                    G1PreBarrierStubC2* stub) {
  assert(thread == TREG, "must be");
  assert(pre_val != noreg, "expecting a register");
  // K0 (pre_val) holds the old OOP; AT is the satb-active flag and queue-index
  // scratch; K1 is the queue-buffer scratch.  All three are C2-invisible.
  stub->initialize_registers(obj, pre_val, thread, AT /* queue index */, K1 /* queue buffer */);

  // compute_liveness_at_stubs records LIVEOUT of the barrier node.  Any
  // caller-save register that is live across the barrier only via a
  // post-insert_copies() SpillCopy has its live range extended past the barrier
  // after liveness is computed; the LIVEOUT-based preserve_set misses it.  The
  // C runtime call in the queue-full slow path then corrupts those registers.
  // Force-preserve all caller-save registers (callee-save registers are
  // protected by the C ABI automatically).
  stub->preserve(T0); stub->preserve(T1); stub->preserve(T2); stub->preserve(T3);
  stub->preserve(T8); stub->preserve(T9); stub->preserve(V0); stub->preserve(V1);
  stub->preserve(A0); stub->preserve(A1); stub->preserve(A2); stub->preserve(A3);
  stub->preserve(A4); stub->preserve(A5); stub->preserve(A6); stub->preserve(A7);

  generate_pre_barrier_fast_path(masm, thread);
  // If marking is active (AT != 0), jump to stub slow path
  __ bne_far(AT, R0, *stub->entry());

  __ bind(*stub->continuation());
}

void G1BarrierSetAssembler::generate_c2_pre_barrier_stub(MacroAssembler* masm,
                                                         G1PreBarrierStubC2* stub) const {
  Assembler::InlineSkippedInstructionsCounter skip_counter(masm);
  Label runtime;
  Register obj    = stub->obj();
  Register pre_val = stub->pre_val();
  Register thread = stub->thread();
  Register tmp1   = stub->tmp1();
  Register tmp2   = stub->tmp2();

  __ bind(*stub->entry());
  generate_pre_barrier_slow_path(masm, obj, pre_val, thread, tmp1, tmp2,
                                 *stub->continuation(), runtime);

  __ bind(runtime);
  generate_c2_barrier_runtime_call(masm, stub, pre_val,
      CAST_FROM_FN_PTR(address, G1BarrierSetRuntime::write_ref_field_pre_entry));
  __ b_far(*stub->continuation()); __ delayed()->nop();
}

void G1BarrierSetAssembler::g1_write_barrier_post_c2(MacroAssembler* masm,
                                                     Register store_addr,
                                                     Register new_val,
                                                     Register thread,
                                                     Register /* tmp1 */,  // ignored; K0 used
                                                     Register /* tmp2 */,  // ignored; K1 used
                                                     G1PostBarrierStubC2* stub) {
  assert(thread == TREG, "must be");
  assert(store_addr != noreg && new_val != noreg, "expecting a register");
  // K1 receives the card address and K0 is the freed scratch; both C2-invisible.
  stub->initialize_registers(thread, K1 /* card_addr */, K0 /* freed */);
  // Same preserve rationale as g1_write_barrier_pre_c2: force-preserve all
  // caller-save registers against C runtime corruption in the queue-full path.
  stub->preserve(T0); stub->preserve(T1); stub->preserve(T2); stub->preserve(T3);
  stub->preserve(T8); stub->preserve(T9); stub->preserve(V0); stub->preserve(V1);
  stub->preserve(A0); stub->preserve(A1); stub->preserve(A2); stub->preserve(A3);
  stub->preserve(A4); stub->preserve(A5); stub->preserve(A6); stub->preserve(A7);

  bool new_val_may_be_null = (stub->barrier_data() & G1C2BarrierPostNotNull) == 0;
  generate_post_barrier_fast_path(masm, store_addr, new_val,
                                  *stub->continuation(), new_val_may_be_null);
  // AT holds the card value.  Compare with g1_young_card_val using daddiu (fits simm16,
  // no extra register needed) to avoid overwriting K1 (which holds card_addr).
  __ daddiu(AT, AT, -(int)G1CardTable::g1_young_card_val());
  __ bne_far(AT, R0, *stub->entry());

  __ bind(*stub->continuation());
}

void G1BarrierSetAssembler::generate_c2_post_barrier_stub(MacroAssembler* masm,
                                                          G1PostBarrierStubC2* stub) const {
  Assembler::InlineSkippedInstructionsCounter skip_counter(masm);
  Label runtime;
  Register thread    = stub->thread();
  Register card_addr = stub->tmp1();   // K1: card address set by fast path
  Register scratch   = stub->tmp2();   // K0: freed scratch

  __ bind(*stub->entry());
  generate_post_barrier_slow_path(masm, thread, card_addr, scratch,
                                  *stub->continuation(), runtime);

  __ bind(runtime);
  generate_c2_barrier_runtime_call(masm, stub, card_addr,
      CAST_FROM_FN_PTR(address, G1BarrierSetRuntime::write_ref_field_post_entry));
  __ b_far(*stub->continuation()); __ delayed()->nop();
}

#endif // COMPILER2
