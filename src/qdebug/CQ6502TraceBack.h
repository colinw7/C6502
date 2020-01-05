#ifndef CQ6502TraceBack_H
#define CQ6502TraceBack_H

#include <QTextEdit>

class C6502;

class CQ6502TraceBack : public QTextEdit {
 public:
  CQ6502TraceBack(C6502 *c6502);

  void setFixedFont(const QFont &font);

  void update();

 private:
  C6502 *c6502_ { nullptr };
};

#endif
