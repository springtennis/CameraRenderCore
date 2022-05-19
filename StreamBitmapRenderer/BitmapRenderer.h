#pragma once

#include "framework.h"

class BitmapRenderer
{
public:
	static const UINT Frame_BayerRG = 1;
	static const UINT Frame_BayerGR = 2;
	static const UINT Frame_BayerGB = 3;
	static const UINT Frame_BayerBG = 4;

	BitmapRenderer();
	~BitmapRenderer();

	ID2D1Bitmap* m_pBitmap;

	HRESULT InitInstance(
		ID2D1HwndRenderTarget* pRenderTarge,
		FLOAT startX, FLOAT startY,
		FLOAT lenX, FLOAT lenY,
		FLOAT dpiX, FLOAT dpiY);

	HRESULT ChangeDisplayArea(
		FLOAT startX, FLOAT startY,
		FLOAT lenX, FLOAT lenY);

	HRESULT RegisterBuffer(void* pBuffer, UINT width, UINT height);
	void GetBuffer(void** pBuffer, UINT* width, UINT* height);

	HRESULT Update();
	HRESULT Debayering(
		UINT bayerType,
		void* inputBuffer,
		void** outputBuffer,
		UINT width,
		UINT height);

private:
	void* m_pBitmapBuffer;
	ID2D1BitmapBrush* m_pBitmapBrush;
	ID2D1HwndRenderTarget* m_pRenderTarget;

	FLOAT m_startX, m_startY;
	FLOAT m_lenX, m_lenY;
	FLOAT m_dpiXScale, m_dpiYScale;

	void* m_pInputMat, *m_pOutputMat;
};

