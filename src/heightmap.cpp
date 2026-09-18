#include "core/Frame.h"
#include "core/Shader.h"
#include "core/Timer.h"
#include "utils/image.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iomanip>
#include <iostream>

static constexpr std::uint32_t GRID      = 256;   // what reference/heightmap.png was made with
static constexpr std::uint32_t FLAT_GRID = 64;    // the fragment-stage comparison
static constexpr float         AMPLITUDE = 0.45f;
static constexpr std::uint32_t OCTAVES   = 6;

static const std::array<std::uint32_t, 4> SWEEP = { 64, 128, 256, 512 };

// N x N vertices, xz in [-1,1], two triangles per cell. 8 bytes a vertex and
// nothing else: the height comes out of the noise, not out of a buffer
struct Grid {
  Buffer        vertices;
  Buffer        indices;
  std::uint32_t n          = 0;
  std::uint32_t indexCount = 0;
};

static Grid createGrid(const Device& dev, std::uint32_t n) {
  std::vector<glm::vec2>     points;
  std::vector<std::uint32_t> indices;
  points.reserve(static_cast<std::size_t>(n) * n);
  indices.reserve(static_cast<std::size_t>(n - 1) * (n - 1) * 6);

  float step = 2.0f / static_cast<float>(n - 1);
  for (std::uint32_t z = 0; z < n; ++z)
    for (std::uint32_t x = 0; x < n; ++x)
      points.push_back(glm::vec2(-1.0f + step * static_cast<float>(x),
                                 -1.0f + step * static_cast<float>(z)));

  for (std::uint32_t z = 0; z + 1 < n; ++z)
    for (std::uint32_t x = 0; x + 1 < n; ++x) {
      std::uint32_t corner = z * n + x;
      indices.push_back(corner);
      indices.push_back(corner + n);
      indices.push_back(corner + 1);
      indices.push_back(corner + 1);
      indices.push_back(corner + n);
      indices.push_back(corner + n + 1);
    }

  Grid grid{};
  grid.n          = n;
  grid.indexCount = static_cast<std::uint32_t>(indices.size());
  grid.vertices   = createBuffer(dev, sizeof(glm::vec2) * points.size(),
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  grid.indices    = createBuffer(dev, sizeof(std::uint32_t) * indices.size(),
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  uploadBuffer(grid.vertices, points.data(), sizeof(glm::vec2) * points.size());
  uploadBuffer(grid.indices, indices.data(), sizeof(std::uint32_t) * indices.size());
  return grid;
}

static void destroyGrid(const Device& dev, Grid& grid) {
  destroyBuffer(dev, grid.vertices);
  destroyBuffer(dev, grid.indices);
  grid = Grid{};
}

static VertexLayout gridLayout() {
  VertexLayout layout{};
  layout.binding.binding   = 0;
  layout.binding.stride    = sizeof(glm::vec2);
  layout.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription xz{};
  xz.binding  = 0;
  xz.location = 0;
  xz.format   = VK_FORMAT_R32G32_SFLOAT;
  xz.offset   = 0;
  layout.attrs.push_back(xz);
  return layout;
}

int main(int argc, char** argv) {
  try {
    Args args = parseArgs(argc, argv);

    GLFWwindow* handle =
      args.headless ? nullptr : createHiddenWindow("Lab 07 - Part IV", WIDTH, HEIGHT);

    Device       dev        = createDevice(handle);
    VkRenderPass renderPass = createRenderPass(dev, true);
    Target       target     = createTarget(dev, renderPass, WIDTH, HEIGHT, true);

    Buffer uniform = createBuffer(dev, sizeof(ToyParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    Descriptors      descs  = createDescriptors(dev, uniform);
    VkPipelineLayout layout = createPipelineLayout(dev, descs);

    PipelineState state{};
    state.cullMode     = VK_CULL_MODE_NONE;         // a heightfield has no back faces to cull
    state.depthCompare = VK_COMPARE_OP_GREATER;     // reversed-Z, as in Lab 05
    state.depthWrite   = true;

    Program lifted = createProgram(dev, renderPass, layout, target, "heightmap.vert",
                                   "heightmap.frag", gridLayout(), state);
    Program flat   = createProgram(dev, renderPass, layout, target, "heightmap_flat.vert",
                                   "heightmap_flat.frag", gridLayout(), state);

    Grid grid = createGrid(dev, args.grid > 0 ? args.grid : GRID);

    ToyParams params{};
    params.resolution = glm::vec2(WIDTH, HEIGHT);
    params.time       = args.time;
    params.mode       = args.mode;
    params.octaves    = args.octaves > 0 ? args.octaves : OCTAVES;
    params.knob       = grid.n;
    params.knobf      = AMPLITUDE;
    params.view       = glm::lookAt(glm::vec3(2.2f, 1.6f, 2.2f), glm::vec3(0.0f, 0.15f, 0.0f),
                                    glm::vec3(0.0f, 1.0f, 0.0f));
    // near and far swapped: that is all reversed-Z is. Clear to 0, compare GREATER.
    // Then flip y, because Vulkan's clip space points down and glm's does not
    params.proj       = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 10.0f, 0.1f);
    params.proj[1][1] *= -1.0f;

    VkClearValue clears[2]{};
    clears[0].color        = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clears[1].depthStencil = { 0.0f, 0 };

    VkRenderPassBeginInfo passInfo{};
    passInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass        = renderPass;
    passInfo.framebuffer       = target.framebuffer;
    passInfo.renderArea.extent = { target.width, target.height };
    passInfo.clearValueCount   = 2;
    passInfo.pClearValues      = clears;

    auto draw = [&](VkCommandBuffer cmd, const Program& program) {
      VkDeviceSize offset = 0;
      vkCmdBeginRenderPass(cmd, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program.pipeline);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descs.set,
                              0, nullptr);
      vkCmdBindVertexBuffers(cmd, 0, 1, &grid.vertices.handle, &offset);
      vkCmdBindIndexBuffer(cmd, grid.indices.handle, 0, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(cmd, grid.indexCount, 1, 0, 0, 0);
      vkCmdEndRenderPass(cmd);
    };

    auto record = [&](VkCommandBuffer cmd, const ToyParams&) { draw(cmd, lifted); };

    ToyParams normals = params;
    normals.mode      = 1;
    std::vector<std::uint8_t> normalView = renderOnce(dev, target, uniform, normals, record);

    std::vector<std::uint8_t> rgba = renderOnce(dev, target, uniform, params, record);
    Timing timing = timePass(dev, WARMUP_FRAMES, MEASURED_FRAMES,
                             [&](VkCommandBuffer cmd) { record(cmd, params); });

    double drawn = coverage(rgba);
    double diff  = diffFromReference("heightmap", rgba, target.width, target.height);
    std::vector<std::uint8_t> centre = pixelAt(normalView, target.width, WIDTH / 2, HEIGHT / 2);

    std::cout << std::fixed << std::setprecision(1);
    std::cout << label("shaders") << "heightmap.vert + heightmap.frag, the vertex stage does fbm\n";
    std::cout << label("grid") << "N " << grid.n << ": N*N = " << grid.n * grid.n
              << " vertices, 6*(N-1)^2 = " << grid.indexCount << " indices, "
              << grid.indexCount / 3 << " triangles\n";
    std::cout << label("camera") << "eye (2.2, 1.6, 2.2), 45 deg, reversed-Z: clear 0, "
              << "compare GREATER\n";
    std::cout << label("normals") << "mode 1 centre pixel (" << static_cast<int>(centre[0]) << ", "
              << static_cast<int>(centre[1]) << ", " << static_cast<int>(centre[2])
              << "), flat ground would read (128, 255, 128)\n";
    std::cout << label("coverage") << drawn * 100.0 << "% of the target, "
              << static_cast<std::uint32_t>(drawn * WIDTH * HEIGHT) << " fragments\n";

    std::cout << std::setprecision(3);
    ToyParams sweep = params;
    sweep.mode      = 0;
    sweep.octaves   = OCTAVES;
    // the biggest grid first, thrown away: the first cell of a sweep otherwise
    // measures the GPU clock ramping up rather than the work
    vkDeviceWaitIdle(dev.device);
    destroyGrid(dev, grid);
    grid       = createGrid(dev, SWEEP.back());
    sweep.knob = SWEEP.back();
    uploadBuffer(uniform, &sweep, sizeof(ToyParams));
    timePass(dev, SWEEP_WARMUP, 1, [&](VkCommandBuffer cmd) { draw(cmd, lifted); },
             SWEEP_REPEATS);

    for (std::uint32_t n : SWEEP) {
      vkDeviceWaitIdle(dev.device);
      destroyGrid(dev, grid);
      grid       = createGrid(dev, n);
      sweep.knob = n;
      uploadBuffer(uniform, &sweep, sizeof(ToyParams));

      Timing t = timePass(dev, SWEEP_WARMUP, MEASURED_FRAMES,
                          [&](VkCommandBuffer cmd) { draw(cmd, lifted); }, SWEEP_REPEATS);
      std::cout << label("vertex stage") << "N " << std::setw(3) << n << "  " << std::setw(8)
                << t.medianMs << " ms median, " << std::setw(7) << n * n << " vertices\n";
    }

    vkDeviceWaitIdle(dev.device);
    destroyGrid(dev, grid);
    grid       = createGrid(dev, FLAT_GRID);
    sweep.knob = FLAT_GRID;
    std::vector<std::uint8_t> flatView = renderOnce(dev, target, uniform, sweep,
                                                   [&](VkCommandBuffer cmd, const ToyParams&) {
                                                     draw(cmd, flat);
                                                   });
    Timing flatTiming = timePass(dev, SWEEP_WARMUP, MEASURED_FRAMES,
                                 [&](VkCommandBuffer cmd) { draw(cmd, flat); }, SWEEP_REPEATS);
    double flatDrawn = coverage(flatView);

    std::cout << label("fragment stage") << "N " << std::setw(3) << FLAT_GRID << "  "
              << std::setw(8) << flatTiming.medianMs << " ms median, "
              << static_cast<std::uint32_t>(flatDrawn * WIDTH * HEIGHT)
              << " fragments run the same fbm\n";

    vkDeviceWaitIdle(dev.device);
    destroyGrid(dev, grid);
    grid       = createGrid(dev, params.knob);
    params.knob = grid.n;

    std::cout << label("image") << imageResult(diff) << " at N " << grid.n << "\n";

    if (shaderHasTodo("heightmap.vert") || shaderHasTodo("heightmap.frag"))
      std::cout << label("incomplete") << "TODO(TASK 4) is still in "
                << (shaderHasTodo("heightmap.vert") ? "heightmap.vert " : "")
                << (shaderHasTodo("heightmap.frag") ? "heightmap.frag" : "") << ":\n"
                << std::string(17, ' ') << "the grid is flat, the normals all point straight up\n";

    std::cout << label("gpu time") << timing.medianMs << " ms median, " << timing.p95Ms
              << " ms p95" << std::endl;

    if (!args.save.empty()) writePng(args.save, rgba, target.width, target.height);

    if (!args.headless) {
      Window win = createSwapchain(dev, handle);

      FrameHooks hooks{};
      hooks.record    = record;
      hooks.shaders   = { "heightmap.vert", "heightmap.frag" };
      hooks.savePath  = std::string(REPORT_DIR) + "/heightmap.png";
      hooks.extraKeys = "[ and ] halve and double the grid, which rebuilds the mesh";
      hooks.onReload  = [&](const std::vector<std::string>&) {
        reloadProgram(dev, renderPass, layout, target, lifted);
      };
      hooks.onKey = [&](int key, ToyParams& live) {
        if (key != GLFW_KEY_LEFT_BRACKET && key != GLFW_KEY_RIGHT_BRACKET) return;
        std::uint32_t n = key == GLFW_KEY_LEFT_BRACKET ? std::max(grid.n / 2, 4u)
                                                       : std::min(grid.n * 2, 1024u);
        vkDeviceWaitIdle(dev.device);
        destroyGrid(dev, grid);
        grid      = createGrid(dev, n);
        live.knob = n;
        std::cout << label("grid") << "N " << n << ": " << n * n << " vertices, "
                  << grid.indexCount << " indices" << std::endl;
      };

      runWindow(dev, win, target, uniform, params, args, hooks);
      destroySwapchain(dev, win);
    }

    destroyGrid(dev, grid);
    destroyProgram(dev, flat);
    destroyProgram(dev, lifted);
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
