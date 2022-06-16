#pragma once

#include "BitmapRenderer.h"
#include <vector>

struct DisplayInfo {
	FLOAT startX, startY;
	FLOAT lenX, lenY;
	FLOAT dpiXScale, dpiYScale;
	UINT zIndex;
	UINT displayMode;
};

struct DisplayHandler {
	BitmapRenderer* pBitmapRenderer;
	DisplayInfo displayInfo;
};

struct TextDisplayHandler {
	IDWriteTextFormat* pTextFormat;
	ID2D1SolidColorBrush* pTextBrush;

	WCHAR text[128];
	UINT length;

	DisplayInfo displayInfo;
};

class StreamBitmapRenderer
{
private:
	HWND m_hwndHost;
	ID2D1Factory* m_pDirect2dFactory;
	IDWriteFactory* m_pDWriteFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;

	std::vector<DisplayHandler> m_DisplayHandler;
	std::vector<TextDisplayHandler> m_TextDisplayHandler;

	ID2D1SolidColorBrush* m_pTextBackgroundBrush;
	BOOL m_layoutChanged;

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
	HRESULT ModifyBitmapRenderer(DisplayHandler* displayHandler, DisplayInfo newDisplayInfo);

	TextDisplayHandler* RegisterTextRenderer(
		DisplayInfo displayInfo,
		float fontSize,
		float R, float G, float B);

	HRESULT RegisterBitmapBuffer(
		DisplayHandler* displayHandler,
		void* pBuffer,
		UINT width,
		UINT height,
		UINT bitmapType);

	HRESULT RegisterText(
		TextDisplayHandler* textDisplayHandler,
		WCHAR* text,
		UINT length);

	void Resize(UINT width, UINT height);
	void DrawOnce();
};

