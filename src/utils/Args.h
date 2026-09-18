#ifndef __ARGS_H_INCLUDED__
#define __ARGS_H_INCLUDED__

#include <cstdint>
#include <string>

// Flags every part understands. The panel always measures at args.time, which is
// 1.0 unless --time says otherwise: the reference images were rendered at t = 1.
struct Args {
  float         time      = 1.0f;  // --time T     freeze the clock, panel and window
  bool          timeFixed = false; //              set once --time was given
  bool          headless  = false; // --headless   no window, exit after the panel
  std::string   save;              // --save p.png write the measured frame
  std::uint32_t octaves   = 0;     // --octaves n  0 means "whatever this part defaults to"
  std::uint32_t grid      = 0;     // --grid n     0 means "whatever this part defaults to"
  std::uint32_t mode      = 0;     // --mode m     which view the panel measures
};

Args parseArgs(int argc, char** argv);

#endif
