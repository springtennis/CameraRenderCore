#include "pch.h"

#include "SentechCameraCore.h"

#include <vector>
#include <thread>
#include <mutex>

using namespace StApi;
using namespace std;

#define FPS     270
#define GetCNodePtr(x, nodeMap)	GenApi::CNodePtr(nodeMap->GetNode(x))

DWORD WINAPI mainThreadFunction(LPVOID lpParam)
{
	SentechCameraCore::Core* core = (SentechCameraCore::Core*)lpParam;

	core->loopEnd = 0;

	///////////////////////////////////
	// [[ Set StreamBitmapRenderer ]]
	///////////////////////////////////
	HRESULT hr = S_OK;
	DisplayInfo defaultDisplayInfo = {
		0.0f, 0.0f, // startX, startY
		1.0f, 1.0f, // lenX, lenY
		core->m_dpiXScale, core->m_dpiYScale, // dpiXScale, dpiYScale
		0, // zIndex
		BitmapRenderer::Frame_DisplayModeNone // displayMode
	};

	hr = core->m_streamBitmapRenderer.InitInstance(core->m_hwndHost);

	///////////////////////////////////
	// [[ Init STApi Camera ]]
	///////////////////////////////////
	CStApiAutoInit objStApiAutoInit;
	CIStSystemPtr pIStSystem(CreateIStSystem());
	CIStDevicePtrArray pIStDeviceList;
	CIStDataStreamPtrArray pIStDataStreamList;

	// Try to connect to all possible device
	core->m_cameraCount = 0;
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
		GenApi::INodeMap* pINodeMap = pIStDeviceList[core->m_cameraCount]->GetRemoteIStPort()->GetINodeMap();
		float fpsMax = GenApi::CFloatPtr(GetCNodePtr("AcquisitionFrameRate", pINodeMap))->GetMax();

		// fps must be multiplication of 30
		float fpsTarget = (int)(fpsMax / 30.0) * 30;
		GenApi::CFloatPtr(GetCNodePtr("AcquisitionFrameRate", pINodeMap))->SetValue(fpsTarget);

		size_t frameWidth = (size_t)GenApi::CIntegerPtr(GetCNodePtr("Width", pINodeMap))->GetValue();
		size_t frameHeight = (size_t)GenApi::CIntegerPtr(GetCNodePtr("Height", pINodeMap))->GetValue();
		float fps = GenApi::CFloatPtr(GetCNodePtr("AcquisitionFrameRate", pINodeMap))->GetValue();

		DisplayHandler defaultDisplayHandler = core->m_streamBitmapRenderer.RegisterBitmapRenderer(defaultDisplayInfo);
		char* rawBuffer = new char[frameWidth * frameHeight];
		core->m_CameraHandler.push_back(
			{ frameWidth, frameHeight, fps, 
			defaultDisplayHandler, rawBuffer, 0, {}, NULL, 1, false });

		pIStDataStreamList.Register(pIStDeviceReleasable->CreateIStDataStream(0));
		core->m_cameraCount++;
	}

	///////////////////////////////////
	// [[ Start Capter & display ]]
	///////////////////////////////////
	//pIStDataStreamList.StartAcquisition(GENTL_INFINITE);
	//pIStDeviceList.AcquisitionStart();
	INT mostFastCameraIdx = -1;

	core->m_atomicInt = 0; // Init finish

	while (core->available)
	{
		try {

			if (core->m_displayChange)
			{
				int i;

				// Turn On/Off camera
				for (i = 0; i < core->m_cameraCount; i++)
				{
					if (core->m_CameraHandler[i].acquisitionState &&
						core->m_CameraHandler[i].displayHandler.displayInfo.displayMode == BitmapRenderer::Frame_DisplayModeNone)
					{
						pIStDeviceList[i]->AcquisitionStop();
						//pIStDataStreamList[i]->StopAcquisition();
						core->m_CameraHandler[i].acquisitionState = false;
					}
					else if (!core->m_CameraHandler[i].acquisitionState &&
						core->m_CameraHandler[i].displayHandler.displayInfo.displayMode != BitmapRenderer::Frame_DisplayModeNone)
					{
						pIStDataStreamList[i]->StartAcquisition(GENTL_INFINITE);
						pIStDeviceList[i]->AcquisitionStart();
						core->m_CameraHandler[i].acquisitionState = true;
					}
				}

				mostFastCameraIdx = -1;
				for (i = 0; i < core->m_cameraCount; i++)
				{
					if (mostFastCameraIdx < 0)
					{
						if (core->m_CameraHandler[i].acquisitionState)
							mostFastCameraIdx = i;
						
						continue;
					}

					if (core->m_CameraHandler[i].maxFps > core->m_CameraHandler[mostFastCameraIdx].maxFps)
						mostFastCameraIdx = i;
				}
				core->m_displayChange = false;
			}

			// Get Camera frame
			CIStStreamBufferPtr pIStStreamBuffer(pIStDataStreamList.RetrieveBuffer(1000));

			if (!pIStStreamBuffer->GetIStStreamBufferInfo()->IsImagePresent())
				continue;

			IStDevice* pIStDevice = pIStStreamBuffer->GetIStDataStream()->GetIStDevice();
			IStImage* pIStImage = pIStStreamBuffer->GetIStImage(); // Get Raw Frame
			uint64_t id = pIStStreamBuffer->GetIStStreamBufferInfo()->GetFrameID();

			// Find camera device
			int idx;
			for (idx = 0; idx < core->m_cameraCount; idx++)
				if (pIStDevice == pIStDeviceList[idx]) break;

			if (idx == core->m_cameraCount)
				continue;

			if (!core->m_CameraHandler[idx].acquisitionState)
				continue;

			// Debayering
			const StApi::EStPixelFormatNamingConvention_t ePFNC = pIStImage->GetImagePixelFormat();
			StApi::IStPixelFormatInfo* const pIStPixelFormatInfo = StApi::GetIStPixelFormatInfo(ePFNC);

			if (!pIStPixelFormatInfo->IsBayer())
				continue;

			const size_t nImageWidth = pIStImage->GetImageWidth();
			const size_t nImageHeight = pIStImage->GetImageHeight();

			SentechCameraCore::CameraHandler* targetCameraHandler = &core->m_CameraHandler[idx];
			targetCameraHandler->frameCount++;
			memcpy(
				targetCameraHandler->buffer,
				pIStImage->GetImageBuffer(),
				targetCameraHandler->frameWidth * targetCameraHandler->frameHeight
			);

			switch (pIStPixelFormatInfo->GetPixelColorFilter()) {
			case(StPixelColorFilter_BayerRG): targetCameraHandler->pixelFormat = StreamBitmapRenderer::BITMAP_BAYER_RG;	break;
			case(StPixelColorFilter_BayerGR): targetCameraHandler->pixelFormat = StreamBitmapRenderer::BITMAP_BAYER_GR;	break;
			case(StPixelColorFilter_BayerGB): targetCameraHandler->pixelFormat = StreamBitmapRenderer::BITMAP_BAYER_GB;	break;
			case(StPixelColorFilter_BayerBG): targetCameraHandler->pixelFormat = StreamBitmapRenderer::BITMAP_BAYER_BG;	break;
			default:
				continue;
			}

			// If not the fastest device
			if (mostFastCameraIdx == -1)
				continue;

			if (idx != mostFastCameraIdx)
				continue;

			// <---------------------------------------------------------------------------------------------------------- Critical Section
			while (core->m_atomicInt != 0);
			core->m_atomicInt = 1;

			std::vector<SentechCameraCore::CameraHandler>::iterator it;
			for (it = core->m_CameraHandler.begin(); it != core->m_CameraHandler.end(); it++)
			{
				core->m_streamBitmapRenderer.RegisterBitmapBuffer(
					&it->displayHandler,
					it->buffer,
					it->frameWidth,
					it->frameHeight,
					it->pixelFormat);
			}

			switch (core->m_recordState) {
			case core->REC_STOPPED:
				break;

			case core->REC_STARTED:
			{
				std::vector<SentechCameraCore::CameraHandler>::iterator it;
				for (it = core->m_CameraHandler.begin(); it != core->m_CameraHandler.end(); it++)
				{
					if (it->recorder)
					{
						it->recorder->end();
						delete it->recorder;
					}

					if (it->filepath[0] != '\0' && it->acquisitionState)
					{
						it->recorder = new Mp4Recorder(
							it->filepath,
							it->frameWidth,
							it->frameHeight,
							it->maxFps
						);

						it->recorder->init();
						it->recorder->start();

						it->filepath[0] = '\0'; // delete filepath after recorder start
					}
				}
			}
			PlaySound(L"C:\\Users\\Bluesink\\Music\\play.wav", NULL, SND_FILENAME | SND_ASYNC);
			core->m_recordState = core->REC_RECORDING;
			break;

			case core->REC_RECORDING:
			{
				std::vector<SentechCameraCore::CameraHandler>::iterator it;
				for (it = core->m_CameraHandler.begin(); it != core->m_CameraHandler.end(); it++)
				{
					if (it->recorder && it->acquisitionState)
						it->recorder->put(it->displayHandler.pBitmapRenderer->m_pBitmapBuffer);
				}
			}
			break;

			case core->REC_STOPPING:
			{
				std::vector<SentechCameraCore::CameraHandler>::iterator it;
				for (it = core->m_CameraHandler.begin(); it != core->m_CameraHandler.end(); it++)
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
			core->m_recordState = core->REC_STOPPED;
			break;
			}

			if (id % 10 == 0)
			{
				core->m_streamBitmapRenderer.DrawOnce();
			}

			core->m_atomicInt = 0;
			// Critical Section ----------------------------------------------------------------------------------------------------------->

		}
		catch (...)
		{
			continue;
		}
	}

	for (int i = 0; i < core->m_cameraCount; i++)
	{
		pIStDeviceList[i]->AcquisitionStop();
		pIStDataStreamList[i]->StopAcquisition();
	}

	core->loopEnd = 1;

	return 0;
}

namespace SentechCameraCore {
	
	Core::Core()
		:m_hwndHost(NULL),
		m_atomicInt(-1),
		m_cameraCount(0),
		m_recordState(0),
		m_displayChange(false)
	{}

	Core::~Core()
	{
		available = 0;
		while (loopEnd == 0);
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

		HANDLE hThread = CreateThread(
			NULL,
			0,
			mainThreadFunction,
			this,
			0,
			NULL);
		SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

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
		
		core->m_displayChange = true;
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