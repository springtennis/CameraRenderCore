#pragma once

#include "BitmapRenderer.h"

struct DisplayInfo {
	FLOAT startX, startY;
	FLOAT lenX, lenY;
};

class StreamBitmapRenderer
{
private:
	HWND m_hwndHost;
	ID2D1Factory* m_pDirect2dFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;

	UINT m_nDisplay;
	BitmapRenderer* m_pBitmapRenderer;

public:

	StreamBitmapRenderer();
	~StreamBitmapRenderer();

	HRESULT InitInstance(
		HWND hwndHost,
		UINT nDisplay,
		DisplayInfo* displayInfo);

	HRESULT RegisterBitmapBuffer(
		UINT n,
		void* pBuffer,
		UINT width,
		UINT height);

	void Resize(UINT width, UINT height);
	void DrawOnce();
};

