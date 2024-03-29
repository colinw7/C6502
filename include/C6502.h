#ifndef C6502_H
#define C6502_H

#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cassert>
#include <algorithm>

class C6502 {
 public:
  using uchar  = unsigned char;
  using schar  = signed char;
  using ushort = unsigned short;
  using sshort = signed short;
  using ulong  = unsigned long;

  enum class Reg {
    NONE,
    A,
    X,
    Y,
    SR,
    SP,
    PC
  };

 public:
  C6502();

  virtual ~C6502() { }

  //---

  bool isDebug() const { return debug_; }
  void setDebug(bool b) { debug_ = b; }

  //---

  bool isHalt() const { return halt_; }
  void setHalt(bool b) { halt_ = b; }

  bool isBreak() const { return break_; }
  void setBreak(bool b) { break_ = b; }

  bool isDebugger() const { return debugger_; }
  void setDebugger(bool b) { debugger_ = b; }

  const ushort &org() const { return org_; }
  void setOrg(const ushort &v) { org_ = v; }

  bool isUnsupported() const { return unsupported_; }
  void setUnsupported(bool b) { unsupported_ = b; }

  bool isEnableOutputProcs() const { return enableOutputProcs_; }
  void setEnableOutputProcs(bool b) { enableOutputProcs_ = b; }

  bool inNMI() const { return inNMI_; }

  //------

  // Registers

  inline uchar A() const { return A_; }
  inline void setA(uchar c) { A_ = c; registerChanged(Reg::A); }

  inline uchar X() const { return X_ ; }
  inline void setX(uchar c) { X_ = c; registerChanged(Reg::X); }

  inline uchar Y() const { return Y_ ; }
  inline void setY(uchar c) { Y_ = c; registerChanged(Reg::Y); }

  virtual void registerChanged(Reg) { }

  //--

  inline uchar SR() const { return SR_; }
  inline void setSR(uchar c) { SR_ = c; flagsChanged(); }

  virtual void flagsChanged() { }

  //--

  inline uchar SP() const { return SP_; }
  inline void setSP(uchar c) { SP_ = c; stackChanged(); }

  virtual void stackChanged() { }

  //--

  inline ushort PC() const { return PC_; }
  inline void setPC(ushort a) { PC_ = a; pcChanged(); }

  virtual void pcChanged() { }

  virtual void illegalJump() { }

  //------

  // Flags

  inline bool Cflag() const { return SR() & 0x01; }
  inline void setCFlag(bool b) { setSR(b ? SR() | 0x01 : SR() & ~0x01); }

  inline bool Zflag() const { return SR() & 0x02; }
  inline void setZFlag(bool b) { setSR(b ? SR() | 0x02 : SR() & ~0x02); }

  inline bool Iflag() const { return SR() & 0x04; }
  inline void setIFlag(bool b) { setSR(b ? SR() | 0x04 : SR() & ~0x04); }

  inline bool Dflag() const { return SR() & 0x08; }
  inline void setDFlag(bool b) { setSR(b ? SR() | 0x08 : SR() & ~0x08); }

  inline bool Bflag() const { return SR() & 0x10; }
  inline void setBFlag(bool b) { setSR(b ? SR() | 0x10 : SR() & ~0x10); }

  inline bool Xflag() const { return SR() & 0x20; }
  inline void setXFlag(bool b) { setSR(b ? SR() | 0x20 : SR() & ~0x20); }

  inline bool Vflag() const { return SR() & 0x40; }
  inline void setVFlag(bool b) { setSR(b ? SR() | 0x40 : SR() & ~0x40); }

  inline bool Nflag() const { return SR() & 0x80; }
  inline void setNFlag(bool b) { setSR(b ? SR() | 0x80 : SR() & ~0x80); }

  //---

  inline void setNZFlags() { setNZFlags(A()); }
  inline void setNZFlags(uchar c) { setNFlag(c & 0x80); setZFlag(c == 0); }

  inline void setNZCFlags(bool C) { setNZCFlags(A(), C); }
  inline void setNZCFlags(uchar c, bool C) { setNZFlags(c); setCFlag(C); }

  inline void setNZCVFlags(bool C, bool V) {
    setNZFlags(); setCFlag(C); setVFlag(V);
  }

  //------

  // Ticks

  inline void incT(uchar n) { t_ += n; tick(n); }

  virtual void tick(uchar) { }

  //------

  // Memory

  inline uchar sumBytes(uchar c1, uchar c2) { return (c1 + c2) & 0xFF; }

  inline uchar readByte() { return readByte(PC_); }

  inline uchar readByte(ushort &addr) const { return getByte(addr++); }

  inline schar readSByte() { return readSByte(PC_); }
  inline schar readSByte(ushort &addr) const { return schar(readByte(addr)); }

  inline ushort readWord() { return readWord(PC_); }
  inline ushort readWord(ushort &addr) const { return (readByte(addr) | (readByte(addr) << 8)); }

  inline ushort getWord(ushort addr) const { return (getByte(addr) | (getByte(addr + 1) << 8)); }

  virtual uchar getByte(ushort addr) const { return mem_[addr]; }
  virtual void  setByte(ushort addr, uchar c) { mem_[addr] = c; memChanged(addr, 1); }

  inline void setWord(ushort addr, ushort c) {
    setByte(addr, uchar(c & 0xFF)); setByte(addr + 1, uchar(c >> 8)); }

  inline void pushByte(uchar  c) { setByte(0x0100 | SP(), c); setSP(SP() - 1); }
  inline void pushWord(ushort a) { pushByte(uchar(a >> 8)); pushByte(uchar(a & 0xFF)); }

  inline uchar getSPByte(uchar sp) const { return getByte(0x0100 | sp); }

  inline uchar  popByte() { setSP(SP() + 1); return getByte(0x0100 | SP()); }
  inline ushort popWord() { return (popByte() | (popByte() << 8)); }

  inline uchar peekByte() { return getByte(0x0100 | sumBytes(SP(), 1)); }

//inline uchar mem(ushort addr) { return getByte(addr); }
//inline void setMem(ushort addr, uchar c) { setByte(addr, c); }

//inline ushort memAddr(ushort addr) { return getWord(addr); }

  inline uchar memIndexedIndirectX(uchar c) {
    return getByte(getWord(sumBytes(c, X()))); }
  inline void setMemIndexedIndirectX(uchar c, uchar v) {
    return setByte(getWord(sumBytes(c, X())), v); }

  inline uchar memIndirectIndexedY(uchar c) { return getByte(getWord(c) + Y()); }
  inline void setMemIndirectIndexedY(uchar c, uchar v) { return setByte(getWord(c) + Y(), v); }

  // store len bytes of data at 'data' in CPU at address 'addr'
  virtual void memset(ushort addr, const uchar *data, ushort len) {
    std::memcpy(&mem_[addr], data, len); memChanged(addr, len);
  }

  // get len bytes in 'data' from CPU memory at address 'addr'
  virtual void memget(ushort addr, uchar *data, ushort len) {
    std::memcpy(data, &mem_[addr], len);
  }

  virtual void memChanged(ushort /*addr*/, ushort /*len*/) { }

  //---

  void addByte(ushort &addr, uchar c) { setByte(addr++, c); };

  void addWord(ushort &addr, ushort c) {
    addByte(addr, uchar(c & 0x00FF)); addByte(addr, uchar((c & 0xFF00) >> 8));
  }

  //------

  virtual bool isReadOnly(ushort /*pos*/, ushort /*len*/) const { return false; }
  virtual bool isScreen  (ushort /*pos*/, ushort /*len*/) const { return false; }

  //---

  // Shift Operations

  // Arithmetic Shift Left (Zero Fill)
  inline void aslAOp() {
    uchar c = A();           bool C = aslCalc(c); setA(c);          setNZCFlags(c, C); }

  inline void aslMemOp(ushort addr) {
    uchar c = getByte(addr); bool C = aslCalc(c); setByte(addr, c); setNZCFlags(c, C); }

  inline bool aslCalc(uchar &c) { bool C = (c & 0x80); c <<= 1; return C; }

  // Logical Shift Right (Zero Fill)
  inline void lsrAOp() {
    uchar c = A();           bool C = lsrCalc(c); setA(c);          setNZCFlags(c, C); }
  inline void lsrMemOp(ushort addr) {
    uchar c = getByte(addr); bool C = lsrCalc(c); setByte(addr, c); setNZCFlags(c, C); }

  inline bool lsrCalc(uchar &c) { bool C = c & 0x01; c >>= 1; return C; }

  // Rotate Left
  inline void rolAOp() {
    uchar c = A();           bool C = rolCalc(c); setA(c);          setNZCFlags(c, C); }

  inline void rolMemOp(ushort addr) {
    uchar c = getByte(addr); bool C = rolCalc(c); setByte(addr, c); setNZCFlags(c, C); }

  inline bool rolCalc(uchar &c) {
    bool C1 = Cflag();    // save old carry flag
    bool C  = (c & 0x80); // new carry flag from bit 7

    c = ((c << 1) | C1*0x01); // set result to left shifted value with new bit 0

    return C;
  }

  // Rotate Right
  inline void rorAOp() {
    uchar c = A();           bool C = rorCalc(c); setA(c);          setNZCFlags(c, C); }

  inline void rorMemOp(ushort addr) {
    uchar c = getByte(addr); bool C = rorCalc(c); setByte(addr, c); setNZCFlags(c, C); }

  inline bool rorCalc(uchar &c) {
    bool C1 = Cflag();    // save old carry flag
    bool C  = (c & 0x01); // new carry flag from bit 0

    c = ((c >> 1) | C1*0x80); // set result to right shifted value with new bit 7

    return C;
  }

  //---

  // Bit Operations

  inline void bitOp(uchar c) {
    setVFlag(c & 0x40);          // set V flag from bit 6 of c
    setNFlag(c & 0x80);          // set N flag from bit 7 of c
    setZFlag((c & A()) == 0x00); // set Z flag if byte AND A is zero
  }

  inline void orOp (uchar c) { setA(A() | c); setNZFlags(); }
  inline void andOp(uchar c) { setA(A() & c); setNZFlags(); }
  inline void eorOp(uchar c) { setA(A() ^ c); setNZFlags(); }

  //---

  // Add with carry

  inline void adcOp(uchar c) {
    if (Dflag()) {
      bool   C   = Cflag();
      ushort res = bcdAdd(A(), c, C);
      bool   V   = (res & 0x80);

      setA(uchar(res)); setNZCVFlags(C, V);
    }
    else {
      bool   C   = Cflag();
      ushort res = ushort(A() + c + C);

      C = res > 0xFF;

      bool V = ~(A() ^ c) & (A() ^ res) & 0x80;

      setA(uchar(res)); setNZCVFlags(C, V);
    }
  }

  inline ushort bcdAdd(uchar a, uchar b, bool &c) {
    uchar t1 = (a & 0xF0) >> 4; uchar u1 = (a & 0x0F);
    uchar t2 = (b & 0xF0) >> 4; uchar u2 = (b & 0x0F);

    uchar t = t1 + t2;
    uchar u = uchar(u1 + u2 + c);

    if (u >= 10) {
      ++t; u -= 10;
    }

    c = false;

    if (t >= 10) {
      t -= 10;

      c = true;
    }

    return (((t & 0x0F) << 4) | (u & 0x0F));
  }

  //---

  // Substract with carry

  inline void sbcOp(uchar c) {
    if (Dflag()) {
      bool  C   = ! Cflag();
      uchar res = bcdSub(A(), c, C);
      bool  V   = false; // TODO

      setA(res); setNZCVFlags(C, V);
    }
    else {
/*
      bool  C  = ! Cflag();
      schar a  = A();
      schar c1 = c;

      int res1 = a - (c1 + C);

      uchar res = res1;

      C = (res1 >= 0);

      bool V = (res1 < -128 || res1 > 127);

      setA(res); setNZCVFlags(C, V);
*/
      return adcOp(~c);
    }
  }

  inline uchar bcdSub(uchar a, uchar b, bool &c) {
    uchar t1 = (a & 0xF0) >> 4; uchar u1 = (a & 0x0F);
    uchar t2 = (b & 0xF0) >> 4; uchar u2 = (b & 0x0F);

    schar t, u;

    if (a >= b + c) {
      t = schar(t1 - t2);
      u = schar(u1 - u2 - c);

      if (u < 0) {
        --t; u += 10;
      }

      assert(t >= 0);

      c = true;
    }
    else {
      t = schar(t2 - t1);
      u = schar(u2 - u1);

      if (u < 0) {
        --t; u += 10;
      }

      assert(t >= 0);

      t = 9 - t;

      if (u > 0)
        u = 10 - u;

      c = true;
    }

    return uchar(((t & 0x0F) << 4) | (u & 0x0F));
  }

  //---

  // Compare

  inline void cmpOp(uchar c) {
    uchar c1 = (A() -  c);
    bool  C  = (A() >= c);
    setNZCFlags(c1, C);
  }

  inline void cpxOp(uchar c) {
    uchar c1 = (X() -  c);
    bool  C  = (X() >= c);
    setNZCFlags(c1, C);
  }

  inline void cpyOp(uchar c) {
    uchar c1 = (Y() -  c);
    bool  C  = (Y() >= c);
    setNZCFlags(c1, C);
  }

  //------

  void reset() {
    PC_ = 0;
    A_  = 0;
    X_  = 0;
    Y_  = 0;
    SR_ = 0;
    SP_ = 0xFF;
    t_  = 0;
  }

  //------

  // System vectors

  // Non-Maskable Interrupt Hardware Vector
  inline ushort NMI() const { return getWord(0xFFFA); }
  // System Reset (RES) Hardware Vector
  inline ushort RES() const { return getWord(0xFFFC); }
  // askable Interrupt Request and Break Hardware Vectors
  inline ushort IRQ() const { return getWord(0xFFFE); }

  void resetNMI   ();
  void resetSystem();
  void resetIRQ   ();
  void resetBRK   ();

  void rti();

  virtual void handleNMI() { }
  virtual void handleIRQ() { }
  virtual void handleBreak() { }

  //------

  // assemble

  bool assemble(ushort addr, std::istream &is, ushort &len);

  //------

  // run

  void step();

  bool next();

  bool run();
  bool run(ushort addr);

  bool cont();

  virtual void update();

  //------

  // disassemble

  bool disassemble(ushort addr, std::ostream &os=std::cout) const;

  bool disassembleAddr(ushort addr1, std::string &str, int &len) const;

  //------

  // print

  void print(ushort addr, int len=64);

  void printState();

  void printMemory(ushort addr, int len);

  //------

  // load
  bool loadBin(const std::string &str);

  //------

  // breakpoints

  void addBreakpoint(ushort addr) {
    breakpoints_.insert(addr);

    breakpointsChanged();
  }

  void removeBreakpoint(ushort addr) {
    breakpoints_.erase(addr);

    breakpointsChanged();
  }

  void removeAllBreakpoints() {
    breakpoints_.clear();

    breakpointsChanged();
  }

  void getBreakpoints(std::vector<ushort> &addrs) const {
    for (const auto &addr : breakpoints_)
      addrs.push_back(addr);
  }

  bool isTmpBreakpoint(ushort addr) const {
    return (hasTmpBrk_ && addr == tmpBrk_);
  }

  bool isBreakpoint(ushort addr) const {
    return (breakpoints_.find(addr) != breakpoints_.end());
  }

  virtual void breakpointHit() { }

  virtual void breakpointsChanged() { }

  //------

  // jumpPoints

  void addJumpPoint(ushort addr) {
    jumpPoints_.insert(addr);

    jumpPointsChanged();
  }

  void removeJumpPoint(ushort addr) {
    jumpPoints_.erase(addr);

    jumpPointsChanged();
  }

  void removeAllJumpPoints() {
    jumpPoints_.clear();

    jumpPointsChanged();
  }

  void getJumpPoints(std::vector<ushort> &addrs) const {
    for (const auto &addr : jumpPoints_)
      addrs.push_back(addr);
  }

  bool isJumpPoint(ushort addr) const {
    return (jumpPoints_.find(addr) != jumpPoints_.end());
  }

  virtual void jumpPointsChanged() { }

  virtual void jumpPointHit(uchar /*inst*/) { }

 private:
  // get byte from zero page using offset from next byte
  inline uchar getZeroPage() { return getByte(readByte()); }
  // set byte in zero page using offset from next byte
  inline void setZeroPage(uchar c) { setByte(readByte(), c); }

  // get byte from zero page using offset from next byte and X register
  inline uchar getZeroPageX() { return getByte(sumBytes(readByte(), X())); }
  inline void setZeroPageX(uchar c) { setByte(sumBytes(readByte(), X()), c); }

  // get byte from zero page using offset from next byte and Y register
  inline uchar getZeroPageY() { return getByte(sumBytes(readByte(), Y())); }
  inline void setZeroPageY(uchar c) { setByte(sumBytes(readByte(), Y()), c); }

  // get byte from read address
  inline uchar getAbsolute() { return getByte(readWord()); }
  inline void setAbsolute(uchar c) { setByte(readWord(), c); }

  // get byte from read address offset by X register
  inline uchar getAbsoluteX() { return getByte(readWord() + X()); }
  inline void setAbsoluteX(uchar c) { setByte(readWord() + X(), c); }

  // get byte from read address offset by Y register
  inline uchar getAbsoluteY() { return getByte(readWord() + Y()); }
  inline void setAbsoluteY(uchar c) { setByte(readWord() + Y(), c); }

  // get byte from memory address at zero page address (from next byte plus X register)
  inline uchar getMemIndexedIndirectX() { return memIndexedIndirectX(readByte()); }

  // get byte from memory address at zero page address (from next byte) plus Y register
  inline uchar getMemIndirectIndexedY() { return memIndirectIndexedY(readByte()); }

  //---

  // assemble

  enum class XYMode {
    NONE,
    X,
    Y
  };

  enum class ArgMode {
    NONE,
    LITERAL,
    MEMORY,
    MEMORY_CONTENTS,
    A
  };

  using Lines = std::vector<std::string>;

  void readLines(std::istream &is, Lines &lines);

  bool assembleLines(ushort &addr, const Lines &lines);

  bool assembleLine(ushort &addr, const std::string &line);

  bool assembleOp(ushort &addr, const std::string &opName, const std::string &arg);

  bool decodeAssembleArg(const std::string &arg, ArgMode &mode, XYMode &xyMode,
                         ushort &value, uchar &vlen);

  void clearLabels();
  void setLabel(const std::string &name, ushort addr, uchar len);
  bool getLabel(const std::string &name, ushort &addr, uchar &len) const;

  //---

  // disassemble

  bool disassembleAddr(ushort &addr1, std::ostream &os) const;

  //---

  void outputHex02(std::ostream &os, uchar value) const {
    os << std::setfill('0') << std::setw(2) << std::right << std::hex << int(value);
  }

  void outputHex04(std::ostream &os, ushort value) const {
    os << std::setfill('0') << std::setw(4) << std::right << std::hex << int(value);
  }

  void outputSHex02(std::ostream &os, schar value) const {
    if (value < 0) { os << "-"; value = -value; }

    os << std::setfill('0') << std::setw(2) << std::right << std::hex << int(value);
  }

 private:
  // registers
  ushort PC_ { 0 };
  uchar  A_  { 0 };
  uchar  X_  { 0 };
  uchar  Y_  { 0 };
  uchar  SR_ { 0 };
  uchar  SP_ { 0xFF };
  ulong  t_  { 0 };

  //---

  // memory
  //  stack: 0x0100 to 0x01FF
  uchar mem_[0x10000];

  //---

  // interrupts
  bool inNMI_ { false };
  bool inIRQ_ { false };
  bool inBRK_ { false };

  //---

  // labels (assember)
  struct AddrLen {
    ushort addr { 0 };
    uchar  len  { 0 };

    AddrLen(ushort addr1=0, uchar len1=0) :
     addr(addr1), len(len1) {
    }
  };

  using Labels = std::map<std::string,AddrLen>;

  Labels labels_;
  int    numBadLabels_ { 0 };

  //---

  bool debug_ { false };

  //---

  bool halt_  { false };
  bool break_ { false };

  bool debugger_ { false };

  ushort org_ { 0x0000 };

  bool unsupported_ { false };

  //---

  // breakpoints
  using Breakpoints = std::set<ushort>;

  Breakpoints breakpoints_;

  bool   hasTmpBrk_ { false };
  ushort tmpBrk_    { 0 };

  //---

  // Jump Points
  using JumpPoints = std::set<ushort>;

  JumpPoints jumpPoints_;

  //---

  // Debug Output

  bool enableOutputProcs_ { false };

  ushort outAddr_     { 0xFFF0 };
  ushort outNAddr_    { 0xFFF2 };
  ushort outMemAddr_  { 0xFFF4 };
  ushort outMemNAddr_ { 0xFFF6 };
  ushort outStrAddr_  { 0xFFF8 };
};

#endif
