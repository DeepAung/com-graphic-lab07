#ifndef __TIMER_H_INCLUDED__
#define __TIMER_H_INCLUDED__

#include "core/Device.h"

#include <functional>

struct Timing {
  double medianMs = 0.0;
  double p95Ms    = 0.0;
  double cpuMs    = 0.0;
};

// One submit per frame, `frames` of them, and the median is what gets printed.
// `repeats` records the same frame that many times inside one command buffer and
// divides the result: a single 0.02 ms frame is under this GPU's timer noise, ten in
// a row are not. The number that comes back is still milliseconds per frame.
Timing timePass(const Device& dev,
                std::uint32_t warmup,
                std::uint32_t frames,
                const std::function<void(VkCommandBuffer)>& record,
                std::uint32_t repeats = FRAME_REPEATS);

#endif
