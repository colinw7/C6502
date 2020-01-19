#ifndef C6502Trace_H
#define C6502Trace_H

#include <C6502.h>

using uchar  = unsigned char;
using ushort = unsigned short;

class C6502Trace {
 public:
  C6502Trace(C6502 *cpu) : cpu_(cpu) { }

  virtual ~C6502Trace() { }

  virtual void initProc() { }
  virtual void termProc() { }

  virtual void preStepProc () { }
  virtual void postStepProc() { }

  virtual void regChanged(C6502::Reg) { }
  virtual void memChanged(ushort, ushort) { }

  virtual void breakpointsChanged() { }

  virtual void traceBackChanged() { }

  virtual void setStop(bool) { }
  virtual void setHalt(bool) { }

 protected:
  C6502 *cpu_ { nullptr };
};

#endif
