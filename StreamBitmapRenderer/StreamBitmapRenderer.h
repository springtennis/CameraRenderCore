#pragma once

#include "BitmapRenderer.h"
#include <vector>

struct DisplayInfo {
	FLOAT startX, startY;
	FLOAT lenX, lenY;
	FLOAT dpiXScale, dpiYScale;
	UINT zIndex;
};

struct DisplayHandler {
	BitmapRenderer* pBitmapRenderer;
	DisplayInfo displayInfo;
};

class StreamBitmapRenderer
{
private:
	HWND m_hwndHost;
	ID2D1Factory* m_pDirect2dFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;

	std::vector<DisplayHandler> m_DisplayHandler;

public:

	StreamBitmapRenderer();
	~StreamBitmapRenderer();

	HRESULT InitInstance(HWND hwndHost);

	DisplayHandler RegisterBitmapRenderer(DisplayInfo displayInfo);

	HRESULT RegisterBitmapBuffer(
		DisplayHandler* displayHandler,
		void* pBuffer,
		UINT width,
		UINT height);

	HRESULT RegisterBayerBitmapBuffer(
		DisplayHandler* displayHandler,
		UINT bayerType,
		void* pBuffer,
		UINT width,
		UINT height);

	void Resize(UINT width, UINT height);
	void DrawOnce();
};

