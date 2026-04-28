#ifndef RAYMEOWER_VK_H
#define RAYMEOWER_VK_H
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <assert.h>
#include "MeowMath.h"
#include "nkui.h"
#define error(a) ({printf(a); abort();})

static VkInstance instance;
static VkPhysicalDevice physical_device;
static VkPhysicalDeviceMemoryProperties mem_props;
static VkDevice device;
static VkQueue queue;
static unsigned long device_index = 0;
static VkCommandPool cmdPool;
static VkDescriptorSetLayout setLayout;
static VkPipeline pipeline;
static VkDescriptorSet descriptorSet;
static VkPipelineLayout pipelineLayout;
static struct Vec3 *frameBuffer;
static struct MaterialGpu *gpuMaterials;
static struct PointLights *gpuPointLights;
static int lastTextureIndex = 0;

static uint32_t shaderCode[] = {
#include "../shaders/compiled/raytracer.comp.spv.h"
};

struct PushConstants {
   int width;
   int height;
   struct Vec3 cameraPos;
   struct Mat3 rotmat;
   uint32_t time;
   int depth;
   struct Sun sun;
   struct Vec3 skyColor;
   float cumFact;
};

static void InitVk() {
   uint32_t api_version = VK_API_VERSION_1_0;

   // use Vulkan 1.1 if supported
   uint32_t instance_version = api_version;
   VkResult res = vkEnumerateInstanceVersion(&instance_version);
   if (res == VK_SUCCESS && instance_version >= VK_API_VERSION_1_1)
      api_version = VK_API_VERSION_1_1;

   VkInstanceCreateFlags instance_flags = 0;

   // Instance extensions to use
   const char *inst_exts[3];

   // Look for optional instance extensions
   uint32_t inst_ext_props_count = 0;
   res = vkEnumerateInstanceExtensionProperties(NULL, &inst_ext_props_count,
                                                NULL);
   if (res != VK_SUCCESS)
      error("Failed to get instance extensions properties count");

   VkExtensionProperties *inst_ext_props =
      calloc(inst_ext_props_count, sizeof(VkExtensionProperties));
   if (!inst_ext_props)
      error("Failed to allocate extension properties");

   res = vkEnumerateInstanceExtensionProperties(NULL, &inst_ext_props_count,
                                                inst_ext_props);
   if (res != VK_SUCCESS)
      error("Failed to get instance extensions properties");

   uint32_t count = 0;
   free(inst_ext_props);

   res = vkCreateInstance(
      &(VkInstanceCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
         .flags = instance_flags,
         .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "vkgears",
            .apiVersion = api_version,
         },
         .enabledExtensionCount = count,
         .ppEnabledExtensionNames = inst_exts,
      },
      NULL,
      &instance);

   if (res != VK_SUCCESS)
      error("Failed to create Vulkan instance.");

   res = vkEnumeratePhysicalDevices(instance, &count, NULL);
   if (res != VK_SUCCESS)
      error("Failed to enumerate physical devices.");

   if (count == 0)
      error("No Vulkan devices found.");

   VkPhysicalDevice *physical_devices = calloc(count, sizeof(VkPhysicalDevice));
   if (!physical_devices)
      error("Failed to allocate physical devices.");

   res = vkEnumeratePhysicalDevices(instance, &count, physical_devices);
   if (res != VK_SUCCESS)
      error("Failed to enumerate physical devices.");

   if (device_index >= count)
      error("Invalid device");

   physical_device = physical_devices[device_index];
   free(physical_devices);

   vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

   count = 1;
   VkQueueFamilyProperties props;
   vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, &props);
   assert(props.queueFlags & VK_QUEUE_GRAPHICS_BIT);

   res = vkCreateDevice(physical_device,
      &(VkDeviceCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
         .queueCreateInfoCount = 1,
         .pQueueCreateInfos = &(VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .flags = 0,
            .pQueuePriorities = (float []) { 1.0f },
         },
         .enabledExtensionCount = 1,
         .ppEnabledExtensionNames = (const char * const []) {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
         },
      },
      NULL,
      &device);

   if (res != VK_SUCCESS)
      error("Failed to create Vulkan device.");

   vkGetDeviceQueue(device, 0, 0, &queue);

   vkCreateCommandPool(device,
      &(const VkCommandPoolCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
         .queueFamilyIndex = 0,
         .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
      },
      NULL,
      &cmdPool);
}

static int
find_memory_type(const VkMemoryRequirements *reqs,
                 VkMemoryPropertyFlags flags)
{
    for (unsigned i = 0; (1u << i) <= reqs->memoryTypeBits &&
                         i <= mem_props.memoryTypeCount; ++i) {
        if ((reqs->memoryTypeBits & (1u << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    }
    return -1;
}

static int ImageAllocate(VkImage image, VkMemoryRequirements reqs, int memory_type,
               VkDeviceMemory *image_memory)
{
   int res = vkAllocateMemory(device,
      &(VkMemoryAllocateInfo) {
         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
         .allocationSize = reqs.size,
         .memoryTypeIndex = memory_type,
      },
      NULL,
      image_memory);
   if (res != VK_SUCCESS)
      return -1;

   res = vkBindImageMemory(device, image, *image_memory, 0);
   if (res != VK_SUCCESS)
      return -1;

   return 0;
}

static int CreateImage(VkFormat format,
             VkExtent3D extent,
             VkSampleCountFlagBits samples,
             VkImageUsageFlags usage,
             VkImage *image)
{
   VkResult res = vkCreateImage(device,
      &(VkImageCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
         .flags = 0,
         .imageType = VK_IMAGE_TYPE_2D,
         .format = format,
         .extent = extent,
         .mipLevels = 1,
         .arrayLayers = 1,
         .samples = samples,
         .tiling = VK_IMAGE_TILING_OPTIMAL,
         .usage = usage,
		   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      }, 0, image);
   if (res != VK_SUCCESS)
      return -1;

   return 0;
}

static int
CreateImageView(VkImage image,
                  VkFormat view_format,
                  VkImageAspectFlags aspect_mask,
                  VkImageView *image_view)
{
   int res = vkCreateImageView(device,
      &(VkImageViewCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
         .image = image,
         .viewType = VK_IMAGE_VIEW_TYPE_2D,
         .format = view_format,
         .components = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
         },
         .subresourceRange = {
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
         },
      },
      NULL,
      image_view);

   if(res != VK_SUCCESS)
      return -1;
   return 0;
}

static int CreateBuffer(int size, VkBufferUsageFlags usage, VkBuffer *buffer, void **pp, bool deviceLocal) {
   VkResult res = vkCreateBuffer(device,
      &(VkBufferCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
         .flags = 0,
         .usage = usage,
         .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
         .size = size,
      }, 0, buffer);
   if (res != VK_SUCCESS)
      return -1;

   VkMemoryRequirements mem_reqs;
   vkGetBufferMemoryRequirements(device, *buffer, &mem_reqs);
   VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   if (deviceLocal) {
      memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   }
   else {
      memFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
   }
   int memoryIndex = find_memory_type(&mem_reqs, memFlags);
   if (memoryIndex == -1)
      return -1;

   VkDeviceMemory memory;
   res = vkAllocateMemory(device, &(VkMemoryAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = NULL,
      .allocationSize = mem_reqs.size,
      .memoryTypeIndex = memoryIndex,
   }, NULL, &memory);
   if (res != VK_SUCCESS)
      return -1;

   res = vkMapMemory(device, memory, 0, mem_reqs.size, 0, pp);
   if (memoryIndex == -1)
      return -1;

   res = vkBindBufferMemory(device, *buffer, memory, 0);
   if (res != VK_SUCCESS)
      return -1;

   return 0;
}

static VkResult CreatePipeline() {
   VkResult res;
   VkShaderModule shaderModule;
   res = vkCreateShaderModule(device,
      &(VkShaderModuleCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
         .codeSize = sizeof(shaderCode),
         .pCode = shaderCode,
      }, NULL, &shaderModule);
   if (res != VK_SUCCESS) {
      return -1;
   }

   res = vkCreateDescriptorSetLayout(device, &(VkDescriptorSetLayoutCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT,
      .pNext = NULL,
      .bindingCount = 6,
      .pBindings = (VkDescriptorSetLayoutBinding[]) {
         {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
         },
         {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
         },
         {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
         },
         {
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1024,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
         },
         {
            .binding = 4,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 2,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
         },
         {
            .binding = 5,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
         },
}
   }, NULL, &setLayout);
   if (res != VK_SUCCESS) {
      return -1;
   }

   res = vkCreatePipelineLayout(device, &(VkPipelineLayoutCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .flags = 0,
      .pNext = NULL,
      .pSetLayouts = &setLayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = (VkPushConstantRange[]) {
         {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(struct PushConstants)
         }}
   }, NULL, &pipelineLayout);
   if (res != VK_SUCCESS) {
      return -1;
   }

   res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
      &(VkComputePipelineCreateInfo) {
         .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
         .flags = 0,
         .stage = (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .flags = 0,
            .pNext = NULL,
            .module = shaderModule,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .pName = "main",
            .pSpecializationInfo = NULL,
         },
         .layout = pipelineLayout,
      }, NULL, &pipeline);
   if (res != VK_SUCCESS) {
      return -1;
   }
   return res;
}

static VkResult CreateDescriptorSet(VkBuffer buffer) {
   VkResult res;
   VkDescriptorPool descriptorPool;
   res = vkCreateDescriptorPool(device, &(VkDescriptorPoolCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = 0,
      .maxSets = 1,
      .poolSizeCount = 3,
      .pNext = NULL,
      .pPoolSizes = (const VkDescriptorPoolSize[]) {
         {.descriptorCount = 4, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
         {.descriptorCount = 1024, .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
         {.descriptorCount = 2, .type = VK_DESCRIPTOR_TYPE_SAMPLER},
      }
   }, NULL, &descriptorPool);
   if (res != VK_SUCCESS)
      return -1;

   res = vkAllocateDescriptorSets(device, &(VkDescriptorSetAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptorPool,
      .descriptorSetCount = 1,
      .pNext = NULL,
      .pSetLayouts = &setLayout,
   }, &descriptorSet);
   if (res != VK_SUCCESS)
      return -1;

   vkUpdateDescriptorSets(device, 1, &(const VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptorSet,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .pNext = NULL,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &(VkDescriptorBufferInfo) {
         .buffer = buffer,
         .offset = 0,
         .range = VK_WHOLE_SIZE,
      },
      .pImageInfo = NULL,
      .pTexelBufferView = NULL,
   }, 0, NULL);

   return res;
}

static VkResult AllocateCommandBuffer(VkCommandBuffer *cmdBuffer, int width, int height, struct Vec3 cameraPos, struct Mat3 rotMat, uint32_t time, struct Scene *scene, struct Settings *settings) {
   VkResult res;
   res = vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = NULL,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandPool = cmdPool,
      .commandBufferCount = 1,
   }, cmdBuffer);
   if (res != VK_SUCCESS)
      return -1;

   vkBeginCommandBuffer(*cmdBuffer, &(VkCommandBufferBeginInfo) {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = NULL,
   });

   vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
   vkCmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

   struct PushConstants pc = {width, height, cameraPos, rotMat, time, settings->depth, scene->sun, settings->skyColor, settings->cumFact};
   vkCmdPushConstants(*cmdBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

   vkCmdDispatch(*cmdBuffer, width/8, height/8, 1);

   vkEndCommandBuffer(*cmdBuffer);

   return res;
}

static VkResult SubmitCommandBuffer(VkCommandBuffer cmdBuffer) {
   VkResult res;
   VkFence fence;
   res = vkCreateFence(device, &(VkFenceCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
   }, NULL, &fence);
   if (res != VK_SUCCESS)
      return -1;

   res = vkQueueSubmit(queue, 1, &(VkSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = NULL,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = NULL,
      .pWaitDstStageMask = NULL,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmdBuffer,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = NULL
   }, fence);
   if (res != VK_SUCCESS)
      return -1;

   res = vkWaitForFences(device, 1, &fence, true, UINT64_MAX);
   if (res != VK_SUCCESS)
      return -1;
   vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
   return res;
}

static VkResult UpdateBvhBuffer(VkBuffer buffer) {
   vkUpdateDescriptorSets(device, 1, &(const VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptorSet,
      .dstBinding = 1,
      .dstArrayElement = 0,
      .pNext = NULL,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &(VkDescriptorBufferInfo) {
         .buffer = buffer,
         .offset = 0,
         .range = VK_WHOLE_SIZE,
      },
      .pImageInfo = NULL,
      .pTexelBufferView = NULL,
   }, 0, NULL);
}

VkResult CreateSamplers() {
   VkSampler samplers[2];
   VkResult res;
   res = vkCreateSampler(device, &(VkSamplerCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .anisotropyEnable = false,
      .maxAnisotropy = 16,
      .compareEnable = false,
      .flags = 0,
      .pNext = NULL
   }, NULL, &samplers[0]);
   if (res != VK_SUCCESS)
      return -1;

   res = vkCreateSampler(device, &(VkSamplerCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .anisotropyEnable = false,
      .maxAnisotropy = 16,
      .compareEnable = false,
      .flags = 0,
      .pNext = NULL
   }, NULL, &samplers[1]);
   if (res != VK_SUCCESS)
      return -1;

   vkUpdateDescriptorSets(device, 1, &(const VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptorSet,
      .dstBinding = 4,
      .dstArrayElement = 0,
      .pNext = NULL,
      .descriptorCount = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
      .pBufferInfo = NULL,
      .pImageInfo = (VkDescriptorImageInfo[]) {
         {
            .sampler = samplers[0],
            .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .imageView = VK_NULL_HANDLE,
         },
         {
            .sampler = samplers[1],
            .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .imageView = VK_NULL_HANDLE,
         }
      },
      .pTexelBufferView = NULL,
   }, 0, NULL);

   return res;
}

int AddTexture(SDL_Surface *surface) {
   VkExtent3D extent;
   extent.width = surface->w;
   extent.height = surface->h;
   extent.depth = 1;

   const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
   int pixelSize = details->bytes_per_pixel;
   VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
   if (pixelSize == 1) {
      format = VK_FORMAT_R8_UNORM;
   }

   VkImage image;
   CreateImage(format, extent, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, &image);
   VkMemoryRequirements reqs;
   vkGetImageMemoryRequirements(device, image, &reqs);
   VkDeviceMemory memory;
   ImageAllocate(image, reqs, find_memory_type(&reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), &memory);

   VkImageView view;
   CreateImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT, &view);
   vkUpdateDescriptorSets(device, 1, &(const VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptorSet,
      .dstBinding = 3,
      .dstArrayElement = lastTextureIndex,
      .pNext = NULL,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
      .pBufferInfo = NULL,
      .pImageInfo = (VkDescriptorImageInfo[]) {
         {
            .sampler = VK_NULL_HANDLE,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            .imageView = view
         }
      },
      .pTexelBufferView = NULL,
   }, 0, NULL);
   lastTextureIndex++;


   VkBuffer buffer;
   void *texturedata;
   CreateBuffer(surface->pitch * surface->h * pixelSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &buffer, &texturedata, false);
   memcpy(texturedata, surface->pixels, surface->pitch * surface->h * pixelSize);
   // TODO: Deallocate buffer

   VkResult res;
   VkCommandBuffer cmdBuffer;
   res = vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = NULL,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandPool = cmdPool,
      .commandBufferCount = 1,
   }, &cmdBuffer);
   if (res != VK_SUCCESS)
      return -1;

   vkBeginCommandBuffer(cmdBuffer, &(VkCommandBufferBeginInfo) {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = NULL,
   });

   vkCmdPipelineBarrier(
      cmdBuffer,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0,
      NULL,
      0,
      NULL,
      1,
      &(const VkImageMemoryBarrier) {
         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         .pNext = NULL,
         .image = image,
         .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
         .newLayout = VK_IMAGE_LAYOUT_GENERAL,
         .srcAccessMask = VK_ACCESS_NONE,
         .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
         .subresourceRange = (VkImageSubresourceRange) {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
            .levelCount = 1,
         }
      }
   );

   vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_GENERAL, 1, &(VkBufferImageCopy) {
      .bufferImageHeight = surface->h,
      .bufferRowLength = surface->pitch/pixelSize, // TODO: should never round
      .bufferOffset = 0,
      .imageExtent = extent,
      .imageOffset = 0,
      .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .imageSubresource.mipLevel = 0,
      .imageSubresource.baseArrayLayer = 0,
      .imageSubresource.layerCount = 1,
   });

   vkCmdPipelineBarrier(
      cmdBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      0,
      0,
      NULL,
      0,
      NULL,
      1,
      &(const VkImageMemoryBarrier) {
         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         .pNext = NULL,
         .image = image,
         .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
         .newLayout = VK_IMAGE_LAYOUT_GENERAL,
         .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
         .dstAccessMask = VK_ACCESS_NONE,
         .subresourceRange = (VkImageSubresourceRange) {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
            .levelCount = 1,
         }
      }
   );

   vkEndCommandBuffer(cmdBuffer);
   SubmitCommandBuffer(cmdBuffer);

   return lastTextureIndex - 1;
}

VkResult UploadMaterials(struct Material *materials, int materialCount) {
   VkResult res;
   VkBuffer buffer;
   res = CreateBuffer(sizeof(struct MaterialGpu) * materialCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &buffer, (void**)&gpuMaterials ,true);
   if (res != VK_SUCCESS)
      return -1;

   for (int i = 0; i < materialCount; i++) {
      gpuMaterials[i].color = materials[i].color;
      gpuMaterials[i].emissionColor = materials[i].emissionColor;
      gpuMaterials[i].emissionIntensity = materials[i].emissionIntensity;
      gpuMaterials[i].enableNormalMap = materials[i].enableNormalMap;
      gpuMaterials[i].enableRoughnessMap = materials[i].enableRoughnessMap;
      gpuMaterials[i].enableTexture = materials[i].enableTexture;
      gpuMaterials[i].ior = materials[i].ior;
      gpuMaterials[i].metallic = materials[i].metallic;
      gpuMaterials[i].normalMap = materials[i].normalMap != NULL ? AddTexture(materials[i].normalMap) : -1;
      gpuMaterials[i].roughness = materials[i].roughness;
      gpuMaterials[i].roughnessMap = materials[i].roughnessMap != NULL ? AddTexture(materials[i].roughnessMap) : -1;;
      gpuMaterials[i].texture = materials[i].texture != NULL ? AddTexture(materials[i].texture) : -1;
      gpuMaterials[i].transmissive = materials[i].transmissive;
   }

   vkUpdateDescriptorSets(device, 1, &(const VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptorSet,
      .dstBinding = 2,
      .dstArrayElement = 0,
      .pNext = NULL,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &(VkDescriptorBufferInfo) {
         .buffer = buffer,
         .offset = 0,
         .range = VK_WHOLE_SIZE,
      },
      .pImageInfo = NULL,
      .pTexelBufferView = NULL,
   }, 0, NULL);
   return res;
}

VkResult UploadPointLights(struct PointLight *pointLights, int lightCount) {
   VkResult res;
   VkBuffer buffer;
   res = CreateBuffer(sizeof(int) + sizeof(struct PointLight) * 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &buffer, (void**)&gpuPointLights ,true);
   if (res != VK_SUCCESS)
      return -1;

   gpuPointLights->count = lightCount;
   for (int i = 0; i < lightCount; i++) {
      gpuPointLights->lights[i] = pointLights[i];
   }

   vkUpdateDescriptorSets(device, 1, &(const VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptorSet,
      .dstBinding = 5,
      .dstArrayElement = 0,
      .pNext = NULL,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &(VkDescriptorBufferInfo) {
         .buffer = buffer,
         .offset = 0,
         .range = VK_WHOLE_SIZE,
      },
      .pImageInfo = NULL,
      .pTexelBufferView = NULL,
   }, 0, NULL);
   return res;
}

int CreateVk(int width, int height) {
   InitVk();
   //VkImage image;
   //create_image(VK_FORMAT_R8G8B8A8_SRGB, (VkExtent3D) {256, 256, 1}, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT, &image);

   VkBuffer buffer;
   CreateBuffer(width * height * sizeof(struct Vec3), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &buffer, (void**)&frameBuffer, false);
   CreatePipeline();
   CreateDescriptorSet(buffer);
   CreateSamplers();
   return 0;
}

int RunVk(int width, int height, struct Vec3 cameraPos, struct Mat3 rotMat, uint32_t time, struct Scene *scene, struct Settings *settings) {
   VkCommandBuffer commandBuffer;
   AllocateCommandBuffer(&commandBuffer, width, height, cameraPos, rotMat, time, scene, settings);

   SubmitCommandBuffer(commandBuffer);
   return 0;
}

#endif //RAYMEOWER_VK_H