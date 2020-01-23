#include <C6502Test.h>

int
main(int argc, char **argv)
{
  C6502 cpu;

  ushort org         = 0x0600;
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
      if      (argv[i][1] == 'a')
        assemble = true;
      else if (argv[i][1] == 'd')
        disassemble = true;
      else if (argv[i][1] == 'r')
        run = true;
      else if (argv[i][1] == 'p')
        print = true;
      else if (argv[i][1] == 'D')
        debug = true;
      else if (argv[i][1] == 'o') {
        ++i;

        if (i < argc) {
          org = atoi(argv[i]);
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

      cpu.assemble(org, ifs);
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

    cpu.print(org);
  }

  exit(0);
}
