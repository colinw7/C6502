#include <CQ6502Dbg.h>
#include <CQApp.h>
#include <C6502.h>
#include <fstream>

class C6502Test : public C6502 {
 public:
  C6502Test() { }

  const CQ6502Dbg *dbg() const { return dbg_; }
  void setDbg(CQ6502Dbg *p) { dbg_ = p; }

  void breakpointsChanged() override {
    dbg_->updateBreakpoints();
  }

  void jumpPointHit(uchar) override {
    if      (PC() == 0xFFCF) std::cerr << "CHRIN\n";
    else if (PC() == 0xFFD2) std::cerr << "CHROUT\n";
  }

//void flagsChanged() override { if (dbg_) dbg_->updateSlot(); }
//void stackChanged() override { if (dbg_) dbg_->updateSlot(); }
  void pcChanged   () override { if (dbg_) dbg_->updateSlot(); }
//void memChanged  () override { if (dbg_) dbg_->updateSlot(); }

  void handleBreak  () override { if (dbg_) dbg_->forceHalt (); }
  void breakpointHit() override { if (dbg_) dbg_->forceHalt (); }
  void illegalJump  () override { if (dbg_) dbg_->forceHalt (); }

 private:
  CQ6502Dbg *dbg_ { nullptr };
};

//---

int
main(int argc, char **argv)
{
  CQApp app(argc, argv);

  std::string filename;

  bool binary   = false;
  bool test     = false;
  bool assemble = false;
  int  org      = 0x0600;
  bool jump     = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if      (arg == "-bin") {
      binary = true;
    }
    else if (arg == "-org") {
      ++i;

      if (i < argc) {
        org = atoi(argv[i]);
      }
    }
    else if (arg == "-jump") {
      jump = true;
    }
    else if (arg == "-test") {
      test = true;
    }
    else if (arg == "-a" || arg == "-assemble") {
      assemble = true;
    }
    else {
      filename = argv[i];
    }
  }

  //---

  auto cpu = new C6502Test;

  cpu->setOrg(org);

  if (binary) {
    if (filename != "") {
      cpu->loadBin(filename);

      cpu->setPC(org);
    }
    else {
      std::cerr << "Missing binary filename '" << filename << "'\n";
      exit(1);
    }
  }

#if 0
  if (filename != "") {
    if (binary)
      cpu->loadBin(filename);
    else
      cpu->load(filename);
  }
#endif

  if (assemble) {
    std::ifstream ifs(filename.c_str(), std::ios::in);

    cpu->assemble(org, ifs);

    cpu->setPC(org);
  }

  if (jump) {
    cpu->addJumpPoint(0xFFCF);
    cpu->addJumpPoint(0xFFD2);
  }

  //---

  CQ6502Dbg *dbg = new CQ6502Dbg(cpu);

  cpu->setDbg(dbg);

  dbg->init();

  dbg->setFixedFont(QFont("Courier New", 16));

  dbg->show();

  return app.exec();
}
