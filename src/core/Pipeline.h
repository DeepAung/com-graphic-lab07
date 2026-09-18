#ifndef __PIPELINE_H_INCLUDED__
#define __PIPELINE_H_INCLUDED__

#include "core/Buffer.h"
#include "core/Target.h"

#include <string>

struct Descriptors {
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  VkDescriptorPool      pool   = VK_NULL_HANDLE;
  VkDescriptorSet       set    = VK_NULL_HANDLE;
};

// Empty means no vertex buffers at all, which is what the fullscreen triangle wants
struct VertexLayout {
  VkVertexInputBindingDescription                binding{};
  std::vector<VkVertexInputAttributeDescription> attrs;

  bool empty() const { return attrs.empty(); }
};

struct PipelineState {
  VkFrontFace         frontFace    = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  VkCullModeFlags     cullMode     = VK_CULL_MODE_NONE;
  VkCompareOp         depthCompare = VK_COMPARE_OP_NEVER;   // NEVER means "no depth test"
  bool                depthWrite   = false;
  VkPrimitiveTopology topology     = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
};

VkRenderPass createRenderPass(const Device& dev, bool withDepth = false);
void         destroyRenderPass(const Device& dev, VkRenderPass renderPass);

Descriptors createDescriptors(const Device& dev, const Buffer& uniform);
void        destroyDescriptors(const Device& dev, Descriptors& descs);

VkPipelineLayout createPipelineLayout(const Device& dev, const Descriptors& descs,
                                      std::uint32_t pushSize = 0);

VkPipeline createPipeline(const Device& dev,
                          VkRenderPass renderPass,
                          VkPipelineLayout layout,
                          VkShaderModule vertShader,
                          VkShaderModule fragShader,
                          const Target& target,
                          const VertexLayout& vertexLayout = VertexLayout{},
                          const PipelineState& state = PipelineState{});

#endif
