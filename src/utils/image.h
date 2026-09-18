#ifndef __IMAGE_H_INCLUDED__
#define __IMAGE_H_INCLUDED__

#include <cstdint>
#include <string>
#include <vector>

void writePng(const std::string& path,
              const std::vector<std::uint8_t>& rgba,
              std::uint32_t width,
              std::uint32_t height);

double coverage(const std::vector<std::uint8_t>& rgba);

// Percentage of pixels that differ from reference/<name>.png by more than 3 levels
// in any channel, or -1.0 when there is no reference to compare against.
double diffFromReference(const std::string& name,
                         const std::vector<std::uint8_t>& rgba,
                         std::uint32_t width,
                         std::uint32_t height);

// Under 1% of the pixels off by more than 3 levels. Not zero: sin, cos and exp are
// allowed to differ in the last bits between one GPU and the next.
bool matches(double diffPercent);

// "MATCH (0.12% differ)", "DIFFERS (7.3% differ)" or "no reference"
std::string imageResult(double diffPercent);

std::vector<std::uint8_t> pixelAt(const std::vector<std::uint8_t>& rgba,
                                  std::uint32_t width,
                                  std::uint32_t x,
                                  std::uint32_t y);

#endif
