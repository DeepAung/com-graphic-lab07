#include "core/Frame.h"
#include "core/Shader.h"
#include "core/Timer.h"
#include "utils/Hash.h"
#include "utils/image.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

// TASK 3's scene, in C++. Same numbers, same order of operations, so the panel can
// hold the GPU's answer against one it worked out itself.
static float sdfCircle(glm::vec2 p, float r) { return glm::length(p) - r; }

static float sdfBox(glm::vec2 p, glm::vec2 hs) {
  glm::vec2 q = glm::abs(p) - hs;
  return glm::length(glm::max(q, glm::vec2(0.0f))) + std::min(std::max(q.x, q.y), 0.0f);
}

static float sdfRoundedBox(glm::vec2 p, glm::vec2 hs, float r) {
  return sdfBox(p, hs - r) - r;
}

static float opSmoothUnion(float d1, float d2, float k) {
  float h = std::clamp(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
  return glslMix(d2, d1, h) - k * h * (1.0f - h);
}

static float nearestPrimitive(glm::vec2 p, float time) {
  float body = sdfRoundedBox(p - glm::vec2(-0.35f, 0.0f), glm::vec2(0.40f, 0.25f), 0.08f);
  float ball = sdfCircle(p - glm::vec2(0.45f, 0.15f * std::sin(time)), 0.30f);
  return std::min(body, ball);
}

static float scene(glm::vec2 p, float time) {
  float body = sdfRoundedBox(p - glm::vec2(-0.35f, 0.0f), glm::vec2(0.40f, 0.25f), 0.08f);
  float ball = sdfCircle(p - glm::vec2(0.45f, 0.15f * std::sin(time)), 0.30f);
  float d    = opSmoothUnion(body, ball, 0.20f);
  float hole = sdfCircle(p - glm::vec2(-0.35f, 0.0f), 0.12f);
  return std::max(d, -hole);
}

struct Probe {
  std::uint32_t x;
  std::uint32_t y;
  const char*   what;
};

// One of each: inside the shape, outside everything, in the fillet the smooth union
// invented between the two primitives, and inside the hole that was subtracted
static const std::array<Probe, 4> PROBES = {{
  { 220, 300, "inside the body   " },
  {  60,  60, "outside everything" },
  { 430, 300, "in the blend      " },
  { 295, 300, "inside the hole   " },
}};

static constexpr std::uint32_t BLEND_PROBE = 2;

static glm::vec2 pixelToP(std::uint32_t x, std::uint32_t y, glm::vec2 resolution) {
  glm::vec2 fragCoord(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
  return (2.0f * fragCoord - resolution) / resolution.y;
}

// mode 2 writes clamp(d*0.5 + 0.5, 0, 1), so one 8-bit channel is worth 2/255 of d
static float decodeDistance(const std::vector<std::uint8_t>& rgba, std::uint32_t width,
                            std::uint32_t x, std::uint32_t y)
{
  return static_cast<float>(pixelAt(rgba, width, x, y)[0]) / 255.0f * 2.0f - 1.0f;
}

int main(int argc, char** argv) {
  try {
    Args args = parseArgs(argc, argv);

    GLFWwindow* handle =
      args.headless ? nullptr : createHiddenWindow("Lab 07 - Part III", WIDTH, HEIGHT);

    Device       dev        = createDevice(handle);
    VkRenderPass renderPass = createRenderPass(dev);
    Target       target     = createTarget(dev, renderPass, WIDTH, HEIGHT);

    Buffer uniform = createBuffer(dev, sizeof(ToyParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    Descriptors      descs  = createDescriptors(dev, uniform);
    VkPipelineLayout layout = createPipelineLayout(dev, descs);
    Program          sdf    =
      createProgram(dev, renderPass, layout, target, "fullscreen.vert", "sdf.frag");

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

    auto record = [&](VkCommandBuffer cmd, const ToyParams&) {
      vkCmdBeginRenderPass(cmd, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sdf.pipeline);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descs.set,
                              0, nullptr);
      vkCmdDraw(cmd, 3, 1, 0, 0);
      vkCmdEndRenderPass(cmd);
    };

    ToyParams field = params;
    field.mode      = 2;
    std::vector<std::uint8_t> fieldView = renderOnce(dev, target, uniform, field, record);

    ToyParams shape = params;
    shape.mode      = 0;
    std::vector<std::uint8_t> shapeView = renderOnce(dev, target, uniform, shape, record);

    std::vector<std::uint8_t> rgba = renderOnce(dev, target, uniform, params, record);
    Timing timing = timePass(dev, WARMUP_FRAMES, MEASURED_FRAMES,
                             [&](VkCommandBuffer cmd) { record(cmd, params); });

    double diff = diffFromReference("sdf", rgba, target.width, target.height);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << label("shaders") << "fullscreen.vert + sdf.frag\n";
    std::cout << label("uniform") << "time " << params.time << " s, mode " << params.mode << "\n";

    for (const Probe& p : PROBES) {
      glm::vec2 point  = pixelToP(p.x, p.y, params.resolution);
      float     cpu    = scene(point, params.time);
      float     shader = decodeDistance(fieldView, target.width, p.x, p.y);
      std::cout << label("probe") << "(" << std::setw(3) << p.x << ", " << std::setw(3) << p.y
                << ") " << p.what << "  shader d " << std::setw(6) << shader << "  cpu d "
                << std::setw(6) << cpu << "  " << (cpu < 0.0f ? "inside" : "outside") << "\n";
    }

    glm::vec2 blend = pixelToP(PROBES[BLEND_PROBE].x, PROBES[BLEND_PROBE].y, params.resolution);
    std::cout << label("bound") << "in the blend the field says " << scene(blend, params.time)
              << ", the nearer primitive is\n"
              << std::string(17, ' ') << nearestPrimitive(blend, params.time)
              << " away: after a smooth union d is a bound, not a distance\n";

    std::cout << std::setprecision(1);
    std::cout << label("coverage") << coverage(shapeView) * 100.0
              << "% of the target is shape, the rest is exactly black\n";
    std::cout << label("image") << imageResult(diff) << "\n";

    if (shaderHasTodo("sdf.frag"))
      std::cout << label("incomplete") << "sdf.frag still has TODO(TASK 3): scene() returns 1.0,\n"
                << std::string(17, ' ') << "so every pixel is outside and the target is black\n";

    std::cout << std::setprecision(3);
    std::cout << label("gpu time") << timing.medianMs << " ms median, " << timing.p95Ms
              << " ms p95" << std::endl;

    if (!args.save.empty()) writePng(args.save, rgba, target.width, target.height);

    if (!args.headless) {
      Window win = createSwapchain(dev, handle);

      FrameHooks hooks{};
      hooks.record    = record;
      hooks.shaders   = { "fullscreen.vert", "sdf.frag" };
      hooks.savePath  = std::string(REPORT_DIR) + "/sdf.png";
      hooks.extraKeys = "hold the left mouse button in mode 0 to drag the ball";
      hooks.onReload  = [&](const std::vector<std::string>&) {
        reloadProgram(dev, renderPass, layout, target, sdf);
      };

      runWindow(dev, win, target, uniform, params, args, hooks);
      destroySwapchain(dev, win);
    }

    destroyProgram(dev, sdf);
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
