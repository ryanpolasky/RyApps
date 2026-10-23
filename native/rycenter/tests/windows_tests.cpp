#define wWinMain rycenterApplicationMain
#include "../src/main.cpp"
#undef wWinMain

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <stdexcept>

namespace {
constexpr wchar_t fixtureClass[] = L"RyCenter.TestFixture";
constexpr UINT createFixture = WM_APP + 50;
HWND fixtureRoot;

void require(bool condition, const char *text) {
    if (!condition)
        throw std::runtime_error(text);
    std::printf("PASS: %s\n", text);
}

void pump(DWORD duration) {
    const auto deadline = GetTickCount64() + duration;
    do {
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        const auto now = GetTickCount64();
        if (now >= deadline)
            return;
        MsgWaitForMultipleObjects(0, nullptr, FALSE, static_cast<DWORD>(deadline - now),
                                  QS_ALLINPUT);
    } while (true);
}

bool until(const std::function<bool()> &condition) {
    const auto deadline = GetTickCount64() + 3000;
    while (!condition()) {
        if (GetTickCount64() >= deadline)
            return false;
        pump(10);
    }
    return true;
}

LRESULT CALLBACK fixtureProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == createFixture) {
        const DWORD extended = wparam == 1 ? WS_EX_TOOLWINDOW : 0;
        const auto owner = wparam == 2 ? hwnd : nullptr;
        const auto created =
            CreateWindowExW(extended, fixtureClass, L"RyCenter test window", WS_OVERLAPPEDWINDOW,
                            30, 40, 320, 240, owner, nullptr, GetModuleHandleW(nullptr), nullptr);
        ShowWindow(created, SW_SHOWNOACTIVATE);
        return reinterpret_cast<LRESULT>(created);
    }
    if (message == WM_DESTROY && hwnd == fixtureRoot) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

int runFixture() {
    WNDCLASSW cls{};
    cls.lpfnWndProc = fixtureProc;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.lpszClassName = fixtureClass;
    RegisterClassW(&cls);
    fixtureRoot =
        CreateWindowExW(0, fixtureClass, L"RyCenter existing test window", WS_OVERLAPPEDWINDOW, 30,
                        40, 320, 240, nullptr, nullptr, cls.hInstance, nullptr);
    if (!fixtureRoot)
        return 1;
    ShowWindow(fixtureRoot, SW_SHOWNOACTIVATE);
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
        DispatchMessageW(&message);
    return 0;
}

struct Fixture {
    PROCESS_INFORMATION process{};
    HWND root = nullptr;

    ~Fixture() {
        if (root)
            PostMessageW(root, WM_CLOSE, 0, 0);
        if (process.hProcess) {
            if (WaitForSingleObject(process.hProcess, 3000) != WAIT_OBJECT_0) {
                TerminateProcess(process.hProcess, 1);
            }
            CloseHandle(process.hProcess);
            CloseHandle(process.hThread);
        }
    }

    void start() {
        auto command = startupCommandLine() + L" --fixture";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        require(CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr,
                               nullptr, &startup, &process) != FALSE,
                "launch external-process test windows");
        require(until([&] {
                    struct Search {
                        DWORD process;
                        HWND hwnd;
                    } search{process.dwProcessId, nullptr};
                    EnumWindows(
                        [](HWND hwnd, LPARAM data) -> BOOL {
                            auto &result = *reinterpret_cast<Search *>(data);
                            DWORD pid = 0;
                            GetWindowThreadProcessId(hwnd, &pid);
                            if (pid == result.process && IsWindowVisible(hwnd)) {
                                result.hwnd = hwnd;
                                return FALSE;
                            }
                            return TRUE;
                        },
                        reinterpret_cast<LPARAM>(&search));
                    root = search.hwnd;
                    return root != nullptr;
                }),
                "external window is ready");
    }

    HWND create(WPARAM kind = 0) const {
        DWORD_PTR result = 0;
        require(SendMessageTimeoutW(root, createFixture, kind, 0, SMTO_ABORTIFHUNG, 3000,
                                    &result) != 0 &&
                    result != 0,
                "create test window");
        return reinterpret_cast<HWND>(result);
    }
};

RECT bounds(HWND hwnd) {
    RECT rect{};
    GetWindowRect(hwnd, &rect);
    return rect;
}

bool isCentered(HWND hwnd) {
    DwmFlush();
    const auto outer = bounds(hwnd);
    RECT visible = outer;
    if (FAILED(
            DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &visible, sizeof(visible)))) {
        visible = outer;
    }
    MONITORINFO monitor{sizeof(MONITORINFO), {}, {}, 0};
    GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor);
    const auto position =
        rycenter::centeredPosition(rectangle(outer), rectangle(visible), rectangle(monitor.rcWork));
    return std::abs(outer.left - position.x) <= 1 && std::abs(outer.top - position.y) <= 1;
}

BOOL CALLBACK checkMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    const auto hwnd = reinterpret_cast<HWND>(data);
    MONITORINFO info{sizeof(MONITORINFO), {}, {}, 0};
    GetMonitorInfoW(monitor, &info);
    SetWindowPos(hwnd, nullptr, info.rcWork.left + 20, info.rcWork.top + 30, 320, 240,
                 SWP_NOACTIVATE | SWP_NOZORDER);
    DwmFlush();
    const auto before = bounds(hwnd);
    const auto foreground = GetForegroundWindow();
    require(centerWindow(hwnd, false) == CenterResult::Centered, "accept center request");
    require(until([&] { return isCentered(hwnd); }),
            "center the visible frame on the monitor work area");
    const auto after = bounds(hwnd);
    require(after.right - after.left == before.right - before.left &&
                after.bottom - after.top == before.bottom - before.top,
            "preserve window size");
    require(GetForegroundWindow() == foreground, "preserve foreground focus");
    return TRUE;
}

void runChecks(Fixture &fixture) {
    require(validHotkey(MAKEWORD('C', HOTKEYF_CONTROL | HOTKEYF_ALT)) &&
                !validHotkey(MAKEWORD('C', 0)) && !validHotkey(MAKEWORD('C', HOTKEYF_SHIFT)) &&
                !validHotkey(0x10000),
            "validate shortcuts");
    require(registerShortcut(MAKEWORD(VK_F24, HOTKEYF_CONTROL | HOTKEYF_ALT), 99),
            "register a system hotkey without a keyboard hook");
    UnregisterHotKey(messageWindow, 99);
    EnumDisplayMonitors(nullptr, nullptr, checkMonitor, reinterpret_cast<LPARAM>(fixture.root));

    ShowWindow(fixture.root, SW_MAXIMIZE);
    require(until([&] { return IsZoomed(fixture.root) != FALSE; }), "maximize test window");
    require(centerWindow(fixture.root, false) == CenterResult::Skipped, "skip maximized window");
    ShowWindow(fixture.root, SW_MINIMIZE);
    require(until([&] { return IsIconic(fixture.root) != FALSE; }), "minimize test window");
    require(centerWindow(fixture.root, false) == CenterResult::Skipped, "skip minimized window");
    ShowWindow(fixture.root, SW_RESTORE);
    SetWindowPos(fixture.root, nullptr, 30, 40, 320, 240, SWP_NOACTIVATE | SWP_NOZORDER);

    require(!openHook && !dragHook && pending.empty(),
            "default mode has no hooks or settling timers");
    require(enableAutomatic(), "enable optional automatic mode");
    pump(400);
    require(bounds(fixture.root).left == 30 && bounds(fixture.root).top == 40,
            "do not rearrange existing windows when enabling auto mode");
    const auto automatic = fixture.create();
    require(until([&] { return isCentered(automatic); }),
            "automatically center a newly opened window");
    require(until([] { return pending.empty(); }), "remove the one-shot timer after centering");

    const auto moved = fixture.create();
    require(until([&] {
                return std::any_of(pending.begin(), pending.end(), [moved](const auto &item) {
                    return item.hwnd == moved && item.timer;
                });
            }),
            "new window receives a settling timer");
    SetWindowPos(moved, nullptr, 70, 80, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    pump(400);
    require(bounds(moved).left == 70 && bounds(moved).top == 80,
            "leave a window alone if it moved during the settling delay");
    ShowWindow(moved, SW_HIDE);
    ShowWindow(moved, SW_SHOWNOACTIVATE);
    pump(400);
    require(bounds(moved).left == 70 && bounds(moved).top == 80,
            "do not recenter an existing window when it is shown again");

    const auto tool = fixture.create(1);
    const auto owned = fixture.create(2);
    pump(400);
    require(!eligible(tool, true) && bounds(tool).left == 30, "skip tool windows");
    require(!eligible(owned, true) && bounds(owned).left == 30,
            "skip owned dialogs in automatic mode");
    const auto temporary = fixture.create();
    PostMessageW(temporary, WM_CLOSE, 0, 0);
    require(until([&] { return !IsWindow(temporary); }), "destroy a window before its timer fires");
    pump(400);
    require(pending.empty(), "remove destroyed candidates and their timers");
    disableAutomatic();
    require(!openHook && !dragHook && pending.empty(),
            "unregister hooks and timers when automatic mode is off");
    const auto disabled = fixture.create();
    pump(400);
    require(bounds(disabled).left == 30 && bounds(disabled).top == 40,
            "new windows stay where they open in shortcut-only mode");
}
}

int main(int argc, char **) {
    if (argc > 1)
        return runFixture();
    instance = GetModuleHandleW(nullptr);
    WNDCLASSW cls{};
    cls.lpfnWndProc = windowProc;
    cls.hInstance = instance;
    cls.lpszClassName = windowClass;
    RegisterClassW(&cls);
    messageWindow = CreateWindowExW(0, windowClass, L"RyCenter integration tests", 0, 0, 0, 0, 0,
                                    nullptr, nullptr, instance, nullptr);
    Fixture fixture;
    int result = 0;
    try {
        require(messageWindow != nullptr, "create hidden event receiver");
        fixture.start();
        runChecks(fixture);
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        result = 1;
    }
    disableAutomatic();
    DestroyWindow(messageWindow);
    return result;
}
