#include <CQ6502Memory.h>
#include <CQ6502Dbg.h>
#include <CQUtil.h>

#include <QCheckBox>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QMenu>
#include <QContextMenuEvent>
#include <QPainter>
#include <cassert>

CQ6502MemArea::
CQ6502MemArea(CQ6502Dbg *dbg) :
 QFrame(nullptr), dbg_(dbg)
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

  text_ = new CQ6502Mem(dbg_);

  text_->setFont(dbg_->getFixedFont());

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
CQ6502MemArea::
updateLayout()
{
  vbar_->setPageStep  (dbg_->getNumMemoryLines());
  vbar_->setSingleStep(1);
  vbar_->setRange     (0, text_->maxLines() - vbar_->pageStep());
}

void
CQ6502MemArea::
updateText(ushort pc)
{
  if (scrollCheck_->isChecked()) {
    int mem1 = vbar()->value();
    int mem2 = mem1 + 20;
    int mem  = pc/dbg_->memLineWidth();

    if (mem < mem1 || mem > mem2)
      vbar()->setValue(mem);
  }

  text()->update();
}

//------

CQ6502Mem::
CQ6502Mem(CQ6502Dbg *dbg) :
 QFrame(nullptr), dbg_(dbg)
{
  setObjectName("mem");

  initLines();
}

int
CQ6502Mem::
maxLines() const
{
  return 65536/dbg_->memLineWidth();
}

void
CQ6502Mem::
setFont(const QFont &font)
{
  QWidget::setFont(font);

  updateSize();
}

void
CQ6502Mem::
updateSize()
{
  if (resizable_) {
    QFontMetrics fm(font());

    int charHeight = fm.height();

    int nl = std::min(std::max(height()/charHeight, minLines()), maxLines());

    if (nl != dbg_->getNumMemoryLines())
      dbg_->setNumMemoryLines(nl);
  }
  else {
    QSize s = sizeHint();

    setFixedWidth (s.width ());
    setFixedHeight(s.height());
  }
}

void
CQ6502Mem::
initLines()
{
  lines_.resize(maxLines());
}

void
CQ6502Mem::
setLine(uint pc, const std::string &pcStr, const std::string &memStr, const std::string &textStr)
{
  uint lineNum = pc/dbg_->memLineWidth();

  assert(lineNum < lines_.size());

  lines_[lineNum] = CQ6502MemLine(pc, dbg_->memLineWidth(), pcStr, memStr, textStr);
}

void
CQ6502Mem::
resizeEvent(QResizeEvent *)
{
  if (resizable_)
    updateSize();
}

void
CQ6502Mem::
contextMenuEvent(QContextMenuEvent *event)
{
  QMenu *menu = CQUtil::makeWidget<QMenu>("menu");

  QAction *action = menu->addAction("Dump");

  connect(action, SIGNAL(triggered()), this, SLOT(dumpSlot()));

  menu->exec(event->globalPos());

  delete menu;
}

void
CQ6502Mem::
paintEvent(QPaintEvent *)
{
  C6502 *c6502 = dbg_->getCPU();

  uint pc = c6502->PC();

  QPainter p(this);

  if (isEnabled())
    p.fillRect(rect(), Qt::white);
  else
    p.fillRect(rect(), palette().window().color());

  QFontMetrics fm(font());

  charHeight_ = fm.height();
  charWidth_  = fm.width(" ");

  int charAscent = fm.ascent();

  int lw = dbg_->memLineWidth();

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
    if (y >= ymin && y <= ymax) {
      int x = dx_;

      uint pc1 = line.pc();
      uint pc2 = pc1 + lw;

      if (isEnabled()) {
        if      (c6502->isReadOnly(pc1, lw))
          p.fillRect(QRect(x + w1 + w2, y, w3, charHeight_), dbg_->readOnlyBgColor());
        else if (c6502->isScreen(pc1, lw))
          p.fillRect(QRect(x + w1 + w2, y, w3, charHeight_), dbg_->screenBgColor());
      }

      if (isEnabled())
        p.setPen(dbg_->addrColor());

      p.drawText(x, ya, line.pcStr().c_str());

      x += w1 + w2;

      if (isEnabled())
        p.setPen(dbg_->memDataColor());

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
          p.setPen(dbg_->currentColor());

        p.drawText(x + w1, ya, mid.c_str());
      }
      else {
        p.drawText(x, ya, line.memStr().c_str());
      }

      x += w3 + w4;

      if (isEnabled())
        p.setPen(dbg_->memCharsColor());

      p.drawText(x, ya, line.textStr().c_str());
    }

    y  += charHeight_;
    ya += charHeight_;
  }
}

void
CQ6502Mem::
mouseDoubleClickEvent(QMouseEvent *e)
{
  int ix = (e->pos().x() - dx_                 )/charWidth_ ;
  int iy = (e->pos().y() + yOffset_*charHeight_)/charHeight_;

  if (ix < 4 || ix >= 28  ) return;
  if (iy < 0 || iy >= 8192) return;

  uint pc = int((ix - 4)/3) + iy*dbg_->memLineWidth();

  C6502 *c6502 = dbg_->getCPU();

  c6502->setPC(pc);

  //c6502->callRegChanged(C6502::Reg::PC);
}

void
CQ6502Mem::
sliderSlot(int y)
{
  yOffset_ = y;

  update();
}

void
CQ6502Mem::
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
CQ6502Mem::
sizeHint() const
{
  QFontMetrics fm(font());

  int charWidth  = fm.width("X");
  int charHeight = fm.height();

  int lw = dbg_->memLineWidth();

  int memoryWidth = 0;

  memoryWidth +=    4*charWidth; // "0000"
  memoryWidth += lw*3*charWidth; // " 00" per memory byte
  memoryWidth +=    2*charWidth; // "  "
  memoryWidth +=   lw*charWidth; // <char> per byte

  int w = memoryWidth + 2*dx_;
  int h = charHeight*dbg_->getNumMemoryLines();

  return QSize(w, h);
}
