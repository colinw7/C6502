#include <CQ6502Stack.h>
#include <C6502.h>
#include <CStrUtil.h>

CQ6502Stack::
CQ6502Stack(C6502 *c6502) :
 c6502_(c6502)
{
  setObjectName("stackText");

  setReadOnly(true);
}

void
CQ6502Stack::
setFixedFont(const QFont &font)
{
  setFont(font);
}

void
CQ6502Stack::
update()
{
  clear();

  uchar sp = 0xff;

  std::string str;

  for (ushort i = 0; i < 256; ++i) {
    uchar sp1 = sp - i;

    str = "";

    if (sp1 == c6502_->SP())
      str += "<b><font color=\"red\">&gt;</font></b>";
    else
      str += " ";

    str += CStrUtil::toHexString(sp1, 2);

    str += " ";

    str += CStrUtil::toHexString(c6502_->getSPByte(sp1), 2);

    append(str.c_str());
  }
}
