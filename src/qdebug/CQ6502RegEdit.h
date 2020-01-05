#ifndef CQ6502RegEdit_H
#define CQ6502RegEdit_H

#include <QFrame>
#include <C6502.h>

class QLabel;
class QLineEdit;

class CQ6502RegEdit : public QFrame {
  Q_OBJECT

 public:
  CQ6502RegEdit(C6502 *c6502, C6502::Reg reg);

  void setFont(const QFont &font);

  void setValue(uint value);
  uint getValue() const;

 private slots:
  void valueChangedSlot();

 private:
  C6502*     c6502_   { nullptr };
  C6502::Reg reg_;
  QLabel*    label_ { nullptr };
  QLineEdit* edit_  { nullptr };
};

#endif
