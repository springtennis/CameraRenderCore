#include "pch.h"
#include "framework.h"
#include "StreamBitmapRenderer.h"

StreamBitmapRenderer::StreamBitmapRenderer() :
	m_hwndHost(NULL),
	m_pDirect2dFactory(NULL),
	m_pRenderTarget(NULL),
	m_nDisplay(0),
	m_pBitmapRenderer(NULL)
{}

StreamBitmapRenderer::~StreamBitmapRenderer()
{
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pDirect2dFactory);
}

HRESULT StreamBitmapRenderer::InitInstance(
	HWND hwndHost,
	UINT nDisplay,
	DisplayInfo* displayInfo)
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

	// Register BitmapRenderer
	m_nDisplay = nDisplay;
	m_pBitmapRenderer = new BitmapRenderer[nDisplay];
	for (UINT i = 0; i < nDisplay; i++)
		m_pBitmapRenderer[i].InitInstance(
			m_pRenderTarget,
			displayInfo[i].startX,
			displayInfo[i].startY,
			displayInfo[i].lenX,
			displayInfo[i].lenY,
			displayInfo[i].dpiXScale,
			displayInfo[i].dpiYScale);

	return hr;
}

HRESULT StreamBitmapRenderer::RegisterBitmapBuffer(
	UINT n,
	void* pBuffer,
	UINT width,
	UINT height)
{
	if (!m_pBitmapRenderer || n > m_nDisplay)
		return E_FAIL;

	if (!m_pBitmapRenderer[n].m_pBitmap)
		return m_pBitmapRenderer[n].RegisterBuffer(pBuffer, width, height);

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
	for (UINT i = 0; i < m_nDisplay; i++)
		m_pBitmapRenderer[i].Update();
	m_pRenderTarget->EndDraw();
}