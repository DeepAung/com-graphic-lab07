#include "core/Frame.h"
#include "core/Shader.h"
#include "core/Timer.h"
#include "utils/Hash.h"
#include "utils/image.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

// (300, 200) sits at gl_FragCoord (300.5, 200.5), which terrain.frag turns into
// p = (3, 2) exactly: a lattice point, where value_noise has to be one hash and
// nothing else. Any other pixel would mix four of them.
static constexpr std::uint32_t PROBE_X  = 300;
static constexpr std::uint32_t PROBE_Y  = 200;
static constexpr std::uint32_t OCTAVES  = 6;    // what reference/terrain.png was made with
static constexpr std::uint32_t MAX_OCTAVES = 8;

static int level(float value) {
  return static_cast<int>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

int main(int argc, char** argv) {
  try {
    Args args = parseArgs(argc, argv);

    GLFWwindow* handle =
      args.headless ? nullptr : createHiddenWindow("Lab 07 - Part II", WIDTH, HEIGHT);

    Device       dev        = createDevice(handle);
    VkRenderPass renderPass = createRenderPass(dev);
    Target       target     = createTarget(dev, renderPass, WIDTH, HEIGHT);

    Buffer uniform = createBuffer(dev, sizeof(ToyParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    Descriptors      descs   = createDescriptors(dev, uniform);
    VkPipelineLayout layout  = createPipelineLayout(dev, descs);
    Program          terrain =
      createProgram(dev, renderPass, layout, target, "fullscreen.vert", "terrain.frag");

    ToyParams params{};
    params.resolution = glm::vec2(WIDTH, HEIGHT);
    params.time       = args.time;
    params.mode       = args.mode;
    params.octaves    = args.octaves > 0 ? args.octaves : OCTAVES;

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
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, terrain.pipeline);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descs.set,
                              0, nullptr);
      vkCmdDraw(cmd, 3, 1, 0, 0);
      vkCmdEndRenderPass(cmd);
    };

    ToyParams probe = params;
    probe.mode      = 2;
    std::vector<std::uint8_t> noiseView = renderOnce(dev, target, uniform, probe, record);

    probe.mode    = 1;
    probe.octaves = 1;
    std::vector<std::uint8_t> oneOctave = renderOnce(dev, target, uniform, probe, record);

    probe.octaves = MAX_OCTAVES;
    std::vector<std::uint8_t> deepView = renderOnce(dev, target, uniform, probe, record);

    int shaderNoise = pixelAt(noiseView, target.width, PROBE_X, PROBE_Y)[0];
    int shaderOne   = pixelAt(oneOctave, target.width, PROBE_X, PROBE_Y)[0];
    int cpuNoise    = level(hash(glm::uvec2(1003, 1002)));

    int highest = 0;
    for (std::size_t i = 0; i < deepView.size(); i += 4)
      highest = std::max(highest, static_cast<int>(deepView[i]));

    std::vector<std::uint8_t> rgba = renderOnce(dev, target, uniform, params, record);
    Timing timing = timePass(dev, WARMUP_FRAMES, MEASURED_FRAMES,
                             [&](VkCommandBuffer cmd) { record(cmd, params); });

    double diff = diffFromReference("terrain", rgba, target.width, target.height);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << label("shaders") << "fullscreen.vert + terrain.frag, which includes noise.glsl\n";
    std::cout << label("uniform") << "time " << params.time << " s, mode " << params.mode
              << ", octaves " << params.octaves << "\n";
    std::cout << label("lattice") << "pixel (" << PROBE_X << ", " << PROBE_Y << ") is p = (3, 2): "
              << "value_noise reads " << shaderNoise << ", cpu hash(uvec2(1003, 1002)) is "
              << cpuNoise << "\n";
    std::cout << label("one octave") << "fbm(p, 1) reads " << shaderOne << ", half of "
              << shaderNoise << " is " << shaderNoise / 2 << "\n";
    std::cout << label("fbm ceiling") << "1 - 0.5^" << MAX_OCTAVES << " = "
              << 1.0 - std::pow(0.5, MAX_OCTAVES) << ", brightest pixel of the image "
              << static_cast<double>(highest) / 255.0 << "\n";

    ToyParams sweep = params;
    sweep.mode      = 0;

    // the heaviest cell first, thrown away: without it octave 1 is measured on a GPU
    // that is still waking up and comes out slower than octave 2
    sweep.octaves = MAX_OCTAVES;
    uploadBuffer(uniform, &sweep, sizeof(ToyParams));
    timePass(dev, SWEEP_WARMUP, 1, [&](VkCommandBuffer cmd) { record(cmd, sweep); },
             SWEEP_REPEATS);

    for (std::uint32_t octaves = 1; octaves <= MAX_OCTAVES; ++octaves) {
      sweep.octaves = octaves;
      uploadBuffer(uniform, &sweep, sizeof(ToyParams));
      Timing t = timePass(dev, SWEEP_WARMUP, MEASURED_FRAMES,
                          [&](VkCommandBuffer cmd) { record(cmd, sweep); }, SWEEP_REPEATS);
      std::cout << label("octaves " + std::to_string(octaves)) << std::setw(8) << t.medianMs
                << " ms median\n";
    }

    std::cout << label("image") << imageResult(diff) << " at " << params.octaves << " octaves\n";

    if (shaderHasTodo("noise.glsl"))
      std::cout << label("incomplete") << "noise.glsl still has TODO(TASK 2): value_noise and\n"
                << std::string(17, ' ') << "fbm both return 0, so every band is the same\n";

    std::cout << label("gpu time") << timing.medianMs << " ms median, " << timing.p95Ms
              << " ms p95" << std::endl;

    if (!args.save.empty()) writePng(args.save, rgba, target.width, target.height);

    if (!args.headless) {
      Window win = createSwapchain(dev, handle);

      FrameHooks hooks{};
      hooks.record   = record;
      hooks.shaders  = { "fullscreen.vert", "terrain.frag" };
      hooks.savePath = std::string(REPORT_DIR) + "/terrain.png";
      hooks.onReload = [&](const std::vector<std::string>&) {
        reloadProgram(dev, renderPass, layout, target, terrain);
      };

      runWindow(dev, win, target, uniform, params, args, hooks);
      destroySwapchain(dev, win);
    }

    destroyProgram(dev, terrain);
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
