#include "core/Device.h"
#include "core/Shader.h"

#include <array>
#include <cstdlib>
#include <iostream>

struct TaskFile {
  const char* name;
  const char* task;
};

static const std::array<TaskFile, 6> TASK_FILES = {{
  { "circle.frag",    "TASK 1" },
  { "noise.glsl",     "TASK 2" },
  { "sdf.frag",       "TASK 3" },
  { "heightmap.vert", "TASK 4" },
  { "heightmap.frag", "TASK 4" },
  { "mytoy.frag",     "TASK 5" },
}};

int main() {
  try {
    Device dev = createDevice(nullptr);

    std::cout << label("device") << dev.props.deviceName << "\n";
    std::cout << label("api") << VK_API_VERSION_MAJOR(dev.props.apiVersion) << "."
              << VK_API_VERSION_MINOR(dev.props.apiVersion) << "."
              << VK_API_VERSION_PATCH(dev.props.apiVersion) << "\n";
    std::cout << label("subgroup") << dev.subgroupSize << " lanes\n";

    if (dev.hasTimestamps)
      std::cout << label("timestamps") << dev.props.limits.timestampPeriod << " ns per tick\n";
    else
      std::cout << label("timestamps") << "UNSUPPORTED: this driver cannot measure Parts II to V,\n"
                << std::string(17, ' ') << "see 'If it does not work' in the handout\n";

    std::cout << label("statistics") << "pipeline statistics "
              << (dev.hasPipelineStats ? "supported" : "unsupported") << "\n";
    std::cout << label("vertex inputs") << dev.props.limits.maxVertexInputAttributes
              << " attributes max, and Part I uses none of them\n";
    std::cout << label("glslc") << GLSLC << "\n";
    std::cout << label("version") << glslcVersion() << "\n";

    for (const std::string& dir : shaderSourceDirs())
      std::cout << label("shaders") << dir << "\n";

    for (const TaskFile& file : TASK_FILES) {
      if (shaderSourcePath(file.name).empty())
        std::cout << label(file.name) << file.task << " MISSING\n";
      else
        std::cout << label(file.name) << file.task << " "
                  << (shaderHasTodo(file.name) ? "incomplete" : "done") << "\n";
    }

    destroyDevice(dev);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
