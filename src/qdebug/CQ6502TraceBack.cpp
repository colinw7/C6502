#include <CQ6502TraceBack.h>
#include <C6502.h>

CQ6502TraceBack::
CQ6502TraceBack(C6502 *c6502) :
 c6502_(c6502)
{
  setObjectName("traceBack");

  setReadOnly(true);
}

void
CQ6502TraceBack::
setFixedFont(const QFont &font)
{
  setFont(font);
}

void
CQ6502TraceBack::
update()
{
  clear();

#if 0
  for (int i = 0; i < c6502_->traceNum(); ++i) {
    ushort v = c6502_->traceValue(i);

    std::string str = C6502::hexString(v);

    append(str.c_str());
  }
#endif
}
