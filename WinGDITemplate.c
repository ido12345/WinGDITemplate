#include <stdbool.h>
#include <windows.h>

static int RectWidth(RECT rect) {
    return rect.right - rect.left;
}

static int RectHeight(RECT rect) {
    return rect.bottom - rect.top;
}

static int WindowWidth = 800;
static int WindowHeight = 600;
static HDC hDoubleBufferDC = NULL;
static HBITMAP hDoubleBufferBitmap = NULL;

static HBRUSH hBackgroundBrush = NULL;

static void ResizeWindow(int w, int h) {
    WindowWidth = w;
    WindowHeight = h;
}

static void DestroyDoubleBuffer() {
    SelectObject(hDoubleBufferDC, NULL);
    DeleteDC(hDoubleBufferDC);
    DeleteObject(hDoubleBufferBitmap);
}

static void NewDoubleBuffer(HDC hdc, int w, int h) {
    if (hDoubleBufferDC != NULL) {
        DestroyDoubleBuffer();
    }
    hDoubleBufferDC = CreateCompatibleDC(hdc);
    hDoubleBufferBitmap = CreateCompatibleBitmap(hdc, w, h);
    SelectObject(hDoubleBufferDC, hDoubleBufferBitmap);
}

static void DestroyBrush(HBRUSH *brush) {
    if (!brush) return;
    DeleteObject(*brush);
    *brush = NULL;
}

static void NewBrush(HBRUSH *brush, COLORREF color) {
    if (!brush) return;
    if (*brush != NULL) {
        DestroyBrush(brush);
    }
    *brush = CreateSolidBrush(color);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT ClientRect;
            GetClientRect(hwnd, &ClientRect);

            // Rendering
            {
                // Clear Previous Frame
                FillRect(hDoubleBufferDC, &ClientRect, hBackgroundBrush);

                // Copy Double Buffer To Window Frame
                BitBlt(hdc,
                       0, 0, WindowWidth, WindowHeight,
                       hDoubleBufferDC, 0, 0,
                       SRCCOPY);
            }
        } break;
        case WM_CREATE: {
            RECT ClientRect;
            GetClientRect(hwnd, &ClientRect);
            ResizeWindow(RectWidth(ClientRect), RectHeight(ClientRect));
            NewDoubleBuffer(GetDC(hwnd), WindowWidth, WindowHeight);

            NewBrush(&hBackgroundBrush, RGB(255, 255, 255));
        } break;
        case WM_SIZE: {
            RECT ClientRect;
            GetClientRect(hwnd, &ClientRect);
            ResizeWindow(RectWidth(ClientRect), RectHeight(ClientRect));
            NewDoubleBuffer(GetDC(hwnd), WindowWidth, WindowHeight);

            InvalidateRect(hwnd, NULL, false);
        } break;
        case WM_CLOSE: {
            DestroyWindow(hwnd);
        } break;
        case WM_DESTROY: {
            DestroyDoubleBuffer();
            DestroyBrush(&hBackgroundBrush);

            PostQuitMessage(0);
        } break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

const char WindowClassName[] = "TemplateWindowClass";
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void) hPrevInstance;
    (void) lpCmdLine;

    WNDCLASSEX wc = {
        .cbSize = sizeof(wc),
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .lpszClassName = WindowClassName,
        .lpfnWndProc = WndProc,
        .hInstance = hInstance,
        .style = 0,
        .lpszMenuName = NULL,
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),   // big icon
        .hIconSm = LoadIcon(NULL, IDI_APPLICATION), // small icon
        .hCursor = LoadCursor(NULL, IDC_ARROW),
    };
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }

    DWORD dwExStyle = WS_EX_CLIENTEDGE;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;

    RECT WindowRect = {0, 0, WindowWidth, WindowHeight};
    AdjustWindowRectEx(&WindowRect, dwStyle, false, dwExStyle);

    HWND hwnd = CreateWindowEx(
        dwExStyle,
        WindowClassName,
        "Title",
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        RectWidth(WindowRect), RectHeight(WindowRect),
        NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}