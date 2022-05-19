#include "framework.h"
#include "RandomBitmapDisplay.h"

#include "StreamBitmapRenderer.h"

#include <random>
#include <chrono>

#define MAX_LOADSTRING 100

HINSTANCE hInst;
HWND g_hwnd;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

StreamBitmapRenderer g_streamBitmapRenderer;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void randomBitmap(CHAR* pBuffer, UINT width, UINT height)
{
    UINT i, j;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int>dis(0, 255);

    for (i = 0; i < height; i++)
    {
        UINT startRowIdx = i * width * 4;
        for (j = 0; j < width; j++)
        {
            UINT startIdx = startRowIdx + 4 * j;
            pBuffer[startIdx + 0] = dis(gen); // B
            pBuffer[startIdx + 1] = i == j ? 255 : 0; // G
            pBuffer[startIdx + 2] = i == j ? 255 : 0; // R
            pBuffer[startIdx + 3] = 255; // A
        }
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RANDOMBITMAPDISPLAY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RANDOMBITMAPDISPLAY));

    HRESULT hr;

    // Get monitor dpi scale
    UINT dpiX, dpiY;
    FLOAT dpiXScale, dpiYScale;
    HMONITOR hMonitor = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTOPRIMARY);
    GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

    dpiXScale = (float)dpiX / 96.0f;
    dpiYScale = (float)dpiX / 96.0f;

    // Create Bitmap streamer
    hr = g_streamBitmapRenderer.InitInstance(g_hwnd);
    DisplayHandler display1 = g_streamBitmapRenderer.RegisterBitmapRenderer({ 0.0f, 0.0f, 1.0f, 0.5f, dpiXScale, dpiYScale, 0 });
    DisplayHandler display2 = g_streamBitmapRenderer.RegisterBitmapRenderer({ 0.2f, 0.2f, 0.5f, 0.5f, dpiXScale, dpiYScale, 1 });

    // Pseudo bitmap image in memory
    UINT b1_width = 20;
    UINT b1_height = 20;
    CHAR* b1_buffer = new CHAR[b1_width * b1_height * 4]; // Buffer must be RGBA (multiply by 4)
    ZeroMemory(b1_buffer, b1_width * b1_height * 4);

    UINT b2_width = 10;
    UINT b2_height = 10;
    CHAR* b2_buffer = new CHAR[b2_width * b2_height * 4]; // Buffer must be RGBA (multiply by 4)
    ZeroMemory(b2_buffer, b2_width * b2_height * 4);

    // Register buffer
    g_streamBitmapRenderer.RegisterBitmapBuffer(&display1, b1_buffer, b1_width, b1_height);
    g_streamBitmapRenderer.RegisterBitmapBuffer(&display2, b2_buffer, b2_width, b2_height);

    // Loop
    MSG msg = { 0 };
    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Drawing loop (~10Hz)
        std::chrono::system_clock::time_point current = std::chrono::system_clock::now();
        std::chrono::duration<double>sec = current - start;
        if (sec.count() > 0.1)
        {
            start = current;
            randomBitmap(b1_buffer, b1_width, b1_height);
            randomBitmap(b2_buffer, b2_width, b2_height);
            g_streamBitmapRenderer.DrawOnce();
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RANDOMBITMAPDISPLAY));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RANDOMBITMAPDISPLAY);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   g_hwnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!g_hwnd)
   {
      return FALSE;
   }

   ShowWindow(g_hwnd, nCmdShow);
   UpdateWindow(g_hwnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        g_streamBitmapRenderer.Resize(width, height);
    }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
