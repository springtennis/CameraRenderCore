#include "pch.h"
#include "BitmapRenderer.h"

BitmapRenderer::BitmapRenderer() :
    m_startX(0), m_startY(0),
    m_lenX(0), m_lenY(0),
    m_pBitmap(NULL),
    m_pRenderTarget(NULL),
    m_pBitmapBrush(NULL),
    m_pBitmapBuffer(NULL)
{}

BitmapRenderer::~BitmapRenderer()
{
    SafeRelease(&m_pBitmap);
    SafeRelease(&m_pBitmapBrush);
}

HRESULT BitmapRenderer::InitInstance(
    ID2D1HwndRenderTarget* pRenderTarge,
    FLOAT startX, FLOAT startY,
    FLOAT lenX, FLOAT lenY)
{
    m_pRenderTarget = pRenderTarge;

    m_startX = startX;
    m_startY = startY;

    m_lenX = lenX;
    m_lenY = lenY;

    return S_OK;
}

HRESULT BitmapRenderer::RegisterBuffer(void* pBuffer, UINT width, UINT height)
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget)
        return E_FAIL;

    // Register only once
    if (m_pBitmap)
        return S_OK;

    SafeRelease(&m_pBitmap);

    FLOAT dpiX, dpiY;
    m_pRenderTarget->GetDpi(&dpiX, &dpiY);

    D2D1_BITMAP_PROPERTIES props = {
        D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE),
        dpiX,
        dpiY
    };

    m_pBitmapBuffer = pBuffer;

    hr = m_pRenderTarget->CreateBitmap(
        D2D1::SizeU(width, height),
        m_pBitmapBuffer,
        width * 4,
        &props,
        &m_pBitmap);
    if (FAILED(hr))
        return hr;

    hr = m_pRenderTarget->CreateBitmapBrush(
        m_pBitmap,
        &m_pBitmapBrush
    );

    return hr;
}

void BitmapRenderer::GetBuffer(void** pBuffer, UINT* width, UINT* height)
{
    *pBuffer = m_pBitmapBuffer;

    if (m_pBitmap)
    {
        *width = m_pBitmap->GetPixelSize().width;
        *height = m_pBitmap->GetPixelSize().height;
    }
    else
    {
        *width = 0;
        *height = 0;
    }
}

HRESULT BitmapRenderer::Update()
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget || !m_pBitmap || !m_pBitmapBuffer)
        return E_FAIL;

    // Copy from memory
    D2D1_RECT_U rectBitmap = D2D1::RectU(
        0, 0, m_pBitmap->GetPixelSize().width, m_pBitmap->GetPixelSize().height
    );

    hr = m_pBitmap->CopyFromMemory(
        &rectBitmap,
        m_pBitmapBuffer,
        m_pBitmap->GetPixelSize().width * 4
    );

    if (FAILED(hr))
        return hr;

    // Draw bitmap
    D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();
    float width = rtSize.width;
    float height = rtSize.height;

    float x = width * m_startX;
    float y = height * m_startY;

    float a = width * m_lenX;
    float b = height * m_lenY;

    float w = (float)rectBitmap.right;
    float h = (float)rectBitmap.bottom;

    float R = a / b;
    float r = w / h;

    float scale = 1.0f;
    float translateX = x + (a - w) / 2;
    float translateY = y + (b - h) / 2;

    float tex_x1, tex_x2;
    float tex_y1, tex_y2;

    if (R > r)
    {
        scale = a / w;
        tex_x1 = 0.0f;
        tex_y1 = 0.5f * (1.0f - r / R);
        tex_x2 = 1.0f;
        tex_y2 = 0.5f * (1.0f + r / R);
    }
    else
    {
        tex_x1 = 0.5f * (1.0f - R / r);
        tex_y1 = 0.0f;
        tex_x2 = 0.5f * (1.0f + R / r);
        tex_y2 = 1.0f;
        scale = b / h;
    }

    tex_x1 *= w;
    tex_x2 *= w;
    tex_y1 *= h;
    tex_y2 *= h;

    D2D1::Matrix3x2F trans_scale =
        D2D1::Matrix3x2F::Scale(
            D2D1::Size(scale, scale),
            D2D1::Point2F(w / 2, h / 2));

    D2D1::Matrix3x2F trans_move =
        D2D1::Matrix3x2F::Translation(
            D2D1::SizeF(translateX, translateY));

    D2D1_RECT_F rectDisplay = D2D1::RectF(tex_x1, tex_y1, tex_x2, tex_y2);

    m_pRenderTarget->SetTransform(trans_scale * trans_move);
    m_pRenderTarget->FillRectangle(&rectDisplay, m_pBitmapBrush);
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    return hr;
}