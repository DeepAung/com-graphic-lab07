#ifndef __SHADER_H_INCLUDED__
#define __SHADER_H_INCLUDED__

#include "core/Device.h"

#include <string>

struct Compiled {
  bool        ok = false;
  std::string log;
};

// The directories glslc searches, student directory first when STUDENT_SHADERS is set
const std::vector<std::string>& shaderSourceDirs();

// Full path of shaders/<name>, "" when no directory has it
std::string shaderSourcePath(const std::string& name);

bool shaderHasTodo(const std::string& name);                              // greps TODO(TASK
bool shaderIncludes(const std::string& name, const std::string& needle);  // greps one line

// Runs glslc on shaders/<name> into SHADER_DIR/<name>.spv. Never throws: a failed
// compile comes back as ok = false plus the compiler's own message.
Compiled compileShader(const std::string& name);

VkShaderModule createShaderModule(const Device& dev, const std::string& name);

// First line of "glslc --version", empty when it cannot be run
std::string glslcVersion();

#endif
