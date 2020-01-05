#ifndef CQ6502Memory_H
#define CQ6502Memory_H

#include <QFrame>

class CQ6502Dbg;

class CQ6502MemLine {
 public:
  CQ6502MemLine(ushort pc=0, const std::string &pcStr="", const std::string &memStr="",
               const std::string textStr="") :
   pc_(pc), pcStr_(pcStr), memStr_(memStr), textStr_(textStr) {
  }

  ushort pc() const { return pc_; }

  uint num() const { return pc_ / 8; }

  const std::string &pcStr  () const { return pcStr_  ; }
  const std::string &memStr () const { return memStr_ ; }
  const std::string &textStr() const { return textStr_; }

 private:
  ushort      pc_ { 0 };
  std::string pcStr_;
  std::string memStr_;
  std::string textStr_;
};

//------

class CQ6502Mem : public QFrame {
  Q_OBJECT

 public:
  CQ6502Mem(CQ6502Dbg *dbg);

  void setFont(const QFont &font);

  void setLine(uint i, const std::string &pcStr, const std::string &memStr,
               const std::string &textStr);

  void contextMenuEvent(QContextMenuEvent *event);

 public slots:
  void sliderSlot(int y);

  void dumpSlot();

 private:
  void paintEvent(QPaintEvent *);

  void mouseDoubleClickEvent(QMouseEvent *e);

 private:
  typedef std::vector<CQ6502MemLine> LineList;

  CQ6502Dbg* dbg_        { nullptr };
  LineList  lines_;
  int       yOffset_    { 0 };
  int       charWidth_  { 8 };
  int       charHeight_ { 12 };
  int       dx_         { 2 };
};

#endif
