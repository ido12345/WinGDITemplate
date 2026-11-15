const std = @import("std");
const win = @import("win.zig");

fn RectWidth(rect: win.RECT) i32 {
    return rect.right - rect.left;
}

fn RectHeight(rect: win.RECT) i32 {
    return rect.bottom - rect.top;
}

var WindowWidth: i32 = 500;
var WindowHeight: i32 = 500;
var hDoubleBufferDC: win.HDC = null;
var hDoubleBufferBitmap: win.HBITMAP = null;

const Color = u32;

// WinGDI uses BGRA
fn RGBA(r: u8, g: u8, b: u8, a: u8) Color {
    return (@as(Color, a) << 24) | (@as(Color, r) << 16) | (@as(Color, g) << 8) | (@as(Color, b));
}

fn R(c: Color) u8 {
    return @truncate(c >> 16);
}

fn G(c: Color) u8 {
    return @truncate(c >> 8);
}

fn B(c: Color) u8 {
    return @truncate(c);
}

fn A(c: Color) u8 {
    return @truncate(c >> 24);
}

var Pixels: ?[*]Color = null;

fn PixelsAt(x: i32, y: i32) *Color {
    std.debug.assert(0 <= x and x < WindowWidth and 0 <= y and y < WindowHeight);
    return &Pixels.?[@intCast(WindowWidth * y + x)];
}

fn BlendColor(c1: *Color, c2: Color) void {
    var R1: Color = R(c1.*);
    var G1: Color = G(c1.*);
    var B1: Color = B(c1.*);
    var A1: Color = A(c1.*);

    const R2: Color = R(c2);
    const G2: Color = G(c2);
    const B2: Color = B(c2);
    const A2: Color = A(c2);

    R1 = (R1 * (0xff - A2) + R2 * A2) / 0xff;
    G1 = (G1 * (0xff - A2) + G2 * A2) / 0xff;
    B1 = (B1 * (0xff - A2) + B2 * A2) / 0xff;
    A1 = (A1 * (0xff - A2) + A2 * 0xff) / 0xff;

    c1.* = RGBA(@intCast(R1), @intCast(G1), @intCast(B1), @intCast(A1));
}

const Point = struct {
    x: i32,
    y: i32,
};

const Rect = struct {
    left: i32,
    top: i32,
    right: i32,
    bottom: i32,
};

const Line = struct {
    x1: i32,
    y1: i32,
    x2: i32,
    y2: i32,
};

const Circle = struct {
    x: i32,
    y: i32,
    r: i32,
};

fn AtBoundary(boundary: Rect, x: i32, y: i32) bool {
    return boundary.left <= x and x < boundary.right and boundary.top <= y and y < boundary.bottom;
}

fn DrawPoint(boundary: Rect, point: Point, color: Color) void {
    const x: i32 = point.x + boundary.left;
    const y: i32 = point.y + boundary.top;
    if (AtBoundary(boundary, x, y)) {
        BlendColor(PixelsAt(x, y), color);
    }
}

fn DrawRect(boundary: Rect, rect: Rect, color: Color) void {
    var y = rect.top;
    while (y < rect.bottom) : (y += 1) {
        var x = rect.left;
        while (x < rect.right) : (x += 1) {
            const nx = x + boundary.left;
            const ny = y + boundary.top;
            if (AtBoundary(boundary, nx, ny)) {
                BlendColor(PixelsAt(nx, ny), color);
            }
        }
    }
}

fn DrawBackground(boundary: Rect, color: Color) void {
    var y = boundary.top;
    while (y < boundary.bottom) : (y += 1) {
        var x = boundary.left;
        while (x < boundary.right) : (x += 1) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
    }
}

fn DrawBoundary(boundary: Rect, width: i32, color: Color) void {
    const left = boundary.left;
    const top = boundary.top;
    const right = boundary.right;
    const bottom = boundary.bottom;

    var y = top;
    while (y < @min(top + width, bottom)) : (y += 1) {
        var x = left;
        while (x < right) : (x += 1) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
    }

    y = bottom - width;
    while (y < bottom) : (y += 1) {
        if (y < top) continue;
        var x = left;
        while (x < right) : (x += 1) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
    }

    y = top + width;
    while (y < bottom - width) : (y += 1) {
        var x = left;
        while (x < @min(left + width, right)) : (x += 1) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
        x = right - width;
        while (x < right) : (x += 1) {
            if (AtBoundary(boundary, x, y)) {
                BlendColor(PixelsAt(x, y), color);
            }
        }
    }
}

fn DrawLine(boundary: Rect, line: Line, width: i32, color: Color) void {
    const dx = line.x2 - line.x1;
    const dy = line.y2 - line.y1;
    if (dx == 0 and dy == 0) return;

    const half = @divTrunc(width, 2);

    if (@abs(dx) > @abs(dy)) {
        const x1, const x2, const y1 =
            if (line.x1 <= line.x2)
                .{ line.x1, line.x2, line.y1 }
            else
                .{ line.x2, line.x1, line.y2 };

        var x = x1;
        while (x < x2 + 1) : (x += 1) {
            const y = @divTrunc((x - x1) * dy, dx) + y1;

            var oy = -half;
            while (oy < width - half) : (oy += 1) {
                var ox = -half;
                while (ox < width - half) : (ox += 1) {
                    const nx = x + ox + boundary.left;
                    const ny = y + oy + boundary.top;
                    if (AtBoundary(boundary, nx, ny)) {
                        BlendColor(PixelsAt(nx, ny), color);
                    }
                }
            }
        }
    } else {
        const x1, const y1, const y2 =
            if (line.y1 <= line.y2)
                .{ line.x1, line.y1, line.y2 }
            else
                .{ line.x2, line.y2, line.y1 };

        var y = y1;
        while (y < y2 + 1) : (y += 1) {
            const x = @divTrunc((y - y1) * dx, dy) + x1;

            var oy = -half;
            while (oy < width - half) : (oy += 1) {
                var ox = -half;
                while (ox < width - half) : (ox += 1) {
                    const nx = x + ox + boundary.left;
                    const ny = y + oy + boundary.top;
                    if (AtBoundary(boundary, nx, ny)) {
                        BlendColor(PixelsAt(nx, ny), color);
                    }
                }
            }
        }
    }
}

fn DrawCircle(boundary: Rect, c: Circle, color: Color) void {
    var dy = -c.r;
    while (dy <= c.r) : (dy += 1) {
        var dx = -c.r;
        while (dx <= c.r) : (dx += 1) {
            if (dx * dx + dy * dy <= c.r * c.r) {
                const x = c.x + dx + boundary.left;
                const y = c.y + dy + boundary.top;
                if (AtBoundary(boundary, x, y)) {
                    BlendColor(PixelsAt(x, y), color);
                }
            }
        }
    }
}

fn NewDoubleBuffer(hdc: win.HDC, w: i32, h: i32) void {
    if (hDoubleBufferDC != null) {
        DestroyDoubleBuffer();
    }
    hDoubleBufferDC = win.CreateCompatibleDC(hdc);
    const bmi: win.BITMAPINFO = .{ .bmiHeader = .{
        .biSize = @sizeOf(win.BITMAPINFOHEADER),
        .biWidth = w,
        .biHeight = -h,
        .biPlanes = 1,
        .biBitCount = 32,
        .biCompression = win.BI_RGB,
    } };
    hDoubleBufferBitmap = win.CreateDIBSection(hDoubleBufferDC, &bmi, win.DIB_RGB_COLORS, @ptrCast(&Pixels), null, 0);
    _ = win.SelectObject(hDoubleBufferDC, hDoubleBufferBitmap);
}

fn DestroyDoubleBuffer() void {
    if (hDoubleBufferDC == null) return;

    _ = win.SelectObject(hDoubleBufferDC, null);
    _ = win.DeleteObject(hDoubleBufferBitmap);
    _ = win.DeleteDC(hDoubleBufferDC);
    hDoubleBufferDC = null;
    hDoubleBufferBitmap = null;
    Pixels = null;
}

fn ResizeWindow(hwnd: win.HWND) void {
    var ClientRect: win.RECT = undefined;
    _ = win.GetClientRect(hwnd, &ClientRect);
    const NewWindowHeight = RectHeight(ClientRect);
    const NewWindowWidth = RectWidth(ClientRect);
    if (NewWindowHeight == 0 or NewWindowWidth == 0) return;
    WindowWidth = NewWindowWidth;
    WindowHeight = NewWindowHeight;
    const hdc: win.HDC = win.GetDC(hwnd);
    NewDoubleBuffer(hdc, WindowWidth, WindowHeight);
    _ = win.ReleaseDC(hwnd, hdc);
}

var WindowBoundary: Rect = undefined;
var LeftBoundary: Rect = undefined;
var RightBoundary: Rect = undefined;

var ballX: i32 = 0;
var ballY: i32 = 0;
var ballDx: i32 = 10;
var ballDy: i32 = 10;
var ballSize: i32 = 50;

const backgroundColor: Color = RGBA(0x18, 0x18, 0x18, 0xff);
const boundaryColor: Color = RGBA(0x80, 0x80, 0x80, 0xff);
const boundaryWidth: i32 = 2;

const RenderTimerId: i32 = 0;
const FPS: f64 = 144;
const DeltaTime: f64 = 1.0 / FPS;

fn CalculateBoundaries() void {
    WindowBoundary = .{ .left = 0, .top = 0, .right = WindowWidth, .bottom = WindowHeight };
    const verticalSplit = @divTrunc(WindowWidth, 2);
    LeftBoundary = .{ .left = WindowBoundary.left, .top = 0, .right = verticalSplit, .bottom = WindowHeight };
    RightBoundary = .{ .left = verticalSplit, .top = 0, .right = WindowBoundary.right, .bottom = WindowHeight };
}

fn UpdateLoop(param: ?*anyopaque) callconv(.winapi) win.DWORD {
    _ = param;
    while (true) : (win.Sleep(@intFromFloat(DeltaTime * 1000.0))) {
        var nx = ballX + ballDx;
        if (nx < LeftBoundary.left) {
            ballDx = -ballDx;
            nx = 0;
        } else if (nx + ballSize >= LeftBoundary.right) {
            ballDx = -ballDx;
            nx = @as(i32, LeftBoundary.right) - ballSize - 1;
        }
        ballX = nx;
        var ny = ballY + ballDy;
        if (ny < LeftBoundary.top) {
            ballDy = -ballDy;
            ny = 0;
        } else if (ny + ballSize >= LeftBoundary.bottom) {
            ballDy = -ballDy;
            ny = @as(i32, LeftBoundary.bottom) - ballSize - 1;
        }
        ballY = ny;
    }
    return 0;
}

fn Render() void {
    if (Pixels == null) return;

    DrawBackground(WindowBoundary, backgroundColor);

    const r = @divTrunc(ballSize, 2);
    const x = ballX + r;
    const y = ballY + r;
    DrawCircle(LeftBoundary, .{ .r = r, .x = x, .y = y }, RGBA(0xff, 0, 0, 0xff));
    DrawCircle(RightBoundary, .{ .r = r, .x = x, .y = y }, RGBA(0, 0, 0xff, 0xff));

    DrawBoundary(LeftBoundary, boundaryWidth, boundaryColor);
    DrawBoundary(RightBoundary, boundaryWidth, boundaryColor);
}

fn WndProc(
    hwnd: win.HWND,
    msg: u32,
    wParam: win.WPARAM,
    lParam: win.LPARAM,
) callconv(.winapi) win.LRESULT {
    switch (msg) {
        win.WM_KEYDOWN => {
            switch (wParam) {
                'Q' => {
                    _ = win.DestroyWindow(hwnd);
                },
                else => {},
            }
        },
        win.WM_PAINT => {
            Render();

            var ps: win.PAINTSTRUCT = undefined;
            const hdc: win.HDC = win.BeginPaint(hwnd, &ps);
            _ = win.BitBlt(
                hdc,
                0,
                0,
                WindowWidth,
                WindowHeight,
                hDoubleBufferDC,
                0,
                0,
                win.SRCCOPY,
            );
            _ = win.EndPaint(hwnd, &ps);
        },
        win.WM_TIMER => {
            switch (wParam) {
                RenderTimerId => {
                    _ = win.InvalidateRect(hwnd, null, @intFromBool(false));
                },
                else => unreachable,
            }
        },
        win.WM_CREATE => {
            ResizeWindow(hwnd);
            CalculateBoundaries();
            _ = win.SetTimer(hwnd, RenderTimerId, @intFromFloat(DeltaTime * 1000.0), null);
        },
        win.WM_SIZE => {
            ResizeWindow(hwnd);
            CalculateBoundaries();
            _ = win.InvalidateRect(hwnd, null, @intFromBool(false));
        },
        win.WM_ERASEBKGND => {
            return 1;
        },
        win.WM_CLOSE => {
            _ = win.DestroyWindow(hwnd);
        },
        win.WM_DESTROY => {
            _ = win.KillTimer(hwnd, RenderTimerId);
            DestroyDoubleBuffer();
            _ = win.PostQuitMessage(0);
        },
        else => return win.DefWindowProcA(hwnd, msg, wParam, lParam),
    }
    return 0;
}

pub export fn wWinMain(
    hInstance: win.HINSTANCE,
    hPrevInstance: win.HINSTANCE,
    cmdline: [*:0]u16,
    nCmdShow: c_int,
) callconv(.winapi) c_int {
    _ = hPrevInstance;
    _ = cmdline;

    if (false) {
        _ = win.AllocConsole();
        std.debug.print("Console attached\nWindow: {d}x{d}\n", .{ WindowWidth, WindowHeight });
    }

    const IDI_APPLICATION = @as(c_int, 32512);
    const IDC_ARROW = @as(c_int, 32512);
    const WindowClassName = "WindowClass";
    const wc: win.WNDCLASSEXA = .{
        .cbSize = @sizeOf(win.WNDCLASSEXA),
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .lpszClassName = WindowClassName,
        .lpfnWndProc = WndProc,
        .hInstance = hInstance,
        .style = 0,
        .lpszMenuName = null,
        .hbrBackground = null, //@ptrFromInt(win.COLOR_WINDOW + 1),
        .hIcon = win.LoadIconA(null, IDI_APPLICATION), // big icon
        .hIconSm = win.LoadIconA(null, IDI_APPLICATION), // small icon
        .hCursor = win.LoadCursorA(null, IDC_ARROW),
    };
    if (win.RegisterClassExA(&wc) == 0) {
        _ = win.MessageBoxA(null, "Window Registration Failed!", "Error!", win.MB_ICONEXCLAMATION | win.MB_OK);
        return -1;
    }

    const dwExStyle: win.DWORD = win.WS_EX_CLIENTEDGE;
    const dwStyle: win.DWORD = win.WS_OVERLAPPEDWINDOW;

    var WindowRectResized: win.RECT = .{
        .left = 0,
        .top = 0,
        .right = WindowWidth,
        .bottom = WindowHeight,
    };
    _ = win.AdjustWindowRectEx(&WindowRectResized, dwStyle, @intFromBool(false), dwExStyle);

    const hwnd = win.CreateWindowExA(
        dwExStyle,
        WindowClassName,
        "Title",
        dwStyle,
        win.CW_USEDEFAULT,
        win.CW_USEDEFAULT,
        RectWidth(WindowRectResized),
        RectHeight(WindowRectResized),
        null,
        null,
        hInstance,
        null,
    );
    if (hwnd == null) {
        _ = win.MessageBoxA(null, "Window Creation Failed!", "Error!", win.MB_ICONEXCLAMATION | win.MB_OK);
        return -1;
    }

    _ = win.ShowWindow(hwnd, nCmdShow);
    _ = win.UpdateWindow(hwnd);

    const UpdateThread: win.HANDLE = win.CreateThread(
        null,
        0,
        UpdateLoop,
        null,
        0,
        null,
    );

    var msg: win.MSG = undefined;
    while (win.GetMessageA(&msg, null, 0, 0) > 0) {
        _ = win.TranslateMessage(&msg);
        _ = win.DispatchMessageA(&msg);
    }

    _ = win.SuspendThread(UpdateThread);
    _ = win.CloseHandle(UpdateThread);

    return @intCast(msg.wParam);
}
