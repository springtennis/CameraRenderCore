#pragma once
#include <windows.h>

#include "StreamBitmapRenderer.h"
#include "Mp4Recorder.h"

#include <StApi_TL.h>

#include <atomic>

using namespace System;

namespace SentechCameraCore {

	struct CameraHandler {
		size_t frameWidth;
		size_t frameHeight;
		float maxFps;

		DisplayHandler displayHandler;
		char* buffer;
		size_t frameCount;
	};

	public class Core
	{
	private:
		int available;
		int loopEnd;

		float m_dpiXScale;
		float m_dpiYScale;

	public:
		HWND m_hwndHost;
		StreamBitmapRenderer m_streamBitmapRenderer;
		Mp4Recorder* mp4Recorder;

		std::atomic<int> m_atomicInt;
		UINT m_cameraCount;
		std::vector<CameraHandler> m_CameraHandler;

		Core();
		~Core();

		IntPtr Init(
			int hostHeight,
			int hostWidth,
			float dpiXScale,
			float dpiYScale,
			void* hwndParent);

		void loop();
	};

	public ref class Wrapper
	{
	protected:
		Core* core;

	public:
		Wrapper();
		~Wrapper();

		IntPtr Init(
			int HostHeight,
			int hostWidth,
			float dpiXScale,
			float dpiYScale,
			void* hwndParent);

		void Resize(int HostHeight, int hostWidth);
		UINT GetCameraCount();
		void SetDisplayInfo(UINT cameraIdx, float startX, float startY, float lenX, float lenY, int zIndex, UINT displayMode);
	};
}