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
const std::vector<const char*> layers = {//���õĲ�
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions =//���õ���չ
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
	struct QueueFamilyData {//������������Ϣ�ṹ
		std::optional<uint32_t> graphics;
		std::optional<uint32_t> present;
		bool isComplete() {
			return graphics.has_value() && present.has_value();
		}
	};
	struct Swapchaininfo {//��������Ϣ
		VkSurfaceCapabilitiesKHR cap;
		std::vector<VkSurfaceFormatKHR>formats;
		std::vector<VkPresentModeKHR>presentmodes;
	};
	const int W = 600;
	const int H = 600;
	GLFWwindow* window;//����
	VkInstance instance;//vulkanʵ��
	VkSurfaceKHR surface;//����
	VkPhysicalDevice GPU;//�����豸
	VkDevice device;//�߼��豸
	VkQueue graphicsqueue;//ͼ�ζ���
	VkQueue presentqueue;//չʾ����
	VkSwapchainKHR swapchain;//������
	std::vector<VkImage>images;//��ȡ�������е�ͼ��
	std::vector<VkImageView>imageviews;//Ϊÿ��ͼ�񴴽�����ͼ
	VkFormat swapchainimageformat;//��������ͼ��ĸ�ʽ
	VkPipeline pipeline;//��Ⱦ����
	VkPipelineLayout pipelinelayout;//���߲���,���ݴ���ʱָ�����ݵĲ���
	VkRenderPass renderpass;//Vulkan��RenderPass��Ӧ����һ��������Ⱦ����,����������̿��ܻ�ִ��Pipeline���
	std::vector<VkFramebuffer>framebuffers;//֡����
	VkExtent2D swapchainimageextent;//��������ͼ��ֱ���
	VkCommandPool commandpool;//�����
	VkCommandBuffer commandbuffer;//�����
	VkSemaphore getimagestartrender;//��ȡ��ͼ��,��ʼ��Ⱦ���ź���
	VkSemaphore finishrenderpresentimage;//��Ⱦ���չʾͼ����ź���
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
	void initwindow() {//��ʼ������
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		window = glfwCreateWindow(W, H, "vulkan", nullptr, nullptr);
	}
	std::vector<const char*> GetExtensions() {//��ȡ�������չ
		uint32_t extensionscount = 0;
		const char** extensions;
		extensions = glfwGetRequiredInstanceExtensions(&extensionscount);
		std::vector<const char*> extensionnames(extensions, extensions + extensionscount);
		return extensionnames;
	}
	void CreateInstance() {//����ʵ��
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
	void CreateSurface() {//��������
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
	}
	void FindPhysicalDevice() {//��ѯ�����豸
		uint32_t GPUcount = 0;
		vkEnumeratePhysicalDevices(instance, &GPUcount, nullptr);//ö�����е������豸
		std::vector<VkPhysicalDevice>GPUs(GPUcount);
		vkEnumeratePhysicalDevices(instance, &GPUcount, GPUs.data());
		GPU = GPUs[0];
	}
	QueueFamilyData GetQueueFamilyIndex(VkPhysicalDevice pyhdevice) {//��ȡ����������
		QueueFamilyData rulst;
		uint32_t queuefamilycount;
		vkGetPhysicalDeviceQueueFamilyProperties(pyhdevice, &queuefamilycount, nullptr);//��ȡ���ж���������
		std::vector<VkQueueFamilyProperties> queuefamilyproperties(queuefamilycount);
		vkGetPhysicalDeviceQueueFamilyProperties(pyhdevice, &queuefamilycount, queuefamilyproperties.data());
		uint32_t i = 0;
		for (auto p : queuefamilyproperties) {
			if (p.queueCount != 0 && p.queueFlags & VK_QUEUE_GRAPHICS_BIT) {//���֧��ͼ����Ⱦ
				rulst.graphics = i;
			}
			VkBool32 b = 0;
			vkGetPhysicalDeviceSurfaceSupportKHR(pyhdevice, i, surface, &b);//��鵱ǰ�����Ƿ�֧��չʾ
			if (b) {
				rulst.present = i;
			}
			if (rulst.isComplete()) {//�ҵ��������������Ķ���֮����˳�ѭ��
				break;
			}
			i++;
		}
		return rulst;
	}
	void CreateDevice() {//�����߼��豸
		 QueueFamilyData queuefamilydata = GetQueueFamilyIndex(GPU);
		 float queuepriorities = 1;//�������ȼ�
		 std::vector<VkDeviceQueueCreateInfo>queueinfos;//�߼��豸��Ҫ�����Ķ��е���Ϣ
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
		createinfo.queueCreateInfoCount = static_cast<uint32_t>(queueinfos.size());//Ҫ������������
		createinfo.enabledLayerCount = layers.size();
		createinfo.ppEnabledLayerNames = layers.data();
		createinfo.enabledExtensionCount = deviceExtensions.size();
		createinfo.ppEnabledExtensionNames = deviceExtensions.data();
		createinfo.pEnabledFeatures = &features;//���õ��豸����
		vkCreateDevice(GPU, &createinfo, nullptr, &device);
		vkGetDeviceQueue(device, queuefamilydata.graphics.value(), 0, &graphicsqueue);//��ȡ����
		vkGetDeviceQueue(device, queuefamilydata.present.value(), 0, &presentqueue);
	}
	Swapchaininfo GetSwapchaininfo(VkPhysicalDevice phydevice) {//��ȡ�����豸֧�ֵı��湦��,���б����ʽ,���г���ģʽ
		Swapchaininfo info = {};
		uint32_t formatcount = 0;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phydevice, surface, &info.cap);//��ѯ���湦��
		vkGetPhysicalDeviceSurfaceFormatsKHR(phydevice, surface, &formatcount, nullptr);
		info.formats.resize(formatcount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(phydevice, surface, &formatcount, info.formats.data());//��ȡ�豸֧�ֵ����и�ʽ
		uint32_t presentmodecount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(phydevice, surface, &presentmodecount, nullptr);//��ȡ�豸�ϱ���֧�ֵĳ���ģʽ
		info.presentmodes.resize(presentmodecount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(phydevice, surface, &presentmodecount, info.presentmodes.data());
		return info;
	}
	VkSurfaceFormatKHR GetSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& formats) {//��ȡ��Ҫ�ı����ʽ
		for (const VkSurfaceFormatKHR& format : formats) {
			if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return format;
			}
		}
		return formats[0];
	}
	VkPresentModeKHR GetSwapchainPresentmode(const std::vector<VkPresentModeKHR>& presentmodes) {//��ȡ����ĳ���ģʽ
		for (const VkPresentModeKHR& presentmode : presentmodes) {
			if (presentmode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return presentmode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D GetSwapchainExtent(const VkSurfaceCapabilitiesKHR& cap) {//��ȡ��������ͼ��ķֱ���
		if (cap.maxImageExtent.width != std::numeric_limits<uint32_t>::max()) {//���ͼ���Сû�дﵽuint32_t���͵ļ���
			//���ʾͼ��Ĵ�С�ܵ�������,����ֱ��ʹ�õ�ǰ�Ĵ�С
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
	void CreateSwapchain() {//����������
		Swapchaininfo info = GetSwapchaininfo(GPU);//��ȡ�������ı��湦��,���еı����ʽ,���еĳ���ģʽ
		VkSurfaceFormatKHR format = GetSwapchainFormat(info.formats);//��ȡ����ı����ʽ
		VkPresentModeKHR present = GetSwapchainPresentmode(info.presentmodes);//��ȡ����ĳ���ģʽ
		VkExtent2D extent = GetSwapchainExtent(info.cap);//��ȡ������ͼ��ֱ���
		uint32_t imagecount = info.cap.minImageCount + 1;//������ͼ������Ĭ��ֵ
		if (info.cap.maxImageCount > 0 && imagecount > info.cap.maxImageCount) {//���ͼ��������������޶�
			imagecount = info.cap.maxImageCount;
		}
		VkSwapchainCreateInfoKHR createinfo = {};
		createinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createinfo.imageExtent = extent;
		createinfo.surface = surface;
		createinfo.presentMode = present;
		createinfo.imageArrayLayers = 1;//ͼ�������ά��
		createinfo.imageFormat = format.format;
		createinfo.imageColorSpace = format.colorSpace;
		createinfo.minImageCount = imagecount;
		createinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;//ͼ���ʹ��Ԥ��,������Ϊ��ɫ����
		/*��ɫ�������ڴ�����������ɫ*/
		QueueFamilyData data = GetQueueFamilyIndex(GPU);//��ȡͼ�κ�չʾ����������
		uint32_t datas[] = { data.graphics.value(),data.present.value() };
		if (data.graphics.value() == data.present.value()) {//���ʹ�õ���ͬһ������
			createinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createinfo.queueFamilyIndexCount = 0;
			createinfo.pQueueFamilyIndices = nullptr;
		}
		else {
			createinfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createinfo.pQueueFamilyIndices = datas;
			createinfo.queueFamilyIndexCount = 2;
		}
		createinfo.preTransform = info.cap.currentTransform;//����ͼ��任(������ʾ֮ǰ��ͼ����ת)
		createinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//��ɫ��Aͨ���Ļ�Ϸ�ʽ
		createinfo.clipped = VK_TRUE;//��ͼ���С�����޶�ʱ�Ƿ�ü�
		createinfo.oldSwapchain = VK_NULL_HANDLE;
		/*��ǰ�����Ľ�����,������ڴ�С�����仯,������ҲҪ�ؽ�,�������������Ҫ���پɵĽ�����*/
		vkCreateSwapchainKHR(device, &createinfo, nullptr, &swapchain);
		swapchainimageformat = format.format;
		swapchainimageextent = extent;
	}
	void GetImages() {//��ȡ�������е�ͼ��
		uint32_t imagecount;
		vkGetSwapchainImagesKHR(device, swapchain, &imagecount, nullptr);
		images.resize(imagecount);
		vkGetSwapchainImagesKHR(device, swapchain, &imagecount, images.data());
	}
	void CreateImageViews() {//Ϊ�������е�ÿ��ͼ�񴴽���ͼ
		imageviews.resize(images.size());
		for (int i = 0; i < images.size();i++) {
			VkImageViewCreateInfo createinfo{};
			createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createinfo.image = images[i];
			createinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;//���ý�Ŀ��(ͼ��)����2Dͼ��ȥ��ȡ
			/*componentsָ����ȡͼ��ʱ��ͼ���RGBAͨ����ȡ����ͼ���ĸ�ͨ��,������Խ�ͼ���Rͨ����ȡ����ͼ��Bͨ��*/
			createinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;//����ȫ��ʹ��Ĭ��
			createinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createinfo.format = swapchainimageformat;

			createinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;/*ָ��ͼ�����Щ���ݻᱻͼ����ͼʹ��,
			����ָ����ɫ���ݻᱻʹ��*/
			createinfo.subresourceRange.levelCount = 1;//ָ��mimap����
			createinfo.subresourceRange.baseMipLevel = 0;//ʹ�õڼ���mimap
			createinfo.subresourceRange.layerCount = 1;//ͼ������Ĳ���
			createinfo.subresourceRange.baseArrayLayer = 0;//��ͼ������ĵڼ��㿪ʼ��ȡ
			vkCreateImageView(device, &createinfo, nullptr, &imageviews[i]);
		}
	}
	static std::vector<char>readfile(const std::string filepath) {
        /*std::ios::ate��ʾ���ļ����ļ���ָ�붨λ���ļ�ĩβ,std::ios::binary��ʾ�Զ�����ģʽ���ļ��������ı�ģʽ*/
		std::ifstream file(filepath, std::ios::in | std::ios::binary);
		// ��ȡ�ļ���С
		file.seekg(0, std::ios::end);
		std::streampos filesize = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<char>buffer(filesize);
		// ��ȡ�ļ����ݵ�������
		file.read(buffer.data(), filesize);
		file.close();
		return buffer;
	}
	VkShaderModule CreateShaderModule(const std::vector<char>& code) {//�����ɫ������
		VkShaderModuleCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createinfo.codeSize = code.size();
		createinfo.pCode = (const uint32_t*)code.data();
		VkShaderModule module;
		vkCreateShaderModule(device, &createinfo, nullptr, &module);
		return module;
	}
	void CreatePipeline() {//������Ⱦ����
		VkGraphicsPipelineCreateInfo graphicspipelinecreateinfo{};//ͼ����Ⱦ���ߴ�����Ϣ
		graphicspipelinecreateinfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		VkPipelineVertexInputStateCreateInfo vertexinput{};//�����������ݵĽṹ
		vertexinput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexinput.vertexAttributeDescriptionCount = 0;//������������
		vertexinput.pVertexAttributeDescriptions = nullptr;//�������Ե�����
		vertexinput.vertexBindingDescriptionCount = 0;//��������󶨵�����
		vertexinput.pVertexBindingDescriptions = nullptr;//�������������
		graphicspipelinecreateinfo.pVertexInputState = &vertexinput;//��������
		VkPipelineInputAssemblyStateCreateInfo inputassembly{};//��������Ķ������ݽ����ʲôͼԪ�Ľṹ
		inputassembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputassembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//ָ�����㽫���ʲô���͵�ͼԪ
		inputassembly.primitiveRestartEnable = VK_FALSE;/*ָ���Ƿ�����ͼԪ����,����֮�����ʹ�ô���_STRIP��β��ͼԪ����,
		����������ֵ(0xFFFF��0xFFFFFFFF)֮�����������ΪͼԪ�ĵ�һ������*/
		graphicspipelinecreateinfo.pInputAssemblyState = &inputassembly;//����Ķ������ݽ����ʲô����ͼԪ
		VkPipelineShaderStageCreateInfo vertstage{};
		vertstage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertstage.stage = VK_SHADER_STAGE_VERTEX_BIT;//��ɫ������
		auto vertexshadercode = readfile("vert.spv");
		auto vertexshadermodule = CreateShaderModule(vertexshadercode);
		vertstage.module = vertexshadermodule;
		vertstage.pName = "main";//���������
		VkPipelineShaderStageCreateInfo fragstage{};
		fragstage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragstage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;//��ɫ������
		auto fragshadercode = readfile("frag.spv");
		auto fragshadermodule = CreateShaderModule(fragshadercode);
		fragstage.module = fragshadermodule;
		fragstage.pName = "main";//���������
		VkPipelineShaderStageCreateInfo stages[] = { vertstage,fragstage };
		graphicspipelinecreateinfo.stageCount = 2;//��ɫ���׶ι��м�����ɫ��
		graphicspipelinecreateinfo.pStages = stages;//��������ɫ���׶θ�����ɫ���Ĵ���
		VkPipelineViewportStateCreateInfo viewportstate{};
		viewportstate.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportstate.viewportCount = 1;//�ӿ�����
		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = W;
		viewport.height = H;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		viewportstate.pViewports = &viewport;//������ÿ���ӿڵĿ��λ����ȵ�����
		viewportstate.scissorCount = 1;//�ӿڲü���Ϣ������
		VkRect2D rect{};
		rect.offset = { 0,0 };//��������λ��
		rect.extent = { static_cast<uint32_t>(W),static_cast<uint32_t>(H)};//���δ�С
		viewportstate.pScissors = &rect;//�ӿڲü���Ϣ(ָ���������Ƶ���Ļ�ϵ�����)
		graphicspipelinecreateinfo.pViewportState = &viewportstate;//�ӿڱ任(��Ⱦ���������ʾ����,������Χ�Ľ����ü�)
		VkPipelineRasterizationStateCreateInfo rasterization{};//��դ���׶�����
		rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization.rasterizerDiscardEnable = false;//�Ƿ񽫹�դ���Ľ������
		rasterization.cullMode = VK_CULL_MODE_BACK_BIT;//���޳�,����ȥ������
		rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;//���Ϊ����?�������ôӹ۲��ߵĽǶȿ������ε���������Ϊ˳ʱ��������Ϊ����
		rasterization.polygonMode = VK_POLYGON_MODE_FILL;//��դ��֮��ͼ�ε����ģʽ,���������������
		rasterization.lineWidth = 1;//�߶�,�߿�Ŀ��
		graphicspipelinecreateinfo.pRasterizationState = &rasterization;//��դ���׶�����
		VkPipelineMultisampleStateCreateInfo multsamplecreateinfo{};//����ݶ��ز�������
		multsamplecreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multsamplecreateinfo.sampleShadingEnable = VK_FALSE;/*�Ƿ����ò�������,�������,����ÿ�����صĸ����ʼ���һ����������,
		���������ֿ���ָ����Щ����Ҫ������,�Ӷ�ʹÿ���������������Լ�����ɫֵ*/
		multsamplecreateinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;//���ò�������,������Ϊ1,�����
		graphicspipelinecreateinfo.pMultisampleState = &multsamplecreateinfo;//����ݶ��ز���
		VkPipelineColorBlendStateCreateInfo colorblend{};
		colorblend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		/*
		Ƭ����ɫ�����ص�Ƭ����ɫ��Ҫ��ԭ��֡�����ж�Ӧ���ص���ɫ���л�ϡ�
		��ϵķ�ʽ����������:
		1.��Ͼ�ֵ����ֵ�������յ���ɫ
		2.ʹ��λ������Ͼ�ֵ����ֵ
		����������������ɫ��ϵĽṹ��:
		1.VkPipelineColorBlendAttachmentState --
	    ��ÿ���󶨵�֡������е�������ɫ�������
		2.VkPipelineColorBlendStateCreateInfo --
		����ȫ�ֵ���ɫ�������
		*/
		colorblend.logicOpEnable = VK_FALSE;//�Ƿ������߼������,���ý�ʹ���߼������������ɫ���
		VkPipelineColorBlendAttachmentState colorblendattachment{};//��ɫ�����������
		colorblendattachment.blendEnable = VK_FALSE;//������ɫ���
		colorblendattachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|
			VK_COLOR_COMPONENT_G_BIT|
			VK_COLOR_COMPONENT_B_BIT|
			VK_COLOR_COMPONENT_A_BIT;//��֡����д����ɫʱ��д����Щ��ɫͨ��
		colorblend.pAttachments = &colorblendattachment;//������ɫ��ϸ���
		colorblend.attachmentCount = 1;//��ɫ��ϸ���������
		graphicspipelinecreateinfo.pColorBlendState = &colorblend;//��ɫ���
		VkPipelineLayoutCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		vkCreatePipelineLayout(device, &createinfo, nullptr, &pipelinelayout);
		graphicspipelinecreateinfo.layout = pipelinelayout;//���ù��߲���
		graphicspipelinecreateinfo.renderPass = renderpass;//����renderpass
		vkCreateGraphicsPipelines(device,nullptr , 1, &graphicspipelinecreateinfo, nullptr, &pipeline);
		vkDestroyShaderModule(device, vertexshadermodule, nullptr);//����shadermodule
		vkDestroyShaderModule(device, fragshadermodule, nullptr);
	}
	void CreateRenderPass() {//����renderpass
		VkAttachmentDescription des{};
		des.format = swapchainimageformat;//�����ĸ�ʽ
		des.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;/*ͼ���ڿ�ʼ����renderpassǰ��Ҫʹ�õĲ��ֽṹ,
		undefined��������ͼ�����ʱ�Ĳ���*/
		des.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//renderpass����ʱͼ��ʹ�õĲ���,�����������ڽ�������չʾ
		des.samples = VK_SAMPLE_COUNT_1_BIT;//��������
		des.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//��Ⱦǰ�����ڶ�Ӧ�������еĲ���,�������ý������
		des.storeOp = VK_ATTACHMENT_STORE_OP_STORE;//��Ⱦ��Ը��������ݽ��еĲ���,�������ñ������ڴ���,�����ȡ��
		des.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;//ģ�����ݼ��ؽ���ʱ���еĲ���,������Ϊ������
		des.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;//ģ��������renderpass����ʱ����δ���,������Ϊ������
        
		VkAttachmentReference ref{};//ָ����Ⱦ�������õĽṹ
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//��Ⱦ������ʹ��ʱ�Ĳ���״̬,���ｫ�䵱����ɫ����ʹ��
		ref.attachment = 0;//ָ��ʹ��pAttachments��Ⱦ�������������һ��
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//��ͨ���е���Ⱦ��������
		subpass.pColorAttachments = &ref;//��ͨ����ʹ�õ���ɫ��������
		subpass.colorAttachmentCount = 1;//��ͨ������ɫ��������

		VkSubpassDependency dep{};
		dep.srcSubpass = VK_SUBPASS_EXTERNAL;//��Ҫִ�е���ͨ��
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;//�ڽ���ڶ�����ͨ��ǰ��һ����ͨ���������ɵ���
		dep.dstSubpass = 0;//��ִ�е���ͨ������
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;//dstsubpass���ڴ��������
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;/*��ҪsrcStageMask��ɺ�ִ�еĽ׶�*/

		VkRenderPassCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createinfo.pAttachments = &des;//�����˸�ͨ����Ҫʲô���ĸ���������
		createinfo.attachmentCount = 1;//renderpass�и���������
		createinfo.pSubpasses = &subpass;//��ͨ������
		createinfo.subpassCount = 1;//��ͨ������
		createinfo.pDependencies = &dep;//��ͨ��������ϵ����(���ж��subpass��ʱ��subpass���Ⱥ�˳��)
		createinfo.dependencyCount = 1;//��ͨ��������ϵ��������
		vkCreateRenderPass(device, &createinfo, nullptr, &renderpass);
	}
	void CreateFrameBuffers() {//����֡����
		framebuffers.resize(imageviews.size());
		for (int i = 0; i < imageviews.size(); i++) {
			VkImageView attachment[] = { imageviews[i] };//���δ���֡�������õ�ͼ����ͼ
			VkFramebufferCreateInfo createinfo{};
			createinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createinfo.attachmentCount = 1;//����Ⱦͨ���а󶨵ĸ�������
			createinfo.pAttachments = attachment;//��Ⱦͨ���а󶨵ĸ���,�����ṩͼ����ͼ���ڷ���
			createinfo.renderPass = renderpass;//ָ�������ĸ���Ⱦͨ��ʹ��
			createinfo.layers = 1;//ͼ���������
			createinfo.height = swapchainimageextent.height;//֡�����С
			createinfo.width = swapchainimageextent.width;
			vkCreateFramebuffer(device, &createinfo, nullptr, &framebuffers[i]);
		}
	}
	void CreateCommandPool() {//���������
		VkCommandPoolCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createinfo.queueFamilyIndex = GetQueueFamilyIndex(GPU).graphics.value();//���з��������彫���ύ���ĸ���������ʹ��
		createinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;/*����غ�����������ģʽ, 
		�������ô������������������, ���ᱻһ������*/
		vkCreateCommandPool(device, &createinfo, nullptr, &commandpool);
	}
	void AllocateCommandBuffer() {//��������з��������
		VkCommandBufferAllocateInfo allocateinfo{};
		allocateinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateinfo.commandPool = commandpool;//ָ�����ĸ�������з���
		allocateinfo.commandBufferCount = 1;//Ҫ��������з�����ٸ������
		allocateinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//ָ�������ģʽ,��������Ϊ���Ա�ֱ�Ӵ���GPU��ִ��
		vkAllocateCommandBuffers(device, &allocateinfo, &commandbuffer);
	}
	void RecordCommandBuffer(VkCommandBuffer commandbuffer,uint32_t imageindex) {
		VkCommandBufferBeginInfo begininfo{};//��������ʼ¼�������������
		begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begininfo.flags = 0;//����������ģʽ,�˴���ΪĬ����Ϊ

		VkRenderPassBeginInfo renderpassbegininfo{};//��ʼ��Ⱦͨ���������
		renderpassbegininfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpassbegininfo.framebuffer = framebuffers[imageindex];//ָ��Ҫʹ�õ�֡����
		renderpassbegininfo.renderPass = renderpass;//ָ��Ҫִ�е���Ⱦͨ��
		renderpassbegininfo.renderArea.extent = swapchainimageextent;//��Ⱦ����ķ�Χ
		renderpassbegininfo.renderArea.offset = { 0,0 };//��Ⱦ����(һ������)��λ��
		renderpassbegininfo.clearValueCount = 1;//��pClearValues��ȡ������ֵ
		VkClearValue cc = {{0,0,0,1}};
		renderpassbegininfo.pClearValues = &cc;//��ո���ʱ�ı�����ɫ

		vkBeginCommandBuffer(commandbuffer, &begininfo);//��ʼ¼�������
		vkCmdBeginRenderPass(commandbuffer, &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);/*��ʼͨ��, 
		���һ������Ϊ��ͨ���ĵ��÷�ʽ,��������Ϊ����������ֱ��ִ��*/
		vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);/*����Ⱦ����,�ڶ�������ΪҪ�󶨵Ĺ�������*/
		vkCmdDraw(commandbuffer, 3, 1, 0, 0);/*����,�ڶ�������Ϊ��������,����������Ϊʵ������,���ĸ�����Ϊ��ʼ���������,�����
		Ϊʵ��������*/
		vkCmdEndRenderPass(commandbuffer);//����ͨ��
		vkEndCommandBuffer(commandbuffer);//����¼��
	}
	void CreateSemaphore() {//�����ź���
		VkSemaphoreCreateInfo createinfo{};
		createinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(device, &createinfo, nullptr, &getimagestartrender);
		vkCreateSemaphore(device, &createinfo, nullptr, &finishrenderpresentimage);
	}
	void draw() {//����
		//��ȡͼ��
		uint32_t imageindex = 0;
		vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, getimagestartrender, nullptr, &imageindex);/*��ȡ�������ϵ�һ�ſ���ͼ�����
		��,�����������ǵȴ�ʱ�������,���ĸ��ǻ�ȡͼ���Ҫ������ź�,�������դ��(��һЩ����¿ɱ��ź�������)*/
		vkQueueWaitIdle(graphicsqueue);//�ȴ�����ִ�����
		vkResetCommandBuffer(commandbuffer, 0);/*���������,�ڶ�������Ϊ�������������ģʽ,������ΪĬ��*/
		RecordCommandBuffer(commandbuffer, imageindex);
		//�������ύ������
		VkSubmitInfo submitinfo{};//�������ύ������ʱ�������
		submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitinfo.pWaitSemaphores = &getimagestartrender;//�ȴ����ź���
		submitinfo.waitSemaphoreCount = 1;//�ȴ����ź���������
		VkPipelineStageFlags dst[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitinfo.pWaitDstStageMask = dst;//�ڽ��յ��ź��������ִ�еĹ��߽׶�
		submitinfo.pCommandBuffers = &commandbuffer;//Ҫ�ύ�����е������
		submitinfo.commandBufferCount = 1;//���������
		submitinfo.pSignalSemaphores = &finishrenderpresentimage;//�������ɺ󷢳����ź���
		submitinfo.signalSemaphoreCount = 1; //�������ɺ󷢳����ź�������
		vkQueueSubmit(graphicsqueue, 1, &submitinfo, nullptr);//��������ύ������,���һ������Ϊդ��
		VkPresentInfoKHR presentinfo{};//ͼ������������
		presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentinfo.pWaitSemaphores = &finishrenderpresentimage;//Ҫ�ȴ����ź���
		presentinfo.waitSemaphoreCount = 1;//�ȴ����ź���������
		presentinfo.pSwapchains = &swapchain;//Ҫʹ�õĽ���������
		presentinfo.swapchainCount = 1;//����������
		presentinfo.pImageIndices = &imageindex;//Ҫʹ�õ�ͼ����������
		vkQueuePresentKHR(presentqueue, &presentinfo);
	}
};
int main() {
	APP a;
	a.run();
}