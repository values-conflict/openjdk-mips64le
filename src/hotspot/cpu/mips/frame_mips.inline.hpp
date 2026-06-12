/*
 * Copyright (c) 1997, 2013, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2015, 2022, Loongson Technology. All rights reserved.
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

#ifndef CPU_MIPS_FRAME_MIPS_INLINE_HPP
#define CPU_MIPS_FRAME_MIPS_INLINE_HPP

#include "code/codeCache.hpp"
#include "code/codeCache.inline.hpp"
#include "code/vmreg.inline.hpp"
#include "gc/shared/barrierSetNMethod.hpp"
#include "runtime/stackWatermarkSet.hpp"
#include "runtime/sharedRuntime.hpp"

// Inline functions for Loongson frames:

// Constructors:

inline frame::frame() {
  _pc = nullptr;
  _sp = nullptr;
  _unextended_sp = nullptr;
  _fp = nullptr;
  _cb = nullptr;
  _oop_map = nullptr;
  _deopt_state = unknown;
  DEBUG_ONLY(_frame_index = -1;)
}

inline void frame::init(intptr_t* sp, intptr_t* fp, address pc) {
  _sp = sp;
  _unextended_sp = sp;
  _fp = fp;
  _pc = pc;
  _oop_map = nullptr;
  DEBUG_ONLY(_frame_index = -1;)
  assert(pc != nullptr, "no pc?");
  _cb = CodeCache::find_blob(pc);
  adjust_unextended_sp();

  address original_pc = get_deopt_original_pc();
  if (original_pc != nullptr) {
    _pc = original_pc;
    _deopt_state = is_deoptimized;
  } else {
    _deopt_state = not_deoptimized;
  }
}

inline frame::frame(intptr_t* sp, intptr_t* fp, address pc) {
  init(sp, fp, pc);
}

inline frame::frame(intptr_t* sp, intptr_t* unextended_sp, intptr_t* fp, address pc) {
  DEBUG_ONLY(_frame_index = -1;)
  _sp = sp;
  _unextended_sp = unextended_sp;
  _fp = fp;
  _pc = pc;
  _oop_map = nullptr;
  assert(pc != nullptr, "no pc?");
  _cb = CodeCache::find_blob(pc);
  adjust_unextended_sp();

  address original_pc = get_deopt_original_pc();
  if (original_pc != nullptr) {
    _pc = original_pc;
    _deopt_state = is_deoptimized;
  } else {
    _deopt_state = not_deoptimized;
  }
}

inline frame::frame(intptr_t* sp, intptr_t* unextended_sp, intptr_t* fp, address pc, CodeBlob* cb) {
  DEBUG_ONLY(_frame_index = -1;)
  _sp = sp;
  _unextended_sp = unextended_sp;
  _fp = fp;
  _pc = pc;
  _cb = cb;
  _oop_map = nullptr;
  assert(_cb != nullptr, "must have codeblob");
  setup(pc);
}

inline frame::frame(intptr_t* sp, intptr_t* unextended_sp, intptr_t* fp, address pc, CodeBlob* cb, const ImmutableOopMap* oop_map, bool on_heap) {
  DEBUG_ONLY(_frame_index = -1;)
  _sp = sp;
  _unextended_sp = unextended_sp;
  _fp = fp;
  _pc = pc;
  _cb = cb;
  _oop_map = oop_map;
  _on_heap = on_heap;
  assert(_on_heap || _cb != nullptr, "");
  if (!_on_heap) {
    setup(pc);
  }
}

inline void frame::setup(address pc) {
  adjust_unextended_sp();
  address original_pc = get_deopt_original_pc();
  if (original_pc != nullptr) {
    _pc = original_pc;
    _deopt_state = is_deoptimized;
  } else {
    if (_cb == SharedRuntime::deopt_blob()) {
      _deopt_state = is_deoptimized;
    } else {
      _deopt_state = not_deoptimized;
    }
  }
}

inline frame::frame(intptr_t* sp, intptr_t* fp) {
  DEBUG_ONLY(_frame_index = -1;)
  _sp = sp;
  _unextended_sp = sp;
  _fp = fp;
  _pc = (address)(sp[-1]);
  _oop_map = nullptr;

  // Here's a sticky one. This constructor can be called via AsyncGetCallTrace
  // when last_Java_sp is non-null but the pc fetched is junk. If we are truly
  // unlucky the junk value could be to a zombied method and we'll die on the
  // find_blob call. This is also why we can have no asserts on the validity
  // of the pc we find here. AsyncGetCallTrace -> pd_get_top_frame_for_signal_handler
  // -> pd_last_frame should use a specialized version of pd_last_frame which could
  // call a specilaized frame constructor instead of this one.
  // Then we could use the assert below. However this assert is of somewhat dubious
  // value.
  // assert(_pc != nullptr, "no pc?");

  _cb = CodeCache::find_blob(_pc);
  adjust_unextended_sp();
  address original_pc = get_deopt_original_pc();
  if (original_pc != nullptr) {
    _pc = original_pc;
    _deopt_state = is_deoptimized;
  } else {
    _deopt_state = not_deoptimized;
  }
}

// Accessors

inline bool frame::equal(frame other) const {
  bool ret =  sp() == other.sp()
              && unextended_sp() == other.unextended_sp()
              && fp() == other.fp()
              && pc() == other.pc();
  assert(!ret || ret && cb() == other.cb() && _deopt_state == other._deopt_state, "inconsistent construction");
  return ret;
}

// Return unique id for this frame. The id must have a value where we can distinguish
// identity and younger/older relationship. NULL represents an invalid (incomparable)
// frame.
inline intptr_t* frame::id(void) const { return unextended_sp(); }

// Relationals on frames based
// Return true if the frame is younger (more recent activation) than the frame represented by id
inline bool frame::is_younger(intptr_t* id) const { assert(this->id() != nullptr && id != nullptr, "NULL frame id");
                                                    return this->id() < id ; }

// Return true if the frame is older (less recent activation) than the frame represented by id
inline bool frame::is_older(intptr_t* id) const   { assert(this->id() != nullptr && id != nullptr, "NULL frame id");
                                                    return this->id() > id ; }

inline intptr_t* frame::link() const {
  return (intptr_t*) *(intptr_t **)addr_at(link_offset);
}

inline intptr_t* frame::link_or_null() const {
  intptr_t** ptr = (intptr_t **)addr_at(link_offset);
  return os::is_readable_pointer(ptr) ? *ptr : NULL;
}

inline intptr_t* frame::unextended_sp() const     { return _unextended_sp; }

// Return address:

inline address* frame::sender_pc_addr()      const { return (address*) addr_at( return_addr_offset); }
inline address  frame::sender_pc()           const { return *sender_pc_addr(); }

inline intptr_t*    frame::sender_sp()        const { return            addr_at(   sender_sp_offset); }


inline intptr_t* frame::interpreter_frame_last_sp() const {
  // Stored as a signed word-offset from fp(); 0 means null (no last_sp set).
  intptr_t n = *addr_at(interpreter_frame_last_sp_offset);
  return n != 0 ? &fp()[n] : nullptr;
}

inline intptr_t* frame::interpreter_frame_bcp_addr() const {
  return (intptr_t*)addr_at(interpreter_frame_bcp_offset);
}


inline intptr_t* frame::interpreter_frame_mdp_addr() const {
  return (intptr_t*)addr_at(interpreter_frame_mdp_offset);
}



// Constant pool cache

inline ConstantPoolCache** frame::interpreter_frame_cache_addr() const {
  return (ConstantPoolCache**)addr_at(interpreter_frame_cache_offset);
}

// Method

inline Method** frame::interpreter_frame_method_addr() const {
  return (Method**)addr_at(interpreter_frame_method_offset);
}

// Mirror

inline oop* frame::interpreter_frame_mirror_addr() const {
  return (oop*)addr_at(interpreter_frame_mirror_offset);
}

// top of expression stack
inline intptr_t* frame::interpreter_frame_tos_address() const {
  intptr_t* last_sp = interpreter_frame_last_sp();
  if (last_sp == nullptr ) {
    return sp();
  } else {
    // sp() may have been extended by an adapter
    assert(last_sp <= (intptr_t*)interpreter_frame_monitor_end(), "bad tos");
    return last_sp;
  }
}

inline oop* frame::interpreter_frame_temp_oop_addr() const {
  return (oop *)(fp() + interpreter_frame_oop_temp_offset);
}

inline int frame::interpreter_frame_monitor_size() {
  return BasicObjectLock::size();
}


// expression stack
// (the max_stack arguments are used by the GC; see class FrameClosure)

inline intptr_t* frame::interpreter_frame_expression_stack() const {
  intptr_t* monitor_end = (intptr_t*) interpreter_frame_monitor_end();
  return monitor_end-1;
}

// Entry frames

inline JavaCallWrapper** frame::entry_frame_call_wrapper_addr() const {
  return (JavaCallWrapper**)addr_at(entry_frame_call_wrapper_offset);
}

// Compiled frames

inline oop frame::saved_oop_result(RegisterMap* map) const       {
  return *((oop*) map->location(V0->as_VMReg(), sp()));
}

inline void frame::set_saved_oop_result(RegisterMap* map, oop obj) {
  *((oop*) map->location(V0->as_VMReg(), sp())) = obj;
}

template <typename RegisterMapT>
void frame::update_map_with_saved_link(RegisterMapT* map, intptr_t** link_addr) {
  assert(map != nullptr, "map must be set");
  map->set_location(FP->as_VMReg(), (address) link_addr);
  map->set_location(FP->as_VMReg()->next(), (address) link_addr);
}

#if INCLUDE_JFR
// JFR static frame accessor helpers (MIPS-specific offsets)
inline address frame::interpreter_bcp(const intptr_t* fp) {
  assert(fp != nullptr, "invariant");
  return reinterpret_cast<address>(fp[frame::interpreter_frame_bcp_offset]);
}

inline address frame::interpreter_return_address(const intptr_t* fp) {
  assert(fp != nullptr, "invariant");
  return reinterpret_cast<address>(fp[frame::return_addr_offset]); // fp[1] on MIPS
}

inline intptr_t* frame::interpreter_sender_sp(const intptr_t* fp) {
  assert(fp != nullptr, "invariant");
  return reinterpret_cast<intptr_t*>(fp[frame::interpreter_frame_sender_sp_offset]);
}

inline bool frame::is_interpreter_frame_setup_at(const intptr_t* fp, const void* sp) {
  assert(fp != nullptr, "invariant");
  assert(sp != nullptr, "invariant");
  return sp <= fp + frame::interpreter_frame_initial_sp_offset;
}

inline intptr_t* frame::sender_sp(intptr_t* fp) {
  assert(fp != nullptr, "invariant");
  return fp + frame::sender_sp_offset;
}

inline intptr_t* frame::link(const intptr_t* fp) {
  assert(fp != nullptr, "invariant");
  return reinterpret_cast<intptr_t*>(fp[frame::link_offset]); // fp[0] on MIPS
}

inline intptr_t* frame::fp(const intptr_t* sp) {
  assert(sp != nullptr, "invariant");
  return reinterpret_cast<intptr_t*>(sp[frame::sender_sp_offset]); // MIPS: sp+2 is sender sp
}
#endif // INCLUDE_JFR

// frame::frame(intptr_t* sp) — construct from raw stack pointer
inline frame::frame(intptr_t* sp)
  : frame(sp, sp,
          *(intptr_t**)(sp + frame::link_offset),
          *(address*)(sp + frame::return_addr_offset)) {}

// frame::sender_sp_ret_address_offset
inline int frame::sender_sp_ret_address_offset() {
  return frame::sender_sp_offset - frame::return_addr_offset;
}

// frame::return_address - return address is stored at sp[-1] on MIPS64
inline address frame::return_address(const intptr_t* sp) {
  assert(sp != nullptr, "invariant");
  return reinterpret_cast<address>(sp[-1]);
}

// frame::interpreter_frame_set_locals
inline void frame::interpreter_frame_set_locals(intptr_t* locs) {
  assert(is_interpreted_frame(), "interpreted frame expected");
  ptr_at_put(interpreter_frame_locals_offset, (intptr_t)(locs - fp()));
}

// frame::interpreter_frame_locals
inline intptr_t* frame::interpreter_frame_locals() const {
  intptr_t n = *addr_at(interpreter_frame_locals_offset);
  return &fp()[n]; // relativized locals
}

// frame::frame_size
inline int frame::frame_size() const {
  return is_interpreted_frame()
    ? pointer_delta_as_int(sender_sp(), sp())
    : cb()->frame_size();
}

// frame::compiled_frame_stack_argsize
inline int frame::compiled_frame_stack_argsize() const {
  assert(cb()->is_nmethod(), "");
  return (cb()->as_nmethod()->num_stack_arg_slots() * VMRegImpl::stack_slot_size) >> LogBytesPerWord;
}

// frame unextended_sp accessors for stack-chunk frames
inline void frame::set_unextended_sp(intptr_t* value)   { _unextended_sp = value; }
inline int  frame::offset_unextended_sp() const          { assert_offset(); return _offset_unextended_sp; }
inline void frame::set_offset_unextended_sp(int value)   { assert_on_heap(); _offset_unextended_sp = value; }

// frame::sender_for_compiled_frame
inline frame frame::sender_for_compiled_frame(RegisterMap* map) const {
  assert(map != nullptr, "map must be set");
  assert(_cb->frame_size() >= 0, "must have non-zero frame size");

  intptr_t* sender_sp = unextended_sp() + _cb->frame_size();
  intptr_t* unextended_sp = sender_sp;
  address sender_pc = (address) *(sender_sp - 1);
  intptr_t** saved_fp_addr = (intptr_t**) (sender_sp - frame::sender_sp_offset);

  if (map->update_map()) {
    map->set_include_argument_oops(_cb->caller_must_gc_arguments(map->thread()));
    if (oop_map() != nullptr) {
      oop_map()->update_register_map(this, map);
    }
    update_map_with_saved_link(map, saved_fp_addr);
  }
  assert(sender_sp != sp(), "must have changed");
  return frame(sender_sp, unextended_sp, *saved_fp_addr, sender_pc);
}

// frame::upcall_stub_frame_is_first
inline bool frame::upcall_stub_frame_is_first() const {
  return false; // MIPS does not support upcall stubs yet
}

// frame::sender_for_upcall_stub_frame
inline frame frame::sender_for_upcall_stub_frame(RegisterMap* map) const {
  ShouldNotCallThis();
  return frame();
}

// frame::sender -- dispatches to sender_raw and processes StackWatermarks
frame frame::sender(RegisterMap* map) const {
  frame result = sender_raw(map);
  if (map->process_frames() && !map->in_cont()) {
    StackWatermarkSet::on_iteration(map->thread(), result);
  }
  return result;
}

// frame::sender_raw -- selects the appropriate sender_for_* method
inline frame frame::sender_raw(RegisterMap* map) const {
  assert(map != nullptr, "map must be set");
  map->set_include_argument_oops(false);

  if (map->in_cont()) {
    return map->stack_chunk()->sender(*this, map);
  }

  if (is_entry_frame()) {
    return sender_for_entry_frame(map);
  }
  if (is_interpreted_frame()) {
    return sender_for_interpreter_frame(map);
  }

  assert(_cb == CodeCache::find_blob(pc()), "Must be the same");
  if (_cb != nullptr) {
    return sender_for_compiled_frame(map);
  }

  // native C frame
  intptr_t* sender_sp = (intptr_t*) addr_at(sender_sp_offset);
  intptr_t* sender_fp = (intptr_t*) at(link_offset);
  address   sender_pc = (address) at(return_addr_offset);
  return frame(sender_sp, sender_fp, sender_pc);
}

inline void frame::interpreted_frame_oop_map(InterpreterOopMap* mask) const {
  assert(mask != nullptr, "");
  Method* m = interpreter_frame_method();
  int   bci = interpreter_frame_bci();
  m->mask_for(bci, mask);
}

#endif // CPU_MIPS_FRAME_MIPS_INLINE_HPP
