#include "core/Frame.h"

#include "core/Shader.h"
#include "core/Timer.h"
#include "utils/image.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>

namespace fs = std::filesystem;

using Clock  = std::chrono::steady_clock;
using Stamps = std::map<std::string, fs::file_time_type>;

static const int WATCHED_KEYS[] = {
  GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3,
  GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_S,
  GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET,
};

std::vector<std::uint8_t> renderOnce(const Device& dev,
                                     const Target& target,
                                     const Buffer& uniform,
                                     const ToyParams& params,
                                     const std::function<void(VkCommandBuffer, const ToyParams&)>& record)
{
  uploadBuffer(uniform, &params, sizeof(ToyParams));
  timePass(dev, 0, 1, [&](VkCommandBuffer cmd) { record(cmd, params); }, 1);
  return readTarget(dev, target);
}

Program createProgram(const Device& dev,
                      VkRenderPass renderPass,
                      VkPipelineLayout layout,
                      const Target& target,
                      const std::string& vertName,
                      const std::string& fragName,
                      const VertexLayout& vertexLayout,
                      const PipelineState& state)
{
  Program program{};
  program.vertName     = vertName;
  program.fragName     = fragName;
  program.vertexLayout = vertexLayout;
  program.state        = state;
  program.vert         = createShaderModule(dev, vertName);
  program.frag         = createShaderModule(dev, fragName);
  program.pipeline     = createPipeline(dev, renderPass, layout, program.vert, program.frag,
                                        target, vertexLayout, state);
  return program;
}

void reloadProgram(const Device& dev, VkRenderPass renderPass, VkPipelineLayout layout,
                   const Target& target, Program& program)
{
  std::string vertName = program.vertName;
  std::string fragName = program.fragName;
  VertexLayout vertexLayout = program.vertexLayout;
  PipelineState state = program.state;
  destroyProgram(dev, program);
  program = createProgram(dev, renderPass, layout, target, vertName, fragName, vertexLayout, state);
}

void destroyProgram(const Device& dev, Program& program) {
  vkDestroyPipeline(dev.device, program.pipeline, nullptr);
  vkDestroyShaderModule(dev.device, program.frag, nullptr);
  vkDestroyShaderModule(dev.device, program.vert, nullptr);
  program = Program{};
}

static Stamps shaderStamps() {
  Stamps stamps;
  for (const std::string& dir : shaderSourceDirs()) {
    std::error_code ec;
    for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec))
      if (entry.is_regular_file(ec))
        stamps[entry.path().string()] = entry.last_write_time(ec);
  }
  return stamps;
}

static void printLog(const std::string& log) {
  std::istringstream lines(log);
  bool               first = true;
  for (std::string text; std::getline(lines, text);) {
    if (text.empty()) continue;
    std::cout << (first ? label("compile error") : std::string(17, ' ')) << text << "\n";
    first = false;
  }
  if (first) std::cout << label("compile error") << "glslc said nothing, look at the file\n";
}

// Recompile everything this part draws with. The old pipeline stays alive when the
// compile fails, so a typo costs you a message and not the window.
static void reload(const FrameHooks& hooks) {
  bool ok = true;
  for (const std::string& name : hooks.shaders) {
    Compiled compiled = compileShader(name);
    if (!compiled.ok) {
      ok = false;
      printLog(compiled.log);
    }
  }
  if (!ok) {
    std::cout << std::flush;
    return;
  }

  if (hooks.onReload) hooks.onReload(hooks.shaders);
  for (const std::string& name : hooks.shaders)
    std::cout << label("reloaded") << name << "\n";
  std::cout << std::flush;
}

static void printKeys(const FrameHooks& hooks) {
  std::cout << label("keys") << "0..3 debug modes, Up/Down octaves, S saves a png, Esc quits\n";
  if (!hooks.extraKeys.empty())
    std::cout << std::string(17, ' ') << hooks.extraKeys << "\n";
  for (const std::string& dir : shaderSourceDirs())
    std::cout << label("watching") << dir << "\n";
  std::cout << std::flush;
}

static void handleKey(int key, ToyParams& params, const Device& dev, const Target& target,
                      const FrameHooks& hooks)
{
  switch (key) {
    case GLFW_KEY_0: params.mode = 0; break;
    case GLFW_KEY_1: params.mode = 1; break;
    case GLFW_KEY_2: params.mode = 2; break;
    case GLFW_KEY_3: params.mode = 3; break;
    case GLFW_KEY_UP:   params.octaves = std::min(params.octaves + 1, 8u); break;
    case GLFW_KEY_DOWN: params.octaves = std::max(params.octaves, 2u) - 1; break;
    case GLFW_KEY_S:
      if (!hooks.savePath.empty()) {
        std::filesystem::create_directories(std::filesystem::path(hooks.savePath).parent_path());
        writePng(hooks.savePath, readTarget(dev, target), target.width, target.height);
        std::cout << label("saved") << hooks.savePath << std::endl;
      }
      break;
    default: break;
  }
  if (hooks.onKey) hooks.onKey(key, params);
}

void runWindow(const Device& dev,
               const Window& win,
               const Target& target,
               const Buffer& uniform,
               ToyParams& params,
               const Args& args,
               const FrameHooks& hooks)
{
  printKeys(hooks);
  showWindow(win);

  Stamps            stamps  = shaderStamps();
  Clock::time_point started = Clock::now();
  Clock::time_point polled  = started;
  int               was[std::size(WATCHED_KEYS)] = { GLFW_RELEASE };

  while (windowOpen(win)) {
    Clock::time_point now = Clock::now();

    std::chrono::duration<float> elapsed = now - started;
    params.time = args.timeFixed ? args.time : elapsed.count();

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(win.handle, &x, &y);
    params.mouse.x = static_cast<float>(x);
    params.mouse.y = static_cast<float>(y);
    params.mouse.z =
      glfwGetMouseButton(win.handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ? 1.0f : 0.0f;

    uploadBuffer(uniform, &params, sizeof(ToyParams));
    timePass(dev, 0, 1, [&](VkCommandBuffer cmd) { hooks.record(cmd, params); }, 1);

    for (std::size_t i = 0; i < std::size(WATCHED_KEYS); ++i) {
      int state = glfwGetKey(win.handle, WATCHED_KEYS[i]);
      if (state == GLFW_PRESS && was[i] == GLFW_RELEASE)
        handleKey(WATCHED_KEYS[i], params, dev, target, hooks);
      was[i] = state;
    }
    if (glfwGetKey(win.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(win.handle, GLFW_TRUE);

    if (now - polled > std::chrono::milliseconds(250)) {
      polled        = now;
      Stamps latest = shaderStamps();
      if (latest != stamps) {
        stamps = latest;
        vkDeviceWaitIdle(dev.device);
        reload(hooks);
      }
    }

    presentTarget(dev, win, target);
  }
}
