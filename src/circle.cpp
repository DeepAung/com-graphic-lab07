#include "core/Frame.h"
#include "core/Shader.h"
#include "core/Timer.h"
#include "utils/image.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

struct Corner {
  std::uint32_t x;
  std::uint32_t y;
  const char*   expected;
};

// The three corners of the uv view. Vulkan's y counts down from the top, so green,
// which is uv.y, belongs at the BOTTOM left. A ShaderToy port has it the other way
static const std::array<Corner, 3> CORNERS = {{
  { 0,         0,          "(0, 0, 0)"   },
  { WIDTH - 1, 0,          "(255, 0, 0)" },
  { 0,         HEIGHT - 1, "(0, 255, 0)" },
}};

// TASK 1 again, in C++, so the panel can say what the pixel should have been
static glm::vec3 circleColor(float time, glm::vec2 fragCoord, glm::vec2 resolution) {
  glm::vec2 p   = (2.0f * fragCoord - resolution) / resolution.y;
  float     d   = glm::length(p) - 0.5f;
  glm::vec3 col = 0.5f + 0.5f * glm::cos(time + d * 10.0f + glm::vec3(0.0f, 2.0f, 4.0f));
  return col * glm::smoothstep(0.02f, -0.02f, d);
}

static std::string rgbText(glm::vec3 col) {
  std::string text = "(";
  for (int c = 0; c < 3; ++c) {
    long level = std::lround(std::clamp(col[c], 0.0f, 1.0f) * 255.0f);
    text += std::to_string(level) + (c < 2 ? ", " : ")");
  }
  return text;
}

static std::string rgbText(const std::vector<std::uint8_t>& pixel) {
  return "(" + std::to_string(pixel[0]) + ", " + std::to_string(pixel[1]) + ", " +
         std::to_string(pixel[2]) + ")";
}

int main(int argc, char** argv) {
  try {
    Args args = parseArgs(argc, argv);

    GLFWwindow* handle =
      args.headless ? nullptr : createHiddenWindow("Lab 07 - Part I", WIDTH, HEIGHT);

    Device       dev        = createDevice(handle);
    VkRenderPass renderPass = createRenderPass(dev);
    Target       target     = createTarget(dev, renderPass, WIDTH, HEIGHT);

    Buffer uniform = createBuffer(dev, sizeof(ToyParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    Descriptors      descs  = createDescriptors(dev, uniform);
    VkPipelineLayout layout = createPipelineLayout(dev, descs);
    Program          circle =
      createProgram(dev, renderPass, layout, target, "fullscreen.vert", "circle.frag");

    ToyParams params{};
    params.resolution = glm::vec2(WIDTH, HEIGHT);
    params.time       = args.time;
    params.mode       = args.mode;

    VkClearValue clear{};
    clear.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo passInfo{};
    passInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass        = renderPass;
    passInfo.framebuffer       = target.framebuffer;
    passInfo.renderArea.extent = { target.width, target.height };
    passInfo.clearValueCount   = 1;
    passInfo.pClearValues      = &clear;

    // No vertex buffer, no index buffer: three vertices out of gl_VertexIndex
    auto record = [&](VkCommandBuffer cmd, const ToyParams&) {
      vkCmdBeginRenderPass(cmd, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, circle.pipeline);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descs.set,
                              0, nullptr);
      vkCmdDraw(cmd, 3, 1, 0, 0);
      vkCmdEndRenderPass(cmd);
    };

    ToyParams uvParams = params;
    uvParams.mode      = 1;
    std::vector<std::uint8_t> uvView = renderOnce(dev, target, uniform, uvParams, record);

    std::vector<std::uint8_t> rgba = renderOnce(dev, target, uniform, params, record);
    Timing timing = timePass(dev, WARMUP_FRAMES, MEASURED_FRAMES,
                             [&](VkCommandBuffer cmd) { record(cmd, params); });

    double drawn = coverage(rgba);
    double diff  = diffFromReference("circle", rgba, target.width, target.height);

    glm::vec2 centre(static_cast<float>(WIDTH / 2) + 0.5f, static_cast<float>(HEIGHT / 2) + 0.5f);
    glm::vec3 predicted = circleColor(params.time, centre, params.resolution);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << label("shaders") << "fullscreen.vert + circle.frag, no vertex buffer\n";
    std::cout << label("uniform") << sizeof(ToyParams) << " bytes, time " << params.time
              << " s, mode " << params.mode << "\n";
    std::cout << std::setprecision(1);
    std::cout << label("coverage") << drawn * 100.0
              << "% of the target, pi*0.25 / (2 * 8/3) = 14.7% before the AA ring\n";
    std::cout << label("centre") << "pixel (" << WIDTH / 2 << ", " << HEIGHT / 2 << ") reads "
              << rgbText(pixelAt(rgba, target.width, WIDTH / 2, HEIGHT / 2)) << ", cpu says "
              << rgbText(predicted) << "\n";

    for (const Corner& corner : CORNERS)
      std::cout << label("mode 1") << "(" << corner.x << ", " << corner.y << ") reads "
                << rgbText(pixelAt(uvView, target.width, corner.x, corner.y)) << ", expected "
                << corner.expected << "\n";

    std::cout << label("image") << imageResult(diff) << "\n";

    if (shaderHasTodo("circle.frag"))
      std::cout << label("incomplete") << "circle.frag still has TODO(TASK 1), so d never\n"
                << std::string(17, ' ') << "leaves 1.0 and every pixel stays black\n";

    std::cout << std::setprecision(3);
    std::cout << label("gpu time") << timing.medianMs << " ms median, " << timing.p95Ms
              << " ms p95" << std::endl;

    if (!args.save.empty()) writePng(args.save, rgba, target.width, target.height);

    if (!args.headless) {
      Window win = createSwapchain(dev, handle);

      FrameHooks hooks{};
      hooks.record   = record;
      hooks.shaders  = { "fullscreen.vert", "circle.frag" };
      hooks.savePath = std::string(REPORT_DIR) + "/circle.png";
      hooks.onReload = [&](const std::vector<std::string>&) {
        reloadProgram(dev, renderPass, layout, target, circle);
      };

      runWindow(dev, win, target, uniform, params, args, hooks);
      destroySwapchain(dev, win);
    }

    destroyProgram(dev, circle);
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
