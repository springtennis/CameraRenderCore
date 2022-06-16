#include "pch.h"
#include "framework.h"
#include "StreamBitmapRenderer.h"

#include <algorithm> 

StreamBitmapRenderer::StreamBitmapRenderer() :
	m_hwndHost(NULL),
	m_pDirect2dFactory(NULL),
	m_pRenderTarget(NULL),
	m_layoutChanged(FALSE)
{}

StreamBitmapRenderer::~StreamBitmapRenderer()
{
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pDirect2dFactory);

	for (int i = 0; i < m_DisplayHandler.size(); i++) {
		delete m_DisplayHandler[i].pBitmapRenderer;
	}
	m_DisplayHandler.clear();
	m_DisplayHandler.shrink_to_fit();

	m_TextDisplayHandler.clear();
	m_TextDisplayHandler.shrink_to_fit();
}

HRESULT StreamBitmapRenderer::InitInstance(HWND hwndHost)
{
	HRESULT hr = S_OK;

	if (!hwndHost)
		return E_FAIL;

	m_hwndHost = hwndHost;

	// Create Direct2D Factory & Render target
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
	if (FAILED(hr))
		return hr;

	RECT rc;
	GetClientRect(m_hwndHost, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(
		rc.right - rc.left,
		rc.bottom - rc.top
	);

	hr = m_pDirect2dFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(m_hwndHost, size),
		&m_pRenderTarget
	);
	if (FAILED(hr))
		return hr;

	// Create a DirectWrite factory
	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(m_pDWriteFactory),
		reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
	);
	if (FAILED(hr))
		return hr;

	hr = m_pRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Black, 1.0f),
		&m_pTextBackgroundBrush
	);

	return hr;
}

DisplayHandler StreamBitmapRenderer::RegisterBitmapRenderer(DisplayInfo displayInfo)
{
	if (!m_pRenderTarget)
		return { NULL, { 0 } };

	BitmapRenderer* pBitmapRenderer = new BitmapRenderer();
	pBitmapRenderer->InitInstance(
		m_pRenderTarget,
		displayInfo.startX,
		displayInfo.startY,
		displayInfo.lenX,
		displayInfo.lenY,
		displayInfo.dpiXScale,
		displayInfo.dpiYScale,
		displayInfo.displayMode
	);

	DisplayHandler displayHandler = { pBitmapRenderer, displayInfo };

	std::vector<DisplayHandler>::iterator it;
	for (it = m_DisplayHandler.begin(); it != m_DisplayHandler.end(); it++)
	{
		if (it->displayInfo.zIndex > displayInfo.zIndex)
			break;
	}

	it = m_DisplayHandler.insert(it, displayHandler);

	return displayHandler;
}

TextDisplayHandler* StreamBitmapRenderer::RegisterTextRenderer(
	DisplayInfo displayInfo,
	float fontSize,
	float R, float G, float B)
{
	if (!m_pRenderTarget)
		return NULL;

	HRESULT hr = S_OK;
	TextDisplayHandler handler;

	handler.displayInfo = displayInfo;
	handler.length = 0;

	// Create a DirectWrite text format object
	hr = m_pDWriteFactory->CreateTextFormat(
		L"Verdana",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		L"",
		&handler.pTextFormat
	);
	if (FAILED(hr))
		return NULL;

	handler.pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	handler.pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	hr = m_pRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(R, G, B, 1.0f),
		&handler.pTextBrush
	);

	m_TextDisplayHandler.push_back(handler);

	return &m_TextDisplayHandler.back();
}

bool cmpBitmapRenderer(DisplayHandler a, DisplayHandler b)
{
	return a.displayInfo.zIndex > b.displayInfo.zIndex;
}

HRESULT StreamBitmapRenderer::ModifyBitmapRenderer(DisplayHandler* displayHandler, DisplayInfo newDisplayInfo)
{
	if (!m_pRenderTarget || !displayHandler->pBitmapRenderer)
		return E_FAIL;

	// Check displayHandler is in current handler list
	std::vector<DisplayHandler>::iterator it;
	for (it = m_DisplayHandler.begin(); it != m_DisplayHandler.end(); it++)
	{
		if (it->pBitmapRenderer == displayHandler->pBitmapRenderer)
			break;
	}

	if (it == m_DisplayHandler.end())
		return E_FAIL;

	// Update displayInfo & rearrange list
	it->displayInfo = newDisplayInfo;
	it->pBitmapRenderer->ChangeDisplayArea(
		newDisplayInfo.startX,
		newDisplayInfo.startY,
		newDisplayInfo.lenX,
		newDisplayInfo.lenY,
		newDisplayInfo.displayMode
	);

	*displayHandler = *it;

	std::sort(m_DisplayHandler.rbegin(), m_DisplayHandler.rend(), cmpBitmapRenderer);

	m_layoutChanged = TRUE;

	return S_OK;
}

HRESULT StreamBitmapRenderer::RegisterBitmapBuffer(
	DisplayHandler* displayHandler,
	void* pBuffer,
	UINT width,
	UINT height,
	UINT bitmapType)
{
	if (!m_pRenderTarget || !displayHandler->pBitmapRenderer)
		return E_FAIL;

	switch (bitmapType) {
	case BITMAP_RGBA :
		if (!displayHandler->pBitmapRenderer->m_pBitmap)
			return displayHandler->pBitmapRenderer->RegisterBuffer(pBuffer, width, height);
		break;

	case BITMAP_BAYER_RG: case BITMAP_BAYER_GR: case BITMAP_BAYER_GB: case BITMAP_BAYER_BG:
	{
		void* outputBuffer = NULL;
		UINT bayerType = 0;

		switch (bitmapType) {
		case BITMAP_BAYER_RG: bayerType = BitmapRenderer::Frame_BayerRG; break;
		case BITMAP_BAYER_GR: bayerType = BitmapRenderer::Frame_BayerGR; break;
		case BITMAP_BAYER_GB: bayerType = BitmapRenderer::Frame_BayerGB; break;
		case BITMAP_BAYER_BG: bayerType = BitmapRenderer::Frame_BayerBG; break;
		}

		HRESULT hr = displayHandler->pBitmapRenderer->Debayering(bayerType, pBuffer, &outputBuffer, width, height);
		if (FAILED(hr))
			return hr;

		return displayHandler->pBitmapRenderer->RegisterBuffer(outputBuffer, width, height);
	}
		break;
	}

	return E_FAIL;
}

HRESULT StreamBitmapRenderer::RegisterText(
	TextDisplayHandler* textDisplayHandler,
	WCHAR* text,
	UINT length)
{
	if (!m_pRenderTarget || !textDisplayHandler->pTextFormat || !textDisplayHandler->pTextBrush)
		return E_FAIL;

	if (length > 128) length = 128;
	textDisplayHandler->length = length;
	wmemcpy(textDisplayHandler->text, text, length);

	return S_OK;
}

void StreamBitmapRenderer::Resize(UINT width, UINT height)
{
	if (m_pRenderTarget)
		m_pRenderTarget->Resize(D2D1::SizeU(width, height));
}

void StreamBitmapRenderer::DrawOnce()
{
	if (!m_pRenderTarget)
		return;

	m_pRenderTarget->BeginDraw();
	
	if (m_layoutChanged)
	{
		m_pRenderTarget->Clear();
		m_layoutChanged = FALSE;
	}

	// Draw Bitmap
	for (UINT i = 0; i < m_DisplayHandler.size(); i++)
		m_DisplayHandler[i].pBitmapRenderer->Update();

	// Draw Text
	D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();
	float width = rtSize.width;
	float height = rtSize.height;

	for (UINT i = 0; i < m_TextDisplayHandler.size(); i++)
	{
		if (m_TextDisplayHandler[i].displayInfo.displayMode == BitmapRenderer::Frame_DisplayModeNone)
			continue;

		float startX = width * m_TextDisplayHandler[i].displayInfo.startX;
		float startY = height * m_TextDisplayHandler[i].displayInfo.startY;
		float lenX = width * m_TextDisplayHandler[i].displayInfo.lenX;
		float lenY = height * m_TextDisplayHandler[i].displayInfo.lenY;

		D2D1_RECT_F rectText = D2D1::RectF(startX, startY, lenX, lenY);

		// Draw background Box
		m_pRenderTarget->FillRectangle(&rectText, m_pTextBackgroundBrush);

		m_pRenderTarget->DrawTextW(
			m_TextDisplayHandler[i].text,
			m_TextDisplayHandler[i].length,
			m_TextDisplayHandler[i].pTextFormat,
			rectText,
			m_TextDisplayHandler[i].pTextBrush
		);
	}

	m_pRenderTarget->EndDraw();
}