#include "core/Frame.h"
#include "core/Shader.h"
#include "core/Timer.h"
#include "utils/image.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>

static constexpr double BUDGET_MS = 2.0;   // chapter 7.6's scenario, at 800x600

struct Rule {
  const char* what;
  const char* needle;
};

// The rules from the handout, checked by reading your source. They say nothing about
// whether the picture is any good: that is what your eyes and the write-up are for
static const std::array<Rule, 4> RULES = {{
  { "uses noise.glsl", "noise.glsl" },
  { "uses sdf_",       "sdf_"       },
  { "uses iMouse",     "iMouse"     },
  { "uses iMode",      "iMode"      },
}};

int main(int argc, char** argv) {
  try {
    Args args = parseArgs(argc, argv);

    GLFWwindow* handle =
      args.headless ? nullptr : createHiddenWindow("Lab 07 - Part V", WIDTH, HEIGHT);

    Device       dev        = createDevice(handle);
    VkRenderPass renderPass = createRenderPass(dev);
    Target       target     = createTarget(dev, renderPass, WIDTH, HEIGHT);

    Buffer uniform = createBuffer(dev, sizeof(ToyParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    Descriptors      descs  = createDescriptors(dev, uniform);
    VkPipelineLayout layout = createPipelineLayout(dev, descs);
    Program          toy    =
      createProgram(dev, renderPass, layout, target, "fullscreen.vert", "mytoy.frag");

    ToyParams params{};
    params.resolution = glm::vec2(WIDTH, HEIGHT);
    params.time       = args.time;
    params.mode       = args.mode;
    params.octaves    = args.octaves > 0 ? args.octaves : 5;

    VkClearValue clear{};
    clear.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo passInfo{};
    passInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass        = renderPass;
    passInfo.framebuffer       = target.framebuffer;
    passInfo.renderArea.extent = { target.width, target.height };
    passInfo.clearValueCount   = 1;
    passInfo.pClearValues      = &clear;

    auto record = [&](VkCommandBuffer cmd, const ToyParams&) {
      vkCmdBeginRenderPass(cmd, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, toy.pipeline);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descs.set,
                              0, nullptr);
      vkCmdDraw(cmd, 3, 1, 0, 0);
      vkCmdEndRenderPass(cmd);
    };

    std::vector<std::uint8_t> rgba = renderOnce(dev, target, uniform, params, record);
    Timing timing = timePass(dev, WARMUP_FRAMES, MEASURED_FRAMES,
                             [&](VkCommandBuffer cmd) { record(cmd, params); });

    // Three frames a second apart, for the report
    std::filesystem::create_directories(REPORT_DIR);
    ToyParams frame = params;
    frame.mode      = 0;
    for (int second = 0; second < 3; ++second) {
      frame.time = static_cast<float>(second);
      std::string path = std::string(REPORT_DIR) + "/toy_" + std::to_string(second) + ".png";
      writePng(path, renderOnce(dev, target, uniform, frame, record), target.width, target.height);
    }

    std::cout << std::fixed << std::setprecision(3);
    std::cout << label("shaders") << "fullscreen.vert + mytoy.frag, through shadertoy.glsl\n";
    std::cout << label("uniform") << "time " << params.time << " s, mode " << params.mode
              << ", octaves " << params.octaves << "\n";
    std::cout << label("gpu time") << timing.medianMs << " ms median, " << timing.p95Ms
              << " ms p95\n";
    std::cout << label("budget") << BUDGET_MS << " ms at " << WIDTH << "x" << HEIGHT << ": "
              << (timing.medianMs <= BUDGET_MS ? "PASS" : "FAIL") << "\n";

    for (const Rule& rule : RULES)
      std::cout << label(rule.what, 17) << (shaderIncludes("mytoy.frag", rule.needle) ? "yes" : "no")
                << "\n";
    std::cout << label("TODO removed", 17) << (shaderHasTodo("mytoy.frag") ? "no" : "yes") << "\n";

    if (shaderHasTodo("mytoy.frag"))
      std::cout << label("incomplete") << "mytoy.frag still has TODO(TASK 5): what the panel\n"
                << std::string(17, ' ') << "measured is ShaderToy's default new shader, not yours\n";

    std::cout << std::setprecision(1);
    std::cout << label("coverage") << coverage(rgba) * 100.0 << "% of the target is not black\n";
    std::cout << label("frames") << REPORT_DIR << "/toy_0.png, toy_1.png, toy_2.png"
              << std::endl;

    if (!args.save.empty()) writePng(args.save, rgba, target.width, target.height);

    if (!args.headless) {
      Window win = createSwapchain(dev, handle);

      FrameHooks hooks{};
      hooks.record   = record;
      hooks.shaders  = { "fullscreen.vert", "mytoy.frag" };
      hooks.savePath = std::string(REPORT_DIR) + "/toy.png";
      hooks.onReload = [&](const std::vector<std::string>&) {
        reloadProgram(dev, renderPass, layout, target, toy);
      };

      runWindow(dev, win, target, uniform, params, args, hooks);
      destroySwapchain(dev, win);
    }

    destroyProgram(dev, toy);
    vkDestroyPipelineLayout(dev.device, layout, nullptr);
    destroyDescriptors(dev, descs);
    destroyBuffer(dev, uniform);
    destroyTarget(dev, target);
    destroyRenderPass(dev, renderPass);
    destroyDevice(dev);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
