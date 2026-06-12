/*
 * Copyright (c) 2000, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2015, 2019, Loongson Technology. All rights reserved.
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

#include "code/vmreg.hpp"
#include "opto/compile.hpp"
#include "opto/node.hpp"
#include "opto/optoreg.hpp"

// processor dependent initialization for mips

extern void reg_mask_init();

// Fill the opto2vm array with the correct MIPS register mappings.
//
// This function exists because of a GCC 12 bug on MIPS64el: OptoReg::opto2vm
// is placed in BSS without runtime initialization.  The ADLC-generated static
// initializer in ad_mips.cpp references all_VMRegs via GOT entries that lack
// R_MIPS_REL32 relocations, so they resolve to zero.  Functions in OTHER
// object files (like this one) are not affected and have correct GOT entries.
//
// Called from c2compiler.cpp's init_c2_runtime() via C2Compiler's friend
// access to OptoReg, which passes the private array as an explicit pointer.
// The caller owns the array; this function only fills it.
//
// Order must match alloc_class chunk0/chunk1 in mips_64.ad exactly.
// See the TEMPORARY WORKAROUND comment in c2compiler.cpp for the full context
// and the description of what a proper long-term fix looks like.
void mips_opto2vm_fill(VMReg* p, int count) {
  // all_VMRegs has a correct R_MIPS_REL32+R_MIPS_64 GOT relocation here.
  VMReg base = VMRegImpl::as_VMReg(0);  // first() = all_VMRegs + 1
  int i = 0;
  // GPR order must match alloc_class chunk0 in mips_64.ad exactly.
  // Each register pair (lo/hi) occupies two consecutive opto-reg slots.
  // GPR encodings (MIPS N64): offset = encoding * 2.
  //
  // Callee-save registers (most preferred — values spanning calls land here):
  p[i++]=base+46; p[i++]=base+47; // S7  enc=23
  p[i++]=base+32; p[i++]=base+33; // S0  enc=16
  p[i++]=base+34; p[i++]=base+35; // S1  enc=17
  p[i++]=base+36; p[i++]=base+37; // S2  enc=18
  p[i++]=base+40; p[i++]=base+41; // S4  enc=20
  // S5 (enc=21): heapbase, not allocatable
  // S6 (enc=22): TREG, not allocatable
  p[i++]=base+38; p[i++]=base+39; // S3  enc=19
  //
  // General caller-save temporaries:
  p[i++]=base+28; p[i++]=base+29; // T2  enc=14
  p[i++]=base+30; p[i++]=base+31; // T3  enc=15
  p[i++]=base+48; p[i++]=base+49; // T8  enc=24
  p[i++]=base+50; p[i++]=base+51; // T9  enc=25
  p[i++]=base+26; p[i++]=base+27; // T1  enc=13
  p[i++]=base+ 6; p[i++]=base+ 7; // V1  enc= 3
  p[i++]=base+22; p[i++]=base+23; // A7  enc=11
  p[i++]=base+20; p[i++]=base+21; // A6  enc=10
  p[i++]=base+18; p[i++]=base+19; // A5  enc= 9
  p[i++]=base+16; p[i++]=base+17; // A4  enc= 8
  p[i++]=base+ 4; p[i++]=base+ 5; // V0  enc= 2
  p[i++]=base+14; p[i++]=base+15; // A3  enc= 7
  p[i++]=base+12; p[i++]=base+13; // A2  enc= 6
  p[i++]=base+10; p[i++]=base+11; // A1  enc= 5
  p[i++]=base+ 8; p[i++]=base+ 9; // A0  enc= 4
  p[i++]=base+24; p[i++]=base+25; // T0  enc=12 (receiver/"this")
  p[i++]=base+56; p[i++]=base+57; // GP  enc=28
  p[i++]=base+62; p[i++]=base+63; // RA  enc=31
  p[i++]=base+58; p[i++]=base+59; // SP  enc=29
  p[i++]=base+60; p[i++]=base+61; // FP  enc=30
  // FPR encodings: offset = 64 + encoding * 2
  p[i++]=base+64; p[i++]=base+65; // F0  enc= 0
  p[i++]=base+66; p[i++]=base+67; // F1  enc= 1
  p[i++]=base+68; p[i++]=base+69; // F2  enc= 2
  p[i++]=base+70; p[i++]=base+71; // F3  enc= 3
  p[i++]=base+72; p[i++]=base+73; // F4  enc= 4
  p[i++]=base+74; p[i++]=base+75; // F5  enc= 5
  p[i++]=base+76; p[i++]=base+77; // F6  enc= 6
  p[i++]=base+78; p[i++]=base+79; // F7  enc= 7
  p[i++]=base+80; p[i++]=base+81; // F8  enc= 8
  p[i++]=base+82; p[i++]=base+83; // F9  enc= 9
  p[i++]=base+84; p[i++]=base+85; // F10 enc=10
  p[i++]=base+86; p[i++]=base+87; // F11 enc=11
  p[i++]=base+104;p[i++]=base+105;// F20 enc=20
  p[i++]=base+106;p[i++]=base+107;// F21 enc=21
  p[i++]=base+108;p[i++]=base+109;// F22 enc=22
  p[i++]=base+110;p[i++]=base+111;// F23 enc=23
  p[i++]=base+112;p[i++]=base+113;// F24 enc=24
  p[i++]=base+114;p[i++]=base+115;// F25 enc=25
  p[i++]=base+116;p[i++]=base+117;// F26 enc=26
  p[i++]=base+118;p[i++]=base+119;// F27 enc=27
  p[i++]=base+120;p[i++]=base+121;// F28 enc=28
  p[i++]=base+102;p[i++]=base+103;// F19 enc=19
  p[i++]=base+100;p[i++]=base+101;// F18 enc=18
  p[i++]=base+98; p[i++]=base+99; // F17 enc=17
  p[i++]=base+96; p[i++]=base+97; // F16 enc=16
  p[i++]=base+94; p[i++]=base+95; // F15 enc=15
  p[i++]=base+92; p[i++]=base+93; // F14 enc=14
  p[i++]=base+90; p[i++]=base+91; // F13 enc=13
  p[i++]=base+88; p[i++]=base+89; // F12 enc=12
  p[i++]=base+122;p[i++]=base+123;// F29 enc=29
  p[i++]=base+124;p[i++]=base+125;// F30 enc=30
  p[i++]=base+126;p[i++]=base+127;// F31 enc=31
  // Non-allocatable registers (NS and non-alloc SOE) appended at end of OptoReg space.
  // ADLC omits them from the generated initializer; we initialize them here.
  // Order: NS registers first (R0, AT, AT_H, K0, K1), then SOE non-alloc (S5, S6).
  // Non-allocatable registers (NS and non-alloc SOE) appended in reg_def order.
  // NS: R0(0), AT/AT_H(1), K0(26), K1(27).
  // SOE non-alloc: S5/S5_H(21, heapbase) and S6/S6_H(22, TREG) removed from alloc_class.
  p[i++]=base- 1;               // R0   (enc= 0): VMRegImpl::Bad()
  p[i++]=base+ 2;p[i++]=base+ 3;// AT/AT_H (enc= 1)
  p[i++]=base+42;p[i++]=base+43;// S5/S5_H (enc=21): not allocatable (heapbase)
  p[i++]=base+44; p[i++]=base+45; // S6/S6_H (enc=22): not allocatable (TREG)
  p[i++]=base+52;               // K0   (enc=26)
  p[i++]=base+54;               // K1   (enc=27)
  assert(i == count, "opto2vm count mismatch");
}

void Compile::pd_compiler2_init() {
  guarantee(CodeEntryAlignment >= InteriorEntryAlignment, "");
  reg_mask_init();
}
