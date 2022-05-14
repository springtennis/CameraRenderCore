#pragma once

#include "framework.h"

class BitmapRenderer
{
public:
	BitmapRenderer();
	~BitmapRenderer();

	ID2D1Bitmap* m_pBitmap;

	HRESULT InitInstance(
		ID2D1HwndRenderTarget* pRenderTarge,
		FLOAT startX, FLOAT startY,
		FLOAT lenX, FLOAT lenY,
		FLOAT dpiX, FLOAT dpiY);

	HRESULT RegisterBuffer(void* pBuffer, UINT width, UINT height);
	void GetBuffer(void** pBuffer, UINT* width, UINT* height);

	HRESULT Update();

private:
	void* m_pBitmapBuffer;
	ID2D1BitmapBrush* m_pBitmapBrush;
	ID2D1HwndRenderTarget* m_pRenderTarget;

	FLOAT m_startX, m_startY;
	FLOAT m_lenX, m_lenY;
	FLOAT m_dpiXScale, m_dpiYScale;
};

