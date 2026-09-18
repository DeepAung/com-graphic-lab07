#ifndef __FRAME_H_INCLUDED__
#define __FRAME_H_INCLUDED__

#include "core/Pipeline.h"
#include "core/Window.h"
#include "utils/Args.h"
#include "utils/Toy.h"

#include <functional>
#include <string>

// A vertex shader, a fragment shader and the pipeline built from them. Kept together
// so that hot reload is one call: recompile, rebuild, keep drawing.
struct Program {
  std::string    vertName;
  std::string    fragName;
  VertexLayout   vertexLayout;
  PipelineState  state;
  VkShaderModule vert     = VK_NULL_HANDLE;
  VkShaderModule frag     = VK_NULL_HANDLE;
  VkPipeline     pipeline = VK_NULL_HANDLE;
};

Program createProgram(const Device& dev,
                      VkRenderPass renderPass,
                      VkPipelineLayout layout,
                      const Target& target,
                      const std::string& vertName,
                      const std::string& fragName,
                      const VertexLayout& vertexLayout = VertexLayout{},
                      const PipelineState& state = PipelineState{});

// Same two names, freshly compiled modules, new pipeline. The caller has already
// waited for the device to go idle.
void reloadProgram(const Device& dev, VkRenderPass renderPass, VkPipelineLayout layout,
                   const Target& target, Program& program);

void destroyProgram(const Device& dev, Program& program);

// What a part hands to the window loop. record() draws one frame with the uniform
// block it is given; everything else is optional.
struct FrameHooks {
  std::function<void(VkCommandBuffer, const ToyParams&)> record;
  std::vector<std::string>                               shaders;    // recompiled on a save
  std::function<void(const std::vector<std::string>&)>   onReload;   // rebuild the pipelines
  std::function<void(int, ToyParams&)>                   onKey;      // keys this part adds
  std::string                                            extraKeys;  // one line for the legend
  std::string                                            savePath;   // where S writes a png
};

// Upload the uniform block, submit one frame, read the target back
std::vector<std::uint8_t> renderOnce(const Device& dev,
                                     const Target& target,
                                     const Buffer& uniform,
                                     const ToyParams& params,
                                     const std::function<void(VkCommandBuffer, const ToyParams&)>& record);

// The interactive loop: clock, mouse, keys, hot reload, present. Returns when the
// window is closed. Never called with --headless.
void runWindow(const Device& dev,
               const Window& win,
               const Target& target,
               const Buffer& uniform,
               ToyParams& params,
               const Args& args,
               const FrameHooks& hooks);

#endif
