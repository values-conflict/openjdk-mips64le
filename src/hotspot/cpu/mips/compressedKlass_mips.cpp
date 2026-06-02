/*
 * Copyright (c) 2023, Red Hat, Inc. All rights reserved.
 * Copyright (c) 2023, Oracle and/or its affiliates. All rights reserved.
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

#include "oops/compressedKlass.hpp"
#include "utilities/globalDefinitions.hpp"

char* CompressedKlassPointers::reserve_address_space_for_compressed_classes(size_t size, bool aslr, bool optimize_for_zero_base) {

  char* result = nullptr;

  // MIPS64el loads a 64-bit immediate via lui/ori/dsll sequences.  A "good"
  // base minimises the number of instructions needed:
  // 1) base < 4GB -- fits in the lower 32 bits; most efficient encoding.
  // 2) base aligned to 4GB -- only the upper half needs loading.
  // 3) any other aligned address.

  // First, try to get a mapping below 4 GB (unscaled encoding, base = 0 if
  // optimize_for_zero_base, or a sub-4GB base otherwise).
  result = reserve_address_space_for_unscaled_encoding(size, aslr);

  // Failing that, try a 4 GB-aligned address below 4 TB.
  if (result == nullptr) {
    const uintptr_t from = nth_bit(32);
    constexpr uintptr_t to = nth_bit(42);
    constexpr size_t alignment = nth_bit(32);
    result = reserve_address_space_X(from, to, size, alignment, aslr);
  }

  return result;
}
