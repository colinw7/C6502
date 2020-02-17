#ifndef CQ6502Memory_H
#define CQ6502Memory_H

#include <QFrame>

class CQMemArea;

class QScrollBar;
class QCheckBox;

class CQMemAreaLine {
 public:
  CQMemAreaLine() { }

  CQMemAreaLine(ushort pc, int w, const std::string &pcStr, const std::string &memStr,
                const std::string textStr) :
   pc_(pc), w_(w), pcStr_(pcStr), memStr_(memStr), textStr_(textStr) {
  }

  ushort pc() const { return pc_; }

  uint num() const { return pc_/w_; }

  bool isValid() const { return ! pcStr_.empty(); }

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

class CQMemAreaText : public QFrame {
  Q_OBJECT

  Q_PROPERTY(bool resizable READ isResizable WRITE setResizable)

 public:
  CQMemAreaText(CQMemArea *area);

  int minLines() const { return 8; }
  int maxLines() const;

  // get set resizable
  bool isResizable() const { return resizable_; }
  void setResizable(bool b) { resizable_ = b; }

  void setFont(const QFont &font);

  //---

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

  bool getMemoryColor(ushort addr, ushort len, QColor &c) const;

  void mouseDoubleClickEvent(QMouseEvent *e) override;

 private:
  typedef std::vector<CQMemAreaLine> LineList;

  CQMemArea* area_       { nullptr };
  bool       resizable_  { true };
  LineList   lines_;
  int        yOffset_    { 0 };
  int        charWidth_  { 8 };
  int        charHeight_ { 12 };
  int        dx_         { 2 };
};

//------

class CQMemArea : public QFrame {
  Q_OBJECT

  Q_PROPERTY(int    numMemoryLines  READ numMemoryLines  WRITE setNumMemoryLines)
  Q_PROPERTY(int    memLineWidth    READ memLineWidth    WRITE setMemLineWidth)
  Q_PROPERTY(QColor addrColor       READ addrColor       WRITE setAddrColor)
  Q_PROPERTY(QColor memDataColor    READ memDataColor    WRITE setMemDataColor)
  Q_PROPERTY(QColor memCharsColor   READ memCharsColor   WRITE setMemCharsColor)
  Q_PROPERTY(QColor currentColor    READ currentColor    WRITE setCurrentColor)
  Q_PROPERTY(QColor readOnlyBgColor READ readOnlyBgColor WRITE setReadOnlyBgColor)

 public:
  CQMemArea(QWidget *parent=nullptr);

  int numMemoryLines() const { return numMemoryLines_; }
  void setNumMemoryLines(int i) { numMemoryLines_ = i; }

  int memLineWidth() const { return memLineWidth_; }
  void setMemLineWidth(int i) { memLineWidth_ = i; }

  //---

  const QColor &addrColor() const { return addrColor_; }
  void setAddrColor(const QColor &c) { addrColor_ = c; }

  const QColor &memDataColor() const { return memDataColor_; }
  void setMemDataColor(const QColor &c) { memDataColor_ = c; }

  const QColor &memCharsColor() const { return memCharsColor_; }
  void setMemCharsColor(const QColor &c) { memCharsColor_ = c; }

  const QColor &currentColor() const { return currentColor_; }
  void setCurrentColor(const QColor &c) { currentColor_ = c; }

  const QColor &readOnlyBgColor() const { return readOnlyBgColor_; }
  void setReadOnlyBgColor(const QColor &c) { readOnlyBgColor_ = c; }

  const QColor &screenBgColor() const { return screenBgColor_; }
  void setScreenBgColor(const QColor &c) { screenBgColor_ = c; }

  //---

  struct MemoryColor {
    ushort addr { 0 };
    ushort len  { 0 };
    QColor c;

    MemoryColor(ushort addr1, ushort len1, const QColor &c1) :
     addr(addr1), len(len1), c(c1) {
    }

    bool inside(ushort addr1, ushort len1) const {
      if (len1 == 0) return false;

      ushort addr2 = addr1 + len1 - 1;

      if (addr1 >= addr + len) return false;
      if (addr2 <  addr      ) return false;

      return true;
    }
  };

  void setMemoryColor(ushort addr, ushort len, const QColor &c) {
    memoryColors_.push_back(MemoryColor(addr, len, c));
  }

  bool getMemoryColor(ushort addr, ushort len, QColor &c) const {
    for (const auto &memoryColor : memoryColors_) {
      if (memoryColor.inside(addr, len)) {
        c = memoryColor.c;
        return true;
      }
    }

    return false;
  }

  //---

  CQMemAreaText *text() const { return text_; }
  QScrollBar    *vbar() const { return vbar_; }

  //---

  void setFont(const QFont &font);

  //---

  void updateLayout();

  void updateText(ushort pc);

  void setMemoryText();
  void setMemoryLine(uint pos);

  //---

  virtual uint memorySize() const { return 65536; }

  virtual uint getCurrentAddress() const = 0;
  virtual void setCurrentAddress(uint addr) = 0;

  virtual bool isReadOnlyAddr(uint /*addr*/, uint /*len*/) const { return false; }
  virtual bool isScreenAddr(uint /*addr*/, uint /*len*/) const { return false; }

  virtual uchar getByte(uint addr) const = 0;

  //---

  int numLines() const {
    int size  = memorySize();
    int width = memLineWidth();

    return (size + width - 1)/width;
  }

 private:
  using MemoryColors = std::vector<MemoryColor>;

  int numMemoryLines_ { 32 }; // number of lines per page (fixed or resiable)
  int memLineWidth_   { 16 }; // number of bytes per line

  QColor addrColor_       {   0,   0, 220 }; // line address text color
  QColor memDataColor_    {   0,   0,   0 }; // memory value text color
  QColor memCharsColor_   {   0, 220,   0 }; // memory characters text color
  QColor currentColor_    { 220,   0,   0 }; // current address text color
  QColor readOnlyBgColor_ { 216, 180, 180 }; // read only memory background color
  QColor screenBgColor_   { 180, 216, 180 }; // screen memory background color

  MemoryColors memoryColors_;

  QCheckBox*     scrollCheck_ { nullptr }; // auto scroll check
  CQMemAreaText* text_        { nullptr }; // memory area text
  QScrollBar*    vbar_        { nullptr }; // vertical scroll bar
};

//------

class CQ6502Dbg;

class CQ6502MemArea : public CQMemArea {
  Q_OBJECT

 public:
  CQ6502MemArea(CQ6502Dbg *dbg);

  CQ6502Dbg *dbg() const { return dbg_; }

  //---

  uint getCurrentAddress() const override;
  void setCurrentAddress(uint addr) override;

  bool isReadOnlyAddr(uint addr, uint len) const override;
  bool isScreenAddr(uint addr, uint len) const override;

  uchar getByte(uint addr) const override;

  //---

 private:
  CQ6502Dbg* dbg_ { nullptr };
};

#endif
