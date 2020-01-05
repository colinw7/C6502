#include <CQ6502Dbg.h>
#include <CQ6502Memory.h>
#include <CQ6502Instructions.h>
#include <CQ6502Stack.h>
#include <CQ6502TraceBack.h>
#include <CQ6502RegEdit.h>
#include <C6502.h>
#include <CStrUtil.h>

#include <QApplication>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QScrollBar>
#include <QMouseEvent>
#include <cassert>

CQ6502Dbg::
CQ6502Dbg(C6502 *c6502) :
 C6502Trace(*c6502), c6502_(c6502)
{
  setObjectName("dbg");

  setWindowTitle("6502 Emulator (Debug)");

  //c6502_->addTrace(this);
}

CQ6502Dbg::
~CQ6502Dbg()
{
}

void
CQ6502Dbg::
init()
{
  addWidgets();

  setMemoryText();

  updateInstructions();

  updateStack();

  updateTraceBack();

  updateBreakpoints();

  regChanged(C6502::Reg::NONE);
}

void
CQ6502Dbg::
setFixedFont(const QFont &font)
{
  fixedFont_ = font;

  memoryText_      ->setFont(getFixedFont());
  instructionsText_->setFont(getFixedFont());
  stackText_       ->setFont(getFixedFont());
  traceBack_       ->setFont(getFixedFont());
  breakpointsText_ ->setFont(getFixedFont());

  aEdit_  ->setFont(getFixedFont());
  xEdit_  ->setFont(getFixedFont());
  yEdit_  ->setFont(getFixedFont());
  srEdit_ ->setFont(getFixedFont());
  spEdit_ ->setFont(getFixedFont());
  pcEdit_ ->setFont(getFixedFont());
}

void
CQ6502Dbg::
setMemoryTrace(bool b)
{
  memoryTrace_ = b;

  if (memoryTrace_ && memoryDirty_) {
    memChangedI(0, 65535);
  }
}

void
CQ6502Dbg::
addWidgets()
{
  fixedFont_ = QFont("Courier", 10);

  QFontMetrics fm(getFixedFont());

  //----

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  QWidget *topFrame    = new QWidget;
  QWidget *bottomFrame = new QWidget;

  topFrame   ->setObjectName("topFrame");
  bottomFrame->setObjectName("bottomFrame");

  QHBoxLayout *topLayout    = new QHBoxLayout(topFrame);
  QVBoxLayout *bottomLayout = new QVBoxLayout(bottomFrame);

  topLayout   ->setMargin(2); topLayout   ->setSpacing(2);
  bottomLayout->setMargin(2); bottomLayout->setSpacing(2);

  layout->addWidget(topFrame);
  layout->addWidget(bottomFrame);

  //----

  QWidget *leftFrame  = new QWidget;
  QWidget *rightFrame = new QWidget;

  leftFrame ->setObjectName("leftFrame");
  rightFrame->setObjectName("rightFrame");

  topLayout->addWidget(leftFrame);
  topLayout->addWidget(rightFrame);

  QVBoxLayout *leftLayout  = new QVBoxLayout(leftFrame );
  QVBoxLayout *rightLayout = new QVBoxLayout(rightFrame);

  leftLayout ->setMargin(2); leftLayout ->setSpacing(2);
  rightLayout->setMargin(2); rightLayout->setSpacing(2);

  //----

  memoryGroup_ = new QGroupBox("Memory");

  memoryGroup_->setObjectName("memoryGroup");
  memoryGroup_->setCheckable(true);

  connect(memoryGroup_, SIGNAL(toggled(bool)), this, SLOT(memoryTraceSlot()));

  QHBoxLayout *memoryLayout = new QHBoxLayout(memoryGroup_);

  memoryText_ = new CQ6502Mem(this);

  memoryText_->setFont(getFixedFont());

  memoryVBar_ = new QScrollBar;

  memoryVBar_->setObjectName("memoryVbar");
  memoryVBar_->setPageStep  (getNumMemoryLines());
  memoryVBar_->setSingleStep(1);
  memoryVBar_->setRange     (0, 8192 - memoryVBar_->pageStep());

  connect(memoryVBar_, SIGNAL(valueChanged(int)), memoryText_, SLOT(sliderSlot(int)));

  memoryLayout->addWidget(memoryText_);
  memoryLayout->addWidget(memoryVBar_);
  memoryLayout->addStretch();

  leftLayout->addWidget(memoryGroup_);

  //--

  instructionsGroup_ = new QGroupBox("Instructions");

  instructionsGroup_->setObjectName("instructionsGroup");
  instructionsGroup_->setCheckable(true);

  connect(instructionsGroup_, SIGNAL(toggled(bool)), this, SLOT(instructionsTraceSlot()));

  QHBoxLayout *instructionsLayout = new QHBoxLayout(instructionsGroup_);

  instructionsText_ = new CQ6502Inst(this);

  instructionsText_->setFont(getFixedFont());

  instructionsVBar_ = new QScrollBar;

  instructionsVBar_->setObjectName("instructionsVbar");
  instructionsVBar_->setPageStep  (getNumMemoryLines());
  instructionsVBar_->setSingleStep(1);
  instructionsVBar_->setRange     (0, 8192 - instructionsVBar_->pageStep());

  connect(instructionsVBar_, SIGNAL(valueChanged(int)),
          instructionsText_, SLOT(sliderSlot(int)));

  instructionsLayout->addWidget(instructionsText_);
  instructionsLayout->addWidget(instructionsVBar_);
  instructionsLayout->addStretch();

  instructionsText_->setVBar(instructionsVBar_);

  leftLayout->addWidget(instructionsGroup_);

  opData_ = new QLineEdit;

  opData_->setObjectName("opData");

  opData_->setReadOnly(true);

  leftLayout->addWidget(opData_);

  //----

  registersGroup_ = new QGroupBox("Registers");

  registersGroup_->setObjectName("registersGroup");
  registersGroup_->setCheckable(true);

  connect(registersGroup_, SIGNAL(toggled(bool)), this, SLOT(registersTraceSlot()));

  registersLayout_ = new QGridLayout(registersGroup_);

  addRegistersWidgets();

  rightLayout->addWidget(registersGroup_);

  //--

  flagsGroup_ = new QGroupBox("Flags");

  flagsGroup_->setObjectName("flagsGroup");
  flagsGroup_->setCheckable(true);

  connect(flagsGroup_, SIGNAL(toggled(bool)), this, SLOT(flagsTraceSlot()));

  flagsLayout_ = new QGridLayout(flagsGroup_);

  flagsLayout_->setSpacing(6);

  addFlagsWidgets();

  rightLayout->addWidget(flagsGroup_);

  //--

  stackGroup_ = new QGroupBox("Stack");

  stackGroup_->setObjectName("stackGroup");
  stackGroup_->setCheckable(true);

  connect(stackGroup_, SIGNAL(toggled(bool)), this, SLOT(stackTraceSlot()));

  QVBoxLayout *stackLayout = new QVBoxLayout(stackGroup_);

  stackText_ = new CQ6502Stack(c6502_);

  stackText_->setFixedFont(getFixedFont());

  stackLayout->addWidget(stackText_);

  rightLayout->addWidget(stackGroup_);

  //--

  traceBackGroup_ = new QGroupBox("Trace Back");

  traceBackGroup_->setObjectName("traceBackGroup");
  traceBackGroup_->setCheckable(true);

  connect(traceBackGroup_, SIGNAL(toggled(bool)), this, SLOT(traceBackTraceSlot()));

  QVBoxLayout *traceBackLayout = new QVBoxLayout(traceBackGroup_);

  traceBack_ = new CQ6502TraceBack(c6502_);

  traceBack_->setFixedFont(getFixedFont());

  traceBackLayout->addWidget(traceBack_);

  rightLayout->addWidget(traceBackGroup_);

  //--

  breakpointsGroup_ = new QGroupBox("Breakpoints");

  breakpointsGroup_->setObjectName("breakpointsGroup");
  breakpointsGroup_->setCheckable(true);

  connect(breakpointsGroup_, SIGNAL(toggled(bool)), this, SLOT(breakpointsTraceSlot()));

  breakpointsLayout_ = new QVBoxLayout(breakpointsGroup_);

  addBreakpointWidgets();

  rightLayout->addWidget(breakpointsGroup_);

  //-----

  QFrame *optionsFrame = new QFrame;

  optionsFrame->setObjectName("optionsFrame");

  QHBoxLayout *optionsLayout = new QHBoxLayout(optionsFrame);

  //--

  traceCheck_ = new QCheckBox("Trace");

  traceCheck_->setObjectName("traceCheck");
  traceCheck_->setChecked(true);

  connect(traceCheck_, SIGNAL(stateChanged(int)), this, SLOT(setTraceSlot()));

  optionsLayout->addWidget(traceCheck_);

  //--

  haltCheck_ = new QCheckBox("Halt");

  haltCheck_->setObjectName("haltCheck");
  haltCheck_->setChecked(false);

  connect(haltCheck_, SIGNAL(stateChanged(int)), this, SLOT(setHaltSlot()));

  optionsLayout->addWidget(haltCheck_);

  //--

  optionsLayout->addStretch(1);

  bottomLayout->addWidget(optionsFrame);

  //---

  buttonsToolbar_ = new QFrame;

  buttonsToolbar_->setObjectName("buttonsToolbar");

  buttonsLayout_ = new QHBoxLayout(buttonsToolbar_);

  buttonsLayout_->addStretch(1);

  addButtonsWidgets();

  bottomLayout->addWidget(buttonsToolbar_);
}

void
CQ6502Dbg::
addFlagsWidgets()
{
  cFlagCheck_ = new QCheckBox("");
  zFlagCheck_ = new QCheckBox("");
  iFlagCheck_ = new QCheckBox("");
  dFlagCheck_ = new QCheckBox("");
  bFlagCheck_ = new QCheckBox("");
  vFlagCheck_ = new QCheckBox("");
  nFlagCheck_ = new QCheckBox("");

  cFlagCheck_->setObjectName("cFlagCheck");
  zFlagCheck_->setObjectName("zFlagCheck");
  iFlagCheck_->setObjectName("iFlagCheck");
  dFlagCheck_->setObjectName("dFlagCheck");
  bFlagCheck_->setObjectName("bFlagCheck");
  vFlagCheck_->setObjectName("vFlagCheck");
  nFlagCheck_->setObjectName("nFlagCheck");

  flagsLayout_->addWidget(new QLabel("C"), 0, 0); flagsLayout_->addWidget(cFlagCheck_, 1, 0);
  flagsLayout_->addWidget(new QLabel("Z"), 0, 1); flagsLayout_->addWidget(zFlagCheck_, 1, 1);
  flagsLayout_->addWidget(new QLabel("I"), 0, 2); flagsLayout_->addWidget(iFlagCheck_, 1, 2);
  flagsLayout_->addWidget(new QLabel("D"), 0, 3); flagsLayout_->addWidget(dFlagCheck_, 1, 3);
  flagsLayout_->addWidget(new QLabel("B"), 0, 4); flagsLayout_->addWidget(bFlagCheck_, 1, 4);
  flagsLayout_->addWidget(new QLabel("V"), 0, 5); flagsLayout_->addWidget(vFlagCheck_, 1, 5);
  flagsLayout_->addWidget(new QLabel("N"), 0, 6); flagsLayout_->addWidget(nFlagCheck_, 1, 6);

  flagsLayout_->setColumnStretch(7, 1);
}

void
CQ6502Dbg::
addRegistersWidgets()
{
  aEdit_   = new CQ6502RegEdit(c6502_, C6502::Reg::A );
  xEdit_   = new CQ6502RegEdit(c6502_, C6502::Reg::X );
  yEdit_   = new CQ6502RegEdit(c6502_, C6502::Reg::Y );
  srEdit_  = new CQ6502RegEdit(c6502_, C6502::Reg::SR);
  spEdit_  = new CQ6502RegEdit(c6502_, C6502::Reg::SP);
  pcEdit_  = new CQ6502RegEdit(c6502_, C6502::Reg::PC);

  registersLayout_->addWidget(aEdit_ , 0, 0);
  registersLayout_->addWidget(xEdit_ , 1, 0);
  registersLayout_->addWidget(yEdit_ , 2, 0);
  registersLayout_->addWidget(srEdit_, 3, 0);
  registersLayout_->addWidget(spEdit_, 4, 0);
  registersLayout_->addWidget(pcEdit_, 5, 0);

  registersLayout_->setColumnStretch(2, 1);
}

void
CQ6502Dbg::
addBreakpointWidgets()
{
  breakpointsText_ = new QTextEdit;

  breakpointsText_->setObjectName("breakpointsText");
  breakpointsText_->setReadOnly(true);

  breakpointsText_->setFont(getFixedFont());

  breakpointsLayout_->addWidget(breakpointsText_);

  QFrame *breakpointEditFrame = new QFrame;

  breakpointEditFrame->setObjectName("breakpointEditFrame");

  breakpointsLayout_->addWidget(breakpointEditFrame);

  QHBoxLayout *breakpointEditLayout = new QHBoxLayout(breakpointEditFrame);
  breakpointEditLayout->setMargin(0); breakpointEditLayout->setSpacing(0);

  breakpointsEdit_ = new QLineEdit;

  breakpointEditLayout->addWidget(new QLabel("Addr"));
  breakpointEditLayout->addWidget(breakpointsEdit_);
  breakpointEditLayout->addStretch(1);

  QFrame *breakpointToolbar = new QFrame;

  breakpointToolbar->setObjectName("breakpointToolbar");

  QHBoxLayout *breakpointToolbarLayout = new QHBoxLayout(breakpointToolbar);
  breakpointToolbarLayout->setMargin(0); breakpointToolbarLayout->setSpacing(0);

  QPushButton *addBreakpointButton    = new QPushButton("Add"   );
  QPushButton *deleteBreakpointButton = new QPushButton("Delete");
  QPushButton *clearBreakpointButton  = new QPushButton("Clear" );

  addBreakpointButton   ->setObjectName("addBreakpointButton");
  deleteBreakpointButton->setObjectName("deleteBreakpointButton");
  clearBreakpointButton ->setObjectName("clearBreakpointButton");

  connect(addBreakpointButton   , SIGNAL(clicked()), this, SLOT(addBreakpointSlot   ()));
  connect(deleteBreakpointButton, SIGNAL(clicked()), this, SLOT(deleteBreakpointSlot()));
  connect(clearBreakpointButton , SIGNAL(clicked()), this, SLOT(clearBreakpointSlot ()));

  breakpointToolbarLayout->addWidget(addBreakpointButton);
  breakpointToolbarLayout->addWidget(deleteBreakpointButton);
  breakpointToolbarLayout->addWidget(clearBreakpointButton);
  breakpointToolbarLayout->addStretch(1);

  breakpointsLayout_->addWidget(breakpointToolbar);
}

void
CQ6502Dbg::
addButtonsWidgets()
{
  runButton_      = addButtonWidget("run"     , "Run");
  nextButton_     = addButtonWidget("next"    , "Next");
  stepButton_     = addButtonWidget("step"    , "Step");
  continueButton_ = addButtonWidget("continue", "Continue");
  stopButton_     = addButtonWidget("stop"    , "Stop");
  restartButton_  = addButtonWidget("restart" , "Restart");
  exitButton_     = addButtonWidget("exit"    , "Exit");

  connect(runButton_     , SIGNAL(clicked()), this, SLOT(runSlot()));
  connect(nextButton_    , SIGNAL(clicked()), this, SLOT(nextSlot()));
  connect(stepButton_    , SIGNAL(clicked()), this, SLOT(stepSlot()));
  connect(continueButton_, SIGNAL(clicked()), this, SLOT(continueSlot()));
  connect(stopButton_    , SIGNAL(clicked()), this, SLOT(stopSlot()));
  connect(restartButton_ , SIGNAL(clicked()), this, SLOT(restartSlot()));
  connect(exitButton_    , SIGNAL(clicked()), this, SLOT(exitSlot()));
}

QPushButton *
CQ6502Dbg::
addButtonWidget(const QString &name, const QString &label)
{
  QPushButton *button = new QPushButton(label);

  button->setObjectName(name);

  buttonsLayout_->addWidget(button);

  return button;
}

void
CQ6502Dbg::
setMemoryText()
{
  uint len = 65536;

  ushort numLines = len / 8;

  if ((len % 8) != 0) ++numLines;

  uint pos = c6502_->PC();

  c6502_->setPC(0);

  std::string str;

  uint pos1 = 0;

  for (ushort i = 0; i < numLines; ++i) {
    setMemoryLine(pos1);

    pos1 += 8;
  }

  c6502_->setPC(pos);
}

void
CQ6502Dbg::
setMemoryLine(uint pos)
{
  std::string pcStr = CStrUtil::toHexString(pos, 4);

  //-----

  std::string memStr;

  for (ushort j = 0; j < 8; ++j) {
    if (j > 0) memStr += " ";

    memStr += CStrUtil::toHexString(c6502_->getByte(pos + j), 2);
  }

  std::string textStr;

  for (ushort j = 0; j < 8; ++j) {
    uchar c = c6502_->getByte(pos + j);

    textStr += getByteChar(c);
  }

  memoryText_->setLine(pos, pcStr, memStr, textStr);
}

std::string
CQ6502Dbg::
getByteChar(uchar c)
{
  std::string str;

  if (c >= 0x20 && c < 0x7f)
    str += c;
  else
    str += '.';

  return str;
}

void
CQ6502Dbg::
updateInstructions()
{
  instructionsText_->reload();
}

void
CQ6502Dbg::
updateStack()
{
  stackText_->update();
}

void
CQ6502Dbg::
updateTraceBack()
{
  traceBack_->update();
}

void
CQ6502Dbg::
updateBreakpoints()
{
  breakpointsText_->clear();

  instructionsText_->clearBreakpoints();

  //----

  std::vector<ushort> addrs;

  //c6502_->getBreakpoints(addrs);

  std::string str;

  for (uint i = 0; i < addrs.size(); ++i) {
    str = CStrUtil::toHexString(addrs[i], 4);

    breakpointsText_->append(str.c_str());

    instructionsText_->addBreakPoint(addrs[i]);
  }
}

void
CQ6502Dbg::
postStepProc()
{
  while (qApp->hasPendingEvents())
    qApp->processEvents();
}

void
CQ6502Dbg::
regChanged(C6502::Reg reg)
{
  if (reg == C6502::Reg::A || reg == C6502::Reg::NONE) {
    if (reg == C6502::Reg::NONE || isRegistersTrace())
      aEdit_->setValue(c6502_->A());

    if (reg == C6502::Reg::NONE || isFlagsTrace()) {
      cFlagCheck_->setChecked(c6502_->Cflag());
      zFlagCheck_->setChecked(c6502_->Zflag());
      iFlagCheck_->setChecked(c6502_->Iflag());
      dFlagCheck_->setChecked(c6502_->Dflag());
      bFlagCheck_->setChecked(c6502_->Bflag());
      vFlagCheck_->setChecked(c6502_->Vflag());
      nFlagCheck_->setChecked(c6502_->Nflag());
    }
  }

  if (reg == C6502::Reg::X || reg == C6502::Reg::NONE) {
    if (reg == C6502::Reg::NONE || isRegistersTrace())
      xEdit_->setValue(c6502_->X());
  }

  if (reg == C6502::Reg::Y || reg == C6502::Reg::NONE) {
    if (reg == C6502::Reg::NONE || isRegistersTrace())
      yEdit_->setValue(c6502_->Y());
  }

  if (reg == C6502::Reg::SP || reg == C6502::Reg::NONE) {
    if (reg == C6502::Reg::NONE || isRegistersTrace())
      spEdit_->setValue(c6502_->SP());

    if (reg == C6502::Reg::NONE || isStackTrace())
      updateStack();
  }

  if (reg == C6502::Reg::PC || reg == C6502::Reg::NONE) {
    uint pc = c6502_->PC();

    if (reg == C6502::Reg::NONE || isRegistersTrace())
      pcEdit_->setValue(pc);

    if (reg == C6502::Reg::NONE || isBreakpointsTrace())
      breakpointsEdit_->setText(CStrUtil::toHexString(pc, 4).c_str());

    //----

    int mem1 = memoryVBar_->value();
    int mem2 = mem1 + 20;
    int mem  = pc / 8;

    if (reg == C6502::Reg::NONE || isMemoryTrace()) {
      if (mem < mem1 || mem > mem2) {
        memoryVBar_->setValue(mem);
      }
      else {
        memoryText_->update();
      }
    }

    //----

    if (reg == C6502::Reg::NONE || isInstructionsTrace()) {
      uint lineNum;

      if (! instructionsText_->getLineForPC(pc, lineNum))
        updateInstructions();

      if (instructionsText_->getLineForPC(pc, lineNum))
        instructionsVBar_->setValue(lineNum);

      //----

      // instruction at PC
      int         alen;
      std::string astr;

      c6502_->disassembleAddr(c6502_->PC(), astr, alen);

      opData_->setText(astr.c_str());
    }
  }

  if (reg == C6502::Reg::SR || reg == C6502::Reg::NONE) {
    if (reg == C6502::Reg::NONE || isRegistersTrace())
      srEdit_->setValue(c6502_->SR());
  }
}

void
CQ6502Dbg::
memChanged(ushort pos, ushort len)
{
  if (! isMemoryTrace()) {
    memoryDirty_ = true;
    return;
  }

  //if (! debug_) return;

  memChangedI(pos, len);
}

void
CQ6502Dbg::
memChangedI(ushort pos, ushort len)
{
  ushort pos1 = pos;
  ushort pos2 = pos + len;

  uint lineNum1 = pos1/8;
  uint lineNum2 = pos2/8;

  for (uint lineNum = lineNum1; lineNum <= lineNum2; ++lineNum)
    setMemoryLine(8*lineNum);

  memoryText_->update();

  memoryDirty_ = false;
}

void
CQ6502Dbg::
breakpointsChanged()
{
  if (isBreakpointsTrace())
    updateBreakpoints();
}

void
CQ6502Dbg::
traceBackChanged()
{
  if (isTraceBackTrace())
    updateTraceBack();
}

void
CQ6502Dbg::
setStop(bool)
{
}

void
CQ6502Dbg::
setHalt(bool b)
{
  haltCheck_->setChecked(b);
}

void
CQ6502Dbg::
addBreakpointSlot()
{
  uint value;

  if (! CStrUtil::decodeHexString(breakpointsEdit_->text().toStdString(), &value))
    value = c6502_->PC();

//if (! c6502_->isBreakpoint(value))
//  c6502_->addBreakpoint(value);
}

void
CQ6502Dbg::
deleteBreakpointSlot()
{
  uint value;

  if (! CStrUtil::decodeHexString(breakpointsEdit_->text().toStdString(), &value))
    value = c6502_->PC();

//if (c6502_->isBreakpoint(value))
//  c6502_->removeBreakpoint(value);
}

void
CQ6502Dbg::
clearBreakpointSlot()
{
  //c6502_->removeAllBreakpoints();
}

void
CQ6502Dbg::
memoryTraceSlot()
{
  setMemoryTrace(memoryGroup_->isChecked());
}

void
CQ6502Dbg::
instructionsTraceSlot()
{
  setInstructionsTrace(instructionsGroup_->isChecked());
}

void
CQ6502Dbg::
registersTraceSlot()
{
  setRegistersTrace(registersGroup_->isChecked());
}

void
CQ6502Dbg::
flagsTraceSlot()
{
  setFlagsTrace(flagsGroup_->isChecked());
}

void
CQ6502Dbg::
stackTraceSlot()
{
  setStackTrace(stackGroup_->isChecked());

  if (stackGroup_->isChecked())
    updateStack();
}

void
CQ6502Dbg::
traceBackTraceSlot()
{
  setTraceBackTrace(traceBackGroup_->isChecked());

  if (traceBackGroup_->isChecked())
    updateTraceBack();
}

void
CQ6502Dbg::
breakpointsTraceSlot()
{
  setBreakpointsTrace(breakpointsGroup_->isChecked());
}

void
CQ6502Dbg::
setTraceSlot()
{
  bool checked = traceCheck_->isChecked();

  memoryGroup_      ->setChecked(checked);
  instructionsGroup_->setChecked(checked);
  registersGroup_   ->setChecked(checked);
  flagsGroup_       ->setChecked(checked);
  stackGroup_       ->setChecked(checked);
  traceBackGroup_   ->setChecked(checked);
  breakpointsGroup_ ->setChecked(checked);
}

void
CQ6502Dbg::
setHaltSlot()
{
  //c6502_->setHalt(haltCheck_->isChecked());
}

void
CQ6502Dbg::
runSlot()
{
  //c6502_->execute();

  updateAll();
}

void
CQ6502Dbg::
nextSlot()
{
  //c6502_->next();

  updateAll();
}

void
CQ6502Dbg::
stepSlot()
{
  c6502_->step();

  updateAll();
}

void
CQ6502Dbg::
continueSlot()
{
  //c6502_->cont();

  updateAll();
}

void
CQ6502Dbg::
stopSlot()
{
  //c6502_->stop();

  updateAll();
}

void
CQ6502Dbg::
restartSlot()
{
  c6502_->reset();

  //c6502_->setPC(c6502_->getLoadPos());

  updateAll();
}

void
CQ6502Dbg::
exitSlot()
{
  exit(0);
}

void
CQ6502Dbg::
updateAll()
{
  regChanged(C6502::Reg::NONE);

  memChangedI(0, 65535);

  update();
}
