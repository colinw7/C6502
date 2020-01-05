#include <CQ6502Dbg.h>
#include <CQApp.h>
#include <C6502.h>
#include <fstream>

int
main(int argc, char **argv)
{
  CQApp app(argc, argv);

  std::string filename;

  bool binary   = false;
  bool test     = false;
  bool assemble = false;
  int  org      = 0x0600;

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

  C6502 c6502;

//c6502.setLoadPos(org);

#if 0
  if (filename != "") {
    if (binary)
      c6502.loadBin(filename);
    else
      c6502.load(filename);
  }
#endif

  if (assemble) {
    std::ifstream ifs(filename.c_str(), std::ios::in);

    c6502.assemble(org, ifs);

    c6502.setPC(org);
  }

  CQ6502Dbg dbg(&c6502);

  dbg.init();

  dbg.setFixedFont(QFont("Courier New", 16));

  dbg.show();

  return app.exec();
}
