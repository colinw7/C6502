#include <C6502Test.h>

int
main(int argc, char **argv)
{
  C6502 cpu;

  ushort aorg        = 0x0000; // assemble org
  ushort org         = 0x0000; // disassemble, run, print org
  ushort len         = 0x0100;
  bool   assemble    = false;
  bool   disassemble = false;
  bool   run         = false;
  bool   print       = false;
  bool   c64         = false;
  bool   debug       = false;

  using Args = std::vector<std::string>;

  Args args;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      std::string arg = &argv[i][1];

      if      (arg == "a" || arg == "assemble")
        assemble = true;
      else if (arg == "d")
        disassemble = true;
      else if (arg == "r")
        run = true;
      else if (arg == "p")
        print = true;
      else if (arg == "D")
        debug = true;
      else if (arg == "o" || arg == "org") {
        ++i;

        if (i < argc) {
          org = atoi(argv[i]);
        }
      }
      else if (arg == "l" || arg == "len") {
        ++i;

        if (i < argc) {
          len = atoi(argv[i]);
        }
      }
      else if (strcmp(&argv[i][1], "c64") == 0) {
        c64 = true;
      }
      else {
        std::cerr << "Invalid arg '" << argv[i] << "'\n";
        exit(1);
      }
    }
    else {
      args.push_back(argv[i]);
    }
  }

  if (debug)
    cpu.setDebug(true);

  cpu.setEnableOutputProcs(true);

  if (c64) {
    cpu.load64Rom();
  }

  if (assemble) {
    std::cerr << "--- Assemble ---\n";

    for (const auto &arg : args) {
      std::ifstream ifs(arg.c_str(), std::ios::in);

      cpu.assemble(aorg, ifs, len);
    }
  }

  if (disassemble) {
    std::cerr << "--- Disassemble ---\n";

    cpu.disassemble(org);
  }

  if (run) {
    std::cerr << "--- Run ---\n";

    cpu.run(org);
  }

  if (print) {
    std::cerr << "--- Print ---\n";

    cpu.print(org, len);
  }

  exit(0);
}
