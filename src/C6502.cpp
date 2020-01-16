#include <C6502.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>

#include <c64_basic.h>
#include <c64_kernel.h>

// TODO:

// The available 16-bit address space is conceived as consisting of pages of 256 bytes each, with
// address hi-bytes represententing the page index. An increment with carry may affect the hi-byte
// and may thus result in a crossing of page boundaries, adding an extra cycle to the execution.
// Increments without carry do not affect the hi-byte of an address and no page transitions do
// occur. Generally, increments of 16-bit addresses include a carry, increments of zeropage
// addresses don't.  Notably this is not related in any way to the state of the carry bit of the
// accumulator.

// BCD arithmatic (D flag set)

// Set B flag when break/interrupt processed

C6502::
C6502()
{
  setIFlag(true); // interrupts disabled
}

//---

// NMI interrupt
void
C6502::
resetNMI()
{
  // save state for RTI
  pushWord(PC() + 2);
  pushByte(SR());

  // jump to NMI interrupt vector
  setPC(NMI());

  incT(7);
}

// Reset
void
C6502::
resetSystem()
{
  // jump to Reset interrupt vector (no save)
  setPC(RES());

  incT(7);
}

// IRQ interrupt (maskable)
void
C6502::
resetIRQ()
{
  if (! Iflag())
    return;

  // save state for RTI
  pushWord(PC() + 2);
  pushByte(SR());

  setBFlag(true);

  // jump to IRQ interrupt vector
  setPC(IRQ());

  incT(7);
}

void
C6502::
resetBRK()
{
  // save state for RTI
  pushWord(PC() + 2);
  pushByte(SR());

  setBFlag(false);

  // jump to IRQ interrupt vector
  setPC(IRQ());

  incT(7);
}

//---

void
C6502::
load64Rom()
{
  memset(0xA000, c64_basic_data , 0x2000);
  memset(0xE000, c64_kernel_data, 0x2000);
}

//---

bool
C6502::
run(ushort addr)
{
  reset();

  setPC(addr);

  printState();

  while (true) {
    step();

    printState();

    if (Iflag())
      break;
  }

  return true;
}

void
C6502::
step()
{
  auto hex02 = [&](std::ostream &os, uchar value) {
    os << std::setfill('0') << std::setw(2) << std::right << std::hex << int(value);
  };

  auto c = readByte();

  switch (c) {
    case 0x00: { // BRK implicit
      resetBRK();
      break;
    }

    //---

    // AND ...
    case 0x21: { // AND (indirect,X)
      andOp(memIndexedIndirectX(readByte())); incT(6); break;
    }
    case 0x25: { // AND zero page
      andOp(getByte(readByte())); incT(3); break;
    }
    case 0x29: { // AND immediate
      andOp(readByte()); incT(2); break;
    }
    case 0x2D: { // AND absolute
      andOp(getByte(readWord())); incT(4); break;
    }
    case 0x31: { // AND (indirect),Y
      andOp(memIndirectIndexedY(readByte())); incT(5); break;
    }
    case 0x35: { // AND zero page,X
      andOp(getByte(readByte() + X())); incT(4); break;
    }
    case 0x39: { // AND absolute,Y
      andOp(getByte(readWord() + Y())); incT(4); break;
    }
    case 0x3D: { // AND absolute,X
      andOp(getByte(readWord() + X())); incT(4); break;
    }

    //---

    // ORA ...
    case 0x01: { // ORA (indirect,X)
      orOp(memIndexedIndirectX(readByte())); incT(6); break;
    }
    case 0x05: { // ORA zero page
      orOp(getByte(readByte())); incT(3); break;
    }
    case 0x09: { // ORA immediate
      orOp(readByte()); incT(2); break;
    }
    case 0x0D: { // ORA absolute
      orOp(getByte(readWord())); incT(4); break;
    }
    case 0x11: { // ORA (indirect),Y
      orOp(memIndirectIndexedY(readByte())); incT(5); break;
    }
    case 0x15: { // ORA zero page,X
      orOp(getByte(readByte() + X())); incT(4); break;
    }
    case 0x19: { // ORA absolute,Y
      orOp(getByte(readWord() + Y())); incT(4); break;
    }
    case 0x1D: { // ORA absolute,X
      orOp(getByte(readWord() + X())); incT(4); break;
    }

    //---

    // EOR ...
    case 0x41: { // EOR (indirect,X)
      eorOp(memIndexedIndirectX(readByte())); incT(6); break;
    }
    case 0x45: { // EOR zero page
      eorOp(getByte(readByte())); incT(3); break;
    }
    case 0x49: { // EOR immediate
      eorOp(readByte()); incT(2); break;
    }
    case 0x4D: { // EOR absolute
      eorOp(getByte(readWord())); incT(4); break;
    }
    case 0x51: { // EOR (indirect),Y
      eorOp(memIndirectIndexedY(readByte())); incT(5); break;
    }
    case 0x55: { // EOR zero page,X
      eorOp(getByte(readByte() + X())); incT(4); break;
    }
    case 0x59: { // EOR absolute,Y
      eorOp(getByte(readWord() + Y())); incT(4); break;
    }
    case 0x5D: { // EOR absolute,X
      eorOp(getByte(readWord() + X())); incT(4); break;
    }

    //---

    // ADC ...
    case 0x61: { // ADC (indirect,X)
      adcOp(memIndexedIndirectX(readByte())); incT(6); break;
    }
    case 0x65: { // ADC zero page
      adcOp(getByte(readByte())); incT(3); break;
    }
    case 0x69: { // ADC immediate
      adcOp(readByte()); incT(2); break;
    }
    case 0x6D: { // ADC absolute
      adcOp(getByte(readWord())); incT(4); break;
    }
    case 0x71: { // ADC (indirect),Y
      adcOp(memIndirectIndexedY(readByte())); incT(5); break;
    }
    case 0x75: { // ADC zero page,X
      adcOp(getByte(readByte() + X())); incT(4); break;
    }
    case 0x79: { // ADC absolute,Y
      adcOp(getByte(readWord() + Y())); incT(4); break;
    }
    case 0x7D: { // ADC absolute,X
      adcOp(getByte(readWord() + X())); incT(4); break;
    }

    //---

    // SBC ...
    case 0xE1: { // SBC (indirect,X)
      sbcOp(memIndexedIndirectX(readByte())); incT(6); break;
    }
    case 0xE5: { // SBC zero page
      sbcOp(getByte(readByte())); incT(3); break;
    }
    case 0xE9: { // SBC immediate
      sbcOp(readByte()); incT(2); break;
    }
    case 0xED: { // SBC absolute
      sbcOp(getByte(readWord())); incT(4); break;
    }
    case 0xF1: { // SBC (indirect),Y
      sbcOp(memIndirectIndexedY(readByte())); incT(5); break;
    }
    case 0xF5: { // SBC zero page,X
      sbcOp(getByte(readByte() + X())); incT(4); break;
    }
    case 0xF9: { // SBC absolute,Y
      sbcOp(getByte(readWord() + Y())); incT(4); break;
    }
    case 0xFD: { // SBC absolute,X
      sbcOp(getByte(readWord() + X())); incT(4); break;
    }

    //---

    // Bit functions

    case 0x06: { // ASL zero page
      aslOp(getByte(readByte())); incT(5); break;
    }
    case 0x0A: { // ASL A
      aslOp(readByte()); incT(2); break;
    }
    case 0x0E: { // ASL absolute
      aslOp(getByte(readWord())); incT(6); break;
    }
    case 0x16: { // ASL zero page,X
      aslOp(getByte(readByte() + X())); incT(6); break;
    }
    case 0x1E: { // ASL absolute,X
      aslOp(getByte(readWord() + X())); incT(7); break;
    }

    //-

    case 0x26: { // ROL zero page
      rolOp(getByte(readByte())); incT(5); break;
    }
    case 0x2A: { // ROL A
      rolOp(readByte()); incT(2); break;
    }
    case 0x2E: { // ROL absolute
      rolOp(getByte(readWord())); incT(6); break;
    }
    case 0x36: { // ROL zero page,X
      rolOp(getByte(readByte() + X())); incT(6); break;
    }
    case 0x3E: { // ROL absolute,X
      rolOp(getByte(readWord() + X())); incT(7); break;
    }

    //-

    case 0x46: { // LSR zero page
      lsrOp(getByte(readByte())); incT(5); break;
    }
    case 0x4A: { // LSR A
      lsrOp(readByte()); incT(2); break;
    }
    case 0x4E: { // LSR absolute
      lsrOp(getByte(readWord())); incT(6); break;
    }
    case 0x56: { // LSR zero page,X
      lsrOp(getByte(readByte() + X())); incT(6); break;
    }
    case 0x5E: { // LSR absolute,X
      lsrOp(getByte(readWord() + X())); incT(7); break;
    }

    //-

    case 0x66: { // ROR zero page
      rorOp(getByte(readByte())); incT(5); break;
    }
    case 0x6A: { // ROR A
      rorOp(readByte()); incT(2); break;
    }
    case 0x6E: { // ROR absolute
      rorOp(getByte(readWord())); incT(6); break;
    }
    case 0x76: { // ROR zero page,X
      rorOp(getByte(readByte() + X())); incT(6); break;
    }
    case 0x7E: { // ROR absolute,X
      rorOp(getByte(readWord() + X())); incT(7); break;
    }

    //-

    // BIT...
    case 0x24: { // BIT zero page
      auto c1 = getByte(readByte()) & 0xC0; setSR(SR() & c1); incT(3); break;
    }
    case 0x2C: { // BIT absolute
      auto c1 = getByte(readWord()) & 0xC0; setSR(SR() & c1); incT(4); break;
    }

    //---

    case 0x08: { // PHP implied (Push Processor Status)
      pushByte(SR()); incT(3); break;
    }

    case 0x28: { // PLP implied (Pull Processor Status)
      setSR(popByte()); incT(4); break;
    }

    case 0x48: { // PHA implied (Push A)
      pushByte(A()); incT(3); break;
    }

    case 0x68: { // PLA implied (Pull A)
      setA(popByte()); incT(3); break;
    }

    //---

    // Branch, Jump

    case 0x10: { // BPL (Branch Positive Result)
      schar d = readSByte();

      if (! Nflag())
        setPC(PC() + d);

      incT(2);

      break;
    }

    case 0x20: { // JSR (Jump Save Return)
      pushWord(PC() + 2);

      setPC(readWord());

      incT(6);

      break;
    }

    case 0x30: { // BMI (Branch Negative Result)
      schar d = readSByte();

      if (Nflag())
        setPC(PC() + d);

      incT(2);

      break;
    }

    case 0x50: { // BVC (Branch Overflow Clear)
      schar d = readSByte();

      if (! Vflag())
        setPC(PC() + d);

      incT(2);

      break;
    }

    case 0x70: { // BVS (Branch Overflow Set)
      schar d = readSByte();

      if (Vflag())
        setPC(PC() + d);

      incT(2);

      break;
    }

    case 0x90: { // BCC (Branch Carry Clear)
      schar d = readSByte();

      if (! Cflag())
        setPC(PC() + d);

      incT(2);

      break;
    }

    case 0xB0: { // BCS (Branch Carry Set)
      schar d = readSByte();

      if (Cflag())
        setPC(PC() + d);

      incT(2);

      break;
    }

    case 0xD0: { // BNE (Branch Zero Clear)
      schar d = readSByte();

      if (! Zflag())
        setPC(PC() + d);

      incT(2);

      break;
    }

    case 0xF0: { // BEQ (Branch Zero Set)
      schar d = readSByte();

      if (Zflag())
        setPC(PC() + d);

      incT(2);

      break;
    }

    case 0x4C: { // JMP absolute
      setPC(readWord()); incT(3); break;
    }

    case 0x6C: { // JMP indirect (???)
      setPC(getWord(readWord())); incT(3); break;
    }

    case 0x40: { // RTI (Return from Interrupt)
      setSR(popByte());
      setPC(popWord());

      incT(6);

      break;
    }

    case 0x60: { // RTS (Return from Subroutine)
      setPC(popWord());

      incT(6);

      break;
    }

    //---

    // Compare

    // CPY ...
    case 0xC0: { // CPY immediate
      uchar c1 = readByte(); bool C = (Y() < c1); uchar c2 = (Y() - c1);

      setNZCFlags(c2, C);

      incT(2);

      break;
    }
    case 0xC4: { // CPY zero page
      uchar c1 = getByte(readByte()); bool C = (Y() < c1); uchar c2 = (Y() - c1);

      setNZCFlags(c2, C);

      incT(3);

      break;
    }
    case 0xCC: { // CPY absolute
      uchar c1 = getByte(readWord()); bool C = (Y() < c1); uchar c2 = (Y() - c1);

      setNZCFlags(c2, C);

      incT(4);

      break;
    }

    // CPX ...
    case 0xE0: { // CPX immediate
      uchar c1 = readByte(); bool C = (X() < c1); uchar c2 = (X() - c1);

      setNZCFlags(c2, C);

      incT(2);

      break;
    }
    case 0xE4: { // CPX zero page
      uchar c1 = getByte(readByte()); bool C = (X() < c1); uchar c2 = (X() - c1);

      setNZCFlags(c2, C);

      incT(3);

      break;
    }
    case 0xEC: { // CPX absolute
      uchar c1 = getByte(readWord()); bool C = (X() < c1); uchar c2 = (X() - c1);

      setNZCFlags(c2, C);

      incT(4);

      break;
    }

    // CMP ...
    case 0xC1: { // CMP (indirect,X)
      uchar c1 = memIndexedIndirectX(readByte()); bool C = (A() < c1); uchar c2 = (A() - c1);

      setNZCFlags(c2, C);

      incT(5);

      break;
    }
    case 0xC5: { // CMP zero page
      uchar c1 = getByte(readByte()); bool C = (A() < c1); uchar c2 = (A() - c1);

      setNZCFlags(c2, C);

      incT(3);

      break;
    }
    case 0xC9: { // CMP immediate
      uchar c1 = readByte(); bool C = (A() < c1); uchar c2 = (A() - c1);

      setNZCFlags(c2, C);

      incT(2);

      break;
    }
    case 0xCD: { // CMP absolute
      uchar c1 = getByte(readWord()); bool C = (A() < c1); uchar c2 = (A() - c1);

      setNZCFlags(c2, C);

      incT(4);

      break;
    }
    case 0xD1: { // CMP (indirect),Y
      uchar c1 = memIndirectIndexedY(readByte()); bool C = (A() < c1); uchar c2 = (A() - c1);

      setNZCFlags(c2, C);

      incT(5);

      break;
    }
    case 0xD5: { // CMP zero page,X
      uchar c1 = getByte(readByte() + X()); bool C = (A() < c1); uchar c2 = (A() - c1);

      setNZCFlags(c2, C);

      incT(4);

      break;
    }
    case 0xD9: { // CMP absolute,Y
      uchar c1 = getByte(readWord() + Y()); bool C = (A() < c1); uchar c2 = (A() - c1);

      setNZCFlags(c2, C);

      incT(4);

      break;
    }
    case 0xDD: { // CMP absolute,X
      uchar c1 = getByte(readWord() + X()); bool C = (A() < c1); uchar c2 = (A() - c1);

      setNZCFlags(c2, C);

      incT(4);

      break;
    }

    //---

    case 0x18: { // CLC (Clear carry)
      setCFlag(false); incT(2); break;
    }

    case 0x38: { // SEC (Set carry)
      setCFlag(true); incT(2); break;
    }

    case 0x58: { // CLI (Clear Interrupt Disable)
      setIFlag(false); incT(2); break;
    }

    case 0x78: { // SEI (Set Interrupt Disable)
      setIFlag(true); incT(2); break;
    }

    case 0xB8: { // CLV (Clear overflow)
      setVFlag(false); incT(2); break;
    }

    case 0xD8: { // CLD (Clear Decimal Mode)
      setDFlag(false); incT(2); break;
    }

    case 0xF8: { // SED (Set Decimal Mode)
      setDFlag(true); incT(2); break;
    }

    //---

    // Store

    // STA ...
    case 0x81: { // STA (indirect,X)
      setMemIndexedIndirectX(readByte(), A()); incT(6); break;
    }
    case 0x85: { // STA zero page
      setByte(readByte(), A()); incT(3); break;
    }
    case 0x8D: { // STA absolute
      setByte(readWord(), A()); incT(4); break;
    }
    case 0x91: { // STA (indirect),Y
      setMemIndirectIndexedY(readByte(), A()); incT(6); break;
    }
    case 0x95: { // STA zero page,X
      setByte(readByte() + X(), A()); incT(4); break;
    }
    case 0x99: { // STA absolute,Y
      setByte(readWord() + Y(), A()); incT(5); break;
    }
    case 0x9D: { // STA absolute,X
      setByte(readWord() + X(), A()); incT(5); break;
    }

    // STY ...
    case 0x84: { // STY zero page
      setByte(readByte(), Y()); incT(3); break;
    }
    case 0x8C: { // STY absolute
      setByte(readWord(), Y()); incT(4); break;
    }
    case 0x94: { // STY zero page,X
      setByte(readByte() + X(), Y()); incT(4); break;
    }

    // STX ...
    case 0x86: { // STX zero page
      setByte(readByte(), X()); incT(3); break;
    }
    case 0x8E: { // STX absolute
      setByte(readWord(), X()); incT(4); break;
    }
    case 0x96: { // STX zero page,Y
      setByte(readByte() + Y(), Y()); incT(4); break;
    }

    case 0x8A: { // TXA
      setA(X()); setNZFlags(); incT(2); break;
    }
    case 0x9A: { // TXS
      setSR(X()); incT(2); break;
    }
    case 0xAA: { // TAX
      setX(A()); setNZFlags(X()); incT(2); break;
    }
    case 0xBA: { // TSX
      setX(SP()); setNZFlags(X()); incT(2); break;
    }

    case 0x98: { // TYA
      setA(Y()); setNZFlags(); incT(2); break;
    }
    case 0xA8: { // TAY
      setY(A()); setNZFlags(Y()); incT(2); break;
    }

    // LDY...
    case 0xA0: { // LDY immediate
      setY(readByte()); setNZFlags(Y()); incT(3); break;
    }
    case 0xA4: { // LDY zero page
      setY(getByte(readByte())); setNZFlags(Y()); incT(3); break;
    }
    case 0xAC: { // LDY absolute
      setY(getByte(readWord())); setNZFlags(Y()); incT(4); break;
    }
    case 0xB4: { // LDY zero page,X
      setY(getByte(readByte() + X())); setNZFlags(Y()); incT(4); break;
    }
    case 0xBC: { // LDY absolute,X
      setY(getByte(readWord() + X())); setNZFlags(Y()); incT(4); break;
    }

    // LDX...
    case 0xA2: { // LDX immediate
      setX(readByte()); setNZFlags(X()); incT(3); break;
    }
    case 0xA6: { // LDX zero page
      setX(getByte(readByte())); setNZFlags(X()); incT(3); break;
    }
    case 0xAE: { // LDX absolute
      setX(getByte(readWord())); setNZFlags(X()); incT(4); break;
    }
    case 0xB6: { // LDX zero page,Y
      setX(getByte(readByte() + Y())); setNZFlags(X()); incT(4); break;
    }
    case 0xBE: { // LDX absolute,Y
      setX(getByte(readWord() + Y())); setNZFlags(X()); incT(4); break;
    }

    // LDA...
    case 0xA1: { // LDA (indirect,X)
      setA(memIndexedIndirectX(readByte())); setNZFlags(A()); incT(6); break;
    }
    case 0xA5: { // LDA zero page
      setA(getByte(readByte())); setNZFlags(A()); incT(3); break;
    }
    case 0xA9: { // LDA immediate
      setA(readByte()); setNZFlags(X()); incT(2); break;
    }
    case 0xAD: { // LDA absolute
      setA(getByte(readWord())); setNZFlags(A()); incT(4); break;
    }
    case 0xB1: { // LDA (indirect),Y
      setA(memIndirectIndexedY(readByte())); setNZFlags(A()); incT(5); break;
    }
    case 0xB5: { // LDA zero page,X
      setA(getByte(readByte() + X())); setNZFlags(A()); incT(4); break;
    }
    case 0xB9: { // LDA absolute,Y
      setA(getByte(readWord() + Y())); setNZFlags(A()); incT(4); break;
    }
    case 0xBD: { // LDA absolute,X
      setA(getByte(readWord() + X())); setNZFlags(A()); incT(4); break;
    }

    //---

    // Increment/Decrement

    case 0x88: { // DEY
      setY(Y() - 1); setNZFlags(Y()); incT(2); break;
    }
    case 0xCA: { // DEX
      setX(X() - 1); setNZFlags(X()); incT(2); break;
    }

    case 0xC8: { // INY
      setY(Y() + 1); setNZFlags(Y()); incT(2); break;
    }
    case 0xE8: { // INX
      setX(X() + 1); setNZFlags(X()); incT(2); break;
    }

    // DEC ...
    case 0xC6: { // DEC zero page
      ushort a = readByte(); uchar c1 = getByte(a) - 1;
      setByte(a, c1); setNZFlags(c1); incT(5); break;
    }
    case 0xCE: { // DEC absolute
      ushort a = readWord(); uchar c1 = getByte(a) - 1;
      setByte(a, c1); setNZFlags(c1); incT(6); break;
    }
    case 0xD6: { // DEC zero page,X
      ushort a = readByte() + X(); uchar c1 = getByte(a) - 1;
      setByte(a, c1); setNZFlags(c1); incT(6); break;
    }
    case 0xDE: { // DEC absolute,X
      ushort a = readWord() + X(); uchar c1 = getByte(a) - 1;
      setByte(a, c1); setNZFlags(c1); incT(7); break;
    }

    // INC ...
    case 0xE6: { // INC zero page
      ushort a = readByte(); uchar c1 = getByte(a) + 1;
      setByte(a, c1); setNZFlags(c1); incT(5); break;
    }
    case 0xEE: { // INC absolute
      ushort a = readWord(); uchar c1 = getByte(a) + 1;
      setByte(a, c1); setNZFlags(c1); incT(6); break;
    }
    case 0xF6: { // INC zero page,X
      ushort a = readByte() + X(); uchar c1 = getByte(a) + 1;
      setByte(a, c1); setNZFlags(c1); incT(6); break;
    }
    case 0xFE: { // INC absolute,X
      ushort a = readWord() + X(); uchar c1 = getByte(a) + 1;
      setByte(a, c1); setNZFlags(c1); incT(7); break;
    }

    //---

    case 0xEA: { // NOP
      incT(2); break;
    }

    //---

    // unimplemented

              case 0x02:case 0x03:case 0x04:case 0x07:
                        case 0x0B:case 0x0C:          case 0x0F:
              case 0x12:case 0x13:case 0x14:case 0x17:
              case 0x1A:case 0x1B:case 0x1C:          case 0x1F:
              case 0x22:case 0x23:          case 0x27:
                        case 0x2B:                    case 0x2F:
              case 0x32:case 0x33:case 0x34:case 0x37:
              case 0x3A:case 0x3B:case 0x3C:          case 0x3F:
              case 0x42:case 0x43:case 0x44:case 0x47:
                        case 0x4B:                    case 0x4F:
              case 0x52:case 0x53:case 0x54:case 0x57:
              case 0x5A:case 0x5B:case 0x5C:          case 0x5F:
              case 0x62:case 0x63:case 0x64:case 0x67:
                        case 0x6B:                    case 0x6F:
              case 0x72:case 0x73:case 0x74:case 0x77:
              case 0x7A:case 0x7B:case 0x7C:          case 0x7F:
    case 0x80:case 0x82:case 0x83:          case 0x87:
    case 0x89:          case 0x8B:                    case 0x8F:
              case 0x92:case 0x93:          case 0x97:
                        case 0x9B:case 0x9C:case 0x9E:case 0x9F:
                        case 0xA3:          case 0xA7:
                        case 0xAB:                    case 0xAF:
              case 0xB2:case 0xB3:          case 0xB7:
                        case 0xBB:                    case 0xBF:
              case 0xC2:case 0xC3:          case 0xC7:
                        case 0xCB:                    case 0xCF:
              case 0xD2:case 0xD3:case 0xD4:case 0xD7:
              case 0xDA:case 0xDB:case 0xDC:          case 0xDF:
              case 0xE2:case 0xE3:          case 0xE7:
                        case 0xEB:                    case 0xEF:
              case 0xF2:case 0xF3:case 0xF4:case 0xF7:
              case 0xFA:case 0xFB:case 0xFC:          case 0xFF: {
      std::cerr << "Invalid byte "; hex02(std::cerr, c); std::cerr << "\n";
      break;
    }

    default:
      assert(false);
      break;
  }
}

//---

bool
C6502::
assemble(ushort addr, std::istream &is)
{
  Lines lines;

  readLines(is, lines);

  clearLabels();

  numBadLabels_ = 0;

  bool rc = assembleLines(addr, lines);

  if (numBadLabels_ > 0)
    rc = assembleLines(addr, lines);

  return rc;
}

void
C6502::
readLines(std::istream &is, Lines &lines)
{
  while (! is.eof()) {
    std::string line;

    std::getline(is, line);

    lines.push_back(line);
  }
}

bool
C6502::
assembleLines(ushort addr, const Lines &lines)
{
  for (auto &line : lines) {
    if (! assembleLine(addr, line))
      return false;
  }

  return true;
}

bool
C6502::
assembleLine(ushort &addr, const std::string &line)
{
  auto toUpper = [](const std::string &s) {
    std::string s1 = s;

    std::transform(s1.begin(), s1.end(), s1.begin(),
                   [](unsigned char c){ return std::toupper(c); } // correct
                  );
    return s1;
  };

  //---

  if (isDebug())
    std::cerr << "Line: '" << line << "'\n";

  int pos = 0;
  int len = line.size();

  auto skipSpace    = [&]() { while (pos < len && isspace(line[pos])) ++pos; };
  auto skipNonSpace = [&]() { while (pos < len && ! isspace(line[pos])) ++pos; };
  auto isChar       = [&](char c) { return (pos < len && line[pos] == c); };

  auto readWord = [&](std::string &word) {
    skipSpace();
    int pos1 = pos;
    skipNonSpace();
    if (pos1 == pos) return false;
    word = line.substr(pos1, pos - pos1);
    return true;
  };

  while (pos < len) {
    skipSpace();

    // skip comment
    if (isChar(';')) break;

    // read first word
    std::string word;

    if (! readWord(word))
      continue;

    // handle label and update first word
    if (word[word.size() - 1] == ':') {
      std::string label = word.substr(0, word.size() - 1);

      setLabel(label, addr, 4);

      if (! readWord(word))
        continue;
    }

    std::string opName = toUpper(word);

    //---

    // handle define

    if (opName == "DEFINE") {
      auto stringToValue = [](const std::string &str, ushort &ivalue, int &ilen) {
        static std::string xchars = "0123456789abcdef";
        static std::string dchars = "0123456789";

        ivalue = 0;
        ilen   = 0;

        int ipos = 0;
        int len1 = str.size();

        if (ipos < len1 && str[ipos] == '$') {
          ++ipos;

          while (ipos < len1 && std::isxdigit(str[ipos])) {
            char c1 = tolower(str[ipos]);

            auto p = xchars.find(c1);

            ivalue = ivalue*16 + int(p);

            ++ipos;
            ++ilen;
          }
        }
        else {
          while (ipos < len1 && std::isdigit(str[ipos])) {
            char c1 = tolower(str[ipos]);

            auto p = dchars.find(c1);

            ivalue = ivalue*10 + int(p);

            ++ipos;
            ++ilen;
          }
        }

        return true;
      };

      // read label
      std::string label;

      if (! readWord(label)) {
        std::cerr << "Invalid define '" << line << "'\n";
        return false;
      }

      std::string value;

      if (! readWord(value)) {
        std::cerr << "Invalid define '" << line << "'\n";
        return false;
      }

      ushort ivalue;
      int    ilen;

      if (! stringToValue(value, ivalue, ilen)) {
        std::cerr << "Invalid define '" << line << "'\n";
        return false;
      }

      setLabel(label, ivalue, ilen);

      break;
    }

    //---

    // handle op

    if (isDebug())
      std::cerr << "  " << opName;

    // read optional arg
    std::string arg;

    if (readWord(arg)) {
      if (isDebug())
        std::cerr << " " << arg;
    }

    if (! assembleOp(addr, opName, arg)) {
      std::cerr << "Bad OP : '" << line << "'\n";
      break;
    }

    if (isDebug())
      std::cerr << "\n";

    break;
  }

  return true;
}

bool
C6502::
assembleOp(ushort &addr, const std::string &opName, const std::string &arg)
{
  auto addByte = [&](ushort c) { mem_[addr++] = (c & 0xFF); };
  auto addWord = [&](ushort c) { addByte(c & 0x00FF); addByte((c & 0xFF00) >> 8); };

  auto addRelative = [&](schar c) { mem_[addr++] = c; };

  auto addOp     = [&](ushort op) { addByte(op); return true; };
  auto addOpByte = [&](ushort op, ushort value) { addOp(op); addByte(value); return true; };
  auto addOpWord = [&](ushort op, ushort value) { addOp(op); addWord(value); return true; };

  auto addOpRelative = [&](ushort op, schar rvalue) {
    addByte(op); addRelative(rvalue); return true; };

  //---

  ArgMode mode   = ArgMode::NONE;
  XYMode  xyMode = XYMode::NONE;
  ushort  value  = 0;
  uchar   vlen   = 0;
  schar   rvalue = 0;

  if (arg != "") {
    if (! decodeAssembleArg(arg, mode, xyMode, value, vlen)) {
      std::cerr << "Invalid arg '" << arg << "'\n";
      return false;
    }

    if (vlen > 2)
      rvalue = int(value) - int(addr) - 2;
  }

  //---

  if      (opName == "ADC") {
    if      (xyMode == XYMode::NONE) {
      // ADC #$xx (immediate)
      if      (mode == ArgMode::LITERAL) {
        if (vlen <= 2) { return addOpByte(0x69, value); }
      }
      // ADC $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x65, value); } // zero page
        else           { return addOpWord(0x6D, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // ADC $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpWord(0x75, value); } // zero page
        else           { return addOpWord(0x7D, value); }
      }
      // ADC ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x61, value);
      }
    }
    else if (xyMode == XYMode::Y) {
      // ADC $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return false; } // zero page
        else           { return addOpWord(0x79, value); }
      }
      // ADC ($xx),Y
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x71, value);
      }
    }

    return false;
  }

  else if (opName == "AND") {
    if      (xyMode == XYMode::NONE) {
      // AND #$xx (immediate)
      if      (mode == ArgMode::LITERAL) {
        if (vlen <= 2) { return addOpByte(0x29, value); }
      }
      // AND $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x25, value); } // zero page
        else           { return addOpWord(0x2D, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // AND $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x35, value); } // zero page
        else           { return addOpWord(0x3D, value); }
      }
      // AND ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x21, value);
      }
    }
    else if (xyMode == XYMode::Y) {
      // AND $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return false; } // zero page
        else           { return addOpWord(0x39, value); }
      }
      // AND ($xx),Y
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x31, value);
      }
    }

    return false;
  }

  else if (opName == "ASL") {
    if      (xyMode == XYMode::NONE) {
      // ASL #$xx (immediate)
      if      (mode == ArgMode::A) {
        return addOp(0x0A);
      }
      // ASL $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x06, value); } // zero page
        else           { return addOpWord(0x0E, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // ASL $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x16, value); } // zero page
        else           { return addOpWord(0x1E, value); }
      }
      // ASL ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x21, value);
      }
    }

    return false;
  }

  else if (opName == "BCC") { return addOpRelative(0x90, rvalue); }
  else if (opName == "BCS") { return addOpRelative(0xB0, rvalue); }
  else if (opName == "BEQ") { return addOpRelative(0xF0, rvalue); }

  else if (opName == "BIT") {
    if (xyMode == XYMode::NONE) {
      // BIT $xx (absolute or zero page)
      if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x24, value); } // zero page
        else           { return addOpWord(0x2C, value); }
      }
    }

    return false;
  }

  else if (opName == "BMI") { return addOpRelative(0x30, rvalue); }
  else if (opName == "BNE") { return addOpRelative(0xD0, rvalue); }
  else if (opName == "BPL") { return addOpRelative(0x10, rvalue); }

  else if (opName == "BRK") { return addOp(0x00); }

  else if (opName == "BVC") { return addOpRelative(0x50, rvalue); }
  else if (opName == "BVS") { return addOpRelative(0x70, rvalue); }

  else if (opName == "CLC") { return addOp(0x18); }
  else if (opName == "CLD") { return addOp(0xD8); }
  else if (opName == "CLI") { return addOp(0x58); }
  else if (opName == "CLV") { return addOp(0xB8); }

  else if (opName == "CMP") {
    if      (xyMode == XYMode::NONE) {
      // CMP #$xx (immediate)
      if      (mode == ArgMode::LITERAL && vlen <= 2) {
        return addOpByte(0xC9, value);
      }
      // CMP $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xC5, value); } // zero page
        else           { return addOpWord(0xCD, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // CMP $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpWord(0xD5, value); } // zero page
        else           { return addOpWord(0xDD, value); }
      }
      // CMP ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0xC1, value);
      }
    }
    else if (xyMode == XYMode::Y) {
      // CMP $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return false; } // zero page
        else           { return addOpWord(0xD9, value); }
      }
      // CMP ($xx),Y
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0xD1, value);
      }
    }

    return false;
  }

  else if (opName == "CPX") {
    if (xyMode == XYMode::NONE) {
      // CPX #$xx (immediate)
      if      (mode == ArgMode::LITERAL && vlen <= 2) {
        return addOpByte(0xE0, value);
      }
      // CPX $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xE4, value); } // zero page
        else           { return addOpWord(0xEC, value); }
      }
    }

    return false;
  }

  else if (opName == "CPY") {
    if (xyMode == XYMode::NONE) {
      // CPY #$xx (immediate)
      if      (mode == ArgMode::LITERAL && vlen <= 2) {
        return addOpByte(0xC0, value);
      }
      // CPY $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xC4, value); } // zero page
        else           { return addOpWord(0xCC, value); }
      }
    }

    return false;
  }

  else if (opName == "DEC") {
    if      (xyMode == XYMode::NONE) {
      // DEC $xx (absolute or zero page)
      if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xC6, value); } // zero page
        else           { return addOpWord(0xCE, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // DEC $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpWord(0xD6, value); } // zero page
        else           { return addOpWord(0xDE, value); }
      }
    }

    return false;
  }

  else if (opName == "DEX") { return addOp(0xCA); }
  else if (opName == "DEY") { return addOp(0x88); }

  else if (opName == "EOR") {
    if      (xyMode == XYMode::NONE) {
      // EOR #$xx (immediate)
      if      (mode == ArgMode::LITERAL) {
        if (vlen <= 2) { return addOpByte(0x49, value); }
      }
      // EOR $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x45, value); } // zero page
        else           { return addOpWord(0x4D, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // EOR $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x55, value); } // zero page
        else           { return addOpWord(0x5D, value); }
      }
      // EOR ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x41, value);
      }
    }
    else if (xyMode == XYMode::Y) {
      // EOR $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return false; } // zero page
        else           { return addOpWord(0x59, value); }
      }
      // EOR ($xx),Y
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x51, value);
      }
    }

    return false;
  }

  else if (opName == "INC") {
    if      (xyMode == XYMode::NONE) {
      // INC $xx (absolute or zero page)
      if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xE6, value); } // zero page
        else           { return addOpWord(0xEE, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // INC $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpWord(0xF6, value); } // zero page
        else           { return addOpWord(0xFE, value); }
      }
    }

    return false;
  }

  else if (opName == "INX") { return addOp(0xE8); }
  else if (opName == "INY") { return addOp(0xC8); }

  else if (opName == "JMP") {
    if (xyMode == XYMode::NONE) {
      // JMP $xx (absolute)
      if      (mode == ArgMode::MEMORY) {
        return addOpWord(0x4C, value);
      }
      // JMP $xx (indirect)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x6C, value);
      }
    }

    return false;
  }

  else if (opName == "JSR") {
    if (xyMode == XYMode::NONE) {
      // JMP $xx (absolute)
      if (mode == ArgMode::MEMORY) {
        return addOpWord(0x20, value);
      }
    }

    return false;
  }

  else if (opName == "LDA") {
    if      (xyMode == XYMode::NONE) {
      // LDA #$xx (immediate)
      if      (mode == ArgMode::LITERAL) {
        if (vlen <= 2) { return addOpByte(0xA9, value); }
      }
      // LDA $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xA5, value); } // zero page
        else           { return addOpWord(0xAD, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // LDA $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpWord(0xB5, value); } // zero page
        else           { return addOpWord(0xBD, value); }
      }
      // LDA ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0xA1, value);
      }
    }
    else if (xyMode == XYMode::Y) {
      // LDA $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return false; } // zero page
        else           { return addOpWord(0xB9, value); }
      }
      // LDA ($xx),Y
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0xB1, value);
      }
    }

    return false;
  }

  else if (opName == "LDX") {
    if      (xyMode == XYMode::NONE) {
      // LDX #$xx (immediate)
      if      (mode == ArgMode::LITERAL && vlen <= 2) {
        return addOpByte(0xA2, value);
      }
      // LDX $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xA6, value); } // zero page
        else           { return addOpWord(0xAE, value); }
      }
    }
    else if (xyMode == XYMode::Y) {
      // LDX $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpWord(0xB6, value); } // zero page
        else           { return addOpWord(0xBE, value); }
      }
    }

    return false;
  }
  else if (opName == "LDY") {
    if      (xyMode == XYMode::NONE) {
      // LDY #$xx (immediate)
      if      (mode == ArgMode::LITERAL && vlen <= 2) {
        return addOpByte(0xA0, value);
      }
      // LDY $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xA4, value); } // zero page
        else           { return addOpWord(0xAC, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // LDY $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpWord(0xB4, value); } // zero page
        else           { return addOpWord(0xBC, value); }
      }
    }

    return false;
  }

  else if (opName == "LSR") {
    if      (xyMode == XYMode::NONE) {
      // LSR #$xx (immediate)
      if      (mode == ArgMode::A || mode == ArgMode::NONE) {
        return addOp(0x4A);
      }
      // LSR $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x46, value); } // zero page
        else           { return addOpWord(0x4E, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // LSR $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x56, value); } // zero page
        else           { return addOpWord(0x5E, value); }
      }
    }

    return false;
  }

  else if (opName == "NOP") { return addOp(0xEA); }

  else if (opName == "ORA") {
    if      (xyMode == XYMode::NONE) {
      // ORA #$xx (immediate)
      if      (mode == ArgMode::LITERAL) {
        if (vlen <= 2) { return addOpByte(0x09, value); }
      }
      // ORA $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x05, value); } // zero page
        else           { return addOpWord(0x0D, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // ORA $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x15, value); } // zero page
        else           { return addOpWord(0x1D, value); }
      }
      // ORA ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x01, value);
      }
    }
    else if (xyMode == XYMode::Y) {
      // ORA $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return false; } // zero page
        else           { return addOpWord(0x19, value); }
      }
      // ORA ($xx),Y
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x11, value);
      }
    }

    return false;
  }

  else if (opName == "PHA") { return addOp(0x48); }
  else if (opName == "PHP") { return addOp(0x08); }
  else if (opName == "PLA") { return addOp(0x68); }
  else if (opName == "PLP") { return addOp(0x28); }

  else if (opName == "RTI") { return addOp(0x40); }
  else if (opName == "RTS") { return addOp(0x60); }

  else if (opName == "SBC") {
    if      (xyMode == XYMode::NONE) {
      // SBC #$xx (immediate)
      if      (mode == ArgMode::LITERAL) {
        if (vlen <= 2) { return addOpByte(0xE9, value); }
      }
      // SBC $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0xE5, value); } // zero page
        else           { return addOpWord(0xED, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // SBC $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpWord(0xF5, value); } // zero page
        else           { return addOpWord(0xFD, value); }
      }
      // SBC ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0xE1, value);
      }
    }
    else if (xyMode == XYMode::Y) {
      // SBC $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return false; } // zero page
        else           { return addOpWord(0xF9, value); }
      }
      // SBC ($xx),Y
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0xF1, value);
      }
    }

    return false;
  }

  else if (opName == "SEC") { return addOp(0x38); }
  else if (opName == "SED") { return addOp(0xF8); }
  else if (opName == "SEI") { return addOp(0x78); }

  else if (opName == "STA") {
    if      (xyMode == XYMode::NONE) {
      // STA $xx (absolute or zero page)
      if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { // zero page
          return addOpByte(0x85, value);
        }
        else {
          return addOpWord(0x8D, value);
        }
      }
    }
    else if (xyMode == XYMode::X) {
      // STA $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x95, value); } // zero page
        else           { return addOpWord(0x9D, value); }
      }
      // STA ($xx,X)
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x81, value);
      }
    }
    else if (xyMode == XYMode::Y) {
      // STA $xx,Y (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return false; } // zero page
        else           { return addOpWord(0x99, value); }
      }
      // STA ($xx),Y
      else if (mode == ArgMode::MEMORY_CONTENTS) {
        return addOpWord(0x91, value);
      }
    }

    return false;
  }

  else if (opName == "STX") {
    if      (xyMode == XYMode::NONE) {
      // STX $xx (absolute or zero page)
      if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x86, value); } // zero page
        else           { return addOpWord(0x8E, value); }
      }
    }
    else if (xyMode == XYMode::Y) {
      // STX $xx,Y (absolute or zero page)
      if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x96, value); } // zero page
      }
    }

    return false;
  }
  else if (opName == "STY") {
    if      (xyMode == XYMode::NONE) {
      // STY $xx (absolute or zero page)
      if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x84, value); } // zero page
        else           { return addOpWord(0x8C, value); }
      }
    }
    else if (xyMode == XYMode::Y) {
      // STY $xx,Y (absolute or zero page)
      if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x94, value); } // zero page
      }
    }

    return false;
  }

  else if (opName == "TAX") { return addOp(0xAA); }
  else if (opName == "TAY") { return addOp(0xA8); }
  else if (opName == "TSX") { return addOp(0xBA); }
  else if (opName == "TXA") { return addOp(0x8A); }
  else if (opName == "TXS") { return addOp(0x9A); }
  else if (opName == "TYA") { return addOp(0x98); }

  else {
    std::cerr << "Invalid OP '" << opName << "\n";
    return false;
  }

  return true;
}

bool
C6502::
decodeAssembleArg(const std::string &arg, ArgMode &mode, XYMode &xyMode, ushort &value, uchar &vlen)
{
  mode  = ArgMode::NONE;
  value = 0;
  vlen  = 0;

  if (arg == "")
    return false;

  //---

  int pos = 0;
  int len = arg.size();

  //---

  auto isChar   = [&](char c) { return (pos < len && arg[pos] == c); };
  auto skipChar = [&]() { ++pos; };
  auto isDigit  = [&]() { return (pos < len && isdigit(arg[pos])); };

  auto getHexValue = [&](uchar &alen) {
    static std::string xchars = "0123456789abcdef";

    alen = 0;

    ushort hvalue = 0;

    while (pos < len && std::isxdigit(arg[pos])) {
      char c1 = tolower(arg[pos]);

      auto p = xchars.find(c1);

      hvalue = hvalue*16 + int(p);

      ++pos;
      ++alen;
    }

    return hvalue;
  };

  auto getDecValue = [&](uchar &alen) {
    static std::string dchars = "0123456789";

    alen = 0;

    ushort dvalue = 0;

    while (pos < len && std::isdigit(arg[pos])) {
      auto p = dchars.find(arg[pos]);

      dvalue = dvalue*16 + int(p);

      ++pos;
      ++alen;
    }

    return dvalue;
  };

  auto readXYMode = [&](XYMode &xyMode1) {
    if (isChar(',')) {
      skipChar();

      if      (isChar('X') || isChar('x')) { skipChar(); xyMode1 = XYMode::X; return true; }
      else if (isChar('Y') || isChar('y')) { skipChar(); xyMode1 = XYMode::Y; return true; }

      return false;
    }
    else {
      return true;
    }
  };

  auto readLabel = [&](std::string &label) {
    label = "";

    while (pos < len) {
      if (isspace(arg[pos]) || arg[pos] == ',' || arg[pos] == ')')
        break;

      label += arg[pos++];
    }

    return (label.size() > 0);
  };

  //---

  // immediate #<value>
  if      (isChar('#')) {
    skipChar();

    mode = ArgMode::LITERAL;

    // byte/word
    if      (isChar('$')) {
      skipChar();

      value = getHexValue(vlen);
    }
    else if (isDigit()) {
      value = getDecValue(vlen);
    }
    else {
      std::string label;

      if (readLabel(label)) {
        ushort lvalue;
        uchar  llen;

        if (! getLabel(label, lvalue, llen))
          ++numBadLabels_;

        value = lvalue;
        vlen  = llen;
      }
    }
  }
  // <value> byte/word
  else if (isChar('$')) {
    skipChar();

    mode  = ArgMode::MEMORY;
    value = getHexValue(vlen);

    if (! readXYMode(xyMode))
      return false;
  }
  else if (isDigit()) {
    mode  = ArgMode::MEMORY;
    value = getDecValue(vlen);

    if (! readXYMode(xyMode))
      return false;
  }
  // indirect (<value>)
  else if (isChar('(')) {
    skipChar();

    mode = ArgMode::MEMORY_CONTENTS;

    // <value> byte/word
    if      (isChar('$')) {
      skipChar();

      value = getHexValue(vlen);
    }
    else if (isDigit()) {
      value = getDecValue(vlen);
    }
    else {
      std::string label;

      if (readLabel(label)) {
        ushort lvalue;
        uchar  llen;

        if (! getLabel(label, lvalue, llen))
          ++numBadLabels_;

        value = lvalue;
        vlen  = llen;
      }
    }

    if (isChar(',')) {
      if (! readXYMode(xyMode))
        return false;

      if (! isChar(')'))
        return false;

      skipChar();
    }
    else {
      if (! isChar(')'))
        return false;

      skipChar();

      if (isChar(',')) {
        if (! readXYMode(xyMode))
          return false;
      }
    }
  }
  else if (arg == "A") {
    mode = ArgMode::A;
    vlen = 2;
  }
  
  // <label>
  else {
    mode = ArgMode::MEMORY;

    if (! getLabel(arg, value, vlen))
      ++numBadLabels_;
  }

  return true;
}

void
C6502::
clearLabels()
{
  labels_.clear();
}

void
C6502::
setLabel(const std::string &name, ushort addr, uchar len)
{
  if (isDebug()) {
    std::cerr << "setLabel '" << name << "' " << std::hex << int(addr) << "\n";
  }

  labels_[name] = AddrLen(addr, len);
}

bool
C6502::
getLabel(const std::string &name, ushort &value, uchar &len) const
{
  value = 0;

  auto p = labels_.find(name);
  if (p == labels_.end()) return false;

  value = (*p).second.addr;
  len   = (*p).second.len;

  if (isDebug()) {
    std::cerr << "getLabel '" << name << "' " << std::hex << int(value) << "\n";
  }

  return true;
}

//---

bool
C6502::
disassemble(ushort addr, std::ostream &os) const
{
  auto hex04 = [&](ushort value) {
    os << std::setfill('0') << std::setw(4) << std::right << std::hex << int(value);
  };

  ushort addr1 = addr;

  while (true) {
    hex04(addr1); os << " ";

    if (! disassembleAddr(addr1, os))
      break;
  }

  return true;
}

bool
C6502::
disassembleAddr(ushort addr, std::string &str, int &len) const
{
  std::stringstream ss;

  ushort addr1 = addr;

  bool rc = disassembleAddr(addr1, ss);

  len = addr1 - addr;

  std::string sstr = ss.str();

  if (sstr.size())
    str = sstr.substr(0, sstr.size() - 1);
  else
    str = "";

  return rc;
}

bool
C6502::
disassembleAddr(ushort &addr, std::ostream &os) const
{
  auto hex02 = [&](uchar value) {
    os << std::setfill('0') << std::setw(2) << std::right << std::hex << int(value);
  };

  auto shex02 = [&](schar value) {
    if (value < 0) { os << "-"; value = -value; }

    os << std::setfill('0') << std::setw(2) << std::right << std::hex << int(value);
  };

  auto hex04 = [&](ushort value) {
    os << std::setfill('0') << std::setw(4) << std::right << std::hex << int(value);
  };

  auto outputByte  = [&]() { hex02(readByte(addr)); };
  auto outputWord  = [&]() { hex04(ushort(readByte(addr) | (readByte(addr) << 8))); };
//auto outputSByte = [&]() { shex02(readSByte(addr)); };

  auto outputAbsolute  = [&]() { os << "$"; outputWord(); };
  auto outputZeroPage  = [&]() { os << "$"; outputByte(); };
  auto outputZeroPageX = [&]() { outputZeroPage(); os << ",X"; };
  auto outputZeroPageY = [&]() { outputZeroPage(); os << ",Y"; };
  auto outputAbsoluteX = [&]() { outputAbsolute(); os << ",X"; };
  auto outputAbsoluteY = [&]() { outputAbsolute(); os << ",Y"; };
  auto outputImmediate = [&]() { os << "#$"; outputByte(); };

  auto outputRelative  = [&]() {
    schar c = readSByte(addr); os << "$"; shex02(c); os << " ; ($"; hex04(addr + c); os << ")";
  };

  auto outputIndirect  = [&]() { os << "("; outputAbsolute(); os << ")"; };
  auto outputIndirectX = [&]() { os << "("; outputAbsolute(); os << ",X)"; };
  auto outputIndirectY = [&]() { os << "("; outputAbsolute(); os << ",Y)"; };

  auto c = readByte(addr);

  switch (c) {
    case 0x00: { os << "BRK\n"; return false; }

    //---

    // AND ...
    case 0x21: { os << "AND "; outputIndirectX(); os << "\n"; break; }
    case 0x25: { os << "AND "; outputZeroPage (); os << "\n"; break; }
    case 0x29: { os << "AND "; outputImmediate(); os << "\n"; break; }
    case 0x2D: { os << "AND "; outputAbsolute (); os << "\n"; break; }
    case 0x31: { os << "AND "; outputIndirectY(); os << "\n"; break; }
    case 0x35: { os << "AND "; outputZeroPageX(); os << "\n"; break; }
    case 0x39: { os << "AND "; outputAbsoluteY(); os << "\n"; break; }
    case 0x3D: { os << "AND "; outputAbsoluteX(); os << "\n"; break; }

    //---

    // ORA ...
    case 0x01: { os << "ORA "; outputIndirectX(); os << "\n"; break; }
    case 0x05: { os << "ORA "; outputZeroPage (); os << "\n"; break; }
    case 0x09: { os << "ORA "; outputImmediate(); os << "\n"; break; }
    case 0x0D: { os << "ORA "; outputAbsolute (); os << "\n"; break; }
    case 0x11: { os << "ORA "; outputIndirectY(); os << "\n"; break; }
    case 0x15: { os << "ORA "; outputZeroPageX(); os << "\n"; break; }
    case 0x19: { os << "ORA "; outputAbsoluteY(); os << "\n"; break; }
    case 0x1D: { os << "ORA "; outputAbsoluteX(); os << "\n"; break; }

    //---

    // EOR ...
    case 0x41: { os << "EOR "; outputIndirectX(); os << "\n"; break; }
    case 0x45: { os << "EOR "; outputZeroPage (); os << "\n"; break; }
    case 0x49: { os << "EOR "; outputImmediate(); os << "\n"; break; }
    case 0x4D: { os << "EOR "; outputAbsolute (); os << "\n"; break; }
    case 0x51: { os << "EOR "; outputIndirectY(); os << "\n"; break; }
    case 0x55: { os << "EOR "; outputZeroPageX(); os << "\n"; break; }
    case 0x59: { os << "EOR "; outputAbsoluteY(); os << "\n"; break; }
    case 0x5D: { os << "EOR "; outputAbsoluteX(); os << "\n"; break; }

    //---

    // ADC ...
    case 0x61: { os << "ADC "; outputIndirectX(); os << "\n"; break; }
    case 0x65: { os << "ADC "; outputZeroPage (); os << "\n"; break; }
    case 0x69: { os << "ADC "; outputImmediate(); os << "\n"; break; }
    case 0x6D: { os << "ADC "; outputAbsolute (); os << "\n"; break; }
    case 0x71: { os << "ADC "; outputIndirectY(); os << "\n"; break; }
    case 0x75: { os << "ADC "; outputZeroPageX(); os << "\n"; break; }
    case 0x79: { os << "ADC "; outputAbsoluteY(); os << "\n"; break; }
    case 0x7D: { os << "ADC "; outputAbsoluteX(); os << "\n"; break; }

    //---

    // SBC ...
    case 0xE1: { os << "SBC "; outputIndirectX(); os << "\n"; break; }
    case 0xE5: { os << "SBC "; outputZeroPage (); os << "\n"; break; }
    case 0xE9: { os << "SBC "; outputImmediate(); os << "\n"; break; }
    case 0xED: { os << "SBC "; outputAbsolute (); os << "\n"; break; }
    case 0xF1: { os << "SBC "; outputIndirectY(); os << "\n"; break; }
    case 0xF5: { os << "SBC "; outputZeroPageX(); os << "\n"; break; }
    case 0xF9: { os << "SBC "; outputAbsoluteY(); os << "\n"; break; }
    case 0xFD: { os << "SBC "; outputAbsoluteX(); os << "\n"; break; }

    //---

    // Bit functions

    case 0x06: { os << "ASL "; outputZeroPage (); os << "\n"; break; }
    case 0x0A: { os << "ASL A\n"; break; }
    case 0x0E: { os << "ASL "; outputAbsolute (); os << "\n"; break; }
    case 0x16: { os << "ASL "; outputZeroPageX(); os << "\n"; break; }
    case 0x1E: { os << "ASL "; outputAbsoluteX(); os << "\n"; break; }

    //-

    case 0x26: { os << "ROL "; outputZeroPage (); os << "\n"; break; }
    case 0x2A: { os << "ROL A\n"; break; }
    case 0x2E: { os << "ROL "; outputAbsolute (); os << "\n"; break; }
    case 0x36: { os << "ROL "; outputZeroPageX(); os << "\n"; break; }
    case 0x3E: { os << "ROL "; outputAbsoluteX(); os << "\n"; break; }

    //-

    case 0x46: { os << "LSR "; outputZeroPage (); os << "\n"; break; }
    case 0x4A: { os << "LSR A\n"; break; }
    case 0x4E: { os << "LSR "; outputAbsolute (); os << "\n"; break; }
    case 0x56: { os << "LSR "; outputZeroPageX(); os << "\n"; break; }
    case 0x5E: { os << "LSR "; outputAbsoluteX(); os << "\n"; break; }

    //-

    case 0x66: { os << "ROR "; outputZeroPage (); os << "\n"; break; }
    case 0x6A: { os << "ROR A\n"; break; }
    case 0x6E: { os << "ROR "; outputAbsolute (); os << "\n"; break; }
    case 0x76: { os << "ROR "; outputZeroPageX(); os << "\n"; break; }
    case 0x7E: { os << "ROR "; outputAbsoluteX(); os << "\n"; break; }

    //-

    // BIT...
    case 0x24: { os << "BIT "; outputZeroPage(); os << "\n"; break; }
    case 0x2C: { os << "BIT "; outputAbsolute(); os << "\n"; break; }

    //---

    case 0x08: { os << "PHP\n"; break; }
    case 0x28: { os << "PLP\n"; break; }
    case 0x48: { os << "PHA\n"; break; }
    case 0x68: { os << "PLA\n"; break; }

    //---

    // Branch, Jump

    case 0x10: { os << "BPL "; outputRelative(); os << "\n"; break; }
    case 0x20: { os << "JSR "; outputAbsolute(); os << "\n"; break; }
    case 0x30: { os << "BMI "; outputRelative(); os << "\n"; break; }
    case 0x50: { os << "BVC "; outputRelative(); os << "\n"; break; }
    case 0x70: { os << "BVS "; outputRelative(); os << "\n"; break; }
    case 0x90: { os << "BCC "; outputRelative(); os << "\n"; break; }
    case 0xB0: { os << "BCS "; outputRelative(); os << "\n"; break; }
    case 0xD0: { os << "BNE "; outputRelative(); os << "\n"; break; }
    case 0xF0: { os << "BEQ "; outputRelative(); os << "\n"; break; }

    case 0x4C: { os << "JMP "; outputAbsolute(); os << "\n"; break; }
    case 0x6C: { os << "JMP "; outputIndirect(); os << "\n"; break; }

    case 0x40: { os << "RTI\n"; break; }
    case 0x60: { os << "RTS\n"; break; }

    //---

    // Compare

    // CPY ...
    case 0xC0: { os << "CPY "; outputImmediate(); os << "\n"; break; }
    case 0xC4: { os << "CPY "; outputZeroPage (); os << "\n"; break; }
    case 0xCC: { os << "CPY "; outputAbsolute (); os << "\n"; break; }

    // CPX ...
    case 0xE0: { os << "CPX "; outputImmediate(); os << "\n"; break; }
    case 0xE4: { os << "CPX "; outputZeroPage (); os << "\n"; break; }
    case 0xEC: { os << "CPX "; outputAbsolute (); os << "\n"; break; }

    // CMP ...
    case 0xC1: { os << "CMP ", outputIndirectX(); os << "\n"; break; }
    case 0xC5: { os << "CMP "; outputZeroPage (); os << "\n"; break; }
    case 0xC9: { os << "CMP "; outputImmediate(); os << "\n"; break; }
    case 0xCD: { os << "CMP "; outputAbsolute (); os << "\n"; break; }
    case 0xD1: { os << "CMP ", outputIndirectY(); os << "\n"; break; }
    case 0xD5: { os << "CMP "; outputZeroPageX(); os << "\n"; break; }
    case 0xD9: { os << "CMP "; outputAbsoluteY(); os << "\n"; break; }
    case 0xDD: { os << "CMP "; outputAbsoluteX(); os << "\n"; break; }

    //---

    case 0x18: { os << "CLC\n"; break; }
    case 0x38: { os << "SEC\n"; break; }

    case 0x58: { os << "CLI\n"; break; }
    case 0x78: { os << "SEI\n"; break; }

    case 0xB8: { os << "CLV\n"; break; }

    case 0xD8: { os << "CLD\n"; break; }
    case 0xF8: { os << "SED\n"; break; }

    //---

    // Store

    // STA ...
    case 0x81: { os << "STA ", outputIndirectX(); os << "\n"; break; }
    case 0x85: { os << "STA "; outputZeroPage (); os << "\n"; break; }
    case 0x8D: { os << "STA "; outputAbsolute (); os << "\n"; break; }
    case 0x91: { os << "STA ", outputIndirectY(); os << "\n"; break; }
    case 0x95: { os << "STA "; outputZeroPageX(); os << "\n"; break; }
    case 0x99: { os << "STA "; outputAbsoluteY(); os << "\n"; break; }
    case 0x9D: { os << "STA "; outputAbsoluteX(); os << "\n"; break; }

    // STY ...
    case 0x84: { os << "STY "; outputZeroPage (); os << "\n"; break; }
    case 0x8C: { os << "STY "; outputAbsolute (); os << "\n"; break; }
    case 0x94: { os << "STY "; outputZeroPageX(); os << "\n"; break; }

    // STX ...
    case 0x86: { os << "STX "; outputZeroPage (); os << "\n"; break; }
    case 0x8E: { os << "STX "; outputAbsolute (); os << "\n"; break; }
    case 0x96: { os << "STX "; outputZeroPageY(); os << "\n"; break; }

    case 0x8A: { os << "TXA\n"; break; }
    case 0x9A: { os << "TXS\n"; break; }

    case 0xAA: { os << "TAX\n"; break; }
    case 0xBA: { os << "TSX\n"; break; }

    case 0x98: { os << "TYA\n"; break; }
    case 0xA8: { os << "TAY\n"; break; }

    // LDY...
    case 0xA0: { os << "LDY ", outputImmediate(); os << "\n"; break; }
    case 0xA4: { os << "LDY "; outputZeroPage (); os << "\n"; break; }
    case 0xAC: { os << "LDY "; outputAbsolute (); os << "\n"; break; }
    case 0xB4: { os << "LDY ", outputZeroPageX(); os << "\n"; break; }
    case 0xBC: { os << "LDY "; outputAbsoluteX(); os << "\n"; break; }

    // LDX...
    case 0xA2: { os << "LDX ", outputImmediate(); os << "\n"; break; }
    case 0xA6: { os << "LDX "; outputZeroPage (); os << "\n"; break; }
    case 0xAE: { os << "LDX "; outputAbsolute (); os << "\n"; break; }
    case 0xB6: { os << "LDX ", outputZeroPageY(); os << "\n"; break; }
    case 0xBE: { os << "LDX "; outputAbsoluteY(); os << "\n"; break; }

    // LDA...
    case 0xA1: { os << "LDA ", outputIndirectX(); os << "\n"; break; }
    case 0xA5: { os << "LDA "; outputZeroPage (); os << "\n"; break; }
    case 0xA9: { os << "LDA "; outputImmediate(); os << "\n"; break; }
    case 0xAD: { os << "LDA "; outputAbsolute (); os << "\n"; break; }
    case 0xB1: { os << "LDA ", outputIndirectY(); os << "\n"; break; }
    case 0xB5: { os << "LDA "; outputZeroPageX(); os << "\n"; break; }
    case 0xB9: { os << "LDA "; outputAbsoluteY(); os << "\n"; break; }
    case 0xBD: { os << "LDA "; outputAbsoluteX(); os << "\n"; break; }

    //---

    // Increment/Decrement

    case 0x88: { os << "DEY\n"; break; }
    case 0xCA: { os << "DEX\n"; break; }

    case 0xC8: { os << "INY\n"; break; }
    case 0xE8: { os << "INX\n"; break; }

    // DEC ...
    case 0xC6: { os << "DEC "; outputZeroPage (); os << "\n"; break; }
    case 0xCE: { os << "DEC "; outputAbsolute (); os << "\n"; break; }
    case 0xD6: { os << "DEC ", outputZeroPageX(); os << "\n"; break; }
    case 0xDE: { os << "DEC "; outputAbsoluteX(); os << "\n"; break; }

    // INC ...
    case 0xE6: { os << "INC "; outputZeroPage (); os << "\n"; break; }
    case 0xEE: { os << "INC "; outputAbsolute (); os << "\n"; break; }
    case 0xF6: { os << "INC ", outputZeroPageX(); os << "\n"; break; }
    case 0xFE: { os << "INC "; outputAbsoluteX(); os << "\n"; break; }

    //---

    case 0xEA: { os << "NOP\n"; break; }

    //---

    // unimplemented

              case 0x02:case 0x03:case 0x04:case 0x07:
                        case 0x0B:case 0x0C:          case 0x0F:
              case 0x12:case 0x13:case 0x14:case 0x17:
              case 0x1A:case 0x1B:case 0x1C:          case 0x1F:
              case 0x22:case 0x23:          case 0x27:
                        case 0x2B:                    case 0x2F:
              case 0x32:case 0x33:case 0x34:case 0x37:
              case 0x3A:case 0x3B:case 0x3C:          case 0x3F:
              case 0x42:case 0x43:case 0x44:case 0x47:
                        case 0x4B:                    case 0x4F:
              case 0x52:case 0x53:case 0x54:case 0x57:
              case 0x5A:case 0x5B:case 0x5C:          case 0x5F:
              case 0x62:case 0x63:case 0x64:case 0x67:
                        case 0x6B:                    case 0x6F:
              case 0x72:case 0x73:case 0x74:case 0x77:
              case 0x7A:case 0x7B:case 0x7C:          case 0x7F:
    case 0x80:case 0x82:case 0x83:          case 0x87:
    case 0x89:          case 0x8B:                    case 0x8F:
              case 0x92:case 0x93:          case 0x97:
                        case 0x9B:case 0x9C:case 0x9E:case 0x9F:
                        case 0xA3:          case 0xA7:
                        case 0xAB:                    case 0xAF:
              case 0xB2:case 0xB3:          case 0xB7:
                        case 0xBB:                    case 0xBF:
              case 0xC2:case 0xC3:          case 0xC7:
                        case 0xCB:                    case 0xCF:
              case 0xD2:case 0xD3:case 0xD4:case 0xD7:
              case 0xDA:case 0xDB:case 0xDC:          case 0xDF:
              case 0xE2:case 0xE3:          case 0xE7:
                        case 0xEB:                    case 0xEF:
              case 0xF2:case 0xF3:case 0xF4:case 0xF7:
              case 0xFA:case 0xFB:case 0xFC:          case 0xFF: {
      hex02(c); os << " (" << "???" << ")\n";
      break;
    }

    default:
      assert(false);
      return false;
  }

  return true;
}

//---

void
C6502::
print(ushort addr, int len)
{
  printState ();
  printMemory(addr, len);
}

void
C6502::
printState()
{
  auto hex02 = [&](std::ostream &os, uchar value) {
    os << std::setfill('0') << std::setw(2) << std::right << std::hex << int(value);
  };

  auto hex04 = [&](std::ostream &os, ushort value) {
    os << std::setfill('0') << std::setw(4) << std::right << std::hex << int(value);
  };

  //---

  std::cout << "PC=$"; hex04(std::cout, PC()); std::cout << " ";
  std::cout << "A=$";  hex02(std::cout, A ()); std::cout << " ";
  std::cout << "X=$";  hex02(std::cout, X ()); std::cout << " ";
  std::cout << "Y=$";  hex02(std::cout, Y ()); std::cout << " ";
  std::cout << "SP=$"; hex02(std::cout, SP()); std::cout << " ";

  std::cout << "Flags: ";

  auto printFlag = [&](bool b, char c) {
    std::cout << (b ? c : '-');
  };

  // Flags : N Z C I D V
  printFlag(Nflag(), 'N'); printFlag(Zflag(), 'Z'); printFlag(Cflag(), 'C');
  printFlag(Iflag(), 'I'); printFlag(Dflag(), 'D'); printFlag(Vflag(), 'V');

  //---

  int         alen;
  std::string astr;

  disassembleAddr(PC(), astr, alen);

  if (astr.size())
    std::cout << " " << astr;

  //---

  std::cout << "\n";
}

void
C6502::
printMemory(ushort addr, int len)
{
  auto hex02 = [&](std::ostream &os, uchar value) {
    os << std::setfill('0') << std::setw(2) << std::right << std::hex << int(value);
  };

  auto hex04 = [&](std::ostream &os, ushort value) {
    os << std::setfill('0') << std::setw(4) << std::right << std::hex << int(value);
  };

  //---

  int nl = (len + 15)/16;

  int i = 0;

  for (int il = 0; il < nl; ++il) {
    hex04(std::cout, addr + i); std::cout << ":";

    for (int i1 = 0; i1 < 16 && i < len; ++i1, ++i) {
      std::cout << " "; hex02(std::cout, getByte(addr + i));
    }

    std::cout << "\n";
  }
}
