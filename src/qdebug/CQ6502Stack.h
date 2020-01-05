#ifndef CQ6502Stack_H
#define CQ6502Stack_H

#include <QTextEdit>

class C6502;

class CQ6502Stack : public QTextEdit {
 public:
  CQ6502Stack(C6502 *c6502);

  void setFixedFont(const QFont &font);

  void update();

 private:
  C6502 *c6502_ { nullptr };
};

#endif
