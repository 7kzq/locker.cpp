// language: C++, file: locker.cpp, target: Windows 11 x64, MSVC
// WinLocker TEST — fullscreen password lock, panic exit, safe
#include <windows.h>
#include <windowsx.h>
#include <string>

const unsigned long long PASS_HASH = 0xa72a9d1e3f0f5b07ULL;

unsigned long long fnv1a(const std::wstring& s) {
    unsigned long long h = 0xcbf29ce484222325ULL;
    for (wchar_t c : s) {
        h ^= (unsigned char)(c & 0xFF);
        h *= 0x100000001b3ULL;
        h ^= (unsigned char)((c >> 8) & 0xFF);
        h *= 0x100000001b3ULL;
    }
    return h;
}

std::wstring g_input;
bool g_unlocked = false;
RECT g_panicBtn{};

const COLORREF BG     = RGB(0, 0, 0);
const COLORREF FG     = RGB(255, 255, 255);
const COLORREF DIM    = RGB(90, 90, 90);
const COLORREF ACCENT = RGB(200, 30, 30);
const COLORREF PANIC  = RGB(60, 60, 60);
const COLORREF PANIC_HOT = RGB(120, 40, 40);

void DrawTextEx(HDC dc, int x, int y, const wchar_t* s, COLORREF c,
                int size, bool center, int winW) {
    HFONT font = CreateFontW(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH, L"Consolas");
    HFONT old = (HFONT)SelectObject(dc, font);
    SetTextColor(dc, c);
    SetBkMode(dc, TRANSPARENT);
    if (center) {
        SIZE sz;
        GetTextExtentPoint32W(dc, s, (int)wcslen(s), &sz);
        x = (winW - sz.cx) / 2;
    }
    TextOutW(dc, x, y, s, (int)wcslen(s));
    SelectObject(dc, old);
    DeleteObject(font);
}

void Paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT rc; GetClientRect(hwnd, &rc);

    HBRUSH bg = CreateSolidBrush(BG);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    int cx = rc.right / 2;

    DrawTextEx(dc, 0, 140, L"SYSTEM LOCKED", ACCENT, 72, true, rc.right);
    DrawTextEx(dc, 0, 240, L"enter password to continue", DIM, 22, true, rc.right);

    RECT box{ cx - 250, 340, cx + 250, 400 };
    HBRUSH bf = CreateSolidBrush(RGB(15, 15, 15));
    FillRect(dc, &box, bf);
    DeleteObject(bf);
    FrameRect(dc, &box, (HBRUSH)GetStockObject(WHITE_BRUSH));

    std::wstring masked(g_input.size(), L'*');
    DrawTextEx(dc, cx - 230, 358, masked.c_str(), FG, 26, false, 0);

    DrawTextEx(dc, 0, 440, L"[ Enter ]  unlock", DIM, 20, true, rc.right);

    g_panicBtn = { cx - 150, 520, cx + 150, 570 };
    POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
    bool hot = PtInRect(&g_panicBtn, mp) != 0;
    HBRUSH pb = CreateSolidBrush(hot ? PANIC_HOT : PANIC);
    FillRect(dc, &g_panicBtn, pb);
    DeleteObject(pb);
    FrameRect(dc, &g_panicBtn, (HBRUSH)GetStockObject(WHITE_BRUSH));
    DrawTextEx(dc, 0, 532, L"PANIC EXIT", FG, 22, true, rc.right);

    DrawTextEx(dc, 0, 620,
               L"exit: password / panic / ctrl+alt+del",
               RGB(60, 60, 60), 16, true, rc.right);

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: Paint(hwnd); return 0;

    case WM_MOUSEMOVE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN: {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (PtInRect(&g_panicBtn, p)) {
            g_unlocked = true;
            PostQuitMessage(0);
        }
        return 0;
    }

    case WM_CHAR: {
        if (wp == VK_BACK) {
            if (!g_input.empty()) g_input.pop_back();
        } else if (wp == VK_RETURN) {
            if (fnv1a(g_input) == PASS_HASH) {
                g_unlocked = true;
                PostQuitMessage(0);
            } else {
                g_input.clear();
                MessageBeep(MB_ICONHAND);
            }
        } else if (wp >= 32) {
            g_input.push_back((wchar_t)wp);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int show) {
    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hi;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"winlocker";
    RegisterClassW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        L"winlocker", L"System Locked",
        WS_POPUP,
        0, 0, sw, sh,
        nullptr, nullptr, hi, nullptr);

    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
