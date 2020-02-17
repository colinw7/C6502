#include <CQ6502Memory.h>
#include <CQ6502Dbg.h>
#include <CQUtil.h>
#include <CStrUtil.h>

#include <QCheckBox>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QMenu>
#include <QContextMenuEvent>
#include <QPainter>
#include <cassert>

CQ6502MemArea::
CQ6502MemArea(CQ6502Dbg *dbg) :
 CQMemArea(nullptr), dbg_(dbg)
{
}

uint
CQ6502MemArea::
getCurrentAddress() const
{
  auto *cpu = dbg()->getCPU();

  return cpu->PC();
}

void
CQ6502MemArea::
setCurrentAddress(uint addr)
{
  auto *cpu = dbg()->getCPU();

  cpu->setPC(addr);

  //cpu->callRegChanged(C6502::Reg::PC);
}

bool
CQ6502MemArea::
isReadOnlyAddr(uint addr, uint len) const
{
  auto *cpu = dbg()->getCPU();

  return cpu->isReadOnly(addr, len);
}

bool
CQ6502MemArea::
isScreenAddr(uint addr, uint len) const
{
  auto *cpu = dbg()->getCPU();

  return cpu->isScreen(addr, len);
}

uchar
CQ6502MemArea::
getByte(uint addr) const
{
  auto *cpu = dbg()->getCPU();

  return cpu->getByte(addr);
}

//------

CQMemArea::
CQMemArea(QWidget *parent) :
 QFrame(parent)
{
  setObjectName("memArea");

  auto layout = CQUtil::makeLayout<QVBoxLayout>(this, 2, 2);

  //------

  auto toolbar_      = CQUtil::makeWidget<QFrame>("toolbar");
  auto toolBarLayout = CQUtil::makeLayout<QHBoxLayout>(toolbar_, 2, 2);

  toolbar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  scrollCheck_ = CQUtil::makeLabelWidget<QCheckBox>("Auto Scroll", "scrollCheck");

  toolBarLayout->addWidget(scrollCheck_);
  toolBarLayout->addStretch();

  layout->addWidget(toolbar_);

  //------

  auto textArea       = CQUtil::makeWidget<QFrame>("textArea");
  auto textAreaLayout = CQUtil::makeLayout<QHBoxLayout>(textArea, 0, 0);

  text_ = new CQMemAreaText(this);

  //---

  vbar_ = CQUtil::makeWidget<QScrollBar>("vbar");

  connect(vbar_, SIGNAL(valueChanged(int)), text_, SLOT(sliderSlot(int)));

  //---

  textAreaLayout->addWidget(text_);
  textAreaLayout->addStretch();
  textAreaLayout->addWidget(vbar_);

  //------

  layout->addWidget(textArea);

  //---

  updateLayout();
}

void
CQMemArea::
setFont(const QFont &font)
{
  text_->setFont(font);
}

void
CQMemArea::
updateLayout()
{
  vbar_->setPageStep  (numMemoryLines());
  vbar_->setSingleStep(1);
  vbar_->setRange     (0, text_->maxLines() - vbar_->pageStep());
}

void
CQMemArea::
updateText(ushort pc)
{
  if (scrollCheck_->isChecked()) {
    int mem1 = vbar()->value();
    int mem2 = mem1 + vbar()->pageStep();
    int mem  = pc/memLineWidth();

    if (mem < mem1 || mem > mem2)
      vbar()->setValue(mem);
  }

  text()->update();
}

void
CQMemArea::
setMemoryText()
{
  ushort numLines = this->numLines();

  std::string str;

  uint pos1 = 0;

  text()->initLines();

  for (ushort i = 0; i < numLines; ++i) {
    setMemoryLine(pos1);

    pos1 += memLineWidth();
  }
}

void
CQMemArea::
setMemoryLine(uint pos)
{
  auto getByteChar = [](uchar c) {
    std::string str;

    if (c >= 0x20 && c < 0x7f)
      str += c;
    else
      str += '.';

    return str;
  };

  //---

  std::string pcStr = CStrUtil::toHexString(pos, 4);

  //-----

  std::string memStr;

  for (ushort j = 0; j < memLineWidth(); ++j) {
    if (j > 0) memStr += " ";

    memStr += CStrUtil::toHexString(getByte(pos + j), 2);
  }

  std::string textStr;

  for (ushort j = 0; j < memLineWidth(); ++j) {
    uchar c = getByte(pos + j);

    textStr += getByteChar(c);
  }

  text()->setLine(pos, pcStr, memStr, textStr);
}

//------

CQMemAreaText::
CQMemAreaText(CQMemArea *area) :
 QFrame(nullptr), area_(area)
{
  setObjectName("mem");

  initLines();
}

int
CQMemAreaText::
maxLines() const
{
  int lw = area_->memLineWidth();

  return area_->memorySize()/lw;
}

void
CQMemAreaText::
setFont(const QFont &font)
{
  QWidget::setFont(font);

  updateSize();
}

void
CQMemAreaText::
updateSize()
{
  if (resizable_) {
    QFontMetrics fm(font());

    int charHeight = fm.height();

    int nl = std::min(std::max(height()/charHeight, minLines()), maxLines());

    if (nl != area_->numMemoryLines()) {
      area_->setNumMemoryLines(nl);

      area_->updateLayout();
    }
  }
  else {
    QSize s = sizeHint();

    setFixedWidth (s.width ());
    setFixedHeight(s.height());
  }
}

void
CQMemAreaText::
initLines()
{
  lines_.resize(maxLines());
}

void
CQMemAreaText::
setLine(uint pc, const std::string &pcStr, const std::string &memStr, const std::string &textStr)
{
  int lw = area_->memLineWidth();

  uint lineNum = pc/lw;

  assert(lineNum < lines_.size());

  lines_[lineNum] = CQMemAreaLine(pc, lw, pcStr, memStr, textStr);
}

void
CQMemAreaText::
resizeEvent(QResizeEvent *)
{
  if (resizable_)
    updateSize();
}

void
CQMemAreaText::
contextMenuEvent(QContextMenuEvent *event)
{
  QMenu *menu = CQUtil::makeWidget<QMenu>("menu");

  QAction *action = menu->addAction("Dump");

  connect(action, SIGNAL(triggered()), this, SLOT(dumpSlot()));

  menu->exec(event->globalPos());

  delete menu;
}

void
CQMemAreaText::
paintEvent(QPaintEvent *)
{
  uint pc = area_->getCurrentAddress();

  QPainter p(this);

  if (isEnabled())
    p.fillRect(rect(), Qt::white);
  else
    p.fillRect(rect(), palette().window().color());

  QFontMetrics fm(font());

  charHeight_ = fm.height();
  charWidth_  = fm.width(" ");

  int charAscent = fm.ascent();

  int lw = area_->memLineWidth();

  int w1 =          4*charWidth_; // address (4 digits)
  int w2 =            charWidth_; // spacer (1 char)
  int w3 = (3*lw - 1)*charWidth_; // data (2*lw digits + (lw - 1) spaces)
  int w4 =            charWidth_; // spacer (1 char)

  int y  = -yOffset_*charHeight_;
  int ya = y + charAscent;

  int ymin = -charHeight_;
  int ymax = height() + charHeight_;

  if (! isEnabled())
    p.setPen(palette().color(QPalette::Disabled, QPalette::WindowText));

  for (const auto &line : lines_) {
    if (! line.isValid())
      continue;

    if (y >= ymin && y <= ymax) {
      int x = dx_;

      uint pc1 = line.pc();
      uint pc2 = pc1 + lw;

      if (isEnabled()) {
        QColor c;

        if (getMemoryColor(pc1, lw, c))
          p.fillRect(QRect(x + w1 + w2, y, w3, charHeight_), c);
      }

      if (isEnabled())
        p.setPen(area_->addrColor());

      p.drawText(x, ya, line.pcStr().c_str());

      x += w1 + w2;

      if (isEnabled())
        p.setPen(area_->memDataColor());

      if (pc >= pc1 && pc < pc2) {
        int i1 = 3*(pc - pc1);
        int i2 = i1 + 2;

        std::string lhs = line.memStr().substr(0, i1);
        std::string mid = line.memStr().substr(i1, 2);
        std::string rhs = line.memStr().substr(i2);

        int w1 = fm.width(lhs.c_str());
        int w2 = fm.width(mid.c_str());

        p.drawText(x          , ya, lhs.c_str());
        p.drawText(x + w1 + w2, ya, rhs.c_str());

        if (isEnabled())
          p.setPen(area_->currentColor());

        p.drawText(x + w1, ya, mid.c_str());
      }
      else {
        p.drawText(x, ya, line.memStr().c_str());
      }

      x += w3 + w4;

      if (isEnabled())
        p.setPen(area_->memCharsColor());

      p.drawText(x, ya, line.textStr().c_str());
    }

    y  += charHeight_;
    ya += charHeight_;
  }
}

bool
CQMemAreaText::
getMemoryColor(ushort addr, ushort len, QColor &c) const
{
  if (area_->getMemoryColor(addr, len, c))
    return true;

  if      (area_->isReadOnlyAddr(addr, len))
    c = area_->readOnlyBgColor();
  else if (area_->isScreenAddr(addr, len))
    c = area_->screenBgColor();
  else
    return false;

  return true;
}

void
CQMemAreaText::
mouseDoubleClickEvent(QMouseEvent *e)
{
  int nl = area_->numLines();
  int lw = area_->memLineWidth();

  int ix = (e->pos().x() - dx_                 )/charWidth_ - 4;
  int iy = (e->pos().y() + yOffset_*charHeight_)/charHeight_;

  if (ix < 0 || ix >= 3*lw) return;
  if (iy < 0 || iy >= nl  ) return;

  uint pc = int(ix/3) + iy*lw;

  area_->setCurrentAddress(pc);
}

void
CQMemAreaText::
sliderSlot(int y)
{
  yOffset_ = y;

  update();
}

void
CQMemAreaText::
dumpSlot()
{
  FILE *fp = fopen("memory.txt", "w");
  if (! fp) return;

  for (const auto &line : lines_) {
    fprintf(fp, "%s %s %s\n", line.pcStr().c_str(), line.memStr().c_str(),
            line.textStr().c_str());
  }

  fclose(fp);
}

QSize
CQMemAreaText::
sizeHint() const
{
  QFontMetrics fm(font());

  int charWidth  = fm.width("X");
  int charHeight = fm.height();

  int lw = area_->memLineWidth();

  int memoryWidth = 0;

  memoryWidth +=    4*charWidth; // "0000"
  memoryWidth += lw*3*charWidth; // " 00" per memory byte
  memoryWidth +=    2*charWidth; // "  "
  memoryWidth +=   lw*charWidth; // <char> per byte

  int w = memoryWidth + 2*dx_;
  int h = charHeight*area_->numMemoryLines();

  return QSize(w, h);
}
