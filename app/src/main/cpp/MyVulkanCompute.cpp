//#include <jni.h>
//#include <android/log.h>
//#include <vulkan/vulkan.h>
//
//#include <vector>
//#include <string>
//#include <fstream>
//#include <cassert>
//#include <cstring>    // for memcpy
//
//// Macros for logging to Logcat
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "VulkanCompute", __VA_ARGS__)
//#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VulkanCompute", __VA_ARGS__)
//
//// A helper macro to check VkResult in a minimal way
//#define VK_CHECK(x) do {                         \
//    VkResult err = x;                            \
//    if (err != VK_SUCCESS) {                     \
//        LOGE("Detected Vulkan error: %d", err);  \
//        assert(false);                           \
//    }                                            \
//} while(0)
//
//// -----------------------------------------------------------------------------
//// Global context storing Vulkan objects used in compute
//// (For simplicity, we keep them all in static variables. In production, you'd
////  manage them in a class or struct with proper lifecycle management.)
//// -----------------------------------------------------------------------------
//
//static VkInstance          g_Instance         = VK_NULL_HANDLE;
//static VkPhysicalDevice    g_PhysicalDevice   = VK_NULL_HANDLE;
//static VkDevice            g_Device           = VK_NULL_HANDLE;
//static VkQueue             g_ComputeQueue     = VK_NULL_HANDLE;
//static uint32_t            g_ComputeQueueFamilyIndex = 0;
//
//static VkCommandPool       g_CommandPool      = VK_NULL_HANDLE;
//static VkCommandBuffer     g_CommandBuffer    = VK_NULL_HANDLE;
//
//static VkDescriptorSetLayout g_DescriptorSetLayout = VK_NULL_HANDLE;
//static VkPipelineLayout      g_PipelineLayout      = VK_NULL_HANDLE;
//static VkPipeline            g_ComputePipeline     = VK_NULL_HANDLE;
//
//static VkDescriptorPool      g_DescriptorPool      = VK_NULL_HANDLE;
//static VkDescriptorSet       g_DescriptorSet       = VK_NULL_HANDLE;
//
//// We'll store the loaded shader module in a global for re-use
//static VkShaderModule        g_ComputeShaderModule = VK_NULL_HANDLE;
//
//// -----------------------------------------------------------------------------
//// Helper: Load SPIR-V from file
//// -----------------------------------------------------------------------------
//static std::vector<char> loadFile(const std::string &filename) {
//    std::ifstream file(filename, std::ios::ate | std::ios::binary);
//    if (!file.is_open()) {
//        LOGE("Failed to open SPIR-V file: %s", filename.c_str());
//        return {};
//    }
//    size_t fileSize = (size_t) file.tellg();
//    std::vector<char> buffer(fileSize);
//    file.seekg(0);
//    file.read(buffer.data(), fileSize);
//    file.close();
//    return buffer;
//}
//
//// -----------------------------------------------------------------------------
//// Helper: Create a shader module from SPIR-V code
//// -----------------------------------------------------------------------------
//static VkShaderModule createShaderModule(const std::vector<char>& code) {
//    VkShaderModuleCreateInfo createInfo = {};
//    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//    createInfo.codeSize = code.size();
//    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());
//
//    VkShaderModule shaderModule;
//    VK_CHECK(vkCreateShaderModule(g_Device, &createInfo, nullptr, &shaderModule));
//    return shaderModule;
//}
//
//// -----------------------------------------------------------------------------
//// Helper: Pick a physical device that has a compute queue
//// -----------------------------------------------------------------------------
//static void pickPhysicalDevice() {
//    uint32_t deviceCount = 0;
//    vkEnumeratePhysicalDevices(g_Instance, &deviceCount, nullptr);
//    assert(deviceCount > 0);
//
//    std::vector<VkPhysicalDevice> devices(deviceCount);
//    vkEnumeratePhysicalDevices(g_Instance, &deviceCount, devices.data());
//
//    // Just pick the first device that has a compute queue
//    for (auto& d : devices) {
//        VkPhysicalDeviceProperties props;
//        vkGetPhysicalDeviceProperties(d, &props);
//
//        // Find a queue with compute
//        uint32_t queueFamilyCount = 0;
//        vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, nullptr);
//        std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
//        vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, families.data());
//
//        for (uint32_t i = 0; i < queueFamilyCount; i++) {
//            if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
//                g_PhysicalDevice = d;
//                g_ComputeQueueFamilyIndex = i;
//                LOGI("Using device: %s", props.deviceName);
//                return;
//            }
//        }
//    }
//
//    LOGE("No suitable physical device found for compute!");
//    assert(false);
//}
//
//// -----------------------------------------------------------------------------
//// Helper: Create a 2D image with host-visible memory (for demonstration).
//// WARNING: Not all devices support host-visible images in optimal form.
//// -----------------------------------------------------------------------------
//struct SimpleImage {
//    VkImage       image  = VK_NULL_HANDLE;
//    VkDeviceMemory memory = VK_NULL_HANDLE;
//    VkImageView   view   = VK_NULL_HANDLE;
//    int           width;
//    int           height;
//};
//
//static void createImage2D(int width, int height, SimpleImage& outImage) {
//    outImage.width  = width;
//    outImage.height = height;
//
//    // Create the image
//    VkImageCreateInfo imageInfo = {};
//    imageInfo.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//    imageInfo.imageType   = VK_IMAGE_TYPE_2D;
//    imageInfo.extent.width  = width;
//    imageInfo.extent.height = height;
//    imageInfo.extent.depth  = 1;
//    imageInfo.mipLevels   = 1;
//    imageInfo.arrayLayers = 1;
//    imageInfo.format      = VK_FORMAT_R8G8B8A8_UNORM;
//    imageInfo.tiling      = VK_IMAGE_TILING_LINEAR; // host-visible layout
//    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
//    imageInfo.usage       = VK_IMAGE_USAGE_STORAGE_BIT;
//    imageInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
//    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//    VK_CHECK(vkCreateImage(g_Device, &imageInfo, nullptr, &outImage.image));
//
//    // Allocate memory for the image
//    VkMemoryRequirements memReq;
//    vkGetImageMemoryRequirements(g_Device, outImage.image, &memReq);
//
//    // Find a memory type that is HOST_VISIBLE
//    // For simplicity, we do not handle all cases
//    VkPhysicalDeviceMemoryProperties memProperties;
//    vkGetPhysicalDeviceMemoryProperties(g_PhysicalDevice, &memProperties);
//
//    uint32_t memoryTypeIndex = UINT32_MAX;
//    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
//        if ((memReq.memoryTypeBits & (1 << i)) &&
//            (memProperties.memoryTypes[i].propertyFlags &
//             (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
//            memoryTypeIndex = i;
//            break;
//        }
//    }
//    assert(memoryTypeIndex != UINT32_MAX);
//
//    VkMemoryAllocateInfo allocInfo = {};
//    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//    allocInfo.allocationSize  = memReq.size;
//    allocInfo.memoryTypeIndex = memoryTypeIndex;
//
//    VK_CHECK(vkAllocateMemory(g_Device, &allocInfo, nullptr, &outImage.memory));
//    VK_CHECK(vkBindImageMemory(g_Device, outImage.image, outImage.memory, 0));
//
//    // Create an image view
//    VkImageViewCreateInfo viewInfo = {};
//    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//    viewInfo.image    = outImage.image;
//    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//    viewInfo.format   = VK_FORMAT_R8G8B8A8_UNORM;
//    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
//    viewInfo.subresourceRange.baseMipLevel   = 0;
//    viewInfo.subresourceRange.levelCount     = 1;
//    viewInfo.subresourceRange.baseArrayLayer = 0;
//    viewInfo.subresourceRange.layerCount     = 1;
//    VK_CHECK(vkCreateImageView(g_Device, &viewInfo, nullptr, &outImage.view));
//}
//
//// -----------------------------------------------------------------------------
//// Create descriptor set layout, pipeline layout, compute pipeline
//// -----------------------------------------------------------------------------
//static void createComputePipeline(const std::string& shaderSpvPath) {
//    // 1) Load SPIR-V
//    auto spvCode = loadFile(shaderSpvPath);
//    assert(!spvCode.empty());
//    g_ComputeShaderModule = createShaderModule(spvCode);
//
//    // 2) Create descriptor set layout:
//    //    We have 2 bindings: binding=0 inputImage (readOnly), binding=1 outputImage (writeOnly).
//    VkDescriptorSetLayoutBinding bindings[2] = {};
//    bindings[0].binding         = 0;
//    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//    bindings[0].descriptorCount = 1;
//    bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
//
//    bindings[1].binding         = 1;
//    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//    bindings[1].descriptorCount = 1;
//    bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
//
//    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
//    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    layoutInfo.bindingCount = 2;
//    layoutInfo.pBindings    = bindings;
//
//    VK_CHECK(vkCreateDescriptorSetLayout(g_Device, &layoutInfo, nullptr, &g_DescriptorSetLayout));
//
//    // 3) Create pipeline layout (with our descriptor set layout)
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
//    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount         = 1;
//    pipelineLayoutInfo.pSetLayouts            = &g_DescriptorSetLayout;
//    VK_CHECK(vkCreatePipelineLayout(g_Device, &pipelineLayoutInfo, nullptr, &g_PipelineLayout));
//
//    // 4) Create compute pipeline
//    VkPipelineShaderStageCreateInfo stageInfo = {};
//    stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    stageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
//    stageInfo.module = g_ComputeShaderModule;
//    stageInfo.pName  = "main"; // entry point
//
//    VkComputePipelineCreateInfo computeCreateInfo = {};
//    computeCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
//    computeCreateInfo.stage  = stageInfo;
//    computeCreateInfo.layout = g_PipelineLayout;
//
//    VK_CHECK(vkCreateComputePipelines(g_Device, VK_NULL_HANDLE, 1, &computeCreateInfo, nullptr, &g_ComputePipeline));
//}
//
//// -----------------------------------------------------------------------------
//// JNI: initVulkan - Creates instance, device, queue, pipeline, etc.
//// -----------------------------------------------------------------------------
//extern "C"
//JNIEXPORT void JNICALL
//Java_com_ecse420_parallelcompute_NativeVulkan_initVulkan(JNIEnv* env, jclass /*clazz*/, jstring jShaderPath)
//{
//    // 1) Create Vulkan instance
//    {
//        VkApplicationInfo appInfo = {};
//        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
//        appInfo.pApplicationName   = "VulkanCompute";
//        appInfo.applicationVersion = 1;
//        appInfo.pEngineName        = "NoEngine";
//        appInfo.engineVersion      = 1;
//        appInfo.apiVersion         = VK_API_VERSION_1_0;
//
//        VkInstanceCreateInfo createInfo = {};
//        createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//        createInfo.pApplicationInfo = &appInfo;
//
//        // No extensions/layers for brevity
//        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &g_Instance));
//    }
//
//    // 2) Pick physical device w/ compute queue
//    pickPhysicalDevice();
//
//    // 3) Create logical device + get compute queue
//    {
//        float queuePriority = 1.0f;
//        VkDeviceQueueCreateInfo queueCreateInfo = {};
//        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//        queueCreateInfo.queueFamilyIndex = g_ComputeQueueFamilyIndex;
//        queueCreateInfo.queueCount       = 1;
//        queueCreateInfo.pQueuePriorities = &queuePriority;
//
//        VkDeviceCreateInfo deviceCreateInfo = {};
//        deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//        deviceCreateInfo.queueCreateInfoCount    = 1;
//        deviceCreateInfo.pQueueCreateInfos       = &queueCreateInfo;
//        // no extra features or layers for brevity
//
//        VK_CHECK(vkCreateDevice(g_PhysicalDevice, &deviceCreateInfo, nullptr, &g_Device));
//        vkGetDeviceQueue(g_Device, g_ComputeQueueFamilyIndex, 0, &g_ComputeQueue);
//    }
//
//    // 4) Create a command pool & allocate one command buffer
//    {
//        VkCommandPoolCreateInfo poolInfo = {};
//        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//        poolInfo.queueFamilyIndex = g_ComputeQueueFamilyIndex;
//        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//
//        VK_CHECK(vkCreateCommandPool(g_Device, &poolInfo, nullptr, &g_CommandPool));
//
//        VkCommandBufferAllocateInfo allocInfo = {};
//        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//        allocInfo.commandPool        = g_CommandPool;
//        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//        allocInfo.commandBufferCount = 1;
//        VK_CHECK(vkAllocateCommandBuffers(g_Device, &allocInfo, &g_CommandBuffer));
//    }
//
//    // 5) Convert jstring to std::string
//    const char* nativePath = env->GetStringUTFChars(jShaderPath, nullptr);
//    std::string shaderPath(nativePath);
//    env->ReleaseStringUTFChars(jShaderPath, nativePath);
//
//    // 6) Create compute pipeline
//    createComputePipeline(shaderPath);
//
//    // 7) Create descriptor pool for 1 set
//    {
//        VkDescriptorPoolSize poolSize = {};
//        poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//        poolSize.descriptorCount = 2; // input + output
//
//        VkDescriptorPoolCreateInfo poolInfo = {};
//        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//        poolInfo.maxSets       = 1;
//        poolInfo.poolSizeCount = 1;
//        poolInfo.pPoolSizes    = &poolSize;
//
//        VK_CHECK(vkCreateDescriptorPool(g_Device, &poolInfo, nullptr, &g_DescriptorPool));
//    }
//
//    // 8) Allocate the descriptor set
//    {
//        VkDescriptorSetAllocateInfo allocInfo = {};
//        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//        allocInfo.descriptorPool     = g_DescriptorPool;
//        allocInfo.descriptorSetCount = 1;
//        allocInfo.pSetLayouts        = &g_DescriptorSetLayout;
//
//        VK_CHECK(vkAllocateDescriptorSets(g_Device, &allocInfo, &g_DescriptorSet));
//    }
//
//    LOGI("initVulkan: Done");
//}
//
//// -----------------------------------------------------------------------------
//// JNI: runComputeShader - Actually runs the pipeline on a given image
////   inBuffer: RGBA data from Java, ByteBuffer (width*height*4 bytes)
////   outBuffer: ByteBuffer for GPU-processed result
//// -----------------------------------------------------------------------------
//extern "C"
//JNIEXPORT void JNICALL
//Java_com_ecse420_parallelcompute_NativeVulkan_runComputeShader(
//        JNIEnv* env, jclass /*clazz*/,
//        jobject inBuffer, jint width, jint height,
//        jobject outBuffer)
//{
//    // 1) Get pointers to the input/output data
//    uint8_t* inPtr  = (uint8_t*)env->GetDirectBufferAddress(inBuffer);
//    uint8_t* outPtr = (uint8_t*)env->GetDirectBufferAddress(outBuffer);
//    if (!inPtr || !outPtr) {
//        LOGE("runComputeShader: Invalid inBuffer/outBuffer addresses.");
//        return;
//    }
//
//    // 2) Create two images: one for input, one for output
//    SimpleImage inImage, outImage;
//    createImage2D(width, height, inImage);
//    createImage2D(width, height, outImage);
//
//    // 3) Copy CPU data -> inImage memory
//    //    Because we used VK_IMAGE_TILING_LINEAR, we can map image memory directly
//    void* mappedMem = nullptr;
//    VK_CHECK(vkMapMemory(g_Device, inImage.memory, 0, VK_WHOLE_SIZE, 0, &mappedMem));
//    // RGBA data -> each row is width * 4 bytes. We can do a naive memcpy if row pitch == width*4
//    // But in real code, you'd check subresource layout. For brevity, assume it matches.
//    memcpy(mappedMem, inPtr, width * height * 4);
//    vkUnmapMemory(g_Device, inImage.memory);
//
//    // 4) Update descriptor set to point to these images
//    VkDescriptorImageInfo descIn = {};
//    descIn.imageView   = inImage.view;
//    descIn.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//
//    VkDescriptorImageInfo descOut = {};
//    descOut.imageView   = outImage.view;
//    descOut.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//
//    VkWriteDescriptorSet writeSets[2] = {};
//
//    // Binding 0: input
//    writeSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    writeSets[0].dstSet          = g_DescriptorSet;
//    writeSets[0].dstBinding      = 0;
//    writeSets[0].descriptorCount = 1;
//    writeSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//    writeSets[0].pImageInfo      = &descIn;
//
//    // Binding 1: output
//    writeSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    writeSets[1].dstSet          = g_DescriptorSet;
//    writeSets[1].dstBinding      = 1;
//    writeSets[1].descriptorCount = 1;
//    writeSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//    writeSets[1].pImageInfo      = &descOut;
//
//    vkUpdateDescriptorSets(g_Device, 2, writeSets, 0, nullptr);
//
//    // 5) Record command buffer:
//    {
//        VkCommandBufferBeginInfo beginInfo = {};
//        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//        VK_CHECK(vkBeginCommandBuffer(g_CommandBuffer, &beginInfo));
//
//        // The images are VK_IMAGE_LAYOUT_PREINITIALIZED; we need them to be GENERAL for storage
//        // For brevity, we skip layout transitions or rely on ephemeral usage.
//        // A real app would do vkCmdPipelineBarrier with an image memory barrier.
//
//        vkCmdBindPipeline(g_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, g_ComputePipeline);
//        vkCmdBindDescriptorSets(g_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
//                                g_PipelineLayout, 0, 1, &g_DescriptorSet, 0, nullptr);
//
//        // Dispatch enough workgroups to cover the entire image
//        uint32_t groupCountX = (width  + 8 - 1) / 8;
//        uint32_t groupCountY = (height + 8 - 1) / 8;
//        vkCmdDispatch(g_CommandBuffer, groupCountX, groupCountY, 1);
//
//        VK_CHECK(vkEndCommandBuffer(g_CommandBuffer));
//    }
//
//    // 6) Submit the command buffer to compute queue, wait for completion
//    {
//        VkSubmitInfo submitInfo = {};
//        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//        submitInfo.commandBufferCount = 1;
//        submitInfo.pCommandBuffers    = &g_CommandBuffer;
//
//        VK_CHECK(vkQueueSubmit(g_ComputeQueue, 1, &submitInfo, VK_NULL_HANDLE));
//        VK_CHECK(vkQueueWaitIdle(g_ComputeQueue));
//    }
//
//    // 7) Read data from outImage into outPtr
//    VK_CHECK(vkMapMemory(g_Device, outImage.memory, 0, VK_WHOLE_SIZE, 0, &mappedMem));
//    memcpy(outPtr, mappedMem, width * height * 4);
//    vkUnmapMemory(g_Device, outImage.memory);
//
//    // 8) Cleanup temporary images
//    vkDestroyImageView(g_Device, inImage.view, nullptr);
//    vkDestroyImageView(g_Device, outImage.view, nullptr);
//    vkDestroyImage(g_Device, inImage.image, nullptr);
//    vkDestroyImage(g_Device, outImage.image, nullptr);
//    vkFreeMemory(g_Device, inImage.memory, nullptr);
//    vkFreeMemory(g_Device, outImage.memory, nullptr);
//
//    // Reset command buffer for next usage
//    vkResetCommandBuffer(g_CommandBuffer, 0);
//
//    LOGI("runComputeShader: Done processing image %dx%d", width, height);
//}
//
//// -----------------------------------------------------------------------------
//// JNI: cleanupVulkan - free everything we created
//// -----------------------------------------------------------------------------
//extern "C"
//JNIEXPORT void JNICALL
//Java_com_ecse420_parallelcompute_NativeVulkan_cleanupVulkan(JNIEnv* env, jclass /*clazz*/) {
//    vkDeviceWaitIdle(g_Device);
//
//    // Destroy pipeline & layout
//    if (g_ComputePipeline) {
//        vkDestroyPipeline(g_Device, g_ComputePipeline, nullptr);
//        g_ComputePipeline = VK_NULL_HANDLE;
//    }
//    if (g_PipelineLayout) {
//        vkDestroyPipelineLayout(g_Device, g_PipelineLayout, nullptr);
//        g_PipelineLayout = VK_NULL_HANDLE;
//    }
//    if (g_ComputeShaderModule) {
//        vkDestroyShaderModule(g_Device, g_ComputeShaderModule, nullptr);
//        g_ComputeShaderModule = VK_NULL_HANDLE;
//    }
//    if (g_DescriptorSetLayout) {
//        vkDestroyDescriptorSetLayout(g_Device, g_DescriptorSetLayout, nullptr);
//        g_DescriptorSetLayout = VK_NULL_HANDLE;
//    }
//    if (g_DescriptorPool) {
//        vkDestroyDescriptorPool(g_Device, g_DescriptorPool, nullptr);
//        g_DescriptorPool = VK_NULL_HANDLE;
//    }
//
//    // Free command buffer & pool
//    if (g_CommandBuffer) {
//        vkFreeCommandBuffers(g_Device, g_CommandPool, 1, &g_CommandBuffer);
//        g_CommandBuffer = VK_NULL_HANDLE;
//    }
//    if (g_CommandPool) {
//        vkDestroyCommandPool(g_Device, g_CommandPool, nullptr);
//        g_CommandPool = VK_NULL_HANDLE;
//    }
//
//    // Destroy device
//    if (g_Device) {
//        vkDestroyDevice(g_Device, nullptr);
//        g_Device = VK_NULL_HANDLE;
//    }
//
//    // Destroy instance
//    if (g_Instance) {
//        vkDestroyInstance(g_Instance, nullptr);
//        g_Instance = VK_NULL_HANDLE;
//    }
//
//    LOGI("cleanupVulkan: Done");
//}

#include <jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <fstream>
#include <cassert>
#include <cstring>    // for memcpy

// Macros for logging to Logcat
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "VulkanCompute", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VulkanCompute", __VA_ARGS__)

// A helper macro to check VkResult in a minimal way
#define VK_CHECK(x) do {                         \
    VkResult err = x;                            \
    if (err != VK_SUCCESS) {                     \
        LOGE("Detected Vulkan error: %d", err);  \
        assert(false);                           \
    }                                            \
} while(0)

// -----------------------------------------------------------------------------
// Global context storing Vulkan objects used in compute
// (For simplicity, we keep them all in static variables. In production, you'd
//  manage them in a class or struct with proper lifecycle management.)
// -----------------------------------------------------------------------------

static VkInstance          g_Instance         = VK_NULL_HANDLE;
static VkPhysicalDevice    g_PhysicalDevice   = VK_NULL_HANDLE;
static VkDevice            g_Device           = VK_NULL_HANDLE;
static VkQueue             g_ComputeQueue     = VK_NULL_HANDLE;
static uint32_t            g_ComputeQueueFamilyIndex = 0;

static VkCommandPool       g_CommandPool      = VK_NULL_HANDLE;
static VkCommandBuffer     g_CommandBuffer    = VK_NULL_HANDLE;

static VkDescriptorSetLayout g_DescriptorSetLayout = VK_NULL_HANDLE;
static VkPipelineLayout      g_PipelineLayout      = VK_NULL_HANDLE;
static VkPipeline            g_ComputePipeline     = VK_NULL_HANDLE;

static VkDescriptorPool      g_DescriptorPool      = VK_NULL_HANDLE;
static VkDescriptorSet       g_DescriptorSet       = VK_NULL_HANDLE;

// We'll store the loaded shader module in a global for re-use
static VkShaderModule        g_ComputeShaderModule = VK_NULL_HANDLE;

// -----------------------------------------------------------------------------
// Helper: Load SPIR-V from file
// -----------------------------------------------------------------------------
static std::vector<char> loadFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open SPIR-V file: %s", filename.c_str());
        return {};
    }
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

// -----------------------------------------------------------------------------
// Helper: Create a shader module from SPIR-V code
// -----------------------------------------------------------------------------
static VkShaderModule createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(g_Device, &createInfo, nullptr, &shaderModule));
    return shaderModule;
}

// -----------------------------------------------------------------------------
// Helper: Pick a physical device that has a compute queue
// -----------------------------------------------------------------------------
static void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(g_Instance, &deviceCount, nullptr);
    assert(deviceCount > 0);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(g_Instance, &deviceCount, devices.data());

    // Just pick the first device that has a compute queue
    for (auto& d : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(d, &props);

        // Find a queue with compute
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, families.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                g_PhysicalDevice = d;
                g_ComputeQueueFamilyIndex = i;
                LOGI("Using device: %s", props.deviceName);
                return;
            }
        }
    }

    LOGE("No suitable physical device found for compute!");
    assert(false);
}

// -----------------------------------------------------------------------------
// Simple struct to hold an image + memory + view
// -----------------------------------------------------------------------------
struct SimpleImage {
    VkImage       image  = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView   view   = VK_NULL_HANDLE;
    int           width;
    int           height;
};

// -----------------------------------------------------------------------------
// Create a 2D image with VK_IMAGE_TILING_LINEAR (host-visible). Not all devices
// support this in a fully portable manner, but it works for demonstration.
// -----------------------------------------------------------------------------
static void createImage2D(int width, int height, SimpleImage& outImage) {
    outImage.width  = width;
    outImage.height = height;

    // 1) Create the image
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType   = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels   = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format      = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling      = VK_IMAGE_TILING_LINEAR; // host-visible layout
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageInfo.usage       = VK_IMAGE_USAGE_STORAGE_BIT;
    imageInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(g_Device, &imageInfo, nullptr, &outImage.image));

    // 2) Allocate memory
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(g_Device, outImage.image, &memReq);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(g_PhysicalDevice, &memProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memReq.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags &
             (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            memoryTypeIndex = i;
            break;
        }
    }
    assert(memoryTypeIndex != UINT32_MAX);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReq.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    VK_CHECK(vkAllocateMemory(g_Device, &allocInfo, nullptr, &outImage.memory));
    VK_CHECK(vkBindImageMemory(g_Device, outImage.image, outImage.memory, 0));

    // 3) Create an image view
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = outImage.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;
    VK_CHECK(vkCreateImageView(g_Device, &viewInfo, nullptr, &outImage.view));
}

// -----------------------------------------------------------------------------
// Create descriptor set layout, pipeline layout, compute pipeline
// -----------------------------------------------------------------------------
static void createComputePipeline(const std::string& shaderSpvPath) {
    // 1) Load SPIR-V
    auto spvCode = loadFile(shaderSpvPath);
    assert(!spvCode.empty());
    g_ComputeShaderModule = createShaderModule(spvCode);

    // 2) Create descriptor set layout:
    //    We have 2 bindings: binding=0 inputImage (readOnly), binding=1 outputImage (writeOnly).
    VkDescriptorSetLayoutBinding bindings[2] = {};
    bindings[0].binding         = 0;
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding         = 1;
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings    = bindings;

    VK_CHECK(vkCreateDescriptorSetLayout(g_Device, &layoutInfo, nullptr, &g_DescriptorSetLayout));

    // 3) Create pipeline layout (with our descriptor set layout)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &g_DescriptorSetLayout;
    VK_CHECK(vkCreatePipelineLayout(g_Device, &pipelineLayoutInfo, nullptr, &g_PipelineLayout));

    // 4) Create compute pipeline
    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = g_ComputeShaderModule;
    stageInfo.pName  = "main"; // entry point

    VkComputePipelineCreateInfo computeCreateInfo = {};
    computeCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computeCreateInfo.stage  = stageInfo;
    computeCreateInfo.layout = g_PipelineLayout;

    VK_CHECK(vkCreateComputePipelines(g_Device, VK_NULL_HANDLE, 1, &computeCreateInfo, nullptr, &g_ComputePipeline));
}

// -----------------------------------------------------------------------------
// JNI: initVulkan - Creates instance, device, queue, pipeline, etc.
// -----------------------------------------------------------------------------
extern "C"
JNIEXPORT void JNICALL
Java_com_ecse420_parallelcompute_NativeVulkan_initVulkan(JNIEnv* env, jclass /*clazz*/, jstring jShaderPath)
{
    // 1) Create Vulkan instance
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "VulkanCompute";
        appInfo.applicationVersion = 1;
        appInfo.pEngineName        = "NoEngine";
        appInfo.engineVersion      = 1;
        appInfo.apiVersion         = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // No extensions/layers for brevity
        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &g_Instance));
    }

    // 2) Pick physical device w/ compute queue
    pickPhysicalDevice();

    // 3) Create logical device + get compute queue
    {
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = g_ComputeQueueFamilyIndex;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount    = 1;
        deviceCreateInfo.pQueueCreateInfos       = &queueCreateInfo;
        // no extra features or layers for brevity

        VK_CHECK(vkCreateDevice(g_PhysicalDevice, &deviceCreateInfo, nullptr, &g_Device));
        vkGetDeviceQueue(g_Device, g_ComputeQueueFamilyIndex, 0, &g_ComputeQueue);
    }

    // 4) Create a command pool & allocate one command buffer
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = g_ComputeQueueFamilyIndex;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK(vkCreateCommandPool(g_Device, &poolInfo, nullptr, &g_CommandPool));

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = g_CommandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(g_Device, &allocInfo, &g_CommandBuffer));
    }

    // 5) Convert jstring to std::string
    const char* nativePath = env->GetStringUTFChars(jShaderPath, nullptr);
    std::string shaderPath(nativePath);
    env->ReleaseStringUTFChars(jShaderPath, nativePath);

    // 6) Create compute pipeline
    createComputePipeline(shaderPath);

    // 7) Create descriptor pool for 1 set
    {
        VkDescriptorPoolSize poolSize = {};
        poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSize.descriptorCount = 2; // input + output

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets       = 1;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes    = &poolSize;

        VK_CHECK(vkCreateDescriptorPool(g_Device, &poolInfo, nullptr, &g_DescriptorPool));
    }

    // 8) Allocate the descriptor set
    {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = g_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &g_DescriptorSetLayout;

        VK_CHECK(vkAllocateDescriptorSets(g_Device, &allocInfo, &g_DescriptorSet));
    }

    LOGI("initVulkan: Done");
}

// -----------------------------------------------------------------------------
// Row-by-row copy from CPU pointer -> a linear-tiled image memory
// -----------------------------------------------------------------------------
static void copyBufferToImageRowByRow(const void* srcMem, VkDeviceMemory imageMem, VkImage image,
                                      int width, int height)
{
    // 1) Map the image memory
    void* mapped = nullptr;
    VK_CHECK(vkMapMemory(g_Device, imageMem, 0, VK_WHOLE_SIZE, 0, &mapped));

    // 2) Get subresource layout
    VkImageSubresource subres{};
    subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subres.mipLevel   = 0;
    subres.arrayLayer = 0;

    VkSubresourceLayout layout;
    vkGetImageSubresourceLayout(g_Device, image, &subres, &layout);

    // 3) Copy row by row
    const uint8_t* src = (const uint8_t*)srcMem;
    uint8_t* dst       = (uint8_t*)mapped + layout.offset;

    for (int y = 0; y < height; y++) {
        memcpy(dst, src, width * 4);  // RGBA => 4 bytes per pixel
        dst += layout.rowPitch;      // jump by the actual row pitch
        src += (width * 4);          // next row in the source buffer
    }

    vkUnmapMemory(g_Device, imageMem);
}

// -----------------------------------------------------------------------------
// Row-by-row copy from a linear-tiled image memory -> CPU pointer
// -----------------------------------------------------------------------------
static void copyImageToBufferRowByRow(void* dstMem, VkDeviceMemory imageMem, VkImage image,
                                      int width, int height)
{
    // 1) Map the image memory
    void* mapped = nullptr;
    VK_CHECK(vkMapMemory(g_Device, imageMem, 0, VK_WHOLE_SIZE, 0, &mapped));

    // 2) Get subresource layout
    VkImageSubresource subres{};
    subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subres.mipLevel   = 0;
    subres.arrayLayer = 0;

    VkSubresourceLayout layout;
    vkGetImageSubresourceLayout(g_Device, image, &subres, &layout);

    // 3) Copy row by row
    uint8_t* dst       = (uint8_t*)dstMem;
    uint8_t* src       = (uint8_t*)mapped + layout.offset;

    for (int y = 0; y < height; y++) {
        memcpy(dst, src, width * 4);
        src += layout.rowPitch;
        dst += (width * 4);
    }

    vkUnmapMemory(g_Device, imageMem);
}

// -----------------------------------------------------------------------------
// JNI: runComputeShader - runs the pipeline on a given image
//   inBuffer: RGBA data from Java ByteBuffer (width*height*4 bytes)
//   outBuffer: ByteBuffer for GPU-processed result
// -----------------------------------------------------------------------------
extern "C"
JNIEXPORT void JNICALL
Java_com_ecse420_parallelcompute_NativeVulkan_runComputeShader(
        JNIEnv* env, jclass /*clazz*/,
        jobject inBuffer, jint width, jint height,
        jobject outBuffer)
{
    // 1) Get pointers to the input/output data
    uint8_t* inPtr  = (uint8_t*)env->GetDirectBufferAddress(inBuffer);
    uint8_t* outPtr = (uint8_t*)env->GetDirectBufferAddress(outBuffer);
    if (!inPtr || !outPtr) {
        LOGE("runComputeShader: Invalid inBuffer/outBuffer addresses.");
        return;
    }

    // 2) Create two images: one for input, one for output
    SimpleImage inImage, outImage;
    createImage2D(width, height, inImage);
    createImage2D(width, height, outImage);

    // 3) Copy CPU data -> inImage (row by row)
    copyBufferToImageRowByRow(inPtr, inImage.memory, inImage.image, width, height);

    // 4) Update descriptor set to point to these images
    VkDescriptorImageInfo descIn = {};
    descIn.imageView   = inImage.view;
    descIn.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // We'll rely on a simple layout

    VkDescriptorImageInfo descOut = {};
    descOut.imageView   = outImage.view;
    descOut.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writeSets[2] = {};

    // Binding 0: input
    writeSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[0].dstSet          = g_DescriptorSet;
    writeSets[0].dstBinding      = 0;
    writeSets[0].descriptorCount = 1;
    writeSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeSets[0].pImageInfo      = &descIn;

    // Binding 1: output
    writeSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[1].dstSet          = g_DescriptorSet;
    writeSets[1].dstBinding      = 1;
    writeSets[1].descriptorCount = 1;
    writeSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeSets[1].pImageInfo      = &descOut;

    vkUpdateDescriptorSets(g_Device, 2, writeSets, 0, nullptr);

    // 5) Record command buffer:
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(g_CommandBuffer, &beginInfo));

        // Typically we'd do a vkCmdPipelineBarrier to transition
        // from PREINITIALIZED to GENERAL, etc. (omitted for brevity).
        vkCmdBindPipeline(g_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, g_ComputePipeline);
        vkCmdBindDescriptorSets(g_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                g_PipelineLayout, 0, 1, &g_DescriptorSet, 0, nullptr);

        // Dispatch enough workgroups to cover the entire image
        uint32_t groupCountX = (width  + 8 - 1) / 8;
        uint32_t groupCountY = (height + 8 - 1) / 8;
        vkCmdDispatch(g_CommandBuffer, groupCountX, groupCountY, 1);

        VK_CHECK(vkEndCommandBuffer(g_CommandBuffer));
    }

    // 6) Submit the command buffer to compute queue, wait for completion
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &g_CommandBuffer;

        VK_CHECK(vkQueueSubmit(g_ComputeQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(g_ComputeQueue));
    }

    // 7) Read data from outImage into outPtr (row by row)
    copyImageToBufferRowByRow(outPtr, outImage.memory, outImage.image, width, height);

    // 8) Cleanup temporary images
    vkDestroyImageView(g_Device, inImage.view, nullptr);
    vkDestroyImageView(g_Device, outImage.view, nullptr);
    vkDestroyImage(g_Device, inImage.image, nullptr);
    vkDestroyImage(g_Device, outImage.image, nullptr);
    vkFreeMemory(g_Device, inImage.memory, nullptr);
    vkFreeMemory(g_Device, outImage.memory, nullptr);

    // Reset command buffer for next usage
    vkResetCommandBuffer(g_CommandBuffer, 0);

    LOGI("runComputeShader: Done processing image %dx%d", width, height);
}

// -----------------------------------------------------------------------------
// JNI: cleanupVulkan - free everything we created
// -----------------------------------------------------------------------------
extern "C"
JNIEXPORT void JNICALL
Java_com_ecse420_parallelcompute_NativeVulkan_cleanupVulkan(JNIEnv* env, jclass /*clazz*/) {
    vkDeviceWaitIdle(g_Device);

    // Destroy pipeline & layout
    if (g_ComputePipeline) {
        vkDestroyPipeline(g_Device, g_ComputePipeline, nullptr);
        g_ComputePipeline = VK_NULL_HANDLE;
    }
    if (g_PipelineLayout) {
        vkDestroyPipelineLayout(g_Device, g_PipelineLayout, nullptr);
        g_PipelineLayout = VK_NULL_HANDLE;
    }
    if (g_ComputeShaderModule) {
        vkDestroyShaderModule(g_Device, g_ComputeShaderModule, nullptr);
        g_ComputeShaderModule = VK_NULL_HANDLE;
    }
    if (g_DescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(g_Device, g_DescriptorSetLayout, nullptr);
        g_DescriptorSetLayout = VK_NULL_HANDLE;
    }
    if (g_DescriptorPool) {
        vkDestroyDescriptorPool(g_Device, g_DescriptorPool, nullptr);
        g_DescriptorPool = VK_NULL_HANDLE;
    }

    // Free command buffer & pool
    if (g_CommandBuffer) {
        vkFreeCommandBuffers(g_Device, g_CommandPool, 1, &g_CommandBuffer);
        g_CommandBuffer = VK_NULL_HANDLE;
    }
    if (g_CommandPool) {
        vkDestroyCommandPool(g_Device, g_CommandPool, nullptr);
        g_CommandPool = VK_NULL_HANDLE;
    }

    // Destroy device
    if (g_Device) {
        vkDestroyDevice(g_Device, nullptr);
        g_Device = VK_NULL_HANDLE;
    }

    // Destroy instance
    if (g_Instance) {
        vkDestroyInstance(g_Instance, nullptr);
        g_Instance = VK_NULL_HANDLE;
    }

    LOGI("cleanupVulkan: Done");
}

