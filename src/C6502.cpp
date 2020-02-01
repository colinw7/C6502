#include <C6502.h>
#include <CParser.h>

#include <algorithm>
#include <iostream>
#include <cassert>

//#include <c64_basic.h>
//#include <c64_kernel.h>

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

  std::memset(&mem_[0], 0, 0x10000*sizeof(mem_[0]));
}

//---

// NMI interrupt
void
C6502::
resetNMI()
{
  if (inNMI_) {
    std::cerr << "NMI in NMI\n";
    setBreak(true);
  }

  inNMI_ = true;

  setBFlag(false);
  setXFlag(true);

  // save state for RTI
  pushWord(PC());
  pushByte(SR());

  setIFlag(true);

  // jump to NMI interrupt vector
  setPC(NMI());

  incT(7);

  handleNMI();
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
  if (Iflag())
    return;

  if (inIRQ_) {
    std::cerr << "IRQ in IRQ\n";
    setBreak(true);
  }

  inIRQ_ = true;

  setBFlag(false);
  setXFlag(true);

  // save state for RTI
  pushWord(PC());
  pushByte(SR());

  setIFlag(true);

  // jump to IRQ interrupt vector
  setPC(IRQ());

  incT(7);

  handleIRQ();
}

void
C6502::
resetBRK()
{
  if (inBRK_) {
    std::cerr << "BRK in BRK\n";
    setBreak(true);
  }

  inBRK_ = true;

  // save state for RTI
  pushWord(PC());
  pushByte(SR());

  setIFlag(true);

  // jump to IRQ interrupt vector
  setPC(IRQ());

  incT(7);
}

void
C6502::
rti()
{
  if (! inNMI_ && ! inIRQ_ && ! inBRK_) {
    std::cerr << "RTI not in NMI, IRQ or BRK \n";
    setBreak(true);
  }

  inNMI_ = false;
  inIRQ_ = false;
  inBRK_ = false;

  setSR(popByte());
  setPC(popWord());

  incT(6);
}

//---

bool
C6502::
run()
{
  return run(org());
}

bool
C6502::
run(ushort addr)
{
  reset();

  setPC(addr);

  update();

  return cont();
}

bool
C6502::
cont()
{
  setBreak(false);

  while (! isHalt()) {
    step();

    update();

    if (isBreak())
      break;

    if (isTmpBreakpoint(PC()) || isBreakpoint(PC())) {
      breakpointHit();
      break;
    }
  }

  return true;
}

void
C6502::
step()
{
  auto c = readByte();

  switch (c) {
    case 0x00: { // BRK implicit
      setBFlag(true);
      setXFlag(true);

      resetBRK();

      setBreak(true);

      handleBreak();

      break;
    }

    //---

    // AND ...
    case 0x21: { // AND (indirect,X)
      andOp(getMemIndexedIndirectX()); incT(6); break;
    }
    case 0x25: { // AND zero page
      andOp(getZeroPage()); incT(3); break;
    }
    case 0x29: { // AND immediate
      andOp(readByte()); incT(2); break;
    }
    case 0x2D: { // AND absolute
      andOp(getAbsolute()); incT(4); break;
    }
    case 0x31: { // AND (indirect),Y
      andOp(getMemIndirectIndexedY()); incT(5); break;
    }
    case 0x35: { // AND zero page,X
      andOp(getZeroPageX()); incT(4); break;
    }
    case 0x39: { // AND absolute,Y
      andOp(getAbsoluteY()); incT(4); break;
    }
    case 0x3D: { // AND absolute,X
      andOp(getAbsoluteX()); incT(4); break;
    }

    //---

    // ORA ...
    case 0x01: { // ORA (indirect,X)
      orOp(getMemIndexedIndirectX()); incT(6); break;
    }
    case 0x05: { // ORA zero page
      orOp(getZeroPage()); incT(3); break;
    }
    case 0x09: { // ORA immediate
      orOp(readByte()); incT(2); break;
    }
    case 0x0D: { // ORA absolute
      orOp(getAbsolute()); incT(4); break;
    }
    case 0x11: { // ORA (indirect),Y
      orOp(getMemIndirectIndexedY()); incT(5); break;
    }
    case 0x15: { // ORA zero page,X
      orOp(getZeroPageX()); incT(4); break;
    }
    case 0x19: { // ORA absolute,Y
      orOp(getAbsoluteY()); incT(4); break;
    }
    case 0x1D: { // ORA absolute,X
      orOp(getAbsoluteX()); incT(4); break;
    }

    //---

    // EOR ...
    case 0x41: { // EOR (indirect,X)
      eorOp(getMemIndexedIndirectX()); incT(6); break;
    }
    case 0x45: { // EOR zero page
      eorOp(getZeroPage()); incT(3); break;
    }
    case 0x49: { // EOR immediate
      eorOp(readByte()); incT(2); break;
    }
    case 0x4D: { // EOR absolute
      eorOp(getAbsolute()); incT(4); break;
    }
    case 0x51: { // EOR (indirect),Y
      eorOp(getMemIndirectIndexedY()); incT(5); break;
    }
    case 0x55: { // EOR zero page,X
      eorOp(getZeroPageX()); incT(4); break;
    }
    case 0x59: { // EOR absolute,Y
      eorOp(getAbsoluteY()); incT(4); break;
    }
    case 0x5D: { // EOR absolute,X
      eorOp(getAbsoluteX()); incT(4); break;
    }

    //---

    // ADC ...
    case 0x61: { // ADC (indirect,X)
      adcOp(getMemIndexedIndirectX()); incT(6); break;
    }
    case 0x65: { // ADC zero page
      adcOp(getZeroPage()); incT(3); break;
    }
    case 0x69: { // ADC immediate
      adcOp(readByte()); incT(2); break;
    }
    case 0x6D: { // ADC absolute
      adcOp(getAbsolute()); incT(4); break;
    }
    case 0x71: { // ADC (indirect),Y
      adcOp(getMemIndirectIndexedY()); incT(5); break;
    }
    case 0x75: { // ADC zero page,X
      adcOp(getZeroPageX()); incT(4); break;
    }
    case 0x79: { // ADC absolute,Y
      adcOp(getAbsoluteY()); incT(4); break;
    }
    case 0x7D: { // ADC absolute,X
      adcOp(getAbsoluteX()); incT(4); break;
    }

    //---

    // SBC ...
    case 0xE1: { // SBC (indirect,X)
      sbcOp(getMemIndexedIndirectX()); incT(6); break;
    }
    case 0xE5: { // SBC zero page
      sbcOp(getZeroPage()); incT(3); break;
    }
    case 0xE9: { // SBC immediate
      sbcOp(readByte()); incT(2); break;
    }
    case 0xED: { // SBC absolute
      sbcOp(getAbsolute()); incT(4); break;
    }
    case 0xF1: { // SBC (indirect),Y
      sbcOp(getMemIndirectIndexedY()); incT(5); break;
    }
    case 0xF5: { // SBC zero page,X
      sbcOp(getZeroPageX()); incT(4); break;
    }
    case 0xF9: { // SBC absolute,Y
      sbcOp(getAbsoluteY()); incT(4); break;
    }
    case 0xFD: { // SBC absolute,X
      sbcOp(getAbsoluteX()); incT(4); break;
    }

    //---

    // Bit functions

    case 0x0A: { // ASL A
      aslAOp(); incT(2); break;
    }
    case 0x06: { // ASL zero page
      aslMemOp(readByte()); incT(5); break;
    }
    case 0x16: { // ASL zero page,X
      aslMemOp(sumBytes(readByte(), X())); incT(6); break;
    }
    case 0x0E: { // ASL absolute
      aslMemOp(readWord()); incT(6); break;
    }
    case 0x1E: { // ASL absolute,X
      aslMemOp(readWord() + X()); incT(7); break;
    }

    //-

    case 0x2A: { // ROL A
      rolAOp(); incT(2); break;
    }
    case 0x26: { // ROL zero page
      rolMemOp(readByte()); incT(5); break;
    }
    case 0x36: { // ROL zero page,X
      rolMemOp(sumBytes(readByte(), X())); incT(6); break;
    }
    case 0x2E: { // ROL absolute
      rolMemOp(readWord()); incT(6); break;
    }
    case 0x3E: { // ROL absolute,X
      rolMemOp(readWord() + X()); incT(7); break;
    }

    //-

    case 0x4A: { // LSR A
      lsrAOp(); incT(2); break;
    }
    case 0x46: { // LSR zero page
      lsrMemOp(readByte()); incT(5); break;
    }
    case 0x56: { // LSR zero page,X
      lsrMemOp(sumBytes(readByte(), X())); incT(6); break;
    }
    case 0x4E: { // LSR absolute
      lsrMemOp(readWord()); incT(6); break;
    }
    case 0x5E: { // LSR absolute,X
      lsrMemOp(readWord() + X()); incT(7); break;
    }

    //-

    case 0x6A: { // ROR A
      rorAOp(); incT(2); break;
    }
    case 0x66: { // ROR zero page
      rorMemOp(readByte()); incT(5); break;
    }
    case 0x76: { // ROR zero page,X
      rorMemOp(sumBytes(readByte(), X())); incT(6); break;
    }
    case 0x6E: { // ROR absolute
      rorMemOp(readWord()); incT(6); break;
    }
    case 0x7E: { // ROR absolute,X
      rorMemOp(readWord() + X()); incT(7); break;
    }

    //-

    // BIT...
    case 0x24: { // BIT zero page
      bitOp(getZeroPage()); incT(3); break;
    }
    case 0x2C: { // BIT absolute
      bitOp(getAbsolute()); incT(4); break;
    }

    //---

    case 0x08: { // PHP implied (Push Processor Status)
      setBFlag(true);
      setXFlag(true);

      pushByte(SR()); incT(3); break;
    }

    case 0x28: { // PLP implied (Pull Processor Status)
      setSR(popByte()); incT(4); break;
    }

    case 0x48: { // PHA implied (Push A)
      pushByte(A()); incT(3); break;
    }

    case 0x68: { // PLA implied (Pull A)
      setA(popByte()); setNZFlags(A()); incT(3); break;
    }

    //---

    // Branch, Jump

    case 0x10: { // BPL (Branch Positive Result)
      schar d = readSByte();

      if (! Nflag()) {
        setPC(PC() + d);

        if (d == -2)
          illegalJump();
      }

      incT(2);

      break;
    }

    case 0x20: { // JSR (Jump Save Return)
      ushort oldPC = PC();

      pushWord(PC() + 1); // next address (PC + 2) - 1

      setPC(readWord());

      incT(6);

      if (isEnableOutputProcs()) {
        if      (PC() == outAddr_ || PC() == outNAddr_) {
          bool nl = (PC() == outAddr_);

          uchar c1 = getByte(oldPC + 3);

          if (c1 & 0x01) { std::cout << " A=" ; outputHex02(std::cout, A()); }
          if (c1 & 0x02) { std::cout << " X=" ; outputHex02(std::cout, X()); }
          if (c1 & 0x04) { std::cout << " Y=" ; outputHex02(std::cout, Y()); }
          if (c1 & 0x08) { std::cout << " SP="; outputHex02(std::cout, SP()); }
          if (c1 & 0x10) { std::cout << " PC="; outputHex04(std::cout, PC()); }

          if (c1 & 0x80) {
            std::cout << " SR=";
            std::cout << (Nflag() ? "N" : "-");
            std::cout << (Vflag() ? "V" : "-");
            std::cout << (Xflag() ? "X" : "-");
            std::cout << (Bflag() ? "B" : "-");
            std::cout << (Dflag() ? "D" : "-");
            std::cout << (Iflag() ? "I" : "-");
            std::cout << (Zflag() ? "Z" : "-");
            std::cout << (Cflag() ? "C" : "-");
          }

          if (nl)
            std::cout << "\n";

          (void) popWord();

          setPC(oldPC + 4);

          break;
        }
        else if (PC() == outMemAddr_ || PC() == outMemNAddr_) {
          bool nl = (PC() == outMemAddr_);

          ushort addr1 = getWord(oldPC + 3);

          uchar c1 = getByte(addr1);

          outputHex02(std::cout, c1);

          if (nl)
            std::cout << "\n";

          (void) popWord();

          setPC(oldPC + 5);

          break;
        }
        else if (PC() == outStrAddr_) {
          ushort addr1 = getWord(oldPC + 3);

          uchar c1 = getByte(addr1);

          while (c1) {
            char c2 = char(c1);

            if (isspace(c2) || isprint(c2))
              std::cout << c2;
            else
              std::cout << '.';

            c1 = getByte(++addr1);
          }

          (void) popWord();

          setPC(oldPC + 5);

          break;
        }
      }

      if (isJumpPoint(PC()))
        jumpPointHit(c);

      if (PC() == oldPC - 1)
        illegalJump();

      break;
    }

    case 0x30: { // BMI (Branch Negative Result)
      schar d = readSByte();

      if (Nflag()) {
        setPC(PC() + d);

        if (d == -2)
          illegalJump();
      }

      incT(2);

      break;
    }

    case 0x50: { // BVC (Branch Overflow Clear)
      schar d = readSByte();

      if (! Vflag()) {
        setPC(PC() + d);

        if (d == -2)
          illegalJump();
      }

      incT(2);

      break;
    }

    case 0x70: { // BVS (Branch Overflow Set)
      schar d = readSByte();

      if (Vflag()) {
        setPC(PC() + d);

        if (d == -2)
          illegalJump();
      }

      incT(2);

      break;
    }

    case 0x90: { // BCC (Branch Carry Clear)
      schar d = readSByte();

      if (! Cflag()) {
        setPC(PC() + d);

        if (d == -2)
          illegalJump();
      }

      incT(2);

      break;
    }

    case 0xB0: { // BCS (Branch Carry Set)
      schar d = readSByte();

      if (Cflag()) {
        setPC(PC() + d);

        if (d == -2)
          illegalJump();
      }

      incT(2);

      break;
    }

    case 0xD0: { // BNE (Branch Zero Clear)
      schar d = readSByte();

      if (! Zflag()) {
        setPC(PC() + d);

        if (d == -2)
          illegalJump();
      }

      incT(2);

      break;
    }

    case 0xF0: { // BEQ (Branch Zero Set)
      schar d = readSByte();

      if (Zflag()) {
        setPC(PC() + d);

        if (d == -2)
          illegalJump();
      }

      incT(2);

      break;
    }

    case 0x4C: { // JMP absolute
      ushort oldPC = PC();

      setPC(readWord()); incT(3);

      if (isJumpPoint(PC()))
        jumpPointHit(c);

      if (PC() == oldPC - 1)
        illegalJump();

      break;
    }

    case 0x6C: { // JMP indirect (???)
      ushort oldPC = PC();

      setPC(getWord(readWord())); incT(3);

      if (isJumpPoint(PC()))
        jumpPointHit(c);

      if (PC() == oldPC - 1)
        illegalJump();

      break;
    }

    case 0x40: { // RTI (Return from Interrupt)
      rti();

      break;
    }

    case 0x60: { // RTS (Return from Subroutine)
      setPC(popWord() + 1); // JSR pushed return address - 1

      incT(6);

      break;
    }

    //---

    // Compare

    // CPY ...
    case 0xC0: { // CPY immediate
      uchar c1 = readByte();    cpyOp(c1); incT(2); break;
    }
    case 0xC4: { // CPY zero page
      uchar c1 = getZeroPage(); cpyOp(c1); incT(3); break;
    }
    case 0xCC: { // CPY absolute
      uchar c1 = getAbsolute(); cpyOp(c1); incT(4); break;
    }

    // CPX ...
    case 0xE0: { // CPX immediate
      uchar c1 = readByte();    cpxOp(c1); incT(2); break;
    }
    case 0xE4: { // CPX zero page
      uchar c1 = getZeroPage(); cpxOp(c1); incT(3); break;
    }
    case 0xEC: { // CPX absolute
      uchar c1 = getAbsolute(); cpxOp(c1); incT(4); break;
    }

    // CMP ...
    case 0xC1: { // CMP (indirect,X)
      uchar c1 = getMemIndexedIndirectX(); cmpOp(c1); incT(5); break;
    }
    case 0xC5: { // CMP zero page
      uchar c1 = getZeroPage();            cmpOp(c1); incT(3); break;
    }
    case 0xC9: { // CMP immediate
      uchar c1 = readByte();               cmpOp(c1); incT(2); break;
    }
    case 0xCD: { // CMP absolute
      uchar c1 = getAbsolute();            cmpOp(c1); incT(4); break;
    }
    case 0xD1: { // CMP (indirect),Y
      uchar c1 = getMemIndirectIndexedY(); cmpOp(c1); incT(5); break;
    }
    case 0xD5: { // CMP zero page,X
      uchar c1 = getZeroPageX();           cmpOp(c1); incT(4); break;
    }
    case 0xD9: { // CMP absolute,Y
      uchar c1 = getAbsoluteY();           cmpOp(c1); incT(4); break;
    }
    case 0xDD: { // CMP absolute,X
      uchar c1 = getAbsoluteX();           cmpOp(c1); incT(4); break;
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
      setZeroPage(A()); incT(3); break;
    }
    case 0x8D: { // STA absolute
      setAbsolute(A()); incT(4); break;
    }
    case 0x91: { // STA (indirect),Y
      setMemIndirectIndexedY(readByte(), A()); incT(6); break;
    }
    case 0x95: { // STA zero page,X
      setZeroPageX(A()); incT(4); break;
    }
    case 0x99: { // STA absolute,Y
      setAbsoluteY(A()); incT(5); break;
    }
    case 0x9D: { // STA absolute,X
      setAbsoluteX(A()); incT(5); break;
    }

    // STY ...
    case 0x84: { // STY zero page
      setZeroPage(Y()); incT(3); break;
    }
    case 0x8C: { // STY absolute
      setAbsolute(Y()); incT(4); break;
    }
    case 0x94: { // STY zero page,X
      setZeroPageX(Y()); incT(4); break;
    }

    // STX ...
    case 0x86: { // STX zero page
      setZeroPage(X()); incT(3); break;
    }
    case 0x8E: { // STX absolute
      setAbsolute(X()); incT(4); break;
    }
    case 0x96: { // STX zero page,Y
      setZeroPageY(X()); incT(4); break;
    }

    case 0x8A: { // TXA (Transfer X to A)
      setA(X()); setNZFlags(A()); incT(2); break;
    }
    case 0x9A: { // TXS (Transfer X to SP)
      setSP(X()); incT(2); break;
    }
    case 0xAA: { // TAX (Transfer A to X)
      setX(A()); setNZFlags(X()); incT(2); break;
    }
    case 0xBA: { // TSX (Transfer SP to X)
      setX(SP()); setNZFlags(X()); incT(2); break;
    }

    case 0x98: { // TYA (Transfer Y to A)
      setA(Y()); setNZFlags(A()); incT(2); break;
    }
    case 0xA8: { // TAY (Transfer A to Y)
      setY(A()); setNZFlags(Y()); incT(2); break;
    }

    // LDY...
    case 0xA0: { // LDY immediate
      setY(readByte()); setNZFlags(Y()); incT(3); break;
    }
    case 0xA4: { // LDY zero page
      setY(getZeroPage()); setNZFlags(Y()); incT(3); break;
    }
    case 0xAC: { // LDY absolute
      setY(getAbsolute()); setNZFlags(Y()); incT(4); break;
    }
    case 0xB4: { // LDY zero page,X
      setY(getZeroPageX()); setNZFlags(Y()); incT(4); break;
    }
    case 0xBC: { // LDY absolute,X
      setY(getAbsoluteX()); setNZFlags(Y()); incT(4); break;
    }

    // LDX...
    case 0xA2: { // LDX immediate
      setX(readByte()); setNZFlags(X()); incT(3); break;
    }
    case 0xA6: { // LDX zero page
      setX(getZeroPage()); setNZFlags(X()); incT(3); break;
    }
    case 0xAE: { // LDX absolute
      setX(getAbsolute()); setNZFlags(X()); incT(4); break;
    }
    case 0xB6: { // LDX zero page,Y
      setX(getZeroPageY()); setNZFlags(X()); incT(4); break;
    }
    case 0xBE: { // LDX absolute,Y
      setX(getAbsoluteY()); setNZFlags(X()); incT(4); break;
    }

    // LDA...
    case 0xA1: { // LDA (indirect,X)
      setA(getMemIndexedIndirectX()); setNZFlags(A()); incT(6); break;
    }
    case 0xA5: { // LDA zero page
      setA(getZeroPage()); setNZFlags(A()); incT(3); break;
    }
    case 0xA9: { // LDA immediate
      setA(readByte()); setNZFlags(A()); incT(2); break;
    }
    case 0xAD: { // LDA absolute
      setA(getAbsolute()); setNZFlags(A()); incT(4); break;
    }
    case 0xB1: { // LDA (indirect),Y
      setA(getMemIndirectIndexedY()); setNZFlags(A()); incT(5); break;
    }
    case 0xB5: { // LDA zero page,X
      setA(getZeroPageX()); setNZFlags(A()); incT(4); break;
    }
    case 0xB9: { // LDA absolute,Y
      setA(getAbsoluteY()); setNZFlags(A()); incT(4); break;
    }
    case 0xBD: { // LDA absolute,X
      setA(getAbsoluteX()); setNZFlags(A()); incT(4); break;
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
      ushort a = sumBytes(readByte(), X()); uchar c1 = getByte(a) - 1;
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
      ushort a = sumBytes(readByte(), X()); uchar c1 = getByte(a) + 1;
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
      std::cerr << "Invalid byte "; outputHex02(std::cerr, c);
      std::cerr << " @ "; outputHex04(std::cerr, PC() - 1); std::cerr << "\n";
      setBreak(true);
      break;
    }

    default:
      assert(false);
      break;
  }
}

bool
C6502::
next()
{
  std::string str;
  int         len;

  if (! disassembleAddr(PC(), str, len))
    return false;

  hasTmpBrk_ = true;
  tmpBrk_    = PC() + len;

  cont();

  hasTmpBrk_ = false;

  return true;
}

void
C6502::
update()
{
  if (isDebug())
    printState();
}

//---

bool
C6502::
assemble(ushort addr, std::istream &is, ushort &len)
{
  Lines lines;

  readLines(is, lines);

  clearLabels();

  numBadLabels_ = 0;

  ushort addr1 = addr;

  bool rc = assembleLines(addr, lines);

  if (numBadLabels_ > 0) {
    addr = addr1;

    rc = assembleLines(addr, lines);
  }

  len = addr - addr1;

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
assembleLines(ushort &addr, const Lines &lines)
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
  if (isDebug())
    std::cerr << "Line: '" << line << "'\n";

  CParser parse(line);

  while (! parse.eof()) {
    parse.skipSpace();

    // skip comment
    if (parse.isChar(';')) break;

    // read first word
    std::string word;

    if (! parse.readWord(word))
      continue;

    // handle label and update first word
    if (word[word.size() - 1] == ':') {
      std::string label = word.substr(0, word.size() - 1);

      setLabel(label, addr, 4);

      if (! parse.readWord(word))
        continue;
    }

    std::string opName = parse.toUpper(word);

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

      if (! parse.readWord(label)) {
        std::cerr << "Invalid define '" << line << "'\n";
        return false;
      }

      std::string value;

      if (! parse.readWord(value)) {
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

    if (opName == "ORG") {
      std::string arg;

      if (! parse.readWord(arg)) {
        std::cerr << "Invalid ORG '" << line << "'\n";
        return false;
      }

      CParser parse1(arg);

      if (! parse1.isValue()) {
        std::cerr << "Invalid ORG value '" << line << "'\n";
        return false;
      }

      uchar vlen;

      addr = parse1.getValue(vlen);

      break;
    }

    //---

    if (opName == "DB") {
      std::string arg;

      if (! parse.readWord(arg)) {
        std::cerr << "Invalid DB '" << line << "'\n";
        return false;
      }

      CParser parse1(arg);

      while (! parse1.eof()) {
        std::string word1;

        if (! parse1.readSepWord(word1, ','))
          break;

        CParser parse2(word1);

        if      (parse2.isString()) {
          char c = parse2.readChar();

          while (! parse2.eof() && ! parse2.isChar(c))
            setByte(addr++, parse2.readChar());
        }
        else if (parse2.isValue()) {
          uchar vlen;

          ushort value1 = parse2.getValue(vlen);

          if (value1 > 0xFF) {
            setByte(addr++, (value1 & 0xFF00) >> 8);
            setByte(addr++,  value1 & 0x00FF      );
          }
          else
            setByte(addr++, value1 & 0xFF);
        }
        else {
          std::cerr << "Invalid DB value '" << word1 << "'\n";
          return false;
        }
      }

      break;
    }

    //---

    if (opName == "OUT" || opName == "OUTN") {
      std::string arg;

      if (! parse.readWord(arg)) {
        std::cerr << "Invalid OUT/OUTN '" << line << "'\n";
        return false;
      }

      if (isEnableOutputProcs()) {
        CParser parse1(arg);

        uchar  o = 0x00;
        ushort a = 0x0000; bool aSet = false;

        while (! parse1.eof()) {
          std::string word1;

          if (! parse1.readSepWord(word1, ','))
            break;

          if      (word1 == "A" ) o |= 0x01;
          else if (word1 == "AF") o |= 0x81;
          else if (word1 == "X" ) o |= 0x02;
          else if (word1 == "Y" ) o |= 0x04;
          else if (word1 == "SP") o |= 0x08;
          else if (word1 == "PC") o |= 0x10;
          else if (word1 == "SR") o |= 0x80;

          else {
            CParser parse2(word1);

            std::string label;

            if      (parse2.isValue()) {
              uchar vlen;

              a    = parse2.getValue(vlen);
              aSet = true;
            }
            else if (parse2.readLabel(label)) {
              ushort lvalue;
              uchar  llen;

              if (! getLabel(label, lvalue, llen))
                ++numBadLabels_;

              a    = lvalue;
              aSet = true;
            }
            else {
              std::cerr << "Invalid OUT arg '" << word1 << "'\n";
              return false;
            }
          }
        }

        if (o != 0x00) {
          addByte(addr, 0x20                                    ); // JSR
          addWord(addr, (opName == "OUT" ? outAddr_ : outNAddr_)); // OUT
          addByte(addr, 0xA9                                    ); // LDA
          addByte(addr, o                                       ); // VALUE
        }

        if (aSet) {
          addByte(addr, 0x20                                          ); // JSR
          addWord(addr, (opName == "OUT" ? outMemAddr_ : outMemNAddr_)); // OUT
          addByte(addr, 0xAD                                          ); // LDA
          addWord(addr, a                                             ); // VALUE
        }
      }
      else {
        std::cerr << "OUT not enabled\n";
        return false;
      }

      break;
    }

    //---

    if (opName == "OUTS") {
      std::string arg;

      if (! parse.readWord(arg)) {
        std::cerr << "Invalid OUTS '" << line << "'\n";
        return false;
      }

      if (isEnableOutputProcs()) {
        CParser parse1(arg);

        ushort a = 0x0000;

        std::string label;

        if      (parse1.isValue()) {
          uchar vlen;

          a = parse1.getValue(vlen);
        }
        else if (parse1.readLabel(label)) {
          ushort lvalue;
          uchar  llen;

          if (! getLabel(label, lvalue, llen))
            ++numBadLabels_;

          a = lvalue;
        }
        else {
          std::cerr << "Invalid OUTS arg '" << arg << "'\n";
          return false;
        }

        addByte(addr, 0x20       ); // JSR
        addWord(addr, outStrAddr_); // OUT
        addByte(addr, 0xAD       ); // LDA
        addWord(addr, a          ); // VALUE
      }
      else {
        std::cerr << "OUTS not enabled\n";
        return false;
      }

      break;
    }

    //---

    // handle op

    if (isDebug())
      std::cerr << "  " << opName;

    // read optional arg
    std::string arg;

    if (parse.readWord(arg)) {
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
  auto addByte = [&](ushort c) { setByte(addr++, c & 0xFF); };
  auto addWord = [&](ushort c) { addByte(c & 0x00FF); addByte((c & 0xFF00) >> 8); };

  auto addRelative = [&](schar c) { setByte(addr++, c); };

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
        if (vlen <= 2) { return addOpByte(0x75, value); } // zero page
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
        if (vlen <= 2) { return addOpByte(0xD5, value); } // zero page
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
        if (vlen <= 2) { return addOpByte(0xD6, value); } // zero page
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
        if (vlen <= 2) { return addOpByte(0xF6, value); } // zero page
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
        if (vlen <= 2) { return addOpByte(0xB5, value); } // zero page
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
        if (vlen <= 2) { return addOpByte(0xB6, value); } // zero page
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
        if (vlen <= 2) { return addOpByte(0xB4, value); } // zero page
        else           { return addOpWord(0xBC, value); }
      }
    }

    return false;
  }

  else if (opName == "LSR") {
    if      (xyMode == XYMode::NONE) {
      // LSR #$xx (immediate)
      if      (mode == ArgMode::A) {
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

  else if (opName == "ROL") {
    if      (xyMode == XYMode::NONE) {
      // ROL #$xx (immediate)
      if      (mode == ArgMode::A) {
        return addOp(0x2A);
      }
      // ROL $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x26, value); } // zero page
        else           { return addOpWord(0x2E, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // ROL $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x36, value); } // zero page
        else           { return addOpWord(0x3E, value); }
      }
    }

    return false;
  }

  else if (opName == "ROR") {
    if      (xyMode == XYMode::NONE) {
      // ROR #$xx (immediate)
      if      (mode == ArgMode::A) {
        return addOp(0x6A);
      }
      // ROR $xx (absolute or zero page)
      else if (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x66, value); } // zero page
        else           { return addOpWord(0x6E, value); }
      }
    }
    else if (xyMode == XYMode::X) {
      // ROR $xx,X (absolute or zero page)
      if      (mode == ArgMode::MEMORY) {
        if (vlen <= 2) { return addOpByte(0x76, value); } // zero page
        else           { return addOpWord(0x7E, value); }
      }
    }

    return false;
  }

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
        if (vlen <= 2) { return addOpByte(0xF5, value); } // zero page
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

  CParser parse(arg);

  //---

  auto readXYMode = [&](XYMode &xyMode1) {
    if (parse.isChar(',')) {
      parse.skipChar();

      if      (parse.isChar('X') || parse.isChar('x')) {
        parse.skipChar(); xyMode1 = XYMode::X; return true;
      }
      else if (parse.isChar('Y') || parse.isChar('y')) {
        parse.skipChar(); xyMode1 = XYMode::Y; return true;
      }

      return false;
    }
    else {
      return true;
    }
  };

  //---

  // immediate #<value>
  if      (parse.isChar('#')) {
    parse.skipChar();

    mode = ArgMode::LITERAL;

    // byte/word
    if (parse.isValue()) {
      value = parse.getValue(vlen);
    }
    else {
      std::string label;

      if (parse.readLabel(label)) {
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
  else if (parse.isValue()) {
    mode  = ArgMode::MEMORY;
    value = parse.getValue(vlen);

    if (! readXYMode(xyMode))
      return false;
  }
  // indirect (<value>)
  else if (parse.isChar('(')) {
    parse.skipChar();

    mode = ArgMode::MEMORY_CONTENTS;

    // <value> byte/word
    if (parse.isValue()) {
      value = parse.getValue(vlen);
    }
    else {
      std::string label;

      if (parse.readLabel(label)) {
        ushort lvalue;
        uchar  llen;

        if (! getLabel(label, lvalue, llen))
          ++numBadLabels_;

        value = lvalue;
        vlen  = llen;
      }
    }

    if (parse.isChar(',')) {
      if (! readXYMode(xyMode))
        return false;

      if (! parse.isChar(')'))
        return false;

      parse.skipChar();
    }
    else {
      if (! parse.isChar(')'))
        return false;

      parse.skipChar();

      if (parse.isChar(',')) {
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
  ushort addr1 = addr;

  while (true) {
    outputHex04(os, addr1); os << " ";

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

  if (addr1 < addr) // wrapped
    len = 65536 - addr;
  else
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
  auto outputByte  = [&]() { outputHex02(os, readByte(addr)); };
  auto outputWord  = [&]() { outputHex04(os, ushort(readByte(addr) | (readByte(addr) << 8))); };
//auto outputSByte = [&]() { outputSHex02(os, readSByte(addr)); };

  auto outputAbsolute  = [&]() { os << "$"; outputWord(); };
  auto outputZeroPage  = [&]() { os << "$"; outputByte(); };
  auto outputZeroPageX = [&]() { outputZeroPage(); os << ",X"; };
  auto outputZeroPageY = [&]() { outputZeroPage(); os << ",Y"; };
  auto outputAbsoluteX = [&]() { outputAbsolute(); os << ",X"; };
  auto outputAbsoluteY = [&]() { outputAbsolute(); os << ",Y"; };
  auto outputImmediate = [&]() { os << "#$"; outputByte(); };

  auto outputRelative  = [&]() {
    schar c = readSByte(addr); os << "$"; outputSHex02(os, c);
    os << " ; ($"; outputHex04(os, addr + c); os << ")";
  };

  auto outputIndirect  = [&]() { os << "("; outputAbsolute(); os << ")"; };
  auto outputIndirectX = [&]() { os << "("; outputZeroPage(); os << ",X)"; };
  auto outputIndirectY = [&]() { os << "("; outputZeroPage(); os << "),Y"; };

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
      outputHex02(os, c); os << " (" << "???" << ")\n";
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
  printState();

  printMemory(addr, len);
}

void
C6502::
printState()
{
  std::cout << "PC=$"; outputHex04(std::cout, PC()); std::cout << " ";
  std::cout << "A=$";  outputHex02(std::cout, A ()); std::cout << " ";
  std::cout << "X=$";  outputHex02(std::cout, X ()); std::cout << " ";
  std::cout << "Y=$";  outputHex02(std::cout, Y ()); std::cout << " ";
  std::cout << "SP=$"; outputHex02(std::cout, SP()); std::cout << " ";

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
  int nl = (len + 15)/16;

  int i = 0;

  for (int il = 0; il < nl; ++il) {
    outputHex04(std::cout, addr + i); std::cout << ":";

    for (int i1 = 0; i1 < 16 && i < len; ++i1, ++i) {
      std::cout << " "; outputHex02(std::cout, getByte(addr + i));
    }

    std::cout << "\n";
  }
}

bool
C6502::
loadBin(const std::string &str)
{
  FILE *fp = fopen(str.c_str(), "r");
  if (! fp) return false;

  int i = 0;
  int c = 0;

  while ((c = fgetc(fp)) != EOF) {
    setByte(i, c & 0xFF);

    ++i;

    if (i >= 65536)
      break;
  }

  fclose(fp);

  return true;
}
