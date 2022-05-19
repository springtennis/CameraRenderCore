#include "pch.h"

#include "SentechCameraCore.h"

#include <vector>
#include <thread>

using namespace StApi;
using namespace std;

#define FPS     60
#define GetCNodePtr(x, nodeMap)	GenApi::CNodePtr(nodeMap->GetNode(x))

namespace SentechCameraCore {
	
	Core::Core()
		:m_hwndHost(NULL)
	{}

	Core::~Core()
	{
		available = 0;
		while (loopEnd == 0);
	}

	void Core::loop()
	{
		loopEnd = 0;

		vector<size_t> frameWidth;
		vector<size_t> frameHeight;

		///////////////////////////////////
		// [[ Init STApi Camera ]]
		///////////////////////////////////
		CStApiAutoInit objStApiAutoInit;
		CIStSystemPtr pIStSystem(CreateIStSystem());
		CIStDevicePtrArray pIStDeviceList;
		CIStDataStreamPtrArray pIStDataStreamList;

		// Try to connect to all possible device
		int i = 0;
		while (1) {
			IStDeviceReleasable* pIStDeviceReleasable = NULL;
			try {
				pIStDeviceReleasable = pIStSystem->CreateFirstIStDevice(GenTL::DEVICE_ACCESS_EXCLUSIVE);
			}
			catch (...) {
				break;
			}

			pIStDeviceList.Register(pIStDeviceReleasable);

			// Set & Get basic camera info
			GenApi::INodeMap* pINodeMap = pIStDeviceList[i]->GetRemoteIStPort()->GetINodeMap();
			GenApi::CFloatPtr(GetCNodePtr("AcquisitionFrameRate", pINodeMap))->SetValue(FPS);

			frameWidth.push_back((size_t)GenApi::CIntegerPtr(GetCNodePtr("Width", pINodeMap))->GetValue());
			frameHeight.push_back((size_t)GenApi::CIntegerPtr(GetCNodePtr("Height", pINodeMap))->GetValue());
			float fps = GenApi::CFloatPtr(GetCNodePtr("AcquisitionFrameRate", pINodeMap))->GetValue();
			pIStDataStreamList.Register(pIStDeviceReleasable->CreateIStDataStream(0));
			i++;
		}

		///////////////////////////////////
		// [[ Set StreamBitmapRenderer ]]
		///////////////////////////////////

		// Init StreamBitmapRenderer
		HRESULT hr = S_OK;
		hr = m_streamBitmapRenderer.InitInstance(m_hwndHost);
		DisplayHandler camera1Handler = m_streamBitmapRenderer.RegisterBitmapRenderer({0.0f, 0.0f, 1.0f, 1.0f, m_dpiXScale , m_dpiYScale,1});

		///////////////////////////////////
		// [[ Start Capter & display ]]
		///////////////////////////////////
		pIStDataStreamList.StartAcquisition(GENTL_INFINITE);
		pIStDeviceList.AcquisitionStart();

		while (available)
		{
			// Get Camera frame
			CIStStreamBufferPtr pIStStreamBuffer(pIStDataStreamList.RetrieveBuffer(5000));
			if (!pIStStreamBuffer->GetIStStreamBufferInfo()->IsImagePresent())
				continue;

			IStDevice* pIStDevice = pIStStreamBuffer->GetIStDataStream()->GetIStDevice();
            IStImage* pIStImage = pIStStreamBuffer->GetIStImage(); // Get Raw Frame
            uint64_t id = pIStStreamBuffer->GetIStStreamBufferInfo()->GetFrameID();

			// Debayering
			const StApi::EStPixelFormatNamingConvention_t ePFNC = pIStImage->GetImagePixelFormat();
			StApi::IStPixelFormatInfo* const pIStPixelFormatInfo = StApi::GetIStPixelFormatInfo(ePFNC);
			
			if (!pIStPixelFormatInfo->IsBayer())
				continue;

			const size_t nImageWidth = pIStImage->GetImageWidth();
			const size_t nImageHeight = pIStImage->GetImageHeight();

			int nConvert = 0;
			switch (pIStPixelFormatInfo->GetPixelColorFilter()){
			case(StPixelColorFilter_BayerRG): nConvert = BitmapRenderer::Frame_BayerRG;	break;
			case(StPixelColorFilter_BayerGR): nConvert = BitmapRenderer::Frame_BayerGR;	break;
			case(StPixelColorFilter_BayerGB): nConvert = BitmapRenderer::Frame_BayerGB;	break;
			case(StPixelColorFilter_BayerBG): nConvert = BitmapRenderer::Frame_BayerBG;	break;
			default:
				continue;
			}

			m_streamBitmapRenderer.RegisterBayerBitmapBuffer(&camera1Handler, nConvert, pIStImage->GetImageBuffer(), frameWidth[0], frameHeight[0]);
			m_streamBitmapRenderer.DrawOnce();
		}

		pIStDeviceList.AcquisitionStop();
		pIStDataStreamList.StopAcquisition();

		loopEnd = 1;
	}

	IntPtr Core::Init(
		int hostHeight,
		int hostWidth,
		float dpiXScale,
		float dpiYScale,
		void* hwndParent)
	{
		// Create window
		HWND parent = reinterpret_cast<HWND>(hwndParent);

		m_hwndHost = CreateWindowEx(0, L"static", L"",
			WS_CHILD | WS_VISIBLE,
			0, 0,
			hostWidth, hostHeight,
			parent,
			0, 0, 0);

		m_dpiXScale = dpiXScale;
		m_dpiYScale = dpiYScale;

		available = 1;
		std::thread t = std::thread(&Core::loop, this);
		t.detach();

		return IntPtr(m_hwndHost);
	}
}

namespace SentechCameraCore {
	Wrapper::Wrapper()
		:core(new Core)
	{}

	Wrapper::~Wrapper()
	{
		delete core;
		core = 0;
	}

	IntPtr Wrapper::Init(
		int hostHeight,
		int hostWidth,
		float dpiXScale,
		float dpiYScale,
		void* hwndParent)
	{
		return core->Init(hostHeight, hostWidth, dpiXScale, dpiYScale, hwndParent);
	}

	void Wrapper::Resize(int HostHeight, int hostWidth)
	{
		core->m_streamBitmapRenderer.Resize(hostWidth, HostHeight);
	}
}