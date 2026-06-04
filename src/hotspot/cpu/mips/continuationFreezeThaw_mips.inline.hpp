/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2022, Loongson Technology. All rights reserved.
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

#ifndef CPU_MIPS_CONTINUATIONFREEZETHAW_MIPS_INLINE_HPP
#define CPU_MIPS_CONTINUATIONFREEZETHAW_MIPS_INLINE_HPP

#include "code/codeBlob.inline.hpp"
#include "oops/stackChunkOop.inline.hpp"
#include "runtime/frame.hpp"
#include "runtime/frame.inline.hpp"


inline void patch_callee_link(const frame& f, intptr_t* fp) {
  DEBUG_ONLY(intptr_t* orig = *ContinuationHelper::Frame::callee_link_address(f));
  *ContinuationHelper::Frame::callee_link_address(f) = fp;
}

inline void patch_callee_link_relative(const frame& f, intptr_t* fp) {
  intptr_t* la = (intptr_t*)ContinuationHelper::Frame::callee_link_address(f);
  intptr_t new_value = fp - la;
  *la = new_value;
}

////// Freeze

// Fast path

inline void FreezeBase::patch_stack_pd(intptr_t* frame_sp, intptr_t* heap_sp) {
  // copy the spilled fp from the heap to the stack
  *(frame_sp - 2) = *(heap_sp - 2);
}

// Slow path

template<typename FKind>
inline frame FreezeBase::sender(const frame& f) {
  assert(FKind::is_instance(f), "");
  if (FKind::interpreted) {
    return frame(f.sender_sp(), f.interpreter_frame_sender_sp(), f.link(), f.sender_pc());
  }

  intptr_t** link_addr = link_address<FKind>(f);
  intptr_t* sender_sp = (intptr_t*)(link_addr + 2);
  address sender_pc = (address) *(sender_sp - 1);
  assert(sender_sp != f.sp(), "must have changed");

  int slot = 0;
  CodeBlob* sender_cb = CodeCache::find_blob_and_oopmap(sender_pc, slot);
  return sender_cb != nullptr
    ? frame(sender_sp, sender_sp, *link_addr, sender_pc, sender_cb,
            slot == -1 ? nullptr : sender_cb->oop_map_for_slot(slot, sender_pc),
            false /* on_heap ? */)
    : frame(sender_sp, sender_sp, *link_addr, sender_pc);
}

template<typename FKind> frame FreezeBase::new_heap_frame(frame& f, frame& caller) {
  assert(FKind::is_instance(f), "");
  assert(!caller.is_interpreted_frame()
    || caller.unextended_sp() == (intptr_t*)caller.at(frame::interpreter_frame_last_sp_offset), "");

  intptr_t *sp, *fp;
  if (FKind::interpreted) {
    assert((intptr_t*)f.at(frame::interpreter_frame_last_sp_offset) == nullptr
      || f.unextended_sp() == (intptr_t*)f.at_relative(frame::interpreter_frame_last_sp_offset), "");
    intptr_t locals_offset = *f.addr_at(frame::interpreter_frame_locals_offset);
    bool overlap_caller = caller.is_interpreted_frame() || caller.is_empty();
    fp = caller.unextended_sp() - 1 - locals_offset + (overlap_caller ? ContinuationHelper::InterpretedFrame::stack_argsize(f) : 0);
    sp = fp - (f.fp() - f.unextended_sp());
    assert(sp <= fp, "");
    assert(fp <= caller.unextended_sp(), "");
    caller.set_sp(fp + frame::sender_sp_offset);

    assert(_cont.tail()->is_in_chunk(sp), "");

    frame hf(sp, sp, fp, f.pc(), nullptr, nullptr, true /* on_heap */);
    *hf.addr_at(frame::interpreter_frame_locals_offset) = locals_offset;
    return hf;
  } else {
    fp = *(intptr_t**)(f.sp() - 2);

    int fsize = FKind::size(f);
    sp = caller.unextended_sp() - fsize;
    if (caller.is_interpreted_frame()) {
      int argsize = FKind::stack_argsize(f);
      sp -= argsize;
    }
    caller.set_sp(sp + fsize);

    assert(_cont.tail()->is_in_chunk(sp), "");

    return frame(sp, sp, fp, f.pc(), nullptr, nullptr, true /* on_heap */);
  }
}

void FreezeBase::adjust_interpreted_frame_unextended_sp(frame& f) {
  assert((f.at(frame::interpreter_frame_last_sp_offset) != 0) || (f.unextended_sp() == f.sp()), "");
  intptr_t* real_unextended_sp = (intptr_t*)f.at_relative_or_null(frame::interpreter_frame_last_sp_offset);
  if (real_unextended_sp != nullptr) {
    f.set_unextended_sp(real_unextended_sp);
  }
}

inline void FreezeBase::prepare_freeze_interpreted_top_frame(frame& f) {
  assert(f.interpreter_frame_last_sp() == nullptr, "should be null for top frame");
  f.interpreter_frame_set_last_sp(f.unextended_sp());
}

inline void FreezeBase::relativize_interpreted_frame_metadata(const frame& f, const frame& hf) {
  assert(hf.fp() == hf.unextended_sp() + (f.fp() - f.unextended_sp()), "");
  assert((f.at(frame::interpreter_frame_last_sp_offset) != 0)
    || (f.unextended_sp() == f.sp()), "");
  assert(f.fp() > (intptr_t*)f.at_relative(frame::interpreter_frame_initial_sp_offset), "");

  assert((intptr_t*)hf.at_relative(frame::interpreter_frame_last_sp_offset) == hf.unextended_sp(), "");
  assert(hf.at_absolute(frame::interpreter_frame_monitor_block_top_offset) <= frame::interpreter_frame_initial_sp_offset, "");

  assert((hf.fp() - hf.unextended_sp()) == (f.fp() - f.unextended_sp()), "");
  assert(hf.unextended_sp() == (intptr_t*)hf.at(frame::interpreter_frame_last_sp_offset), "");
  assert(hf.unextended_sp() <= (intptr_t*)hf.at(frame::interpreter_frame_initial_sp_offset), "");
  assert(hf.fp()            >  (intptr_t*)hf.at(frame::interpreter_frame_initial_sp_offset), "");

}

inline void FreezeBase::set_top_frame_metadata_pd(const frame& hf) {
  stackChunkOop chunk = _cont.tail();
  assert(chunk->is_in_chunk(hf.sp() - 1), "");
  assert(chunk->is_in_chunk(hf.sp() - 2), "");

  *(hf.sp() - 1) = (intptr_t)hf.pc();

  intptr_t* fp_addr = hf.sp() - 2;
  *fp_addr = hf.is_interpreted_frame() ? (intptr_t)(hf.fp() - fp_addr)
                                       : (intptr_t)hf.fp();
}

inline void FreezeBase::patch_pd(frame& hf, const frame& caller) {
  if (caller.is_interpreted_frame()) {
    assert(!caller.is_empty(), "");
    patch_callee_link_relative(caller, caller.fp());
  } else {
    patch_callee_link(caller, caller.fp());
  }
}

//////// Thaw

// Fast path

inline void ThawBase::prefetch_chunk_pd(void* start, int size) {
  size <<= LogBytesPerWord;
  Prefetch::read(start, size);
  Prefetch::read(start, size - 64);
}

template <typename ConfigT>
inline void Thaw<ConfigT>::patch_caller_links(intptr_t* sp, intptr_t* bottom) {
  assert(!PreserveFramePointer, "Frame pointers need to be fixed");
}

// Slow path

inline frame ThawBase::new_entry_frame() {
  intptr_t* sp = _cont.entrySP();
  return frame(sp, sp, _cont.entryFP(), _cont.entryPC());
}

template<typename FKind> frame ThawBase::new_stack_frame(const frame& hf, frame& caller, bool bottom) {
  assert(FKind::is_instance(hf), "");

  if (FKind::interpreted) {
    intptr_t* heap_sp = hf.unextended_sp();
    int overlap = caller.is_interpreted_frame() ? ContinuationHelper::InterpretedFrame::stack_argsize(hf) : 0;
    const int fsize = (int)(ContinuationHelper::InterpretedFrame::frame_bottom(hf) - hf.unextended_sp() - overlap);
    intptr_t* frame_sp = caller.unextended_sp() - fsize;
    intptr_t* fp = frame_sp + (hf.fp() - heap_sp);
    DEBUG_ONLY(intptr_t* unextended_sp = fp + *hf.addr_at(frame::interpreter_frame_last_sp_offset);)
    assert(frame_sp == unextended_sp, "");
    caller.set_sp(fp + frame::sender_sp_offset);
    frame f(frame_sp, frame_sp, fp, hf.pc());
    *f.addr_at(frame::interpreter_frame_locals_offset) = *hf.addr_at(frame::interpreter_frame_locals_offset);
    return f;
  } else {
    int fsize = FKind::size(hf);
    intptr_t* frame_sp = caller.unextended_sp() - fsize;
    if (bottom || caller.is_interpreted_frame()) {
      int argsize = FKind::stack_argsize(hf);

      fsize += argsize;
      frame_sp -= argsize;
      caller.set_sp(caller.sp() - argsize);
      assert(caller.sp() == frame_sp + (fsize-argsize), "");

      frame_sp = align(hf, frame_sp, caller, bottom);
    }

    assert(hf.cb() != nullptr, "");
    assert(hf.oop_map() != nullptr, "");
    intptr_t* fp;
    if (PreserveFramePointer) {
      fp = frame_sp + FKind::size(hf) - frame::sender_sp_offset;
    } else {
      fp = FKind::stub || FKind::native
        ? frame_sp + fsize - frame::sender_sp_offset
        : *(intptr_t**)(hf.sp() - 2);
    }
    return frame(frame_sp, frame_sp, fp, hf.pc(), hf.cb(), hf.oop_map(), false);
  }
}

inline intptr_t* ThawBase::align(const frame& hf, intptr_t* frame_sp, frame& caller, bool bottom) {
#ifdef _LP64
  if (((intptr_t)frame_sp % frame::frame_alignment) != 0) {
    frame_sp--;
    caller.set_sp(caller.sp() - 1);
  }
  assert(is_aligned(frame_sp, frame::frame_alignment), "");
#endif

  return frame_sp;
}

inline void ThawBase::patch_pd(frame& f, const frame& caller) {
  patch_callee_link(caller, caller.fp());
}

inline void ThawBase::patch_pd(frame& f, intptr_t* caller_sp) {
  intptr_t* fp = caller_sp - frame::sender_sp_offset;
  patch_callee_link(f, fp);
}

inline intptr_t* ThawBase::push_cleanup_continuation() {
  frame enterSpecial = new_entry_frame();
  intptr_t* sp = enterSpecial.sp();

  sp[-1] = (intptr_t)ContinuationEntry::cleanup_pc();
  sp[-2] = (intptr_t)enterSpecial.fp();

  log_develop_trace(continuations, preempt)("push_cleanup_continuation initial sp: " INTPTR_FORMAT " final sp: " INTPTR_FORMAT, p2i(sp + 2 * frame::metadata_words), p2i(sp));
  return sp;
}

inline void ThawBase::derelativize_interpreted_frame_metadata(const frame& hf, const frame& f) {
  assert((intptr_t*)f.at_relative(frame::interpreter_frame_last_sp_offset) == f.unextended_sp(), "");
  assert(f.at_absolute(frame::interpreter_frame_monitor_block_top_offset) <= frame::interpreter_frame_initial_sp_offset, "");

  // After thaw, FP[-9] (initial_sp / monitor_block_top) is the stale value from the
  // original VT stack.  remove_activation walks from FP[-9] down to frame_sp checking
  // for locked BasicObjectLocks.  If FP[-9] points outside the new frame (different
  // carrier or different entry_sp), the scan reads garbage and may find a non-null
  // obj → IllegalMonitorStateException.  Reset it to f.sp() so the monitor block
  // appears empty (correct for frames without synchronized blocks).
  // generate_fixed_frame stores `SP` at FP[-9] (= FP - 9*wordSize = actual frame bottom).
  // After thaw the frame is at a new address, so this stale value is wrong.
  // remove_activation reads FP[-9] and scans from there to FP[-9]'s address
  // looking for locked monitors; if they differ it scans garbage and may throw IMSE.
  // Restore FP[-9] = (address of FP[-9]) so the monitor block appears empty.
  intptr_t* initial_sp_addr = f.addr_at(frame::interpreter_frame_initial_sp_offset);
  *initial_sp_addr = (intptr_t)initial_sp_addr;
}

#endif // CPU_MIPS_CONTINUATIONFREEZETHAW_MIPS_INLINE_HPP
