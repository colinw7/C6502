#include <CQ6502Instructions.h>
#include <CQ6502Dbg.h>

#include <CStrUtil.h>
#include <QMenu>
#include <QScrollBar>
#include <QContextMenuEvent>
#include <QPainter>
#include <cassert>

CQ6502Inst::
CQ6502Inst(CQ6502Dbg *dbg) :
 QFrame(nullptr), dbg_(dbg)
{
  setObjectName("inst");

  lines_.resize(65536);
}

void
CQ6502Inst::
setFont(const QFont &font)
{
  QWidget::setFont(font);

  QFontMetrics fm(font);

  int instructionsWidth = fm.width("0000  123456789012  AAAAAAAAAAAAAAAAAA");
  int charHeight        = fm.height();

  setFixedWidth (instructionsWidth + 32);
  setFixedHeight(charHeight*dbg_->getNumMemoryLines());
}

void
CQ6502Inst::
clear()
{
  lineNum_ = 0;

  pcLineMap_.clear();
  linePcMap_.clear();
}

void
CQ6502Inst::
setLine(uint pc, const std::string &pcStr, const std::string &codeStr, const std::string &textStr)
{
  assert(lineNum_ < int(lines_.size()));

  lines_[lineNum_] = CQ6502InstLine(pc, pcStr, codeStr, textStr);

  pcLineMap_[pc      ] = lineNum_;
  linePcMap_[lineNum_] = pc;

  ++lineNum_;
}

bool
CQ6502Inst::
getLineForPC(uint pc, uint &lineNum) const
{
  auto p = pcLineMap_.find(pc);

  if (p == pcLineMap_.end())
    return false;

  lineNum = (*p).second;

  return true;
}

uint
CQ6502Inst::
getPCForLine(uint lineNum)
{
  return linePcMap_[lineNum];
}

void
CQ6502Inst::
contextMenuEvent(QContextMenuEvent *event)
{
  QMenu *menu = new QMenu;

  QAction *dumpAction = menu->addAction("Dump");

  connect(dumpAction, SIGNAL(triggered()), this, SLOT(dumpSlot()));

  QAction *reloadAction = menu->addAction("Reload");

  connect(reloadAction, SIGNAL(triggered()), this, SLOT(reloadSlot()));

  menu->exec(event->globalPos());

  delete menu;
}

void
CQ6502Inst::
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

  int charWidth  = fm.width(" ");
  int charAscent = fm.ascent();

  int w1 =  4*charWidth;
  int w2 = 12*charWidth;

  int y = -yOffset_*charHeight_ + charAscent;

  int ymin = -charHeight_;
  int ymax = height() + charHeight_;

  if (! isEnabled())
    p.setPen(palette().color(QPalette::Disabled, QPalette::WindowText));

  for (const auto &line : lines_) {
    if (y >= ymin && y <= ymax) {
      int x = 2;

      if (line.pc() == pc) {
        if (isEnabled())
          p.setPen(dbg_->currentColor());

        p.drawText(x, y, ">");
      }

      x += charWidth;

      if (isEnabled())
        p.setPen(dbg_->addrColor());

      p.drawText(x, y, line.pcStr().c_str());

      x += w1 + charWidth;

      if (isEnabled())
        p.setPen(dbg_->memDataColor());

      p.drawText(x, y, line.codeStr().c_str());

      x += w2 + charWidth;

      if (isEnabled())
        p.setPen(dbg_->memCharsColor());

      p.drawText(x, y, line.textStr().c_str());
    }

    y += charHeight_;
  }
}

void
CQ6502Inst::
mouseDoubleClickEvent(QMouseEvent *e)
{
  int iy = (e->pos().y() + yOffset_*charHeight_)/charHeight_;

  C6502 *c6502 = dbg_->get6502();

  c6502->setPC(getPCForLine(iy));

  //c6502->callRegChanged(C6502::Reg::PC);
}

void
CQ6502Inst::
sliderSlot(int y)
{
  yOffset_ = y;

  update();
}

void
CQ6502Inst::
dumpSlot()
{
  FILE *fp = fopen("inst.txt", "w");
  if (! fp) return;

  for (const auto &line : lines_) {
    fprintf(fp, "%s %s %s\n", line.pcStr().c_str(), line.codeStr().c_str(),
            line.textStr().c_str());
  }

  fclose(fp);
}

void
CQ6502Inst::
reloadSlot()
{
  reload();

  C6502 *c6502 = dbg_->get6502();

  uint lineNum;

  if (getLineForPC(c6502->PC(), lineNum))
    vbar_->setValue(lineNum);

}

void
CQ6502Inst::
reload()
{
  C6502 *c6502 = dbg_->get6502();

  uint pos1 = 0;
  uint pos2 = 65536;

  clear();

  uint pc      = pos1;
  bool pcFound = false;

  while (pc < pos2) {
    // resync to PC (should be legal instruction here)
    if (! pcFound && pc >= c6502->PC()) {
      pc      = c6502->PC();
      pcFound = true;
    }

    //-----

    std::string pcStr = CStrUtil::toHexString(pc, 4);

    //-----

    // instruction at PC
    int         alen;
    std::string astr;

    uint pc1;

    if (c6502->disassembleAddr(pc, astr, alen))
      pc1 = pc + alen;
    else
      pc1 = pc + 1;

    //-----

    std::string codeStr;

    ushort len1 = 0;

    for (uint i = pc; i < pc1; ++i) {
      if (i > pc) codeStr += " ";

      codeStr += CStrUtil::toHexString(c6502->getByte(i), 2);

      len1 += 3;
    }

    for ( ; len1 < 12; ++len1)
      codeStr += " ";

    //-----

    std::string textStr = "; ";

    textStr += astr;

    setLine(pc, pcStr, codeStr, textStr);

    //------

    pc = pc1;
  }

  uint numLines = getNumLines();

  vbar_->setRange(0, numLines - vbar_->pageStep());

  vbar_->setValue(0);

  update();
}
