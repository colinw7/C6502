#include <CQ6502HexEdit.h>

#include <CStrUtil.h>
#include <cassert>

CQ6502HexEdit::
CQ6502HexEdit(uint size) :
 QLineEdit(nullptr), size_(size)
{
  assert(size == 2 || size == 4);

  setObjectName("hexEdit");

  connect(this, SIGNAL(returnPressed()), this, SLOT(valueChangedSlot()));
}

unsigned int
CQ6502HexEdit::
value() const
{
  uint value;

  if (! CStrUtil::decodeHexString(this->text().toStdString(), &value))
    return 0;

  return value;
}

void
CQ6502HexEdit::
setValue(uint value)
{
  setText(CStrUtil::toHexString(value, size_).c_str());
}

void
CQ6502HexEdit::
valueChangedSlot()
{
  uint value = this->value();;

  emit valueChanged(value);
}

QSize
CQ6502HexEdit::
sizeHint() const
{
  QFontMetrics fm(font());

  QSize s = QLineEdit::sizeHint();

  if (size_ == 2)
    s.setWidth(fm.horizontalAdvance("00") + 6);
  else
    s.setWidth(fm.horizontalAdvance("0000") + 6);

  return s;
}
