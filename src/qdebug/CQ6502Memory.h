#ifndef CQ6502Memory_H
#define CQ6502Memory_H

#include <QFrame>

class CQ6502Dbg;

class CQ6502MemLine {
 public:
  CQ6502MemLine() { }

  CQ6502MemLine(ushort pc, int w, const std::string &pcStr, const std::string &memStr,
                const std::string textStr) :
   pc_(pc), w_(w), pcStr_(pcStr), memStr_(memStr), textStr_(textStr) {
  }

  ushort pc() const { return pc_; }

  uint num() const { return pc_/w_; }

  const std::string &pcStr  () const { return pcStr_  ; }
  const std::string &memStr () const { return memStr_ ; }
  const std::string &textStr() const { return textStr_; }

 private:
  ushort      pc_ { 0 };
  int         w_  { 8 };
  std::string pcStr_;
  std::string memStr_;
  std::string textStr_;
};

//------

class CQ6502Mem : public QFrame {
  Q_OBJECT

  Q_PROPERTY(bool resizable READ isResizable WRITE setResizable)

 public:
  CQ6502Mem(CQ6502Dbg *dbg);

  int minLines() const { return 8; }
  int maxLines() const;

  // get set resizable
  bool isResizable() const { return resizable_; }
  void setResizable(bool b) { resizable_ = b; }

  void setFont(const QFont &font);

  void initLines();

  void setLine(uint i, const std::string &pcStr, const std::string &memStr,
               const std::string &textStr);

  void resizeEvent(QResizeEvent *) override;

  void contextMenuEvent(QContextMenuEvent *event) override;

  QSize sizeHint() const override;

 public slots:
  void sliderSlot(int y);

  void dumpSlot();

 private:
  void updateSize();

  void paintEvent(QPaintEvent *) override;

  void mouseDoubleClickEvent(QMouseEvent *e) override;

 private:
  typedef std::vector<CQ6502MemLine> LineList;

  CQ6502Dbg* dbg_        { nullptr };
  bool       resizable_  { true };
  LineList   lines_;
  int        yOffset_    { 0 };
  int        charWidth_  { 8 };
  int        charHeight_ { 12 };
  int        dx_         { 2 };
};

//------

class QScrollBar;

class CQ6502MemArea : public QFrame {
  Q_OBJECT

 public:
  CQ6502MemArea(CQ6502Dbg *dbg);

  CQ6502Mem *text() const { return text_; }

  QScrollBar *vbar() const { return vbar_; }

  void updateLayout();

 private:
  CQ6502Dbg*  dbg_  { nullptr };
  CQ6502Mem*  text_ { nullptr };
  QScrollBar* vbar_ { nullptr };
};

#endif
