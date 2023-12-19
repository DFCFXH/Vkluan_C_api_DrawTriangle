#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <optional>
#include <vector>
#include <set>
#include <fstream>
uint32_t clamp(uint32_t val, uint32_t min, uint32_t max) {
	if (val < min) {
		val = min;
	}
	if (val > max) {
		val = max;
	}
	return val;
}
const std::vector<const char*> layers = {//启用的层
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions =//启用的拓展
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
class APP {
public:
	void run() {
		initwindow();
		initvulkan();
		mainloop();
		cleanup();
	}
private:
	struct QueueFamilyData {//队列族索引信息结构
		std::optional<uint32_t> graphics;
		std::optional<uint32_t> present;
		bool isComplete() {
			return graphics.has_value() && present.has_value();
		}
	};
	struct Swapchaininfo {//交换链信息
		VkSurfaceCapabilitiesKHR cap;
		std::vector<VkSurfaceFormatKHR>formats;
		std::vector<VkPresentModeKHR>presentmodes;
	};
	const int W = 600;
	const int H = 600;
	GLFWwindow* window;//窗口
	VkInstance instance;//vulkan实例
	VkSurfaceKHR surface;//表面
	VkPhysicalDevice GPU;//物理设备
	VkDevice device;//逻辑设备
	VkQueue graphicsqueue;//图形队列
	VkQueue presentqueue;//展示队列
	VkSwapchainKHR swapchain;//交换链
	std::vector<VkImage>images;//获取交换链中的图像
	std::vector<VkImageView>imageviews;//为每个图像创建的视图
	VkFormat swapchainimageformat;//交换链中图像的格式
	VkPipeline pipeline;//渲染管线
	VkPipelineLayout pipelinelayout;//管线布局,数据传输时指定数据的布局
	VkRenderPass renderpass;//Vulkan的RenderPass对应的是一个完整渲染流程,这个完整流程可能会执行Pipeline多次
	std::vector<VkFramebuffer>framebuffers;//帧缓冲
	VkExtent2D swapchainimageextent;//交换链上图像分辨率
	VkCommandPool commandpool;//命令池
	VkCommandBuffer commandbuffer;//命令缓冲
	VkSemaphore getimagestartrender;//获取到图像,开始渲染的信号量
	VkSemaphore finishrenderpresentimage;//渲染完成展示图象的信号量
	void cleanup() {
		vkDestroySemaphore(device, getimagestartrender, nullptr);
		vkDestroySemaphore(device, finishrenderpresentimage, nullptr);
		vkDestroyCommandPool(device, commandpool, nullptr);
		for (int i = 0; i < imageviews.size(); i++) {
			vkDestroyFramebuffer(device, framebuffers[i], nullptr);
		}
		vkDestroyRenderPass(device, renderpass, nullptr);
		vkDestroyPipelineLayout(device, pipelinelayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		for (int i = 0; i < imageviews.size(); i++) {
			vkDestroyImageView(device, imageviews[i], nullptr);
		}
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void initvulkan() {
		CreateInstance();
		CreateSurface();
		FindPhysicalDevice();
		CreateDevice();
		CreateSwapchain();
		GetImages();
		CreateImageViews();
		CreateRenderPass();
		CreatePipeline();
		CreateFrameBuffers();
		CreateCommandPool();
		AllocateCommandBuffer();
		CreateSemaphore();
	}
	void mainloop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			draw();
		}
		vkDeviceWaitIdle(device);
	}
	void initwindow() {//初始化窗口
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		window = glfwCreateWindow(W, H, "vulkan", nullptr, nullptr);
	}
	std::vector<const char*> GetExtensions() {//获取必须的拓展
		uint32_t extensionscount = 0;
		const char** extensions;
		extensions = glfwGetRequiredInstanceExtensions(&extensionscount);
		std::vector<const char*> extensionnames(extensions, extensions + extensionscount);
		return extensionnames;
	}
	void CreateInstance() {//创建实例
		VkApplicationInfo appinfo = {};
		appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appinfo.pApplicationName = "vulkan";
		appinfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
		appinfo.apiVersion = VK_VERSION_1_3;
		appinfo.pEngineName = nullptr;
		appinfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
		VkInstanceCreateInfo createinfo = {};
		createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createinfo.pApplicationInfo = &appinfo;
		auto extensions = GetExtensions();
		createinfo.enabledExtensionCount = extensions.size();
		createinfo.ppEnabledExtensionNames = extensions.data();
		createinfo.enabledLayerCount = 1;
		createinfo.ppEnabledLayerNames = layers.data();
		vkCreateInstance(&createinfo, nullptr, &instance);
	}
	void CreateSurface() {//创建表面
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
	}
	void FindPhysicalDevice() {//查询物理设备
		uint32_t GPUcount = 0;
		vkEnumeratePhysicalDevices(instance, &GPUcount, nullptr);//枚举所有的物理设备
		std::vector<VkPhysicalDevice>GPUs(GPUcount);
		vkEnumeratePhysicalDevices(instance, &GPUcount, GPUs.data());
		GPU = GPUs[0];
	}
	QueueFamilyData GetQueueFamilyIndex(VkPhysicalDevice pyhdevice) {//获取队列族索引
		QueueFamilyData rulst;
		uint32_t queuefamilycount;
		vkGetPhysicalDeviceQueueFamilyProperties(pyhdevice, &queuefamilycount, nullptr);//获取所有队列族属性
		std::vector<VkQueueFamilyProperties> queuefamilyproperties(queuefamilycount);
		vkGetPhysicalDeviceQueueFamilyProperties(pyhdevice, &queuefamilycount, queuefamilyproperties.data());
		uint32_t i = 0;
		for (auto p : queuefamilyproperties) {
			if (p.queueCount != 0 && p.queueFlags & VK_QUEUE_GRAPHICS_BIT) {//如果支持图形渲染
				rulst.graphics = i;
			}
			VkBool32 b = 0;
			vkGetPhysicalDeviceSurfaceSupportKHR(pyhdevice, i, surface, &b);//检查当前队列是否支持展示
			if (b) {
				rulst.present = i;
			}
			if (rulst.isComplete()) {//找到连个符合条件的队列之后就退出循环
				break;
			}
			i++;
		}
		return rulst;
	}
	void CreateDevice() {//创建逻辑设备
		 QueueFamilyData queuefamilydata = GetQueueFamilyIndex(GPU);
		 float queuepriorities = 1;//队列优先级
		 std::vector<VkDeviceQueueCreateInfo>queueinfos;//逻辑设备中要创建的队列的信息
		 std::set<uint32_t>queuefamilys = { queuefamilydata.graphics.value(),queuefamilydata.present.value() };
	     for (uint32_t queuefamily : queuefamilys) {
			 VkDeviceQueueCreateInfo queuecreateinfo = {};
			 queuecreateinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			 queuecreateinfo.queueFamilyIndex = queuefamily;
			 queuecreateinfo.queueCount = 1;
			 queuecreateinfo.pQueuePriorities = &queuepriorities;
			 queueinfos.push_back(queuecreateinfo);
		 }
		 VkPhysicalDeviceFeatures features{};
		VkDeviceCreateInfo createinfo = {};
		createinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createinfo.pQueueCreateInfos = queueinfos.data();
		createinfo.queueCreateInfoCount = static_cast<uint32_t>(queueinfos.size());//要创建几个队列
		createinfo.enabledLayerCount = layers.size();
		createinfo.ppEnabledLayerNames = layers.data();
		createinfo.enabledExtensionCount = deviceExtensions.size();
		createinfo.ppEnabledExtensionNames = deviceExtensions.data();
		createinfo.pEnabledFeatures = &features;//启用的设备功能
		vkCreateDevice(GPU, &createinfo, nullptr, &device);
		vkGetDeviceQueue(device, queuefamilydata.graphics.value(), 0, &graphicsqueue);//获取队列
		vkGetDeviceQueue(device, queuefamilydata.present.value(), 0, &presentqueue);
	}
	Swapchaininfo GetSwapchaininfo(VkPhysicalDevice phydevice) {//获取物理设备支持的表面功能,所有表面格式,所有呈现模式
		Swapchaininfo info = {};
		uint32_t formatcount = 0;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phydevice, surface, &info.cap);//查询表面功能
		vkGetPhysicalDeviceSurfaceFormatsKHR(phydevice, surface, &formatcount, nullptr);
		info.formats.resize(formatcount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(phydevice, surface, &formatcount, info.formats.data());//获取设备支持的所有格式
		uint32_t presentmodecount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(phydevice, surface, &presentmodecount, nullptr);//获取设备上表面支持的呈现模式
		info.presentmodes.resize(presentmodecount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(phydevice, surface, &presentmodecount, info.presentmodes.data());
		return info;
	}
	VkSurfaceFormatKHR GetSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& formats) {//获取需要的表面格式
		for (const VkSurfaceFormatKHR& format : formats) {
			if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return format;
			}
		}
		return formats[0];
	}
	VkPresentModeKHR GetSwapchainPresentmode(const std::vector<VkPresentModeKHR>& presentmodes) {//获取所需的呈现模式
		for (const VkPresentModeKHR& presentmode : presentmodes) {
			if (presentmode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return presentmode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D GetSwapchainExtent(const VkSurfaceCapabilitiesKHR& cap) {//获取交换链上图像的分辨率
		if (cap.maxImageExtent.width != std::numeric_limits<uint32_t>::max()) {//如果图像大小没有达到uint32_t类型的极限
			//这表示图像的大小受到了限制,可以直接使用当前的大小
			return cap.maxImageExtent;
		}
		else {
			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			VkExtent2D rulst = {
				static_cast<uint32_t>(w),
				static_cast<uint32_t>(h)
			};
			rulst.width = clamp(w, cap.minImageExtent.width, cap.maxImageExtent.width);
			rulst.height = clamp(h, cap.minImageExtent.height, cap.maxImageExtent.height);
			return rulst;
		}
	}
	void CreateSwapchain() {//创建交换链
		Swapchaininfo info = GetSwapchaininfo(GPU);//获取交换链的表面功能,所有的表面格式,所有的呈现模式
		VkSurfaceFormatKHR format = GetSwapchainFormat(info.formats);//获取所需的表面格式
		VkPresentModeKHR present = GetSwapchainPresentmode(info.presentmodes);//获取所需的呈现模式
		VkExtent2D extent = GetSwapchainExtent(info.cap);//获取交换链图像分辨率
		uint32_t imagecount = info.cap.minImageCount + 1;//交换链图像数量默认值
		if (info.cap.maxImageCount > 0 && imagecount > info.cap.maxImageCount) {//如果图像数量超出最大限度
			imagecount = info.cap.maxImageCount;
		}
		VkSwapchainCreateInfoKHR createinfo = {};
		createinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createinfo.imageExtent = extent;
		createinfo.surface = surface;
		createinfo.presentMode = present;
		createinfo.imageArrayLayers = 1;//图像数组的维度
		createinfo.imageFormat = format.format;
		createinfo.imageColorSpace = format.colorSpace;
		createinfo.minImageCount = imagecount;
		createinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;//图像的使用预期,这里设为颜色附件
		/*颜色附件用于储存计算出的颜色*/
		QueueFamilyData data = GetQueueFamilyIndex(GPU);//获取图形和展示队列族索引
		uint32_t datas[] = { data.graphics.value(),data.present.value() };
		if (data.graphics.value() == data.present.value()) {//如果使用的是同一个队列
			createinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createinfo.queueFamilyIndexCount = 0;
			createinfo.pQueueFamilyIndices = nullptr;
		}
		else {
			createinfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createinfo.pQueueFamilyIndices = datas;
			createinfo.queueFamilyIndexCount = 2;
		}
		createinfo.preTransform = info.cap.currentTransform;//设置图像变换(如在显示之前将图像旋转)
		createinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//颜色和A通道的混合方式
		createinfo.clipped = VK_TRUE;//当图像大小超出限度时是否裁剪
		createinfo.oldSwapchain = VK_NULL_HANDLE;
		/*先前创建的交换链,如果窗口大小发生变化,交换链也要重建,在这种情况下需要销毁旧的交换链*/
		vkCreateSwapchainKHR(device, &createinfo, nullptr, &swapchain);
		swapchainimageformat = format.format;
		swapchainimageextent = extent;
	}
	void GetImages() {//获取交换链中的图像
		uint32_t imagecount;
		vkGetSwapchainImagesKHR(device, swapchain, &imagecount, nullptr);
		images.resize(imagecount);
		vkGetSwapchainImagesKHR(device, swapchain, &imagecount, images.data());
	}
	void CreateImageViews() {//为交换链中的每个图像创建视图
		imageviews.resize(images.size());
		for (int i = 0; i < images.size();i++) {
			VkImageViewCreateInfo createinfo{};
			createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createinfo.image = images[i];
			createinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;//设置将目标(图像)当作2D图像去读取
			/*components指定读取图像时将图像的RGBA通道读取到视图的哪个通道,例如可以将图像的R通道读取到视图的B通道*/
			createinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;//这里全部使用默认
			createinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createinfo.format = swapchainimageformat;

			createinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;/*指定图像的哪些数据会被图像视图使用,
			这里指定颜色数据会被使用*/
			createinfo.subresourceRange.levelCount = 1;//指定mimap总数
			createinfo.subresourceRange.baseMipLevel = 0;//使用第几个mimap
			createinfo.subresourceRange.layerCount = 1;//图像数组的层数
			createinfo.subresourceRange.baseArrayLayer = 0;//从图像数组的第几层开始读取
			vkCreateImageView(device, &createinfo, nullptr, &imageviews[i]);
		}
	}
	static std::vector<char>readfile(const std::string filepath) {
        /*std::ios::ate表示打开文件后将文件流指针定位到文件末尾,std::ios::binary表示以二进制模式打开文件而不是文本模式*/
		std::ifstream file(filepath, std::ios::in | std::ios::binary);
		// 获取文件大小
		file.seekg(0, std::ios::end);
		std::streampos filesize = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<char>buffer(filesize);
		// 读取文件内容到缓冲区
		file.read(buffer.data(), filesize);
		file.close();
		return buffer;
	}
	VkShaderModule CreateShaderModule(const std::vector<char>& code) {//打包着色器代码
		VkShaderModuleCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createinfo.codeSize = code.size();
		createinfo.pCode = (const uint32_t*)code.data();
		VkShaderModule module;
		vkCreateShaderModule(device, &createinfo, nullptr, &module);
		return module;
	}
	void CreatePipeline() {//创建渲染管线
		VkGraphicsPipelineCreateInfo graphicspipelinecreateinfo{};//图形渲染管线创建信息
		graphicspipelinecreateinfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		VkPipelineVertexInputStateCreateInfo vertexinput{};//描述顶点数据的结构
		vertexinput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexinput.vertexAttributeDescriptionCount = 0;//顶点属性数量
		vertexinput.pVertexAttributeDescriptions = nullptr;//顶点属性的数组
		vertexinput.vertexBindingDescriptionCount = 0;//顶点输入绑定的数量
		vertexinput.pVertexBindingDescriptions = nullptr;//顶点输入绑定数组
		graphicspipelinecreateinfo.pVertexInputState = &vertexinput;//顶点输入
		VkPipelineInputAssemblyStateCreateInfo inputassembly{};//描述传入的顶点数据将组成什么图元的结构
		inputassembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputassembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//指定顶点将组成什么类型的图元
		inputassembly.primitiveRestartEnable = VK_FALSE;/*指定是否启用图元重启,启用之后如果使用带有_STRIP结尾的图元类型,
		从特殊索引值(0xFFFF或0xFFFFFFFF)之后的索引重置为图元的第一个顶点*/
		graphicspipelinecreateinfo.pInputAssemblyState = &inputassembly;//传入的顶点数据将组成什么样的图元
		VkPipelineShaderStageCreateInfo vertstage{};
		vertstage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertstage.stage = VK_SHADER_STAGE_VERTEX_BIT;//着色器类型
		auto vertexshadercode = readfile("vert.spv");
		auto vertexshadermodule = CreateShaderModule(vertexshadercode);
		vertstage.module = vertexshadermodule;
		vertstage.pName = "main";//主函数入口
		VkPipelineShaderStageCreateInfo fragstage{};
		fragstage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragstage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;//着色器类型
		auto fragshadercode = readfile("frag.spv");
		auto fragshadermodule = CreateShaderModule(fragshadercode);
		fragstage.module = fragshadermodule;
		fragstage.pName = "main";//主函数入口
		VkPipelineShaderStageCreateInfo stages[] = { vertstage,fragstage };
		graphicspipelinecreateinfo.stageCount = 2;//着色器阶段共有几个着色器
		graphicspipelinecreateinfo.pStages = stages;//管线中着色器阶段各个着色器的代码
		VkPipelineViewportStateCreateInfo viewportstate{};
		viewportstate.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportstate.viewportCount = 1;//视口数量
		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = W;
		viewport.height = H;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		viewportstate.pViewports = &viewport;//描述了每个视口的宽高位置深度的数组
		viewportstate.scissorCount = 1;//视口裁剪信息的数量
		VkRect2D rect{};
		rect.offset = { 0,0 };//矩形起点的位置
		rect.extent = { static_cast<uint32_t>(W),static_cast<uint32_t>(H)};//矩形大小
		viewportstate.pScissors = &rect;//视口裁剪信息(指定真正绘制到屏幕上的区域)
		graphicspipelinecreateinfo.pViewportState = &viewportstate;//视口变换(渲染结果可以显示区域,超出范围的将被裁剪)
		VkPipelineRasterizationStateCreateInfo rasterization{};//光栅化阶段设置
		rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization.rasterizerDiscardEnable = false;//是否将光栅化的结果抛弃
		rasterization.cullMode = VK_CULL_MODE_BACK_BIT;//面剔除,这里去除背面
		rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;//如何为正面?这里设置从观察者的角度看三角形的三个顶点为顺时针排列则为正面
		rasterization.polygonMode = VK_POLYGON_MODE_FILL;//光栅化之后图形的填充模式,这里设置其填充满
		rasterization.lineWidth = 1;//线段,边框的宽度
		graphicspipelinecreateinfo.pRasterizationState = &rasterization;//光栅化阶段设置
		VkPipelineMultisampleStateCreateInfo multsamplecreateinfo{};//抗锯齿多重采样设置
		multsamplecreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multsamplecreateinfo.sampleShadingEnable = VK_FALSE;/*是否启用采样遮罩,如果启用,将按每个像素的覆盖率计算一个样本遮罩,
		该样本遮罩可以指定哪些样本要被覆盖,从而使每个样本都可以有自己的颜色值*/
		multsamplecreateinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;//设置采样数量,这里设为1,则禁用
		graphicspipelinecreateinfo.pMultisampleState = &multsamplecreateinfo;//抗锯齿多重采样
		VkPipelineColorBlendStateCreateInfo colorblend{};
		colorblend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		/*
		片段着色器返回的片段颜色需要和原来帧缓冲中对应像素的颜色进行混合。
		混合的方式有下面两种:
		1.混合旧值和新值产生最终的颜色
		2.使用位运算组合旧值和新值
		有两个用于配置颜色混合的结构体:
		1.VkPipelineColorBlendAttachmentState --
	    对每个绑定的帧缓冲进行单独的颜色混合配置
		2.VkPipelineColorBlendStateCreateInfo --
		进行全局的颜色混合配置
		*/
		colorblend.logicOpEnable = VK_FALSE;//是否启用逻辑运算符,启用将使用逻辑运算符计算颜色混合
		VkPipelineColorBlendAttachmentState colorblendattachment{};//颜色附件混合设置
		colorblendattachment.blendEnable = VK_FALSE;//禁用颜色混合
		colorblendattachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|
			VK_COLOR_COMPONENT_G_BIT|
			VK_COLOR_COMPONENT_B_BIT|
			VK_COLOR_COMPONENT_A_BIT;//向帧缓冲写入颜色时都写入哪些颜色通道
		colorblend.pAttachments = &colorblendattachment;//配置颜色混合附件
		colorblend.attachmentCount = 1;//颜色混合附件的数量
		graphicspipelinecreateinfo.pColorBlendState = &colorblend;//颜色混合
		VkPipelineLayoutCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		vkCreatePipelineLayout(device, &createinfo, nullptr, &pipelinelayout);
		graphicspipelinecreateinfo.layout = pipelinelayout;//设置管线布局
		graphicspipelinecreateinfo.renderPass = renderpass;//设置renderpass
		vkCreateGraphicsPipelines(device,nullptr , 1, &graphicspipelinecreateinfo, nullptr, &pipeline);
		vkDestroyShaderModule(device, vertexshadermodule, nullptr);//销毁shadermodule
		vkDestroyShaderModule(device, fragshadermodule, nullptr);
	}
	void CreateRenderPass() {//创建renderpass
		VkAttachmentDescription des{};
		des.format = swapchainimageformat;//附件的格式
		des.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;/*图像在开始进入renderpass前将要使用的布局结构,
		undefined代表不关心图像进入时的布局*/
		des.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//renderpass结束时图像使用的布局,这里设置其在交换链上展示
		des.samples = VK_SAMPLE_COUNT_1_BIT;//采样次数
		des.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//渲染前数据在对应附件进行的操作,这里设置将其清空
		des.storeOp = VK_ATTACHMENT_STORE_OP_STORE;//渲染后对附件里数据进行的操作,这里设置保存在内存里,后面读取用
		des.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;//模板数据加载进来时进行的操作,这里设为不关心
		des.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;//模板数据在renderpass结束时将如何处理,这里设为不关心
        
		VkAttachmentReference ref{};//指定渲染附件引用的结构
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//渲染附件在使用时的布局状态,这里将其当作颜色附件使用
		ref.attachment = 0;//指定使用pAttachments渲染附件数组里的哪一个
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//子通道中的渲染管线类型
		subpass.pColorAttachments = &ref;//子通道中使用的颜色附件引用
		subpass.colorAttachmentCount = 1;//子通道中颜色附件数量

		VkSubpassDependency dep{};
		dep.srcSubpass = VK_SUBPASS_EXTERNAL;//先要执行的子通道
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;//在进入第二个子通道前第一个子通道里必须完成的事
		dep.dstSubpass = 0;//后执行的子通道索引
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;//dstsubpass的内存访问类型
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;/*需要srcStageMask完成后执行的阶段*/

		VkRenderPassCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createinfo.pAttachments = &des;//描述了该通道需要什么样的附件的数组
		createinfo.attachmentCount = 1;//renderpass中附件的数量
		createinfo.pSubpasses = &subpass;//子通道数组
		createinfo.subpassCount = 1;//子通道数量
		createinfo.pDependencies = &dep;//子通道依赖关系描述(当有多个subpass的时候subpass的先后顺序)
		createinfo.dependencyCount = 1;//子通道依赖关系描述数量
		vkCreateRenderPass(device, &createinfo, nullptr, &renderpass);
	}
	void CreateFrameBuffers() {//创建帧缓冲
		framebuffers.resize(imageviews.size());
		for (int i = 0; i < imageviews.size(); i++) {
			VkImageView attachment[] = { imageviews[i] };//本次创建帧缓冲所用的图像视图
			VkFramebufferCreateInfo createinfo{};
			createinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createinfo.attachmentCount = 1;//在渲染通道中绑定的附件数量
			createinfo.pAttachments = attachment;//渲染通道中绑定的附件,这里提供图像视图用于访问
			createinfo.renderPass = renderpass;//指定将被哪个渲染通道使用
			createinfo.layers = 1;//图像数组层数
			createinfo.height = swapchainimageextent.height;//帧缓冲大小
			createinfo.width = swapchainimageextent.width;
			vkCreateFramebuffer(device, &createinfo, nullptr, &framebuffers[i]);
		}
	}
	void CreateCommandPool() {//创建命令池
		VkCommandPoolCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createinfo.queueFamilyIndex = GetQueueFamilyIndex(GPU).graphics.value();//从中分配的命令缓冲将被提交到哪个队列族上使用
		createinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;/*命令池和命令缓冲的重置模式, 
		这里设置创建的命令缓冲对象互相独立, 不会被一起重置*/
		vkCreateCommandPool(device, &createinfo, nullptr, &commandpool);
	}
	void AllocateCommandBuffer() {//从命令池中分配命令缓冲
		VkCommandBufferAllocateInfo allocateinfo{};
		allocateinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateinfo.commandPool = commandpool;//指定从哪个命令池中分配
		allocateinfo.commandBufferCount = 1;//要从命令池中分配多少个命令缓冲
		allocateinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//指定其调用模式,这里设置为可以被直接传到GPU上执行
		vkAllocateCommandBuffers(device, &allocateinfo, &commandbuffer);
	}
	void RecordCommandBuffer(VkCommandBuffer commandbuffer,uint32_t imageindex) {
		VkCommandBufferBeginInfo begininfo{};//缓冲区开始录制命令所需参数
		begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begininfo.flags = 0;//缓冲区重置模式,此处设为默认行为

		VkRenderPassBeginInfo renderpassbegininfo{};//开始渲染通道所需参数
		renderpassbegininfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpassbegininfo.framebuffer = framebuffers[imageindex];//指定要使用的帧缓冲
		renderpassbegininfo.renderPass = renderpass;//指定要执行的渲染通道
		renderpassbegininfo.renderArea.extent = swapchainimageextent;//渲染区域的范围
		renderpassbegininfo.renderArea.offset = { 0,0 };//渲染区域(一个矩形)的位置
		renderpassbegininfo.clearValueCount = 1;//从pClearValues中取出几个值
		VkClearValue cc = {{0,0,0,1}};
		renderpassbegininfo.pClearValues = &cc;//清空附件时的背景颜色

		vkBeginCommandBuffer(commandbuffer, &begininfo);//开始录制命令缓冲
		vkCmdBeginRenderPass(commandbuffer, &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);/*开始通道, 
		最后一个参数为子通道的调用方式,这里设置为在主命令中直接执行*/
		vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);/*绑定渲染管线,第二个参数为要绑定的管线类型*/
		vkCmdDraw(commandbuffer, 3, 1, 0, 0);/*绘制,第二个参数为定点数量,第三个参数为实例数量,第四个参数为起始顶点的索引,第五个
		为实例的索引*/
		vkCmdEndRenderPass(commandbuffer);//结束通道
		vkEndCommandBuffer(commandbuffer);//结束录制
	}
	void CreateSemaphore() {//创建信号量
		VkSemaphoreCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(device, &createinfo, nullptr, &getimagestartrender);
		vkCreateSemaphore(device, &createinfo, nullptr, &finishrenderpresentimage);
	}
	void draw() {//绘制
		//获取图像
		uint32_t imageindex = 0;
		vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, getimagestartrender, nullptr, &imageindex);/*获取交换链上的一张可用图像的索
		引,第三个参数是等待时间的上限,第四个是获取图像后要激活的信号,第五个是栅栏(在一些情况下可被信号量代替)*/
		vkQueueWaitIdle(graphicsqueue);//等待队列执行完毕
		vkResetCommandBuffer(commandbuffer, 0);/*重置命令缓冲,第二个参数为命令缓冲区的重置模式,这里设为默认*/
		RecordCommandBuffer(commandbuffer, imageindex);
		//将命令提交到队列
		VkSubmitInfo submitinfo{};//将命令提交到队列时所需参数
		submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitinfo.pWaitSemaphores = &getimagestartrender;//等待的信号量
		submitinfo.waitSemaphoreCount = 1;//等待的信号量的数量
		VkPipelineStageFlags dst[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitinfo.pWaitDstStageMask = dst;//在接收到信号量后才能执行的管线阶段
		submitinfo.pCommandBuffers = &commandbuffer;//要提交到队列的命令缓冲
		submitinfo.commandBufferCount = 1;//命令缓冲数量
		submitinfo.pSignalSemaphores = &finishrenderpresentimage;//命令缓冲完成后发出的信号量
		submitinfo.signalSemaphoreCount = 1; //命令缓冲完成后发出的信号量数量
		vkQueueSubmit(graphicsqueue, 1, &submitinfo, nullptr);//将命令缓冲提交到队列,最后一个参数为栅栏
		VkPresentInfoKHR presentinfo{};//图像呈现所需参数
		presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentinfo.pWaitSemaphores = &finishrenderpresentimage;//要等待的信号量
		presentinfo.waitSemaphoreCount = 1;//等待的信号量的数量
		presentinfo.pSwapchains = &swapchain;//要使用的交换链数组
		presentinfo.swapchainCount = 1;//交换链数量
		presentinfo.pImageIndices = &imageindex;//要使用的图像索引数量
		vkQueuePresentKHR(presentqueue, &presentinfo);
	}
};
int main() {
	APP a;
	a.run();
}