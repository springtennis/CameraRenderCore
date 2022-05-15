#pragma once
#include <windows.h>

#include "StreamBitmapRenderer.h"
#include "Mp4Recorder.h"

#include <StApi_TL.h>

using namespace System;

namespace SentechCameraCore {
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
	};
}