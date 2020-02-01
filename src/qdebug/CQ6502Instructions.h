#ifndef CQ6502Instructions_H
#define CQ6502Instructions_H

#include <QFrame>
#include <set>

class CQ6502Dbg;
class QScrollBar;
class QCheckBox;

class CQ6502InstLine {
 public:
  CQ6502InstLine(uint pc=0, const std::string &pcStr="", const std::string &codeStr="",
                const std::string textStr="") :
   pc_(pc), pcStr_(pcStr), codeStr_(codeStr), textStr_(textStr) {
  }

  uint pc() const { return pc_; }

  uint num() const { return pc_ / 8; }

  const std::string &pcStr  () const { return pcStr_  ; }
  const std::string &codeStr() const { return codeStr_; }
  const std::string &textStr() const { return textStr_; }

 private:
  uint        pc_ { 0 };
  std::string pcStr_;
  std::string codeStr_;
  std::string textStr_;
};

//------

class CQ6502Inst : public QFrame {
  Q_OBJECT

  Q_PROPERTY(bool resizable READ isResizable WRITE setResizable)

 public:
  CQ6502Inst(CQ6502Dbg *dbg);

  int minLines() const { return 8; }
  int maxLines() const { return 65536; }

  // get set resizable
  bool isResizable() const { return resizable_; }
  void setResizable(bool b) { resizable_ = b; }

  void setVBar(QScrollBar *vbar) { vbar_ = vbar; }

  void setFont(const QFont &font);

  void clear();

  void setLine(uint pc, const std::string &pcStr, const std::string &codeStr,
               const std::string &textStr);

  uint getNumLines() const { return lineNum_; }

  bool getLineForPC(uint pc, uint &lineNum) const;
  uint getPCForLine(uint lineNum);

  // breakpoints
  void clearBreakpoints() { breakpoints_.clear(); }
  void addBreakPoint(uint pc) { breakpoints_.insert(pc); }

  void mouseDoubleClickEvent(QMouseEvent *e) override;

  void contextMenuEvent(QContextMenuEvent *event) override;

  void reload();

  QSize sizeHint() const override;

 public slots:
  void sliderSlot(int y);

  void dumpSlot();

  void reloadSlot();

 private:
  void updateSize();

  void resizeEvent(QResizeEvent *) override;

  void paintEvent(QPaintEvent *) override;

 private:
  typedef std::vector<CQ6502InstLine> LineList;
  typedef std::map<uint,uint>         PCLineMap;
  typedef std::set<uint>              BreakpointList;

  CQ6502Dbg*     dbg_        { nullptr };
  bool           resizable_  { true };
  QScrollBar*    vbar_       { nullptr };
  LineList       lines_;
  int            yOffset_    { 0 };
  int            charHeight_ { 8 };
  int            lineNum_    { 0 };
  PCLineMap      pcLineMap_;
  PCLineMap      linePcMap_;
  BreakpointList breakpoints_;
};

//------

class QScrollBar;

class CQ6502InstArea : public QFrame {
  Q_OBJECT

 public:
  CQ6502InstArea(CQ6502Dbg *dbg);

  CQ6502Inst *text() const { return text_; }

  QScrollBar *vbar() const { return vbar_; }

  void updateLayout();

  void updateText(ushort pc);

 private:
  CQ6502Dbg*  dbg_         { nullptr };
  QCheckBox*  scrollCheck_ { nullptr };
  CQ6502Inst* text_        { nullptr };
  QScrollBar* vbar_        { nullptr };
};

#endif
