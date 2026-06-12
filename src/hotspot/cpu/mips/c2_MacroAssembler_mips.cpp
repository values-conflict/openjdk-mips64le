/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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
 *
 */

#include "asm/assembler.hpp"
#include "asm/assembler.inline.hpp"
#include "opto/c2_MacroAssembler.hpp"
#include "opto/intrinsicnode.hpp"

#include "runtime/objectMonitor.hpp"
#include "vmreg_mips.inline.hpp"

// Fast_Lock and Fast_Unlock used by C2

// Because the transitions from emitted code to the runtime
// monitorenter/exit helper stubs are so slow it's critical that
// we inline both the stack-locking fast-path and the inflated fast path.
//
// See also: cmpFastLock and cmpFastUnlock.
//
// What follows is a specialized inline transliteration of the code
// in slow_enter() and slow_exit().  If we're concerned about I$ bloat
// another option would be to emit TrySlowEnter and TrySlowExit methods
// at startup-time.  These methods would accept arguments as
// (Obj, Self, box, Scratch) and return success-failure
// indications in the icc.ZFlag.  Fast_Lock and Fast_Unlock would simply
// marshal the arguments and emit calls to TrySlowEnter and TrySlowExit.
// In practice, however, the # of lock sites is bounded and is usually small.
// Besides the call overhead, TrySlowEnter and TrySlowExit might suffer
// if the processor uses simple bimodal branch predictors keyed by EIP
// Since the helper routines would be called from multiple synchronization
// sites.
//
// An even better approach would be write "MonitorEnter()" and "MonitorExit()"
// in java - using j.u.c and unsafe - and just bind the lock and unlock sites
// to those specialized methods.  That'd give us a mostly platform-independent
// implementation that the JITs could optimize and inline at their pleasure.
// Done correctly, the only time we'd need to cross to native could would be
// to park() or unpark() threads.  We'd also need a few more unsafe operators
// to (a) prevent compiler-JIT reordering of non-volatile accesses, and
// (b) explicit barriers or fence operations.
//
// TODO:
//
// *  Arrange for C2 to pass "Self" into Fast_Lock and Fast_Unlock in one of the registers (scr).
//    This avoids manifesting the Self pointer in the Fast_Lock and Fast_Unlock terminals.
//    Given TLAB allocation, Self is usually manifested in a register, so passing it into
//    the lock operators would typically be faster than reifying Self.
//
// *  Ideally I'd define the primitives as:
//       fast_lock   (nax Obj, nax box, res, tmp, nax scr) where tmp and scr are KILLED.
//       fast_unlock (nax Obj, box, res, nax tmp) where tmp are KILLED
//    Unfortunately ADLC bugs prevent us from expressing the ideal form.
//    Instead, we're stuck with a rather awkward and brittle register assignments below.
//    Furthermore the register assignments are overconstrained, possibly resulting in
//    sub-optimal code near the synchronization site.
//
// *  Eliminate the sp-proximity tests and just use "== Self" tests instead.
//    Alternately, use a better sp-proximity test.
//
// *  Currently ObjectMonitor._Owner can hold either an sp value or a (THREAD *) value.
//    Either one is sufficient to uniquely identify a thread.
//    TODO: eliminate use of sp in _owner and use get_thread(tr) instead.
//
// *  Intrinsify notify() and notifyAll() for the common cases where the
//    object is locked by the calling thread but the waitlist is empty.
//    avoid the expensive JNI call to JVM_Notify() and JVM_NotifyAll().
//
// *  use jccb and jmpb instead of jcc and jmp to improve code density.
//    But beware of excessive branch density on AMD Opterons.
//
// *  Both Fast_Lock and Fast_Unlock set the ICC.ZF to indicate success
//    or failure of the fast-path.  If the fast-path fails then we pass
//    control to the slow-path, typically in C.  In Fast_Lock and
//    Fast_Unlock we often branch to DONE_LABEL, just to find that C2
//    will emit a conditional branch immediately after the node.
//    So we have branches to branches and lots of ICC.ZF games.
//    Instead, it might be better to have C2 pass a "FailureLabel"
//    into Fast_Lock and Fast_Unlock.  In the case of success, control
//    will drop through the node.  ICC.ZF is undefined at exit.
//    In the case of failure, the node will branch directly to the
//    FailureLabel

// obj: object to lock
// box: on-stack box address (displaced header location)
// tmp: tmp -- KILLED
// scr: tmp -- KILLED
// obj: object to lock
// box: on-stack BasicLock
// res: result -- 1 on success, 0 on failure (only meaningful for LM_LEGACY)
// tmp, scr: killed temporaries
void C2_MacroAssembler::fast_lock(Register objReg, Register boxReg, Register resReg,
                                  Register tmpReg, Register scrReg) {
  assert(LockingMode != LM_LIGHTWEIGHT, "lightweight locking should use fast_lock_lightweight");
  assert_different_registers(objReg, boxReg, resReg, tmpReg, scrReg);

  Label IsInflated, DONE, DONE_SET;
  Label object_has_monitor;

  block_comment("fast_lock {");

  if (DiagnoseSyncOnValueBasedClasses != 0) {
    load_klass(tmpReg, objReg);
    lbu(tmpReg, Address(tmpReg, Klass::misc_flags_offset()));
    andi(AT, tmpReg, KlassFlags::_misc_is_value_based_class);
    move(resReg, R0);
    bne(AT, R0, DONE);
    delayed()->nop();
  }

  if (LockingMode == LM_MONITOR) {
    move(resReg, R0);
    b(DONE);
    delayed()->nop();
  } else {
    assert(LockingMode == LM_LEGACY, "must be");

    // Load markWord from object into tmpReg.
    ld(tmpReg, Address(objReg, oopDesc::mark_offset_in_bytes()));

    // Check for existing monitor (monitor_value bit set).
    andi(AT, tmpReg, markWord::monitor_value);
    bne(AT, R0, object_has_monitor);
    delayed()->nop();

    // Set tmpReg = markWord | unlocked_value.
    ori(tmpReg, tmpReg, markWord::unlocked_value);

    // Initialize the box (displaced header) with tmpReg before CAS.
    sd(tmpReg, Address(boxReg, BasicLock::displaced_header_offset_in_bytes()));

    // CAS: atomically replace *objReg with boxReg if *objReg == tmpReg.
    // scrReg = 1 on success, 0 on failure.
    cmpxchg(Address(objReg, 0), tmpReg, boxReg, scrReg, true /* retold */, true /* acquire */);
    bne(scrReg, R0, DONE_SET);
    delayed()->nop();

    // CAS failed -- check for recursive lock.
    // If (mark - sp) < page_size with lock bits masked, it's a recursive lock.
    dsubu(tmpReg, tmpReg, SP);
    li(AT, (intptr_t)(~(os::vm_page_size() - 1) | (uintptr_t)markWord::lock_mask_in_place));
    andr(tmpReg, tmpReg, AT);
    sd(tmpReg, Address(boxReg, BasicLock::displaced_header_offset_in_bytes()));
    sltiu(scrReg, tmpReg, 1);  // scrReg = (tmpReg == 0) ? 1 : 0
    b(DONE_SET);
    delayed()->nop();
  }

  // Handle inflated monitor.
  bind(object_has_monitor);
  {
    // disp_hdr holds the tagged monitor pointer.
    // Use boxReg slot for store, compute monitor address into tmpReg.
    li(AT, (address)markWord::unused_mark().value());
    sd(AT, Address(boxReg, BasicLock::displaced_header_offset_in_bytes()));

    daddiu(tmpReg, tmpReg, in_bytes(ObjectMonitor::owner_offset()) - markWord::monitor_value);

    // Try to CAS owner from 0 to current thread's _monitor_owner_id.
    ld(scrReg, Address(TREG, JavaThread::monitor_owner_id_offset()));
    cmpxchg(Address(tmpReg, 0), R0, scrReg, resReg, true /* retold */, true /* acquire */);
    bne(resReg, R0, DONE);  // CAS succeeded
    delayed()->nop();

    // Check if recursive (current owner == current thread).
    bne(resReg, scrReg, DONE);  // resReg = old owner; if != tid, failure
    delayed()->ori(resReg, R0, 0);

    // Recursive: increment recursions.
    daddiu(AT, tmpReg, in_bytes(ObjectMonitor::recursions_offset()) - in_bytes(ObjectMonitor::owner_offset()));
    ld(scrReg, Address(AT, 0));
    daddiu(scrReg, scrReg, 1);
    sd(scrReg, Address(AT, 0));
    li(resReg, 1);
    b(DONE);
    delayed()->nop();
  }

  bind(DONE_SET);
  move(resReg, scrReg);

  bind(DONE);
  block_comment("} fast_lock");
  // resReg: 1 = success, 0 = failure (take slow path)
}

void C2_MacroAssembler::fast_unlock(Register objReg, Register boxReg, Register resReg,
                                    Register tmpReg, Register scrReg) {
  assert(LockingMode != LM_LIGHTWEIGHT, "lightweight locking should use fast_unlock_lightweight");
  assert_different_registers(objReg, boxReg, resReg, tmpReg, scrReg);

  Label DONE, DONE_SET, Stacked, object_has_monitor;

  block_comment("fast_unlock {");

  if (LockingMode == LM_MONITOR) {
    move(resReg, R0);
    b(DONE);
    delayed()->nop();
  } else {
    assert(LockingMode == LM_LEGACY, "must be");

    // Check for recursive lock: displaced header == 0.
    ld(tmpReg, Address(boxReg, BasicLock::displaced_header_offset_in_bytes()));
    beq(tmpReg, R0, DONE_SET);
    delayed()->sltiu(AT, tmpReg, 1);  // AT will be 0 (failure is not happening)

    // Check if stack-locked or inflated.
    ld(tmpReg, Address(objReg, oopDesc::mark_offset_in_bytes()));
    andi(AT, tmpReg, markWord::monitor_value);
    beq(AT, R0, Stacked);
    delayed()->nop();

    // Inflated monitor.
    bind(object_has_monitor);
    {
      daddiu(scrReg, tmpReg, in_bytes(ObjectMonitor::owner_offset()) - markWord::monitor_value);

      // Check owner == current thread.
#ifndef OPT_THREAD
      get_thread(TREG);
#endif
      ld(AT, Address(scrReg, 0));
      xorr(AT, AT, TREG);

      // Check recursions == 0.
      ld(resReg, Address(tmpReg, in_bytes(ObjectMonitor::recursions_offset()) - markWord::monitor_value));
      orr(AT, AT, resReg);
      bne(AT, R0, DONE_SET);
      delayed()->ori(AT, R0, 0);

      // Release lock: store 0 into owner with release semantics.
      sync();
      sd(R0, Address(scrReg, 0));
      b(DONE);
      delayed()->ori(resReg, R0, 1);
    }

    // Stack-locked case.
    bind(Stacked);
    {
      ld(AT, Address(boxReg, BasicLock::displaced_header_offset_in_bytes()));
      cmpxchg(Address(objReg, 0), boxReg, AT, resReg, false /* retold */, false /* acquire */);
    }

    bind(DONE_SET);
    move(resReg, AT);
  }

  bind(DONE);
  block_comment("} fast_unlock");
  // resReg: non-0 = success, 0 = failure
}

// Lightweight locking (LM_LIGHTWEIGHT).
// flag: set to non-0 on success, 0 on failure.
void C2_MacroAssembler::fast_lock_lightweight(Register obj, Register box, Register flag,
                                              Register tmp1, Register tmp2, Register tmp3,
                                              Register tmp4) {
  assert(LockingMode == LM_LIGHTWEIGHT, "must be");
  assert_different_registers(obj, box, flag, tmp1, tmp2, tmp3, tmp4);

  Label inflated, locked, slow_path;

  block_comment("fast_lock_lightweight {");

  move(flag, R0);

  if (DiagnoseSyncOnValueBasedClasses != 0) {
    load_klass(tmp1, obj);
    lbu(tmp1, Address(tmp1, Klass::misc_flags_offset()));
    andi(tmp1, tmp1, KlassFlags::_misc_is_value_based_class);
    bne(tmp1, R0, slow_path);
    delayed()->nop();
  }

  { // Lightweight locking: push to lock stack.
    Label push;
    const Register tmp2_top = tmp2;
    const Register tmp1_mark = tmp1;
    const Register tmp3_t = tmp3;

    // Check if lock stack is full.
    lwu(tmp2_top, Address(TREG, JavaThread::lock_stack_top_offset()));
    li(tmp3_t, (unsigned)LockStack::end_offset());
    slt(AT, tmp2_top, tmp3_t);
    beq(AT, R0, slow_path);  // stack full
    delayed()->nop();

    // Check if recursive (top of stack == obj).
    daddu(tmp3_t, TREG, tmp2_top);
    ld(tmp3_t, Address(tmp3_t, -oopSize));
    beq(obj, tmp3_t, push);
    delayed()->nop();

    // Relaxed load of markword to check for monitor.
    ld(tmp1_mark, Address(obj, oopDesc::mark_offset_in_bytes()));
    andi(tmp3_t, tmp1_mark, markWord::monitor_value);
    bne(tmp3_t, R0, inflated);
    delayed()->nop();

    // Not inflated: try CAS lock-bits 0b01 => 0b00.
    ori(tmp1_mark, tmp1_mark, markWord::unlocked_value);
    xori(tmp3_t, tmp1_mark, markWord::unlocked_value);
    cmpxchg(Address(obj, 0), tmp1_mark, tmp3_t, flag, true /* retold */, true /* acquire */);
    beq(flag, R0, slow_path);
    delayed()->nop();

    bind(push);
    // Push obj on lock stack.
    daddu(tmp3_t, TREG, tmp2_top);
    sd(obj, Address(tmp3_t, 0));
    addiu(tmp2_top, tmp2_top, oopSize);
    sw(tmp2_top, Address(TREG, JavaThread::lock_stack_top_offset()));
    b(locked);
    delayed()->nop();
  }

  { // Handle inflated monitor.
    bind(inflated);

    const Register tmp1_monitor = tmp1;
    const Register tmp2_owner_addr = tmp2;
    const Register tmp3_owner = tmp3;
    const Register tid = tmp4;

    // Compute owner address.  monitor pointer needs tag stripped.
    daddiu(tmp1_monitor, tmp1, in_bytes(ObjectMonitor::owner_offset()) - (int)markWord::monitor_value);

    // Load current thread id.
    ld(tid, Address(TREG, JavaThread::monitor_owner_id_offset()));

    // Try to CAS owner from 0 to tid.
    move(tmp3_owner, R0);
    cmpxchg(Address(tmp1_monitor, 0), tmp3_owner, tid, flag, true /* retold */, true /* acquire */);
    bne(flag, R0, locked);
    delayed()->nop();

    // Check if recursive (owner == tid).
    bne(tmp3_owner, tid, slow_path);
    delayed()->nop();

    // Recursive: increment recursions counter.
    daddiu(AT, tmp1_monitor, in_bytes(ObjectMonitor::recursions_offset()) - in_bytes(ObjectMonitor::owner_offset()));
    ld(tmp3_owner, Address(AT, 0));
    daddiu(tmp3_owner, tmp3_owner, 1);
    sd(tmp3_owner, Address(AT, 0));
  }

  bind(locked);
  li(flag, 1);

  bind(slow_path);
  block_comment("} fast_lock_lightweight");
  // flag == 1: success; flag == 0: failure (take slow path)
}

// Lightweight unlocking (LM_LIGHTWEIGHT).
// flag: set to non-0 on success, 0 on failure.
void C2_MacroAssembler::fast_unlock_lightweight(Register obj, Register box, Register flag,
                                                Register tmp1, Register tmp2, Register tmp3) {
  assert(LockingMode == LM_LIGHTWEIGHT, "must be");
  assert_different_registers(obj, box, flag, tmp1, tmp2, tmp3);

  Label inflated, unlocked, slow_path;

  block_comment("fast_unlock_lightweight {");

  const Register tmp1_mark = tmp1;
  const Register tmp2_top = tmp2;
  const Register tmp3_t = tmp3;

  // Load markword.
  ld(tmp1_mark, Address(obj, oopDesc::mark_offset_in_bytes()));

  // Check for inflated monitor.
  andi(AT, tmp1_mark, markWord::monitor_value);
  bne(AT, R0, inflated);
  delayed()->nop();

  { // Stack-locked (lightweight) path: pop from lock stack.
    // Get lock stack top.
    lwu(tmp2_top, Address(TREG, JavaThread::lock_stack_top_offset()));

    // Check for recursive: top-1 is obj.
    addiu(tmp2_top, tmp2_top, -(int)oopSize);
    daddu(tmp3_t, TREG, tmp2_top);
    ld(AT, Address(tmp3_t, 0));
    bne(AT, obj, slow_path);
    delayed()->nop();

    // Not recursive -- but check for second recursive (top-2 is also obj).
    daddu(tmp3_t, TREG, tmp2_top);
    ld(tmp3_t, Address(tmp3_t, -oopSize));
    beq(obj, tmp3_t, unlocked);  // if top-2 == obj, recursive unlocking doesn't need CAS
    delayed()->nop();

    // Attempt CAS: restore lock-bits 0b00 => 0b01.
    ori(tmp1_mark, tmp1_mark, markWord::unlocked_value);
    cmpxchg(Address(obj, 0), tmp1_mark, tmp1_mark, flag, false /* retold */, false /* acquire */);
    beq(flag, R0, slow_path);  // CAS failed
    delayed()->nop();

    bind(unlocked);
    // Pop lock stack.
    sw(tmp2_top, Address(TREG, JavaThread::lock_stack_top_offset()));
    b(slow_path);  // will set flag=1 and fall through
    delayed()->addiu(flag, R0, 1);
  }

  { // Inflated monitor path.
    bind(inflated);
    // tmp1 is the mark which has monitor_value set.
    const Register tmp1_monitor = tmp1;
    const Register tid = tmp2;

    // Load thread id.
    ld(tid, Address(TREG, JavaThread::monitor_owner_id_offset()));

    // Compute owner address (stripping monitor_value tag).
    daddiu(tmp1_monitor, tmp1_monitor, in_bytes(ObjectMonitor::owner_offset()) - markWord::monitor_value);

    // Check owner == current thread.
    ld(AT, Address(tmp1_monitor, 0));
    bne(AT, tid, slow_path);
    delayed()->nop();

    // Check recursions.
    daddiu(tmp3_t, tmp1_monitor, in_bytes(ObjectMonitor::recursions_offset()) - in_bytes(ObjectMonitor::owner_offset()));
    ld(AT, Address(tmp3_t, 0));
    bne(AT, R0, slow_path);
    delayed()->nop();

    // No waiters / no recursions: release the lock.
    sync();
    sd(R0, Address(tmp1_monitor, 0));
    li(flag, 1);
    b(slow_path);
    delayed()->nop();
  }

  bind(slow_path);
  block_comment("} fast_unlock_lightweight");
  // flag == 1: success; flag == 0: failure (take slow path)
}

void C2_MacroAssembler::beq_long(Register rs, Register rt, Label& L) {
  Label not_taken;

  bne(rs, rt, not_taken);
  delayed()->nop();

  jmp_far(L);

  bind(not_taken);
}

void C2_MacroAssembler::bne_long(Register rs, Register rt, Label& L) {
  Label not_taken;

  beq(rs, rt, not_taken);
  delayed()->nop();

  jmp_far(L);

  bind(not_taken);
}

void C2_MacroAssembler::bc1t_long(Label& L) {
  Label not_taken;

  bc1f(not_taken);
  delayed()->nop();

  jmp_far(L);

  bind(not_taken);
}

void C2_MacroAssembler::bc1f_long(Label& L) {
  Label not_taken;

  bc1t(not_taken);
  delayed()->nop();

  jmp_far(L);

  bind(not_taken);
}

// Compare strings, used for char[] and byte[].
void C2_MacroAssembler::string_compare(Register str1, Register str2,
                                    Register cnt1, Register cnt2, Register result,
                                    int ae) {
  Label L, Loop, haveResult, done;

  bool isLL = ae == StrIntrinsicNode::LL;
  bool isLU = ae == StrIntrinsicNode::LU;
  bool isUL = ae == StrIntrinsicNode::UL;

  bool str1_isL = isLL || isLU;
  bool str2_isL = isLL || isUL;

  if (!str1_isL) srl(cnt1, cnt1, 1);
  if (!str2_isL) srl(cnt2, cnt2, 1);

  // compute the and difference of lengths (in result)
  subu(result, cnt1, cnt2); // result holds the difference of two lengths

  // compute the shorter length (in cnt1)
  slt(AT, cnt2, cnt1);
  movn(cnt1, cnt2, AT);

  // Now the shorter length is in cnt1 and cnt2 can be used as a tmp register
  bind(Loop);                        // Loop begin
  beq(cnt1, R0, done);
  if (str1_isL) {
    delayed()->lbu(AT, str1, 0);
  } else {
    delayed()->lhu(AT, str1, 0);
  }

  // compare current character
  if (str2_isL) {
    lbu(cnt2, str2, 0);
  } else {
    lhu(cnt2, str2, 0);
  }
  bne(AT, cnt2, haveResult);
  delayed()->addiu(str1, str1, str1_isL ? 1 : 2);
  addiu(str2, str2, str2_isL ? 1 : 2);
  b(Loop);
  delayed()->addiu(cnt1, cnt1, -1);   // Loop end

  bind(haveResult);
  subu(result, AT, cnt2);

  bind(done);
}

// Compare char[] or byte[] arrays or substrings.
void C2_MacroAssembler::arrays_equals(Register str1, Register str2,
                                   Register cnt, Register tmp, Register result,
                                   bool is_char) {
  Label Loop, True, False;

  beq(str1, str2, True);  // same char[] ?
  delayed()->daddiu(result, R0, 1);

  beq(cnt, R0, True);
  delayed()->nop(); // count == 0

  bind(Loop);

  // compare current character
  if (is_char) {
    lhu(AT, str1, 0);
    lhu(tmp, str2, 0);
  } else {
    lbu(AT, str1, 0);
    lbu(tmp, str2, 0);
  }
  bne(AT, tmp, False);
  delayed()->addiu(str1, str1, is_char ? 2 : 1);
  addiu(cnt, cnt, -1);
  bne(cnt, R0, Loop);
  delayed()->addiu(str2, str2, is_char ? 2 : 1);

  b(True);
  delayed()->nop();

  bind(False);
  daddiu(result, R0, 0);

  bind(True);
}

void C2_MacroAssembler::gs_loadstore(Register reg, Register base, Register index, int disp, int type) {
  switch (type) {
    case STORE_BYTE:
      gssbx(reg, base, index, disp);
      break;
    case STORE_CHAR:
    case STORE_SHORT:
      gsshx(reg, base, index, disp);
      break;
    case STORE_INT:
      gsswx(reg, base, index, disp);
      break;
    case STORE_LONG:
      gssdx(reg, base, index, disp);
      break;
    case LOAD_BYTE:
      gslbx(reg, base, index, disp);
      break;
    case LOAD_SHORT:
      gslhx(reg, base, index, disp);
      break;
    case LOAD_INT:
      gslwx(reg, base, index, disp);
      break;
    case LOAD_LONG:
      gsldx(reg, base, index, disp);
      break;
    default:
      ShouldNotReachHere();
  }
}

void C2_MacroAssembler::gs_loadstore(FloatRegister reg, Register base, Register index, int disp, int type) {
  switch (type) {
    case STORE_FLOAT:
      gsswxc1(reg, base, index, disp);
      break;
    case STORE_DOUBLE:
      gssdxc1(reg, base, index, disp);
      break;
    case LOAD_FLOAT:
      gslwxc1(reg, base, index, disp);
      break;
    case LOAD_DOUBLE:
      gsldxc1(reg, base, index, disp);
      break;
    default:
      ShouldNotReachHere();
  }
}

void C2_MacroAssembler::loadstore(Register reg, Register base, int disp, int type) {
  switch (type) {
    case STORE_BYTE:
      sb(reg, base, disp);
      break;
    case STORE_CHAR:
    case STORE_SHORT:
      sh(reg, base, disp);
      break;
    case STORE_INT:
      sw(reg, base, disp);
      break;
    case STORE_LONG:
      sd(reg, base, disp);
      break;
    case LOAD_BYTE:
      lb(reg, base, disp);
      break;
    case LOAD_U_BYTE:
      lbu(reg, base, disp);
      break;
    case LOAD_SHORT:
      lh(reg, base, disp);
      break;
    case LOAD_U_SHORT:
      lhu(reg, base, disp);
      break;
    case LOAD_INT:
      lw(reg, base, disp);
      break;
    case LOAD_U_INT:
      lwu(reg, base, disp);
      break;
    case LOAD_LONG:
      ld(reg, base, disp);
      break;
    case LOAD_LINKED_LONG:
      lld(reg, base, disp);
      break;
     default:
       ShouldNotReachHere();
    }
}

void C2_MacroAssembler::loadstore(FloatRegister reg, Register base, int disp, int type) {
  switch (type) {
    case STORE_FLOAT:
      swc1(reg, base, disp);
      break;
    case STORE_DOUBLE:
      sdc1(reg, base, disp);
      break;
    case LOAD_FLOAT:
      lwc1(reg, base, disp);
      break;
    case LOAD_DOUBLE:
      ldc1(reg, base, disp);
      break;
     default:
       ShouldNotReachHere();
    }
}
