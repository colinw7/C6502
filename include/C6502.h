#ifndef C6502_H
#define C6502_H

#include <map>
#include <vector>
#include <iostream>
#include <cstring>

class C6502 {
 public:
  using uchar  = unsigned char;
  using schar  = signed char;
  using ushort = unsigned short;
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

  //------

  // Registers

  inline uchar A() const { return A_; }
  inline void setA(uchar c) { A_ = c; }

  inline uchar X() const { return X_ ; }
  inline void setX(uchar c) { X_ = c; }

  inline uchar Y() const { return Y_ ; }
  inline void setY(uchar c) { Y_ = c; }

  inline uchar SR() const { return SR_; }
  inline void setSR(uchar c) { SR_ = c; }

  inline uchar SP() const { return SP_; }
  inline void setSP(uchar c) { SP_ = c; }

  inline ushort PC() const { return PC_; }
  inline void setPC(ushort a) { PC_ = a; }

  //------

  // Flags

  inline bool Cflag() const { return SR_ & 0x01; }
  inline void setCFlag(bool b) { SR_ = (b ? SR_ | 0x01 : SR_ & ~0x01); }

  inline bool Zflag() const { return SR_ & 0x02; }
  inline void setZFlag(bool b) { SR_ = (b ? SR_ | 0x02 : SR_ & ~0x02); }

  inline bool Iflag() const { return SR_ & 0x04; }
  inline void setIFlag(bool b) { SR_ = (b ? SR_ | 0x04 : SR_ & ~0x04); }

  inline bool Dflag() const { return SR_ & 0x08; }
  inline void setDFlag(bool b) { SR_ = (b ? SR_ | 0x08 : SR_ & ~0x08); }

  inline bool Bflag() const { return SR_ & 0x10; }
  inline void setBFlag(bool b) { SR_ = (b ? SR_ | 0x10 : SR_ & ~0x10); }

  inline bool Vflag() const { return SR_ & 0x40; }
  inline void setVFlag(bool b) { SR_ = (b ? SR_ | 0x40 : SR_ & ~0x40); }

  inline bool Nflag() const { return SR_ & 0x80; }
  inline void setNFlag(bool b) { SR_ = (b ? SR_ | 0x80 : SR_ & ~0x80); }

  //---

  inline void setNZFlags() { setNZFlags(A()); }
  inline void setNZFlags(uchar c) { setNFlag(c & 0x80); setZFlag(c == 0); }

  inline void setNZCFlags(bool C) { setNZCFlags(A(), C); }
  inline void setNZCFlags(uchar c, bool C) { setNZFlags(c); setCFlag(C); }

  inline void setNZCVFlags(bool C, bool V) {
    SR_ |= ((A() & 0x80) | (A() & 0x02) | C*0x01 | V*0x40); }

  //------

  // Ticks

  inline void incT(uchar n) { t_ += n; }

  //------

  // Memory

  inline uchar readByte() { return readByte(PC_); }

  inline uchar readByte(ushort &addr) const { return getByte(addr++); }

  inline schar readSByte() { return readSByte(PC_); }
  inline schar readSByte(ushort &addr) const { return (schar) readByte(addr); }

  inline ushort readWord() { return readWord(PC_); }
  inline ushort readWord(ushort &addr) const { return (readByte(addr) | (readByte(addr) << 8)); }


  inline ushort getWord(ushort addr) const { return (getByte(addr) | (getByte(addr + 1) << 8)); }

  virtual uchar getByte(ushort addr) const { return mem_[addr]; }
  virtual void  setByte(ushort addr, uchar c) { mem_[addr] = c; }

  inline void setWord(ushort addr, ushort c) { setByte(addr, c & 0xFF); setByte(addr + 1, c >> 8); }

  inline void pushByte(uchar  c) { setByte(0x0100 + SP_--, c); }
  inline void pushWord(ushort a) { pushByte(a >> 8); pushByte(a & 0xFF); }

  inline uchar getSPByte(uchar sp) const { return getByte(0x0100 + sp); }

  inline uchar  popByte() { return getByte(0x0100 + ++SP_); }
  inline ushort popWord() { return (popByte() | (popByte() << 8)); }

//inline uchar mem(ushort addr) { return getByte(addr); }
//inline void setMem(ushort addr, uchar c) { setByte(addr, c); }

//inline ushort memAddr(ushort addr) { return getWord(addr); }

  inline uchar memIndexedIndirectX(uchar c) { return getByte(getWord(c + X())); }
  inline void setMemIndexedIndirectX(uchar c, uchar v) { return setByte(getWord(c + X()), v); }

  inline uchar memIndirectIndexedY(uchar c) { return getByte(getWord(c) + Y()); }
  inline void setMemIndirectIndexedY(uchar c, uchar v) { return setByte(getWord(c) + Y(), v); }

  virtual void memset(ushort addr, const uchar *data, ushort len) {
    memcpy(&mem_[addr], data, len);
  }

  //---

  // Operations

  // Arithmetic Shift Left (Zero Fill)
  inline void aslOp(uchar c) { bool C = c & 0x80; c <<= 1; setA(c); setNZCFlags(C); }

  // Logical Shift Right (Zero Fill)
  inline void lsrOp(uchar c) { bool C = c & 0x01; c >>= 1; setA(c); setNZCFlags(C); }

  // Rotate Left
  inline void rolOp(uchar c) { bool C = c & 0x80; c <<= 1; setA(c | C*0x01); setNZCFlags(C); }

  // Rotate Right
  inline void rorOp(uchar c) { bool C = c & 0x01; c >>= 1; setA(c | C*0x80); setNZCFlags(C); }

  inline void orOp (uchar c) { setA(A() | c); setNZFlags(); }
  inline void andOp(uchar c) { setA(A() & c); setNZFlags(); }
  inline void eorOp(uchar c) { setA(A() ^ c); setNZFlags(); }

  inline void adcOp(uchar c) {
    ushort res = A() + c;
    bool   C   = res > 0xFF;
    bool   V   = (((A() & 0x80) | (c & 0x80)) != (res & 0x80));
    setA(res); setNZCVFlags(C, V);
  }

  inline void sbcOp(uchar c) {
    ushort res = A() - c - Cflag();
    bool   C   = (A() < (c + Cflag()));
    bool   V   = (((A() & 0x80) | (c & 0x80)) != (res & 0x80));
    setA(res); setNZCVFlags(C, V);
  }

  //------

  // TODO: remove
  void load64Rom();

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

  //------

  // assemble

  bool assemble(ushort addr, std::istream &is);

  //------

  // run

  void step();

  bool run(ushort addr);

  //------

  // disassemble

  bool disassemble(ushort addr, std::ostream &os=std::cout) const;

  bool disassembleAddr(ushort addr1, std::string &str, int &len) const;

  //------

  // print

  void print(ushort addr, int len=64);

  void printState();

  void printMemory(ushort addr, int len);

 private:
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

  bool assembleLines(ushort addr, const Lines &lines);

  bool assembleLine(ushort &addr, const std::string &line);

  bool assembleOp(ushort &addr, const std::string &opName, const std::string &arg);

  bool decodeAssembleArg(const std::string &arg, ArgMode &mode, XYMode &xyMode,
                         ushort &value, uchar &vlen);

  void clearLabels();
  void setLabel(const std::string &name, ushort addr, uchar len);
  bool getLabel(const std::string &name, ushort &addr, uchar &len) const;

  bool disassembleAddr(ushort &addr1, std::ostream &os) const;

 private:
  ushort PC_ { 0 };
  uchar  A_  { 0 };
  uchar  X_  { 0 };
  uchar  Y_  { 0 };
  uchar  SR_ { 0 };
  uchar  SP_ { 0xFF };
  ulong  t_  { 0 };

  // memory
  //  stack: 0x0100 to 0x01FF
  uchar mem_[0x10000];

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

  bool debug_ { false };
};

#endif
