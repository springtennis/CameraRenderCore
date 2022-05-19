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
	static const UINT BITMAP_RGBA = 0;
	static const UINT BITMAP_BAYER_RG = 1;
	static const UINT BITMAP_BAYER_GR = 2;
	static const UINT BITMAP_BAYER_GB = 3;
	static const UINT BITMAP_BAYER_BG = 4;
	
	StreamBitmapRenderer();
	~StreamBitmapRenderer();

	HRESULT InitInstance(HWND hwndHost);

	DisplayHandler RegisterBitmapRenderer(DisplayInfo displayInfo);

	HRESULT RegisterBitmapBuffer(
		DisplayHandler* displayHandler,
		void* pBuffer,
		UINT width,
		UINT height,
		UINT bitmapType);

	void Resize(UINT width, UINT height);
	void DrawOnce();
};

