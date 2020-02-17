#include <CQ6502Dbg.h>
#include <CQ6502Memory.h>
#include <CQ6502Instructions.h>
#include <CQ6502Stack.h>
#include <CQ6502TraceBack.h>
#include <CQ6502RegEdit.h>
#include <C6502.h>
#include <CQTabSplit.h>
#include <CQUtil.h>
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
CQ6502Dbg(C6502 *cpu) :
 cpu_(cpu)
{
  setObjectName("dbg");

  setWindowTitle("6502 Emulator (Debug)");

  //cpu_->addTrace(this);
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

  updateRegisters();
}

void
CQ6502Dbg::
setFixedFont(const QFont &font)
{
  fixedFont_ = font;

  memoryArea_      ->setFont(getFixedFont());
  instructionsArea_->setFont(getFixedFont());

  stackText_->setFont(getFixedFont());
  traceBack_->setFont(getFixedFont());

  breakpointsWidgetData_.text->setFont(getFixedFont());

  regWidgetData_.aEdit ->setFont(getFixedFont());
  regWidgetData_.xEdit ->setFont(getFixedFont());
  regWidgetData_.yEdit ->setFont(getFixedFont());
  regWidgetData_.srEdit->setFont(getFixedFont());
  regWidgetData_.spEdit->setFont(getFixedFont());
  regWidgetData_.pcEdit->setFont(getFixedFont());
}

void
CQ6502Dbg::
setNumMemoryLines(int i)
{
  if (i != numMemoryLines_) {
    numMemoryLines_ = i;

    memoryArea_->setFont(getFixedFont());

    memoryArea_->setNumMemoryLines(numMemoryLines());

    memoryArea_->updateLayout();
  }
}

void
CQ6502Dbg::
setMemLineWidth(int i)
{
  if (i != memLineWidth_) {
    memLineWidth_ = i;

    setMemoryText();

    memoryArea_->setFont(getFixedFont());

    memoryArea_->setMemLineWidth(memLineWidth());

    memoryArea_->updateLayout();
  }
}

void
CQ6502Dbg::
setMemoryTrace(bool b)
{
  memoryTrace_ = b;

  if (memoryTrace_ && memoryDirty_) {
    memChanged(0, 65535);
  }
}

//---

void
CQ6502Dbg::
addWidgets()
{
  fixedFont_ = QFont("Courier", 10);

  QFontMetrics fm(getFixedFont());

  //----

  auto layout = CQUtil::makeLayout<QVBoxLayout>(this, 0, 0);

  //---

  // top for left and right frames (main interface)
  auto topFrame = CQUtil::makeWidget<CQTabSplit>("topFrame");
  topFrame->setOrientation(Qt::Horizontal);

  // bottom for options and buttons
  bottomFrame_  = CQUtil::makeWidget<QFrame>("bottomFrame");
  bottomLayout_ = CQUtil::makeLayout<QVBoxLayout>(bottomFrame_, 2, 2);

  layout->addWidget(topFrame);
  layout->addWidget(bottomFrame_);

  //------

  // left frame for memory and instructions
  leftFrame_ = CQUtil::makeWidget<CQTabSplit>("leftFrame");

  leftFrame_->setOrientation(Qt::Vertical);
  leftFrame_->setGrouped(true);

  topFrame->addWidget(leftFrame_, "Memory and Instructions");

  //---

  // right frame for registers, flags, stack, traceback and breakpoints
  rightFrame_ = CQUtil::makeWidget<CQTabSplit>("rightFrame");

  rightFrame_->setOrientation(Qt::Vertical);
  rightFrame_->setGrouped(true);

  topFrame->addWidget(rightFrame_, "Registers, Flags, Stack, Traceback and Breakpoins");

  //--

  topFrame->setSizes(QList<int>({INT_MAX, INT_MAX}));

  //--

  addLeftWidgets ();
  addRightWidgets();

  addBottomWidgets();
}

void
CQ6502Dbg::
addLeftWidgets()
{
  memoryGroup_ = CQUtil::makeLabelWidget<QGroupBox>("Enabled", "memoryGroup");
  memoryGroup_->setCheckable(true);

  connect(memoryGroup_, SIGNAL(toggled(bool)), this, SLOT(memoryTraceSlot()));

  auto memoryGroupLayout = CQUtil::makeLayout<QVBoxLayout>(memoryGroup_, 2, 2);

  memoryArea_ = new CQ6502MemArea(this);

  memoryGroupLayout->addWidget(memoryArea_);

  addLeftWidget(memoryGroup_, "Memory");

  //--

  auto instructionsFrame  = CQUtil::makeWidget<QFrame>("instructionsFrame");
  auto instructionsLayout = CQUtil::makeLayout<QVBoxLayout>(instructionsFrame, 2, 2);

  instructionsGroup_ = CQUtil::makeLabelWidget<QGroupBox>("Enabled", "instructionsGroup");
  instructionsGroup_->setCheckable(true);

  connect(instructionsGroup_, SIGNAL(toggled(bool)), this, SLOT(instructionsTraceSlot()));

  auto instructionsGroupLayout = CQUtil::makeLayout<QVBoxLayout>(instructionsGroup_, 2, 2);

  instructionsArea_ = new CQ6502InstArea(this);

  instructionsGroupLayout->addWidget(instructionsArea_);

  instructionsLayout->addWidget(instructionsGroup_);

  opData_ = CQUtil::makeWidget<QLineEdit>("opData");

  opData_->setReadOnly(true);

  instructionsLayout->addWidget(opData_);

  addLeftWidget(instructionsFrame, "Instructions");
}

void
CQ6502Dbg::
addLeftWidget(QWidget *w, const QString &name)
{
  leftFrame_->addWidget(w, name);
}

void
CQ6502Dbg::
addRightWidgets()
{
  regWidgetData_.group = CQUtil::makeLabelWidget<QGroupBox>("Enabled", "registersGroup");
  regWidgetData_.group->setCheckable(true);

  connect(regWidgetData_.group, SIGNAL(toggled(bool)), this, SLOT(registersTraceSlot()));

  regWidgetData_.layout = CQUtil::makeLayout<QGridLayout>(regWidgetData_.group, 2, 2);

  addRegistersWidgets();

  addRightWidget(regWidgetData_.group, "Registers");

  //--

  flagsWidgetData_.group = CQUtil::makeLabelWidget<QGroupBox>("Enabled", "flagsGroup");
  flagsWidgetData_.group->setCheckable(true);

  connect(flagsWidgetData_.group, SIGNAL(toggled(bool)), this, SLOT(flagsTraceSlot()));

  flagsWidgetData_.layout = CQUtil::makeLayout<QGridLayout>(flagsWidgetData_.group, 2, 6);

  addFlagsWidgets();

  addRightWidget(flagsWidgetData_.group, "Flags");

  //--

  stackGroup_ = CQUtil::makeLabelWidget<QGroupBox>("Enabled", "stackGroup");
  stackGroup_->setCheckable(true);

  connect(stackGroup_, SIGNAL(toggled(bool)), this, SLOT(stackTraceSlot()));

  auto stackLayout = CQUtil::makeLayout<QVBoxLayout>(stackGroup_, 2, 2);

  stackText_ = new CQ6502Stack(cpu_);

  stackText_->setFixedFont(getFixedFont());

  stackLayout->addWidget(stackText_);

  addRightWidget(stackGroup_, "Stack");

  //--

  traceBackGroup_ = CQUtil::makeLabelWidget<QGroupBox>("Enabled", "traceBackGroup");
  traceBackGroup_->setCheckable(true);

  connect(traceBackGroup_, SIGNAL(toggled(bool)), this, SLOT(traceBackTraceSlot()));

  auto traceBackLayout = CQUtil::makeLayout<QVBoxLayout>(traceBackGroup_, 2, 2);

  traceBack_ = new CQ6502TraceBack(cpu_);

  traceBack_->setFixedFont(getFixedFont());

  traceBackLayout->addWidget(traceBack_);

  addRightWidget(traceBackGroup_, "Traceback");

  //--

  breakpointsWidgetData_.group = CQUtil::makeLabelWidget<QGroupBox>("Enabled", "breakpointsGroup");
  breakpointsWidgetData_.group->setCheckable(true);

  connect(breakpointsWidgetData_.group, SIGNAL(toggled(bool)), this, SLOT(breakpointsTraceSlot()));

  breakpointsWidgetData_.layout =
    CQUtil::makeLayout<QVBoxLayout>(breakpointsWidgetData_.group, 2, 2);

  addBreakpointWidgets();

  addRightWidget(breakpointsWidgetData_.group, "Breakpoints");
}

void
CQ6502Dbg::
addRightWidget(QWidget *w, const QString &name)
{
  rightFrame_->addWidget(w, name);
}

void
CQ6502Dbg::
addBottomWidgets()
{
  auto optionsFrame  = CQUtil::makeWidget<QFrame>("optionsFrame");
  auto optionsLayout = CQUtil::makeLayout<QHBoxLayout>(optionsFrame, 2, 2);

  //--

  traceCheck_ = CQUtil::makeLabelWidget<QCheckBox>("Trace", "traceCheck");
  traceCheck_->setChecked(true);

  connect(traceCheck_, SIGNAL(stateChanged(int)), this, SLOT(setTraceSlot()));

  optionsLayout->addWidget(traceCheck_);

  //--

  haltCheck_ = CQUtil::makeLabelWidget<QCheckBox>("Halt", "haltCheck");
  haltCheck_->setChecked(false);

  connect(haltCheck_, SIGNAL(stateChanged(int)), this, SLOT(setHaltSlot()));

  optionsLayout->addWidget(haltCheck_);

  //--

  optionsLayout->addStretch(1);

  bottomLayout_->addWidget(optionsFrame);

  //---

  buttonsToolbar_ = CQUtil::makeWidget<QFrame>("buttonsToolbar");
  buttonsLayout_  = CQUtil::makeLayout<QHBoxLayout>(buttonsToolbar_, 2, 2);

  buttonsLayout_->addStretch(1);

  addButtonsWidgets();

  bottomLayout_->addWidget(buttonsToolbar_);
}

void
CQ6502Dbg::
addFlagsWidgets()
{
  int col = 0;

  auto addCheckLabel = [&](const QString &name) {
    QString lname = name.toLower();

    auto check = CQUtil::makeLabelWidget<QCheckBox>("", lname + "FlagCheck");
    auto label = CQUtil::makeLabelWidget<QLabel>(name, lname + "FlagLabel");

    flagsWidgetData_.layout->addWidget(label, 0, col);
    flagsWidgetData_.layout->addWidget(check, 1, col);

    ++col;

    return check;
  };

  flagsWidgetData_.nCheck = addCheckLabel("N");
  flagsWidgetData_.vCheck = addCheckLabel("V");
  flagsWidgetData_.xCheck = addCheckLabel("-");
  flagsWidgetData_.bCheck = addCheckLabel("B");
  flagsWidgetData_.dCheck = addCheckLabel("D");
  flagsWidgetData_.iCheck = addCheckLabel("I");
  flagsWidgetData_.zCheck = addCheckLabel("Z");
  flagsWidgetData_.cCheck = addCheckLabel("C");

  flagsWidgetData_.layout->setColumnStretch(col, 1);
  flagsWidgetData_.layout->setRowStretch   (  2, 1);
}

void
CQ6502Dbg::
addRegistersWidgets()
{
  int row = 0;

  auto addRegEdit = [&](C6502::Reg reg) {
    auto edit = new CQ6502RegEdit(cpu_, reg);
    regWidgetData_.layout->addWidget(edit, row++, 0);
    return edit;
  };

  regWidgetData_.pcEdit = addRegEdit(C6502::Reg::PC);
  regWidgetData_.aEdit  = addRegEdit(C6502::Reg::A );
  regWidgetData_.xEdit  = addRegEdit(C6502::Reg::X );
  regWidgetData_.yEdit  = addRegEdit(C6502::Reg::Y );
  regWidgetData_.srEdit = addRegEdit(C6502::Reg::SR);
  regWidgetData_.spEdit = addRegEdit(C6502::Reg::SP);

  regWidgetData_.layout->setColumnStretch(2  , 1);
  regWidgetData_.layout->setRowStretch   (row, 1);
}

void
CQ6502Dbg::
addBreakpointWidgets()
{
  breakpointsWidgetData_.text = CQUtil::makeWidget<QTextEdit>("breakpointsText");
  breakpointsWidgetData_.text->setReadOnly(true);

  breakpointsWidgetData_.text->setFont(getFixedFont());

  breakpointsWidgetData_.layout->addWidget(breakpointsWidgetData_.text);

  auto breakpointEditFrame = CQUtil::makeWidget<QFrame>("breakpointEditFrame");

  breakpointsWidgetData_.layout->addWidget(breakpointEditFrame);

  auto breakpointEditLayout = CQUtil::makeLayout<QHBoxLayout>(breakpointEditFrame, 0, 0);

  breakpointsWidgetData_.edit = CQUtil::makeWidget<QLineEdit>("breakpointsEdit");

  breakpointEditLayout->addWidget(CQUtil::makeLabelWidget<QLabel>("Addr", "addrLabel"));
  breakpointEditLayout->addWidget(breakpointsWidgetData_.edit);
  breakpointEditLayout->addStretch(1);

  auto breakpointToolbar = CQUtil::makeWidget<QFrame>("breakpointToolbar");

  auto breakpointToolbarLayout = CQUtil::makeLayout<QHBoxLayout>(breakpointToolbar, 0, 0);

  auto addBreakpointButton    =
    CQUtil::makeLabelWidget<QPushButton>("Add"   , "addBreakpointButton");
  auto deleteBreakpointButton =
    CQUtil::makeLabelWidget<QPushButton>("Delete", "deleteBreakpointButton");
  auto clearBreakpointButton  =
    CQUtil::makeLabelWidget<QPushButton>("Clear" , "clearBreakpointButton");

  connect(addBreakpointButton   , SIGNAL(clicked()), this, SLOT(addBreakpointSlot   ()));
  connect(deleteBreakpointButton, SIGNAL(clicked()), this, SLOT(deleteBreakpointSlot()));
  connect(clearBreakpointButton , SIGNAL(clicked()), this, SLOT(clearBreakpointSlot ()));

  breakpointToolbarLayout->addWidget(addBreakpointButton);
  breakpointToolbarLayout->addWidget(deleteBreakpointButton);
  breakpointToolbarLayout->addWidget(clearBreakpointButton);
  breakpointToolbarLayout->addStretch(1);

  breakpointsWidgetData_.layout->addWidget(breakpointToolbar);
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
  QPushButton *button = CQUtil::makeLabelWidget<QPushButton>(label, name);

  buttonsLayout_->addWidget(button);

  return button;
}

//---

void
CQ6502Dbg::
setMemoryText()
{
  cpu_->setDebugger(true);

  memoryArea_->setMemoryText();

  cpu_->setDebugger(false);
}

//---

void
CQ6502Dbg::
updateInstructions()
{
  instructionsArea_->text()->reload();
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
  breakpointsWidgetData_.text->clear();

  if (isInstructionsTrace())
    instructionsArea_->text()->clearBreakpoints();

  //----

  std::vector<ushort> addrs;

  cpu_->getBreakpoints(addrs);

  std::string str;

  for (uint i = 0; i < addrs.size(); ++i) {
    str = CStrUtil::toHexString(addrs[i], 4);

    breakpointsWidgetData_.text->append(str.c_str());

    if (isInstructionsTrace())
      instructionsArea_->text()->addBreakPoint(addrs[i]);
  }
}

void
CQ6502Dbg::
updateMemArea()
{
  memoryArea_->setAddrColor      (addrColor      ());
  memoryArea_->setMemDataColor   (memDataColor   ());
  memoryArea_->setMemCharsColor  (memCharsColor  ());
  memoryArea_->setCurrentColor   (currentColor   ());
  memoryArea_->setReadOnlyBgColor(readOnlyBgColor());
  memoryArea_->setScreenBgColor  (screenBgColor  ());
}

void
CQ6502Dbg::
processEvents()
{
  //while (qApp->hasPendingEvents())
  qApp->processEvents();
}

void
CQ6502Dbg::
updateRegisters()
{
  if (isRegistersTrace())
    regWidgetData_.aEdit->setValue(cpu_->A());

  if (isFlagsTrace()) {
    flagsWidgetData_.nCheck->setChecked(cpu_->Nflag());
    flagsWidgetData_.vCheck->setChecked(cpu_->Vflag());
    flagsWidgetData_.xCheck->setChecked(cpu_->Xflag());
    flagsWidgetData_.bCheck->setChecked(cpu_->Bflag());
    flagsWidgetData_.dCheck->setChecked(cpu_->Dflag());
    flagsWidgetData_.iCheck->setChecked(cpu_->Iflag());
    flagsWidgetData_.zCheck->setChecked(cpu_->Zflag());
    flagsWidgetData_.cCheck->setChecked(cpu_->Cflag());
  }

  if (isRegistersTrace())
    regWidgetData_.xEdit->setValue(cpu_->X());

  if (isRegistersTrace())
    regWidgetData_.yEdit->setValue(cpu_->Y());

  if (isRegistersTrace())
    regWidgetData_.spEdit->setValue(cpu_->SP());

  if (isStackTrace())
    updateStack();

  //---

  uint pc = cpu_->PC();

  if (isRegistersTrace())
    regWidgetData_.pcEdit->setValue(pc);

  if (isBreakpointsTrace())
    breakpointsWidgetData_.edit->setText(CStrUtil::toHexString(pc, 4).c_str());

  //----

  if (isMemoryTrace())
    memoryArea_->updateText(pc);

  //----

  if (isInstructionsTrace()) {
#if 0
    uint lineNum;

    if (! instructionsArea_->text()->getLineForPC(pc, lineNum))
      updateInstructions();

    if (instructionsArea_->text()->getLineForPC(pc, lineNum))
      instructionsArea_->vbar()->setValue(lineNum);
#else
    instructionsArea_->updateText(pc);
#endif

    //----

    // instruction at PC
    int         alen;
    std::string astr;

    cpu_->disassembleAddr(pc, astr, alen);

    opData_->setText(astr.c_str());
  }

  if (isRegistersTrace())
    regWidgetData_.srEdit->setValue(cpu_->SR());
}

void
CQ6502Dbg::
memChanged(ushort pos, ushort len)
{
  cpu_->setDebugger(true);

  ushort pos1 = pos;
  ushort pos2 = pos + len;

  uint lineNum1 = pos1/memLineWidth();
  uint lineNum2 = pos2/memLineWidth();

  for (uint lineNum = lineNum1; lineNum <= lineNum2; ++lineNum)
    memoryArea_->setMemoryLine(memLineWidth()*lineNum);

  memoryArea_->text()->update();

  memoryDirty_ = false;

  cpu_->setDebugger(false);
}

void
CQ6502Dbg::
breakpointsChangedSlot()
{
  breakpointsChanged();
}

void
CQ6502Dbg::
breakpointsChanged()
{
  if (isBreakpointsTrace())
    updateBreakpoints();
}

#if 0
void
CQ6502Dbg::
traceBackChanged()
{
  if (isTraceBackTrace())
    updateTraceBack();
}
#endif

//---

void
CQ6502Dbg::
paintEvent(QPaintEvent *e)
{
  if (updateNeeded_) {
    updateNeeded_ = false;
    updateCount   = 0;

    updateAll();
  }

  QFrame::paintEvent(e);
}

//---

void
CQ6502Dbg::
addBreakpointSlot()
{
  uint value;

  if (! CStrUtil::decodeHexString(breakpointsWidgetData_.edit->text().toStdString(), &value))
    value = cpu_->PC();

  if (! cpu_->isBreakpoint(value))
    cpu_->addBreakpoint(value);
}

void
CQ6502Dbg::
deleteBreakpointSlot()
{
  uint value;

  if (! CStrUtil::decodeHexString(breakpointsWidgetData_.edit->text().toStdString(), &value))
    value = cpu_->PC();

  if (cpu_->isBreakpoint(value))
    cpu_->removeBreakpoint(value);
}

void
CQ6502Dbg::
clearBreakpointSlot()
{
  cpu_->removeAllBreakpoints();
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
  setRegistersTrace(regWidgetData_.group->isChecked());
}

void
CQ6502Dbg::
flagsTraceSlot()
{
  setFlagsTrace(flagsWidgetData_.group->isChecked());
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
  setBreakpointsTrace(breakpointsWidgetData_.group->isChecked());
}

void
CQ6502Dbg::
setTraceSlot()
{
  bool checked = traceCheck_->isChecked();

  setTraced(checked);
}

void
CQ6502Dbg::
setTraced(bool traced)
{
  // left
  memoryGroup_      ->setChecked(traced);
  instructionsGroup_->setChecked(traced);

  // right
  regWidgetData_.group        ->setChecked(traced);
  flagsWidgetData_.group      ->setChecked(traced);
  stackGroup_                 ->setChecked(traced);
  traceBackGroup_             ->setChecked(traced);
  breakpointsWidgetData_.group->setChecked(traced);

  updateAll();
}

void
CQ6502Dbg::
setHaltSlot()
{
  cpu_->setHalt(haltCheck_->isChecked());
}

void
CQ6502Dbg::
forceHalt()
{
  traceCheck_->setChecked(true);
  haltCheck_ ->setChecked(true);

  updateSlot();
}

void
CQ6502Dbg::
updateSlot()
{
  bool doUpdate = false;

  if (traceCheck_->isChecked())
    doUpdate = true;
  else {
    ++updateCount;

    if (updateCount > 1000)
      doUpdate = true;
  }

  if (doUpdate) {
    updateNeeded_ = true;

    update();

    processEvents();
  }
}

void
CQ6502Dbg::
runSlot()
{
  haltCheck_->setChecked(false);

  cpu_->run();

  updateAll();
}

void
CQ6502Dbg::
nextSlot()
{
  haltCheck_->setChecked(false);

  cpu_->next();

  updateAll();
}

void
CQ6502Dbg::
stepSlot()
{
  cpu_->step();

  updateAll();
}

void
CQ6502Dbg::
continueSlot()
{
  haltCheck_->setChecked(false);

  cpu_->cont();

  updateAll();
}

void
CQ6502Dbg::
stopSlot()
{
  cpu_->setBreak(true);

  updateAll();
}

void
CQ6502Dbg::
restartSlot()
{
  cpu_->reset();

  cpu_->setPC(cpu_->org());

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
  updateRegisters();

  memChanged(0, 65535);

  update();
}

QSize
CQ6502Dbg::
sizeHint() const
{
  return QSize(1600, 1200);
}
