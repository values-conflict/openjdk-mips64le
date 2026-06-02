/*
 * Copyright (c) 2000, 2012, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2015, 2024, Loongson Technology. All rights reserved.
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

#ifndef CPU_MIPS_REGISTER_MIPS_HPP
#define CPU_MIPS_REGISTER_MIPS_HPP

#include "asm/register.hpp"
#include "utilities/powerOfTwo.hpp"

class VMRegImpl;
typedef VMRegImpl* VMReg;

// ======================================================================
// Integer registers
// ======================================================================

class Register {
 private:
  int _encoding;

  constexpr explicit Register(int encoding) : _encoding(encoding) {}

 public:
  enum {
    number_of_registers    = 32,
    max_slots_per_register = 2,
  };

  class RegisterImpl: public AbstractRegisterImpl {
    friend class Register;

    static constexpr const RegisterImpl* first();

   public:
    constexpr int raw_encoding() const { return checked_cast<int>(this - first()); }
    constexpr int     encoding() const { assert(is_valid(), "invalid register"); return raw_encoding(); }
    constexpr bool    is_valid() const { return 0 <= raw_encoding() && raw_encoding() < number_of_registers; }

    inline Register successor() const;

    VMReg as_VMReg() const;

    const char* name() const;
  };

  inline friend constexpr Register as_Register(int encoding);

  constexpr Register() : _encoding(-1) {} // noreg

  int operator==(const Register r) const { return _encoding == r._encoding; }
  int operator!=(const Register r) const { return _encoding != r._encoding; }

  constexpr const RegisterImpl* operator->() const { return RegisterImpl::first() + _encoding; }
};

extern Register::RegisterImpl all_RegisterImpls[Register::number_of_registers + 1] INTERNAL_VISIBILITY;

inline constexpr const Register::RegisterImpl* Register::RegisterImpl::first() {
  return all_RegisterImpls + 1;
}

constexpr Register noreg = Register();

inline constexpr Register as_Register(int encoding) {
  if (0 <= encoding && encoding < Register::number_of_registers) {
    return Register(encoding);
  }
  return noreg;
}

inline Register Register::RegisterImpl::successor() const {
  assert(is_valid(), "sanity");
  return as_Register(encoding() + 1);
}

// MIPS64 integer registers (r0-r31 = encoding 0-31)
constexpr Register r0  = as_Register( 0);  // zero
constexpr Register r1  = as_Register( 1);  // at  (assembler temporary)
constexpr Register r2  = as_Register( 2);  // v0  (return value)
constexpr Register r3  = as_Register( 3);  // v1  (return value)
constexpr Register r4  = as_Register( 4);  // a0  (argument 0)
constexpr Register r5  = as_Register( 5);  // a1
constexpr Register r6  = as_Register( 6);  // a2
constexpr Register r7  = as_Register( 7);  // a3
constexpr Register r8  = as_Register( 8);  // a4  (n64 ABI)
constexpr Register r9  = as_Register( 9);  // a5
constexpr Register r10 = as_Register(10);  // a6
constexpr Register r11 = as_Register(11);  // a7
constexpr Register r12 = as_Register(12);  // t0  (temporary)
constexpr Register r13 = as_Register(13);  // t1
constexpr Register r14 = as_Register(14);  // t2
constexpr Register r15 = as_Register(15);  // t3
constexpr Register r16 = as_Register(16);  // s0  (callee-saved)
constexpr Register r17 = as_Register(17);  // s1
constexpr Register r18 = as_Register(18);  // s2
constexpr Register r19 = as_Register(19);  // s3
constexpr Register r20 = as_Register(20);  // s4
constexpr Register r21 = as_Register(21);  // s5
constexpr Register r22 = as_Register(22);  // s6  (Java thread register)
constexpr Register r23 = as_Register(23);  // s7
constexpr Register r24 = as_Register(24);  // t8
constexpr Register r25 = as_Register(25);  // t9
constexpr Register r26 = as_Register(26);  // k0  (kernel use)
constexpr Register r27 = as_Register(27);  // k1
constexpr Register r28 = as_Register(28);  // gp  (global pointer)
constexpr Register r29 = as_Register(29);  // sp  (stack pointer)
constexpr Register r30 = as_Register(30);  // fp / s8
constexpr Register r31 = as_Register(31);  // ra  (return address)

// Numeric aliases (I-prefixed, for backward compat with jdk17u code)
constexpr Register I0  = r0;
constexpr Register I1  = r1;
constexpr Register I2  = r2;
constexpr Register I3  = r3;
constexpr Register I4  = r4;
constexpr Register I5  = r5;
constexpr Register I6  = r6;
constexpr Register I7  = r7;
constexpr Register I8  = r8;
constexpr Register I9  = r9;
constexpr Register I10 = r10;
constexpr Register I11 = r11;
constexpr Register I12 = r12;
constexpr Register I13 = r13;
constexpr Register I14 = r14;
constexpr Register I15 = r15;
constexpr Register I16 = r16;
constexpr Register I17 = r17;
constexpr Register I18 = r18;
constexpr Register I19 = r19;
constexpr Register I20 = r20;
constexpr Register I21 = r21;
constexpr Register I22 = r22;
constexpr Register I23 = r23;
constexpr Register I24 = r24;
constexpr Register I25 = r25;
constexpr Register I26 = r26;
constexpr Register I27 = r27;
constexpr Register I28 = r28;
constexpr Register I29 = r29;
constexpr Register I30 = r30;
constexpr Register I31 = r31;

// ABI register names
constexpr Register NOREG = noreg;
constexpr Register R0    = r0;   // zero register
constexpr Register AT    = r1;   // assembler temporary
constexpr Register V0    = r2;   // return value / first result
constexpr Register V1    = r3;   // second result
constexpr Register RA0   = r4;   // argument registers (n64 ABI: a0-a7)
constexpr Register RA1   = r5;
constexpr Register RA2   = r6;
constexpr Register RA3   = r7;
constexpr Register RA4   = r8;
constexpr Register RA5   = r9;
constexpr Register RA6   = r10;
constexpr Register RA7   = r11;
constexpr Register RT0   = r12;  // temporary registers
constexpr Register RT1   = r13;
constexpr Register RT2   = r14;
constexpr Register RT3   = r15;
constexpr Register S0    = r16;  // callee-saved registers
constexpr Register S1    = r17;
constexpr Register S2    = r18;
constexpr Register S3    = r19;
constexpr Register S4    = r20;
constexpr Register S5    = r21;
constexpr Register S6    = r22;
constexpr Register S7    = r23;
constexpr Register RT8   = r24;  // temporaries
constexpr Register RT9   = r25;
constexpr Register K0    = r26;  // kernel-reserved
constexpr Register K1    = r27;
constexpr Register GP    = r28;  // global pointer
constexpr Register SP    = r29;  // stack pointer
constexpr Register FP    = r30;  // frame pointer (also s8)
constexpr Register S8    = r30;  // same as FP
constexpr Register RA    = r31;  // return address

// Caller-save temporaries in ABI order
constexpr Register T0    = RT0;
constexpr Register T1    = RT1;
constexpr Register T2    = RT2;
constexpr Register T3    = RT3;
constexpr Register T8    = RT8;
constexpr Register T9    = RT9;

// Use S6 as dedicated Java thread register (OPT_THREAD avoids Thread::current() calls)
#define OPT_THREAD 1

// Interpreter / JIT special registers
constexpr Register TREG  = S6;   // Java thread register
constexpr Register BCP   = S0;   // bytecode pointer
constexpr Register LVP   = S7;   // local variable pointer
constexpr Register TSR   = S2;   // temporary callee-saved
constexpr Register Rmethod  = S3;
constexpr Register Rsender  = S4;
constexpr Register Rnext    = S1;

constexpr Register S5_heapbase = S5;
constexpr Register mh_SP_save  = SP;

// C calling convention argument registers
constexpr Register c_rarg0 = RT0;
constexpr Register c_rarg1 = RT1;
constexpr Register c_rarg2 = RT2;
constexpr Register c_rarg3 = RT3;

// Misc calling convention registers
constexpr Register RECEIVER   = T0;
constexpr Register IC_Klass   = T1;
constexpr Register SHIFT_count = T3;

const int MIPS_ARGS_IN_REGS_NUM = 8;

// ======================================================================
// Float registers
// ======================================================================

class FloatRegister {
 private:
  int _encoding;

  constexpr explicit FloatRegister(int encoding) : _encoding(encoding) {}

 public:
  inline friend constexpr FloatRegister as_FloatRegister(int encoding);

  enum {
    number_of_registers     = 32,
    save_slots_per_register = 2,
    max_slots_per_register  = 2,
  };

  class FloatRegisterImpl: public AbstractRegisterImpl {
    friend class FloatRegister;

    static constexpr const FloatRegisterImpl* first();

   public:
    constexpr int raw_encoding() const { return checked_cast<int>(this - first()); }
    constexpr int     encoding() const { assert(is_valid(), "invalid register"); return raw_encoding(); }
    constexpr bool    is_valid() const { return 0 <= raw_encoding() && raw_encoding() < number_of_registers; }

    inline FloatRegister successor() const;

    VMReg as_VMReg() const;

    const char* name() const;
  };

  constexpr FloatRegister() : _encoding(-1) {} // fnoreg

  int operator==(const FloatRegister r) const { return _encoding == r._encoding; }
  int operator!=(const FloatRegister r) const { return _encoding != r._encoding; }

  constexpr const FloatRegisterImpl* operator->() const { return FloatRegisterImpl::first() + _encoding; }
};

extern FloatRegister::FloatRegisterImpl all_FloatRegisterImpls[FloatRegister::number_of_registers + 1] INTERNAL_VISIBILITY;

inline constexpr const FloatRegister::FloatRegisterImpl* FloatRegister::FloatRegisterImpl::first() {
  return all_FloatRegisterImpls + 1;
}

constexpr FloatRegister fnoreg = FloatRegister();

inline constexpr FloatRegister as_FloatRegister(int encoding) {
  if (0 <= encoding && encoding < FloatRegister::number_of_registers) {
    return FloatRegister(encoding);
  }
  return fnoreg;
}

inline FloatRegister FloatRegister::FloatRegisterImpl::successor() const {
  assert(is_valid(), "sanity");
  return as_FloatRegister(encoding() + 1);
}

constexpr FloatRegister f0  = as_FloatRegister( 0);
constexpr FloatRegister f1  = as_FloatRegister( 1);
constexpr FloatRegister f2  = as_FloatRegister( 2);
constexpr FloatRegister f3  = as_FloatRegister( 3);
constexpr FloatRegister f4  = as_FloatRegister( 4);
constexpr FloatRegister f5  = as_FloatRegister( 5);
constexpr FloatRegister f6  = as_FloatRegister( 6);
constexpr FloatRegister f7  = as_FloatRegister( 7);
constexpr FloatRegister f8  = as_FloatRegister( 8);
constexpr FloatRegister f9  = as_FloatRegister( 9);
constexpr FloatRegister f10 = as_FloatRegister(10);
constexpr FloatRegister f11 = as_FloatRegister(11);
constexpr FloatRegister f12 = as_FloatRegister(12);
constexpr FloatRegister f13 = as_FloatRegister(13);
constexpr FloatRegister f14 = as_FloatRegister(14);
constexpr FloatRegister f15 = as_FloatRegister(15);
constexpr FloatRegister f16 = as_FloatRegister(16);
constexpr FloatRegister f17 = as_FloatRegister(17);
constexpr FloatRegister f18 = as_FloatRegister(18);
constexpr FloatRegister f19 = as_FloatRegister(19);
constexpr FloatRegister f20 = as_FloatRegister(20);
constexpr FloatRegister f21 = as_FloatRegister(21);
constexpr FloatRegister f22 = as_FloatRegister(22);
constexpr FloatRegister f23 = as_FloatRegister(23);
constexpr FloatRegister f24 = as_FloatRegister(24);
constexpr FloatRegister f25 = as_FloatRegister(25);
constexpr FloatRegister f26 = as_FloatRegister(26);
constexpr FloatRegister f27 = as_FloatRegister(27);
constexpr FloatRegister f28 = as_FloatRegister(28);
constexpr FloatRegister f29 = as_FloatRegister(29);
constexpr FloatRegister f30 = as_FloatRegister(30);
constexpr FloatRegister f31 = as_FloatRegister(31);

constexpr FloatRegister FNOREG = fnoreg;
constexpr FloatRegister F0  = f0;
constexpr FloatRegister F1  = f1;
constexpr FloatRegister F2  = f2;
constexpr FloatRegister F3  = f3;
constexpr FloatRegister F4  = f4;
constexpr FloatRegister F5  = f5;
constexpr FloatRegister F6  = f6;
constexpr FloatRegister F7  = f7;
constexpr FloatRegister F8  = f8;
constexpr FloatRegister F9  = f9;
constexpr FloatRegister F10 = f10;
constexpr FloatRegister F11 = f11;
constexpr FloatRegister F12 = f12;
constexpr FloatRegister F13 = f13;
constexpr FloatRegister F14 = f14;
constexpr FloatRegister F15 = f15;
constexpr FloatRegister F16 = f16;
constexpr FloatRegister F17 = f17;
constexpr FloatRegister F18 = f18;
constexpr FloatRegister F19 = f19;
constexpr FloatRegister F20 = f20;
constexpr FloatRegister F21 = f21;
constexpr FloatRegister F22 = f22;
constexpr FloatRegister F23 = f23;
constexpr FloatRegister F24 = f24;
constexpr FloatRegister F25 = f25;
constexpr FloatRegister F26 = f26;
constexpr FloatRegister F27 = f27;
constexpr FloatRegister F28 = f28;
constexpr FloatRegister F29 = f29;
constexpr FloatRegister F30 = f30;
constexpr FloatRegister F31 = f31;

// Float register role aliases used by the interpreter
// Integer interpreter scratch registers (named FSR/SSR in jdk17u mips convention)
constexpr Register FSR = V0;   // first scratch / return value register
constexpr Register SSR = V1;   // second scratch / second return value register

// Float interpreter scratch registers
constexpr FloatRegister FSF = F0;   // first scratch float
constexpr FloatRegister SSF = F1;   // second scratch float
constexpr FloatRegister FTF = F14;  // float temporary
constexpr FloatRegister STF = F15;  // second float temporary
constexpr FloatRegister AFT = F30;  // return float value (also scratch)

// ======================================================================
// ConcreteRegisterImpl -- total VMReg count
// ======================================================================

class ConcreteRegisterImpl : public AbstractRegisterImpl {
 public:
  enum {
    max_gpr = Register::number_of_registers      * Register::max_slots_per_register,
    max_fpr = max_gpr + FloatRegister::number_of_registers * FloatRegister::max_slots_per_register,

    number_of_registers = max_fpr
  };
};

typedef AbstractRegSet<Register>      RegSet;
typedef AbstractRegSet<FloatRegister> FloatRegSet;

template <>
inline Register AbstractRegSet<Register>::first() {
  if (_bitset == 0) { return noreg; }
  return as_Register(count_trailing_zeros(_bitset));
}

template <>
inline FloatRegister AbstractRegSet<FloatRegister>::first() {
  if (_bitset == 0) { return fnoreg; }
  return as_FloatRegister(count_trailing_zeros(_bitset));
}

#endif // CPU_MIPS_REGISTER_MIPS_HPP
