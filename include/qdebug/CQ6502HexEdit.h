#ifndef CQ6502HexEdit_H
#define CQ6502HexEdit_H

#include <QLineEdit>

class CQ6502HexEdit : public QLineEdit {
  Q_OBJECT

  Q_PROPERTY(unsigned int value READ value WRITE setValue)

 public:
  using uint = unsigned int;

 public:
  CQ6502HexEdit(uint size);

  unsigned int value() const;
  void setValue(unsigned int value);

  QSize sizeHint() const override;

 signals:
  void valueChanged(unsigned int);

 private slots:
  void valueChangedSlot();

 private:
  uint size_ { 0 };
};

#endif
