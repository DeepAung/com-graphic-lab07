#ifndef __UTILS_H_INCLUDED__
#define __UTILS_H_INCLUDED__

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

inline constexpr std::uint32_t WIDTH  = 800;
inline constexpr std::uint32_t HEIGHT = 600;

inline constexpr std::uint32_t WARMUP_FRAMES   = 5;
inline constexpr std::uint32_t MEASURED_FRAMES = 21;

// A sweep measures the same thing over and over with one knob moved, and the first
// entry would otherwise be measured on a GPU that has just woken up
inline constexpr std::uint32_t SWEEP_WARMUP    = 20;

// How many times one frame is recorded into a single command buffer before the timer
// divides by it. Everything in this lab is a fullscreen pass of a few hundred
// microseconds, which is close to the noise of a single timestamp pair
inline constexpr std::uint32_t FRAME_REPEATS   = 20;

// Sweeps compare cells 0.03 ms apart, so they submit far more copies at a time: the
// longer submit also keeps the GPU clock up, instead of measuring it ramping
inline constexpr std::uint32_t SWEEP_REPEATS   = 200;

#ifdef NDEBUG
inline constexpr bool ENABLE_VALIDATION = false;
#else
inline constexpr bool ENABLE_VALIDATION = true;

#endif

inline const std::vector<const char*> VALIDATION_LAYERS = {
  "VK_LAYER_KHRONOS_validation"
};

void check(VkResult result, const char* what);

// Every panel line in this lab starts the same way: two spaces, then the label
// padded out to 15 columns, so the values line up down the page.
std::string label(const std::string& name, std::size_t width = 15);

std::vector<char> readFile(const std::string& path);

#endif
