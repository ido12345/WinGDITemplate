#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

 int RectWidth(RECT rect) {
    return rect.right - rect.left;
}

 int RectHeight(RECT rect) {
    return rect.bottom - rect.top;
}

int WindowWidth = 500;
int WindowHeight = 500;
HDC hDoubleBufferDC = nullptr;
HBITMAP hDoubleBufferBitmap = nullptr;

typedef uint32_t Color;

// WinGDI uses BGRA
#define RGBA(r, g, b, a) ((Color)(((Color)(a) << 24) | ((Color)(r) << 16) | ((Color)(g) << 8) | (Color)(b)))
#define R(c) (((c) >> 16) & 0xff)
#define G(c) (((c) >> 8) & 0xff)
#define B(c) ((c) & 0xff)
#define A(c) (((c) >> 24) & 0xff)

Color *Pixels = nullptr;

Color *PixelsAt(int x, int y) {
    assert((0 <= x && x < WindowWidth && 0 <= y && y < WindowHeight));
    return &Pixels[WindowWidth * y + x];
}

 void BlendColor(Color *c1, Color c2) {
    uint8_t R1 = R(*c1), G1 = G(*c1), B1 = B(*c1), A1 = A(*c1);

    uint8_t R2 = R(c2), G2 = G(c2), B2 = B(c2), A2 = A(c2);

    *c1 = RGBA((R1 * (0xff - A2) + R2 * A2) / 0xff,
               (G1 * (0xff - A2) + G2 * A2) / 0xff,
               (B1 * (0xff - A2) + B2 * A2) / 0xff,
               (A1 * (0xff - A2) + A2) / 0xff);
}

typedef float Unit;

typedef struct {
    Unit x, y;
} Point;

typedef struct {
    Unit left, top, right, bottom;
} Rect;

typedef struct {
    Unit x1, y1;
    Unit x2, y2;
} Line;

typedef struct {
    Unit x, y;
    Unit r;
} Circle;

bool AtBoundary(Rect boundary, int x, int y) {
    return (boundary.left <= x && x < boundary.right) && (boundary.top <= y && y < boundary.bottom);
}

void DrawPoint(Rect boundary, Point point, Color color) {
    int x = point.x + boundary.left;
    int y = point.y + boundary.top;
    if (AtBoundary(boundary, x, y)) {
        BlendColor(PixelsAt(x, y), color);
    }
}

void DrawRect(Rect boundary, Rect rect, Color color) {
    for (Unit y = rect.top; y < rect.bottom; y++) {
        for (Unit x = rect.left; x < rect.right; x++) {
            Unit nx = x + boundary.left;
            Unit ny = y + boundary.top;
            if (AtBoundary(boundary, nx, ny)) {
                BlendColor(PixelsAt(nx, ny), color);
            }
        }
    }
}

void DrawBackground(Rect boundary, Color color) {
    for (Unit y = boundary.top; y < boundary.bottom; y++) {
        for (Unit x = boundary.left; x < boundary.right; x++) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
    }
}

void DrawBoundary(Rect boundary, int width, Color color) {
    int left = boundary.left;
    int top = boundary.top;
    int right = boundary.right;
    int bottom = boundary.bottom;

    for (int y = top; y < top + width && y < bottom; y++) {
        for (int x = left; x < right; x++) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
    }

    for (int y = bottom - width; y < bottom; y++) {
        if (y < top) continue;
        for (int x = left; x < right; x++) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
    }

    for (int y = top + width; y < bottom - width; y++) {
        for (int x = left; x < left + width && x < right; x++) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
        for (int x = right - width; x < right; x++) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
    }
}

#define swap(T, a, b) \
    do {              \
        T t = a;      \
        a = b;        \
        b = t;        \
    } while (0)

void DrawLine(Rect boundary, Line line, int width, Color color) {
    int dx = line.x2 - line.x1;
    int dy = line.y2 - line.y1;
    if (!dx && !dy) return;

    int half = width / 2;

    if (abs(dx) > abs(dy)) {
        if (line.x1 > line.x2) {
            swap(int, line.x1, line.x2);
            swap(int, line.y1, line.y2);
        }

        for (int x = line.x1; x <= line.x2; ++x) {
            int y = (x - line.x1) * dy / dx + line.y1;

            for (int oy = -half; oy < width - half; oy++) {
                for (int ox = -half; ox < width - half; ox++) {
                    int nx = x + ox + boundary.left;
                    int ny = y + oy + boundary.top;
                    if (AtBoundary(boundary, nx, ny)) {
                        BlendColor(PixelsAt(nx, ny), color);
                    }
                }
            }
        }
    } else {
        if (line.y1 > line.y2) {
            swap(int, line.x1, line.x2);
            swap(int, line.y1, line.y2);
        }

        for (int y = line.y1; y <= line.y2; ++y) {
            int x = (y - line.y1) * dx / dy + line.x1;

            for (int oy = -half; oy < width - half; oy++) {
                for (int ox = -half; ox < width - half; ox++) {
                    int nx = x + ox + boundary.left;
                    int ny = y + oy + boundary.top;
                    if (AtBoundary(boundary, nx, ny)) {
                        BlendColor(PixelsAt(nx, ny), color);
                    }
                }
            }
        }
    }
}

void DrawCircle(Rect boundary, Circle c, Color color) {
    for (int dy = -c.r; dy <= c.r; dy++) {
        for (int dx = -c.r; dx <= c.r; dx++) {
            if (dx * dx + dy * dy <= c.r * c.r) {
                int x = c.x + dx + boundary.left;
                int y = c.y + dy + boundary.top;
                if (AtBoundary(boundary, x, y)) {
                    BlendColor(PixelsAt(x, y), color);
                }
            }
        }
    }
}

void DestroyDoubleBuffer() {
    if (hDoubleBufferDC == nullptr) return;

    SelectObject(hDoubleBufferDC, nullptr);
    DeleteObject(hDoubleBufferBitmap);
    DeleteDC(hDoubleBufferDC);
    hDoubleBufferDC = nullptr;
    hDoubleBufferBitmap = nullptr;
    Pixels = nullptr;
}

void NewDoubleBuffer(HDC hdc, int w, int h) {
    if (hDoubleBufferDC != nullptr) {
        DestroyDoubleBuffer();
    }
    hDoubleBufferDC = CreateCompatibleDC(hdc);
    BITMAPINFO bmi = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = w,
            .biHeight = -h,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        },
    };
    hDoubleBufferBitmap = CreateDIBSection(hDoubleBufferDC, &bmi, DIB_RGB_COLORS, (void **)&Pixels, nullptr, 0);
    SelectObject(hDoubleBufferDC, hDoubleBufferBitmap);
}

void ResizeWindow(HWND hwnd) {
    RECT ClientRect;
    GetClientRect(hwnd, &ClientRect);
    int NewWindowHeight = RectHeight(ClientRect);
    int NewWindowWidth = RectWidth(ClientRect);
    if (NewWindowHeight == 0 || NewWindowWidth == 0) return;
    WindowWidth = NewWindowWidth;
    WindowHeight = NewWindowHeight;
    HDC hdc = GetDC(hwnd);
    NewDoubleBuffer(hdc, WindowWidth, WindowHeight);
    ReleaseDC(hwnd, hdc);
}

Rect WindowRect = {};
Rect LeftRect = {};
Rect RightRect = {};

int ballX = 0, ballY = 0;
int ballDx = 10, ballDy = 10;
int ballSize = 50;

constexpr Color backgroundColor = RGBA(0x18, 0x18, 0x18, 0xff);
constexpr Color boundaryColor = RGBA(0x80, 0x80, 0x80, 0xff);
constexpr int boundaryWidth = 2;

constexpr int RenderTimerId = 0;
constexpr double FPS = 144;
constexpr double DeltaTime = 1.0 / FPS;

void CalculateBoundaries() {
    WindowRect = (Rect){0, 0, WindowWidth, WindowHeight};
    int verticalSplit = WindowWidth / 2;
    LeftRect = (Rect){WindowRect.left, 0, verticalSplit, WindowHeight};
    RightRect = (Rect){verticalSplit, 0, WindowRect.right, WindowHeight};
}

DWORD UpdateLoop(void *param [[maybe_unused]]) {
    for (;; Sleep(DeltaTime * 1000.0)) {
        int nx = ballX + ballDx;
        if (nx < LeftRect.left) {
            ballDx = -ballDx;
            nx = 0;
        } else if (nx + ballSize >= LeftRect.right) {
            ballDx = -ballDx;
            nx = LeftRect.right - ballSize - 1;
        }
        ballX = nx;
        int ny = ballY + ballDy;
        if (ny < LeftRect.top) {
            ballDy = -ballDy;
            ny = 0;
        } else if (ny + ballSize >= LeftRect.bottom) {
            ballDy = -ballDy;
            ny = LeftRect.bottom - ballSize - 1;
        }
        ballY = ny;
    }
    return 0;
}

void Render() {
    if (!Pixels) return;

    DrawBackground(WindowRect, backgroundColor);

    // DrawRect(LeftRect, (Rect){x, y, x + size, y + size}, RGBA(0xff, 0, 0, 0xff));
    // DrawRect(RightRect, (Rect){x, y, x + size, y + size}, RGBA(0, 0, 0xff, 0xff));
    int r = ballSize / 2;
    int x = ballX + r;
    int y = ballY + r;
    DrawCircle(LeftRect, (Circle){x, y, r}, RGBA(0xff, 0, 0, 0xff));
    DrawCircle(RightRect, (Circle){x, y, r}, RGBA(0, 0, 0xff, 0xff));

    DrawBoundary(LeftRect, boundaryWidth, boundaryColor);
    DrawBoundary(RightRect, boundaryWidth, boundaryColor);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN: {
            switch (wParam) {
                case 'Q': {
                    DestroyWindow(hwnd);
                } break;
            }
        } break;
        case WM_PAINT: {
            Render();

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            BitBlt(
                hdc,
                0, 0,
                WindowWidth, WindowHeight,
                hDoubleBufferDC,
                0, 0,
                SRCCOPY
            );
            EndPaint(hwnd, &ps);
        } break;
        case WM_TIMER: {
            switch (wParam) {
                case RenderTimerId: {
                    InvalidateRect(hwnd, nullptr, false);
                } break;
            }
        } break;
        case WM_CREATE: {
            ResizeWindow(hwnd);
            SetTimer(hwnd, RenderTimerId, DeltaTime * 1000.0, nullptr);
        } break;
        case WM_SIZE: {
            ResizeWindow(hwnd);
            CalculateBoundaries();
            InvalidateRect(hwnd, nullptr, false);
        } break;
        case WM_ERASEBKGND: {
            return 1;
        } break;
        case WM_CLOSE: {
            DestroyWindow(hwnd);
        } break;
        case WM_DESTROY: {
            KillTimer(hwnd, RenderTimerId);
            DestroyDoubleBuffer();
            PostQuitMessage(0);
        } break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

constexpr char WindowClassName[] = "WindowClass";
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance [[maybe_unused]], LPSTR lpCmdLine [[maybe_unused]], int nCmdShow) {
    if (0) {
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }

    WNDCLASSEX wc = {
        .cbSize = sizeof(wc),
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .lpszClassName = WindowClassName,
        .lpfnWndProc = WndProc,
        .hInstance = hInstance,
        .style = 0,
        .lpszMenuName = nullptr,
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
        .hIcon = LoadIcon(nullptr, IDI_APPLICATION),   // big icon
        .hIconSm = LoadIcon(nullptr, IDI_APPLICATION), // small icon
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
    };
    if (!RegisterClassEx(&wc)) {
        MessageBox(nullptr, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }

    DWORD dwExStyle = WS_EX_CLIENTEDGE;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;

    RECT WindowRectResized = {0, 0, WindowWidth, WindowHeight};
    AdjustWindowRectEx(&WindowRectResized, dwStyle, false, dwExStyle);

    HWND hwnd = CreateWindowEx(
        dwExStyle,
        WindowClassName,
        "Title",
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        RectWidth(WindowRectResized), RectHeight(WindowRectResized),
        nullptr, nullptr, hInstance, nullptr
    );
    if (!hwnd) {
        MessageBox(nullptr, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    HANDLE UpdateThread = CreateThread(nullptr, 0, UpdateLoop, nullptr, 0, nullptr);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SuspendThread(UpdateThread);
    CloseHandle(UpdateThread);
    UpdateThread = nullptr;

    return msg.wParam;
}
