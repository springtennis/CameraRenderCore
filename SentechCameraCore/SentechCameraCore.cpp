#include "pch.h"

#include "SentechCameraCore.h"

#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

using namespace StApi;
using namespace std;

#define FPS     60
#define GetCNodePtr(x, nodeMap)	GenApi::CNodePtr(nodeMap->GetNode(x))

namespace SentechCameraCore {
	
	Core::Core()
		:m_hwndHost(NULL),
		m_atomicInt(-1),
		m_cameraCount(0),
		m_recordState(0)
	{}

	Core::~Core()
	{
		available = 0;
		while (loopEnd == 0);
	}

	void Core::loop()
	{
		loopEnd = 0;

		///////////////////////////////////
		// [[ Set StreamBitmapRenderer ]]
		///////////////////////////////////
		HRESULT hr = S_OK;
		DisplayInfo defaultDisplayInfo = {
			0.0f, 0.0f, // startX, startY
			1.0f, 1.0f, // lenX, lenY
			m_dpiXScale, m_dpiYScale, // dpiXScale, dpiYScale
			0, // zIndex
			BitmapRenderer::Frame_DisplayModeNone // displayMode
		};

		hr = m_streamBitmapRenderer.InitInstance(m_hwndHost);

		///////////////////////////////////
		// [[ Init STApi Camera ]]
		///////////////////////////////////
		CStApiAutoInit objStApiAutoInit;
		CIStSystemPtr pIStSystem(CreateIStSystem());
		CIStDevicePtrArray pIStDeviceList;
		CIStDataStreamPtrArray pIStDataStreamList;

		// Try to connect to all possible device
		m_cameraCount = 0;
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
			GenApi::INodeMap* pINodeMap = pIStDeviceList[m_cameraCount]->GetRemoteIStPort()->GetINodeMap();
			GenApi::CFloatPtr(GetCNodePtr("AcquisitionFrameRate", pINodeMap))->SetValue(FPS);

			size_t frameWidth = (size_t)GenApi::CIntegerPtr(GetCNodePtr("Width", pINodeMap))->GetValue();
			size_t frameHeight = (size_t)GenApi::CIntegerPtr(GetCNodePtr("Height", pINodeMap))->GetValue();
			float fps = GenApi::CFloatPtr(GetCNodePtr("AcquisitionFrameRate", pINodeMap))->GetValue();
			
			DisplayHandler defaultDisplayHandler = m_streamBitmapRenderer.RegisterBitmapRenderer(defaultDisplayInfo);
			char* rawBuffer = new char[frameWidth * frameHeight];
			m_CameraHandler.push_back({ frameWidth, frameHeight, fps, defaultDisplayHandler, rawBuffer, 0, {}, NULL });

			pIStDataStreamList.Register(pIStDeviceReleasable->CreateIStDataStream(0));
			m_cameraCount++;
		}

		///////////////////////////////////
		// [[ Start Capter & display ]]
		///////////////////////////////////
		pIStDataStreamList.StartAcquisition(GENTL_INFINITE);
		pIStDeviceList.AcquisitionStart();

		m_atomicInt = 0; // Init finish

		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		while (available)
		{
			std::chrono::system_clock::time_point current = std::chrono::system_clock::now();
			std::chrono::duration<double>sec = current - start;

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
			case(StPixelColorFilter_BayerRG): nConvert = StreamBitmapRenderer::BITMAP_BAYER_RG;	break;
			case(StPixelColorFilter_BayerGR): nConvert = StreamBitmapRenderer::BITMAP_BAYER_GR;	break;
			case(StPixelColorFilter_BayerGB): nConvert = StreamBitmapRenderer::BITMAP_BAYER_GB;	break;
			case(StPixelColorFilter_BayerBG): nConvert = StreamBitmapRenderer::BITMAP_BAYER_BG;	break;
			default:
				continue;
			}

			// Find camera device
			int idx;
			for (idx = 0; idx < m_cameraCount; idx++)
				if (pIStDevice == pIStDeviceList[idx]) break;

			if (idx == m_cameraCount)
				continue;

			CameraHandler* targetCameraHandler = &m_CameraHandler[idx];
			targetCameraHandler->frameCount++;
			memcpy(
				targetCameraHandler->buffer,
				pIStImage->GetImageBuffer(),
				targetCameraHandler->frameWidth * targetCameraHandler->frameHeight
			);

			// If not last device, then skip
			if (pIStDevice != pIStDeviceList[m_cameraCount - 1])
				continue;

			// <---------------------------------------------------------------------------------------------------------- Critical Section
			while (m_atomicInt != 0);
			m_atomicInt = 1;

			std::vector<CameraHandler>::iterator it;
			for (it = m_CameraHandler.begin(); it != m_CameraHandler.end(); it++)
			{
				m_streamBitmapRenderer.RegisterBitmapBuffer(
					&it->displayHandler,
					it->buffer,
					it->frameWidth,
					it->frameHeight,
					nConvert);
			}

			switch (m_recordState) {
			case REC_STOPPED:
				break;

			case REC_STARTED:
				{
					std::vector<CameraHandler>::iterator it;
					for (it = m_CameraHandler.begin(); it != m_CameraHandler.end(); it++)
					{
						if (it->recorder)
						{
							it->recorder->end();
							delete it->recorder;
						}

						it->recorder = new Mp4Recorder(
							it->filepath,
							it->frameWidth,
							it->frameHeight,
							it->maxFps
						);

						it->recorder->init();
						it->recorder->start();
					}
				}
				m_recordState = REC_RECORDING;
				break;

			case REC_RECORDING:
				{
					std::vector<CameraHandler>::iterator it;
					for (it = m_CameraHandler.begin(); it != m_CameraHandler.end(); it++)
					{
						if (it->recorder)
							it->recorder->put(it->displayHandler.pBitmapRenderer->m_pBitmapBuffer);
					}
				}
				break;

			case REC_STOPPING:
				{
					std::vector<CameraHandler>::iterator it;
					for (it = m_CameraHandler.begin(); it != m_CameraHandler.end(); it++)
					{
						if (it->recorder)
						{
							it->recorder->end();
							delete it->recorder;
							it->recorder = NULL;
							it->filepath[0] = '\0';
						}
					}
				}
				m_recordState = REC_STOPPED;
				break;
			}

			if (sec.count() > 0.034)
			{
				start = current;
				m_streamBitmapRenderer.DrawOnce();
			}

			m_atomicInt = 0;
			// Critical Section ----------------------------------------------------------------------------------------------------------->
		}

		pIStDeviceList.AcquisitionStop();
		pIStDataStreamList.StopAcquisition();

		loopEnd = 1;
	}

	void Core::SetRecordInfo(UINT cameraIdx, char* filepath)
	{
		strcpy(m_CameraHandler[cameraIdx].filepath, filepath);
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
		while (core->m_atomicInt != 0);
		core->m_atomicInt = 1;
		core->m_streamBitmapRenderer.Resize(hostWidth, HostHeight);
		core->m_atomicInt = 0;
	}

	UINT Wrapper::GetCameraCount()
	{
		return core->m_cameraCount;
	}

	void Wrapper::SetDisplayInfo(UINT cameraIdx, float startX, float startY, float lenX, float lenY, int zIndex, UINT displayMode)
	{
		while (core->m_atomicInt != 0);
		core->m_atomicInt = 1;
		
		if (cameraIdx >= core->m_cameraCount)
		{
			core->m_atomicInt = 0;
			return;
		}

		DisplayInfo newDisplayInfo;
		newDisplayInfo.startX = startX;
		newDisplayInfo.startY = startY;
		newDisplayInfo.lenX = lenX;
		newDisplayInfo.lenY = lenY;
		newDisplayInfo.dpiXScale = core->m_CameraHandler[cameraIdx].displayHandler.displayInfo.dpiXScale;
		newDisplayInfo.dpiYScale = core->m_CameraHandler[cameraIdx].displayHandler.displayInfo.dpiYScale;
		newDisplayInfo.zIndex = zIndex;
		newDisplayInfo.displayMode = displayMode;

		core->m_streamBitmapRenderer.ModifyBitmapRenderer(
			&core->m_CameraHandler[cameraIdx].displayHandler,
			newDisplayInfo);
		
		core->m_atomicInt = 0;
	}

	void Wrapper::SetRecordInfo(UINT cameraIdx, char* filepath)
	{
		while (core->m_atomicInt != 0);
		core->m_atomicInt = 1;

		if (cameraIdx >= core->m_cameraCount)
		{
			core->m_atomicInt = 0;
			return;
		}

		core->SetRecordInfo(cameraIdx, filepath);

		core->m_atomicInt = 0;
	}

	void Wrapper::StartRecord()
	{
		while (core->m_atomicInt != 0);
		core->m_atomicInt = 1;
		core->m_recordState = Core::REC_STARTED;
		core->m_atomicInt = 0;
	}

	void Wrapper::StopRecord()
	{
		while (core->m_atomicInt != 0);
		core->m_atomicInt = 1;
		if(core->m_recordState == Core::REC_RECORDING)
			core->m_recordState = Core::REC_STOPPING;
		core->m_atomicInt = 0;
	}
}