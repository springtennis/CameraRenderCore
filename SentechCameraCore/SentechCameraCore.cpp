#include "pch.h"

#include "SentechCameraCore.h"

#include <vector>
#include <random>
#include <chrono>
#include <thread>

using namespace StApi;
using namespace std;

#define FPS     30
#define GetCNodePtr(x, nodeMap)	GenApi::CNodePtr(nodeMap->GetNode(x))

void randomBitmap(CHAR* pBuffer, UINT width, UINT height)
{
	UINT i, j;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int>dis(0, 255);

	for (i = 0; i < height; i++)
	{
		UINT startRowIdx = i * width * 4;
		for (j = 0; j < width; j++)
		{
			UINT startIdx = startRowIdx + 4 * j;
			pBuffer[startIdx + 0] = dis(gen); // B
			pBuffer[startIdx + 1] = i == j ? 255 : 0; // G
			pBuffer[startIdx + 2] = i == j ? 255 : 0; // R
			pBuffer[startIdx + 3] = 255; // A
		}
	}
}

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
		DisplayInfo dInfo[] = {
			{0.0f ,0.0f ,1.0f, 1.0f, m_dpiXScale, m_dpiYScale}
		};
		hr = m_streamBitmapRenderer.InitInstance(m_hwndHost, 1, dInfo);

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

			m_streamBitmapRenderer.RegisterBitmapBuffer(0, pIStImage->GetImageBuffer(), frameWidth[0] / 4, frameHeight[0] / 4);
			m_streamBitmapRenderer.DrawOnce();
			 
			//// Conversion
			//if (!RawFrameToBGRA(pIStImage, &inputMat[0], &outputMat[0]))
			//	continue;

			//if (outputMat[0].size().height <= 0 ||
			//	outputMat[0].size().width <= 0)
			//	continue;

			//m_streamBitmapRenderer.RegisterBitmapBuffer(0, outputMat[0].ptr(0), frameWidth[0], frameHeight[0]);
			//m_streamBitmapRenderer.DrawOnce();
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