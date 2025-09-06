#include <windows.h>
#include <string>
#include <chrono>

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

void AddControls(HWND hWnd, HINSTANCE hInst);

#define WS_NONRESIZEABLEWINDOW (WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

#define WS_STATICNORMAL (WS_VISIBLE | WS_CHILD)
#define WS_STATICCENTERED (WS_VISIBLE | WS_CHILD | SS_CENTER | WS_BORDER)
#define WS_STATICCENTEREDNOBORDER (WS_VISIBLE | WS_CHILD | SS_CENTER)

#define WS_EDITREADONLYCENTERED (WS_VISIBLE | WS_CHILD | ES_READONLY | ES_CENTER)

#define WS_BUTTONNORMAL (WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON)

#define TIMER 1

#define ID_STATICTITLE 101
#define ID_STATICTIME 102
#define ID_STATICTIMEELAPSEDLABEL 103
#define ID_STATICDATE 104

#define ID_EDITTIMEELAPSED 203

#define ID_BUTTONSTART 301
#define ID_BUTTONSTOP 302
#define ID_BUTTONRESET 303

constexpr int WINDOW_WIDTH = 700;
constexpr int WINDOW_HEIGHT = 700;

BOOL StopWatchRunning = FALSE;
BOOL AlreadyStarted = FALSE;

HWND hWnd, hStaticTitle, hStaticTime, hStaticElapsedLabel, hEditTimeElapsed, hButtonStart, hButtonStop, hButtonReset, hStaticDate;

HFONT hFontTitle, hFontNormal;

COLORREF background_color = RGB(0x54, 0x0D, 0x6E);
HBRUSH hBrushBackground = CreateSolidBrush(background_color);

COLORREF frame_text_color = RGB(0xFF, 0xD2, 0x3F);
HBRUSH hBrushFrameTextColor = CreateSolidBrush(frame_text_color);

COLORREF textframe_color = RGB(0xEE, 0x42, 0x66);
HBRUSH hBrushTextframe = CreateSolidBrush(textframe_color);

COLORREF textframe_color2 = RGB(0x03, 0xAD, 0x69);
HBRUSH hBrushTextframe2 = CreateSolidBrush(textframe_color2);

COLORREF foreground_color = RGB(0x3B, 0xCE, 0xAC);
HBRUSH hBrushForeground = CreateSolidBrush(foreground_color);

std::chrono::steady_clock::time_point stopwatchStart;
std::chrono::milliseconds stopwatchPausedTime(0);
std::chrono::steady_clock::time_point startTime;
std::chrono::milliseconds pausedTime(0);

std::wstring FormatTime(std::chrono::milliseconds ms) {
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(ms);
    ms-=minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ms);
    ms -= seconds;
    wchar_t buffer[32];
    swprintf(buffer, 32, L"%02lld:%02lld:%03lld", minutes.count(), seconds.count(), ms.count());
    return buffer;
}


std::wstring GetCurrentTime_local() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    tm localTime{};
    localtime_s(&localTime, &now_c);

    wchar_t buffer[64];
    wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%H:%M:%S", &localTime);
    return buffer;
}

void ShowDay(HWND hWnd) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    const wchar_t* days[] = {L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday", L"Sunday"};
    const wchar_t* months[] = {L"January", L"February", L"March", L"April", L"May", L"June", L"July", L"August", L"September", L"October", L"November", L"December"};
    wchar_t buffer[128];
    swprintf(buffer, 128, L"%ls %02d %04d, %ls", months[st.wMonth - 1], st.wDay, st.wYear, days[st.wDayOfWeek]);
    SetWindowTextW(hWnd, buffer);

}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int nCmdShow) {
    SetProcessDPIAware();
    WNDCLASSW wc = {0};
    wc.hbrBackground = hBrushBackground;
    wc.hInstance = hInst;
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"WINDOW";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (!RegisterClassW(&wc)) {
        return -1;
    }

    hWnd = CreateWindowW(L"WINDOW", L"Clock Application 2.0", WS_NONRESIZEABLEWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, NULL, NULL
    );

    AddControls(hWnd, hInst);

    MSG msg = {0};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        SetTimer(hWnd, TIMER, 50, NULL);
        return 0;
    }
    case WM_PAINT: { // Yeah, if you are reading this piece of code right here that just draws a rectangle, just know that it caused me a lot of pain... *sighs in disappointment* lots of debugging. Anyways, you may continue stalking my code. P.S. If you want to know more about this, contact me.
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT frame_rect = {50, 250, 650, 550};
        FillRect(hdc, &frame_rect, hBrushTextframe);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_COMMAND: {
        switch (LOWORD(wp)) {
            case ID_BUTTONSTART: {
                if (!StopWatchRunning) {
                    if (AlreadyStarted) {
                        // Adjust start time so elapsed time stays correct
                        stopwatchStart = std::chrono::steady_clock::now() - stopwatchPausedTime;
                        StopWatchRunning = TRUE;
                    } else {
                        stopwatchStart = std::chrono::steady_clock::now();
                        stopwatchPausedTime = std::chrono::milliseconds(0);
                        StopWatchRunning = TRUE;
                        AlreadyStarted = TRUE;
                    }
                }
                return 0;
            }

            case ID_BUTTONSTOP: {
                if (StopWatchRunning) { 
                    stopwatchPausedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - stopwatchStart
                    );
                    StopWatchRunning = FALSE;
                    AlreadyStarted = TRUE;
                }
                return 0;
            }
            case ID_BUTTONRESET: {
                StopWatchRunning = FALSE;
                stopwatchPausedTime = std::chrono::milliseconds(0);
                SetWindowTextW(hEditTimeElapsed, L"00:00.000");
                return 0;
            }
        }
        return 0;
    }
    case WM_TIMER: {
        std::wstring timeStr = GetCurrentTime_local();
        SetWindowTextW(hStaticTime, timeStr.c_str());
        std::chrono::milliseconds elapsed(0);
        if (StopWatchRunning) {
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - stopwatchStart
            );
        } else {
            elapsed = stopwatchPausedTime;
        }
        std::wstring swTime = FormatTime(elapsed);
        SetWindowTextW(hEditTimeElapsed, swTime.c_str());
        ShowDay(hStaticDate);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wp;
        HWND hWindow = (HWND)lp;
        int ctrlId = GetDlgCtrlID(hWindow);

        switch (ctrlId) {
        case ID_STATICTITLE: {
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, textframe_color);
            SetTextColor(hdcStatic, foreground_color);
            return (INT_PTR)hBrushTextframe;
        }
        case ID_STATICTIME: {
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, textframe_color);
            SetTextColor(hdcStatic, foreground_color);
            
            return (INT_PTR)hBrushTextframe;
        }
        case ID_STATICTIMEELAPSEDLABEL: {
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, frame_text_color);
            SetTextColor(hdcStatic, textframe_color2);
            
            return (INT_PTR)hBrushFrameTextColor;
        }
        case ID_EDITTIMEELAPSED: {
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, frame_text_color);
            SetTextColor(hdcStatic, textframe_color2);
            
            return (INT_PTR)hBrushFrameTextColor;
        }
        case ID_STATICDATE: {
            SetBkMode(hdcStatic, TRANSPARENT);
            SetTextColor(hdcStatic, frame_text_color);
            return (INT_PTR)hBrushBackground;
        }
        }
        return 0;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    default: 
        return DefWindowProcW(hWnd, msg, wp, lp);
    }
}

void AddControls(HWND hWnd, HINSTANCE hInst) {
    hFontTitle = CreateFontW(40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, L"Helvetica");
    hFontNormal = CreateFontW(30, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, L"Helvetica");
    HFONT hFontButton = CreateFontW(25, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, ANTIALIASED_QUALITY, 0, L"Helvetica");

    hStaticTitle = CreateWindowW(L"STATIC", L"Clock Application", WS_STATICCENTERED, 
        0, 50, WINDOW_WIDTH, 50, hWnd, (HMENU)ID_STATICTITLE, hInst, NULL
    );
    SendMessageW(hStaticTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
    hStaticTime = CreateWindowW(L"STATIC", L"", WS_STATICCENTERED, 
        210, 150, 250, 50, hWnd, (HMENU)ID_STATICTIME, hInst, NULL
    );
    SendMessageW(hStaticTime, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
    hStaticElapsedLabel = CreateWindowW(L"STATIC", L"Time Elapsed: ", WS_STATICCENTERED, 
        100, 290, 220, 40, hWnd, (HMENU)ID_STATICTIMEELAPSEDLABEL, hInst, NULL
    );
    SendMessageW(hStaticElapsedLabel, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    hStaticDate = CreateWindowW(L"STATIC", L"", WS_STATICCENTEREDNOBORDER, 
        (WINDOW_WIDTH - 400) / 2, 570, 400, 40, hWnd, (HMENU)ID_STATICDATE, hInst, NULL
    );
    SendMessageW(hStaticDate, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    hEditTimeElapsed = CreateWindowW(L"EDIT", L"", WS_EDITREADONLYCENTERED, 
        360, 285, 220, 50, hWnd, (HMENU)ID_EDITTIMEELAPSED, hInst, NULL
    );
    SendMessageW(hEditTimeElapsed, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
    hButtonStart = CreateWindowW(L"BUTTON", L"Start Stopwatch", WS_BUTTONNORMAL, 
        70, 400, 180, 50, hWnd, (HMENU)ID_BUTTONSTART, hInst, NULL
    );
    SendMessageW(hButtonStart, WM_SETFONT, (WPARAM)hFontButton, TRUE);
    hButtonStop = CreateWindowW(L"BUTTON", L"Stop Stopwatch", WS_BUTTONNORMAL, 
        260, 400, 180, 50, hWnd, (HMENU)ID_BUTTONSTOP, hInst, NULL
    );
    SendMessageW(hButtonStop, WM_SETFONT, (WPARAM)hFontButton, TRUE);
    hButtonReset = CreateWindowW(L"BUTTON", L"Reset Stopwatch", WS_BUTTONNORMAL, 
        450, 400, 180, 50, hWnd, (HMENU)ID_BUTTONRESET, hInst, NULL
    );
    SendMessageW(hButtonReset, WM_SETFONT, (WPARAM)hFontButton, TRUE);




    SendMessageW(hWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);
}
