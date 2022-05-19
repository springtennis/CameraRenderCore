#include "pch.h"
#include "BitmapRenderer.h"

#include "opencv2/opencv.hpp"

BitmapRenderer::BitmapRenderer() :
    m_startX(0), m_startY(0),
    m_lenX(0), m_lenY(0),
    m_displayMode(1),
    m_dpiXScale(1.0f), m_dpiYScale(1.0f),
    m_pBitmap(NULL),
    m_pRenderTarget(NULL),
    m_pBitmapBrush(NULL),
    m_pBitmapBuffer(NULL),
    m_pInputMat(NULL), m_pOutputMat(NULL)
{}

BitmapRenderer::~BitmapRenderer()
{
    SafeRelease(&m_pBitmap);
    SafeRelease(&m_pBitmapBrush);
    
    delete m_pInputMat;
    delete m_pOutputMat;
}

HRESULT BitmapRenderer::InitInstance(
    ID2D1HwndRenderTarget* pRenderTarge,
    FLOAT startX, FLOAT startY,
    FLOAT lenX, FLOAT lenY,
    FLOAT dpiXScale, FLOAT dpiYScale,
    UINT displayMode)
{
    m_pRenderTarget = pRenderTarge;

    m_startX = startX;
    m_startY = startY;

    m_lenX = lenX;
    m_lenY = lenY;

    m_dpiXScale = dpiXScale;
    m_dpiYScale = dpiYScale;

    m_displayMode = displayMode;

    m_pInputMat = new cv::Mat();
    m_pOutputMat = new cv::Mat();

    return S_OK;
}

HRESULT BitmapRenderer::ChangeDisplayArea(
    FLOAT startX, FLOAT startY,
    FLOAT lenX, FLOAT lenY,
    UINT displayMode)
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget)
        return E_FAIL;

    m_startX = startX;
    m_startY = startY;

    m_lenX = lenX;
    m_lenY = lenY;

    m_displayMode = displayMode;

    return hr;
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

    D2D1_BITMAP_PROPERTIES props = {
        D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE),
        96.0f / m_dpiXScale,
        96.0f / m_dpiYScale
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

HRESULT BitmapRenderer::Debayering(
    UINT bayerType,
    void* inputBuffer,
    void** outputBuffer,
    UINT width,
    UINT height)
{
    HRESULT hr = S_OK;
    
    if (!m_pInputMat || !m_pOutputMat)
        return E_FAIL;

    cv::Mat* inputMat = (cv::Mat*)m_pInputMat;
    cv::Mat* outputMat = (cv::Mat*)m_pOutputMat;

    // Create a OpenCV buffer for the input image.
    if ((inputMat->cols != width) || (inputMat->rows != height) || (inputMat->type() != CV_8UC1))
        inputMat->create(height, width, CV_8UC1);

    // Copy the input image data to the buffer for OpenCV.
    const size_t dwBufferSize = inputMat->rows * inputMat->cols * inputMat->elemSize() * inputMat->channels();
    memcpy(inputMat->ptr(0), inputBuffer, dwBufferSize);

    int nConvert = 0;

    switch (bayerType) {
        case Frame_BayerRG : nConvert = cv::COLOR_BayerRG2RGBA;	break;
        case Frame_BayerGR : nConvert = cv::COLOR_BayerGR2RGBA;	break;
        case Frame_BayerGB : nConvert = cv::COLOR_BayerGB2RGBA;	break;
        case Frame_BayerBG : nConvert = cv::COLOR_BayerBG2RGBA;	break;
        default:
            return E_FAIL;
    }

    cvtColor(*inputMat, *outputMat, nConvert);

    *outputBuffer = outputMat->ptr(0);

    return hr;
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

    float w = (float)rectBitmap.right * m_dpiXScale;
    float h = (float)rectBitmap.bottom * m_dpiYScale;

    float R = a / b;
    float r = w / h;

    float scale = 1.0f;
    float translateX = x + (a - w) / 2;
    float translateY = y + (b - h) / 2;

    D2D1_RECT_F rectDisplay = D2D1::RectF(0, 0, w, h);

    if (m_displayMode == Frame_DisplayModeCrop)
    {
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
            scale = b / h;
            tex_x1 = 0.5f * (1.0f - R / r);
            tex_y1 = 0.0f;
            tex_x2 = 0.5f * (1.0f + R / r);
            tex_y2 = 1.0f;
        }

        tex_x1 *= w;
        tex_x2 *= w;
        tex_y1 *= h;
        tex_y2 *= h;

        rectDisplay.left = tex_x1;
        rectDisplay.right = tex_x2;
        rectDisplay.top = tex_y1;
        rectDisplay.bottom = tex_y2;
    }
    else if (m_displayMode == Frame_DisplayModeFit)
    {
        if (R > r)
            scale = b / h;
        else
            scale = a / w;
    }

    D2D1::Matrix3x2F trans_scale =
        D2D1::Matrix3x2F::Scale(
            D2D1::Size(scale, scale),
            D2D1::Point2F(w / 2, h / 2));

    D2D1::Matrix3x2F trans_move =
        D2D1::Matrix3x2F::Translation(
            D2D1::SizeF(translateX, translateY));

    m_pRenderTarget->SetTransform(trans_scale * trans_move);
    m_pRenderTarget->FillRectangle(&rectDisplay, m_pBitmapBrush);
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    return hr;
}