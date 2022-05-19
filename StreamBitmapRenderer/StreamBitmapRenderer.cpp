#include "pch.h"
#include "framework.h"
#include "StreamBitmapRenderer.h"

StreamBitmapRenderer::StreamBitmapRenderer() :
	m_hwndHost(NULL),
	m_pDirect2dFactory(NULL),
	m_pRenderTarget(NULL)
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
		displayInfo.dpiYScale
	);

	DisplayHandler displayHandler = { pBitmapRenderer, displayInfo };

	std::vector<DisplayHandler>::iterator it;
	for (it = m_DisplayHandler.begin(); it != m_DisplayHandler.end(); it++)
	{
		if (it->displayInfo.zIndex > displayInfo.zIndex)
			break;
	}

	it = m_DisplayHandler.insert(it, displayHandler);

	return *it;
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
	for (UINT i = 0; i < m_DisplayHandler.size(); i++)
		m_DisplayHandler[i].pBitmapRenderer->Update();
	m_pRenderTarget->EndDraw();
}