#include <Windows.h>
#include <shellapi.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "resource1.h"

HHOOK mouseHook;
bool blockNextLeftClick = false;
bool blockNextRightClick = false;
bool leftClickRegistered = false;
bool rightClickRegistered = false;
int leftDebounceTime = 55;
int rightDebounceTime = 0;
int lastLeftClickTime = 0;
int lastRightClickTime = 0;
bool isHidden = false;

NOTIFYICONDATA notifyIconData;

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_LBUTTONDOWN) {
            int currentTime = GetTickCount64();
            int timeSinceLastClick = currentTime - lastLeftClickTime;
            lastLeftClickTime = currentTime;

            if (blockNextLeftClick || timeSinceLastClick <= leftDebounceTime) {
                blockNextLeftClick = false;
                leftClickRegistered = true;
                return 1;
            }
        }
        else if (wParam == WM_LBUTTONUP) {
            if (leftClickRegistered) {
                leftClickRegistered = false;
                return 1;
            }
        }
        else if (wParam == WM_RBUTTONDOWN) {
            int currentTime = GetTickCount64();
            int timeSinceLastClick = currentTime - lastRightClickTime;
            lastRightClickTime = currentTime;

            if (blockNextRightClick || timeSinceLastClick <= rightDebounceTime) {
                blockNextRightClick = false;
                rightClickRegistered = true;
                return 1;
            }
        }
        else if (wParam == WM_RBUTTONUP) {
            if (rightClickRegistered) {
                rightClickRegistered = false;
                return 1;
            }
        }
    }

    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void ShowNotification(HWND hWnd, const wchar_t* message) {
    // Display custom notification
    notifyIconData.uFlags = NIF_INFO;
    notifyIconData.dwInfoFlags = NIIF_INFO;
    notifyIconData.uTimeout = 5000; // 5 seconds
    wcscpy_s(notifyIconData.szInfo, _countof(notifyIconData.szInfo), message);
    Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
}

void CreateNotificationIcon(HWND hWnd) {
    ZeroMemory(&notifyIconData, sizeof(NOTIFYICONDATA));
    notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    notifyIconData.hWnd = hWnd;
    notifyIconData.uID = 1;
    notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyIconData.uCallbackMessage = WM_USER + 1;

    notifyIconData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

    wcsncpy_s(notifyIconData.szTip, L"RBWCM Debouncer", _countof(notifyIconData.szTip) - 1);

    if (!Shell_NotifyIcon(NIM_ADD, &notifyIconData)) {
        std::cerr << "Failed to add system tray icon." << std::endl;
    }
}

void RemoveNotificationIcon() {
    if (!Shell_NotifyIcon(NIM_DELETE, &notifyIconData)) {
        std::cerr << "Failed to remove system tray icon." << std::endl;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_USER + 1:
        switch (LOWORD(lParam)) {
        case WM_RBUTTONDOWN: {
            HMENU hPopupMenu = CreatePopupMenu();
            if (hPopupMenu) {
                AppendMenu(hPopupMenu, MF_STRING, 1, L"Esci");

                POINT cursor;
                GetCursorPos(&cursor);

                SetForegroundWindow(hWnd);

                if (!TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hWnd, NULL)) {
                    std::cerr << "Failed to track popup menu." << std::endl;
                }

                DestroyMenu(hPopupMenu);
            }
            else {
                std::cerr << "Failed to create popup menu." << std::endl;
            }
            break;
        }
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            OutputDebugString(L"Debug message: WM_COMMAND received.\n");
            DestroyWindow(hWnd);
        }
        break;

    case WM_CLOSE:
        RemoveNotificationIcon();
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        RemoveNotificationIcon();
        PostQuitMessage(0);
        break;

    case WM_CONTEXTMENU: {
        HMENU hPopupMenu = CreatePopupMenu();
        if (hPopupMenu) {
            AppendMenu(hPopupMenu, MF_STRING, 1, L"Esci");

            POINT cursor;
            GetCursorPos(&cursor);

            SetForegroundWindow(hWnd);

            if (!TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hWnd, NULL)) {
                std::cerr << "Failed to track popup menu." << std::endl;
            }

            DestroyMenu(hPopupMenu);
        }
        else {
            std::cerr << "Failed to create popup menu." << std::endl;
        }
        break;
    }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {

    HANDLE hMutex = CreateMutex(NULL, TRUE, L"RBWCMDebouncerMutex");

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MouseDebouncerClass";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    RegisterClass(&wc);

    

    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

    HWND hWnd = CreateWindowEx(0, L"MouseDebouncerClass", L"RBWCM Debouncer", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL, hInstance, NULL);
    CreateNotificationIcon(hWnd);

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        ShowNotification(hWnd, L"L'RBWCM Debouncer \u00E8 gi\u00E0 in esecuzione, per chiuderlo guarda le applicazioni in background sulla tua barra della applicazioni.");
        return 0;
    }

    ShowNotification(hWnd, L"RBWCM Debouncer avviato correttamente.");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(mouseHook);

    return 0;
}