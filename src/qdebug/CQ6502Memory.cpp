#include <CQ6502Memory.h>
#include <CQ6502Dbg.h>
#include <QMenu>
#include <QContextMenuEvent>
#include <QPainter>
#include <cassert>

CQ6502Mem::
CQ6502Mem(CQ6502Dbg *dbg) :
 QFrame(nullptr), dbg_(dbg)
{
  setObjectName("mem");

  lines_.resize(8192);
}

void
CQ6502Mem::
setFont(const QFont &font)
{
  QWidget::setFont(font);

  QFontMetrics fm(font);

  int memoryWidth = fm.width("0000  00 00 00 00 00 00 00 00  XXXXXXXX");
  int charHeight  = fm.height();

  setFixedWidth (memoryWidth + 32);
  setFixedHeight(charHeight*dbg_->getNumMemoryLines());
}

void
CQ6502Mem::
setLine(uint pc, const std::string &pcStr, const std::string &memStr, const std::string &textStr)
{
  uint lineNum = pc / 8;

  assert(lineNum < lines_.size());

  lines_[lineNum] = CQ6502MemLine(pc, pcStr, memStr, textStr);
}

void
CQ6502Mem::
contextMenuEvent(QContextMenuEvent *event)
{
  QMenu *menu = new QMenu;

  QAction *action = menu->addAction("Dump");

  connect(action, SIGNAL(triggered()), this, SLOT(dumpSlot()));

  menu->exec(event->globalPos());

  delete menu;
}

void
CQ6502Mem::
paintEvent(QPaintEvent *)
{
  C6502 *c6502 = dbg_->get6502();

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

  int w1 =  4*charWidth_; // address (4 digits)
  int w2 =    charWidth_; // spacer (1 char)
  int w3 = 23*charWidth_; // data (16 digits + 7 spaces)
  int w4 =    charWidth_; // spacer (1 char)

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
      uint pc2 = pc1 + 8;

#if 0
      if (isEnabled()) {
        if      (c6502->isReadOnly(pc1, 8))
          p.fillRect(QRect(x + w1 + w2, y, w3, charHeight_), dbg_->readOnlyBgColor());
        else if (c6502->isScreen(pc1, 8))
          p.fillRect(QRect(x + w1 + w2, y, w3, charHeight_), dbg_->screenBgColor());
      }
#endif

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

  uint pc = int((ix - 4)/3) + iy*8;

  C6502 *c6502 = dbg_->get6502();

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
