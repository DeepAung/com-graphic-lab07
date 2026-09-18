#include "utils/Args.h"

#include <stdexcept>
#include <vector>

static std::string next(const std::vector<std::string>& args, std::size_t& i, const char* flag) {
  if (i + 1 >= args.size()) throw std::runtime_error(std::string(flag) + " needs a value");
  return args[++i];
}

static std::uint32_t number(const std::string& text) {
  return static_cast<std::uint32_t>(std::stoul(text));
}

Args parseArgs(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  Args parsed{};

  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    if (a == "--headless") {
      parsed.headless  = true;
    } else if (a == "--time") {
      parsed.time      = std::stof(next(args, i, "--time"));
      parsed.timeFixed = true;
    } else if (a == "--save") {
      parsed.save      = next(args, i, "--save");
    } else if (a == "--octaves") {
      parsed.octaves   = number(next(args, i, "--octaves"));
    } else if (a == "--grid") {
      parsed.grid      = number(next(args, i, "--grid"));
    } else if (a == "--mode") {
      parsed.mode      = number(next(args, i, "--mode"));
    } else {
      throw std::runtime_error("unknown flag: " + a + "\n"
                               "usage: --time T --headless --save out.png "
                               "--octaves n --grid n --mode m");
    }
  }
  return parsed;
}
