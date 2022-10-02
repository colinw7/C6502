#include <CQ6502RegEdit.h>
#include <C6502.h>
#include <CStrUtil.h>

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <cassert>

CQ6502RegEdit::
CQ6502RegEdit(C6502 *c6502, C6502::Reg reg) :
 QFrame(nullptr), c6502_(c6502), reg_(reg)
{
  QHBoxLayout *layout = new QHBoxLayout(this);

  layout->setMargin(2); layout->setMargin(2);

  QString str;

  switch (reg) {
    case C6502::Reg::A : str = "A" ; break;
    case C6502::Reg::X : str = "X" ; break;
    case C6502::Reg::Y : str = "Y" ; break;
    case C6502::Reg::SR: str = "SR"; break;
    case C6502::Reg::SP: str = "SP"; break;
    case C6502::Reg::PC: str = "PC"; break;
    default            : assert(false);
  }

  setObjectName(str);

  label_ = new QLabel(str);

  label_->setObjectName("label");

  edit_ = new QLineEdit;

  edit_->setObjectName("edit");

  layout->addWidget(label_);
  layout->addWidget(edit_);

  connect(edit_, SIGNAL(returnPressed()), this, SLOT(valueChangedSlot()));
}

void
CQ6502RegEdit::
setFont(const QFont &font)
{
  QWidget::setFont(font);

  label_->setFont(font);
  edit_ ->setFont(font);

  QFontMetrics fm(font);

  label_->setFixedWidth(fm.horizontalAdvance("XXX") + 4);
  edit_ ->setFixedWidth(fm.horizontalAdvance("0000") + 16);
}

void
CQ6502RegEdit::
setValue(uint value)
{
  int len = 2;

  if (reg_ == C6502::Reg::PC) len = 4;

  edit_->setText(CStrUtil::toHexString(value, len).c_str());
}

void
CQ6502RegEdit::
valueChangedSlot()
{
  uint value;

  if (! CStrUtil::decodeHexString(edit_->text().toStdString(), &value))
    return;

  switch (reg_) {
    case C6502::Reg::A : c6502_->setA (value); break;
    case C6502::Reg::X : c6502_->setX (value); break;
    case C6502::Reg::Y : c6502_->setY (value); break;
    case C6502::Reg::SR: c6502_->setSR(value); break;
    case C6502::Reg::SP: c6502_->setSP(value); break;
    case C6502::Reg::PC: c6502_->setPC(value); break;
    default            : assert(false);
  }

  //c6502_->callRegChanged(reg_);
}
