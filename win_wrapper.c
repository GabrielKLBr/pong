#define _WIN32_WINNT 0x0500 // Garante compatibilidade para Layered Windows
#include <windows.h>
#include <stdbool.h>
#include <dwmapi.h>
#include "win_wrapper.h"

LRESULT CALLBACK InternalWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Verifica o estado salvo no UserData da janela
            // 0 = Invisível (Pinta de preto para o ColorKey funcionar)
            // 1 = Visível (Pinta de branco)
            LONG_PTR isVisible = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            
            COLORREF color = (isVisible) ? RGB(255, 255, 255) : RGB(0, 0, 0);
            HBRUSH brush = CreateSolidBrush(color);
            
            FillRect(hdc, &ps.rcPaint, brush);
            DeleteObject(brush);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND: return 1; 
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void Win32_UseDarkMode(bool useDarkMode) {
    HWND hwnd = GetForegroundWindow();
    BOOL useDarkModeB;
    if(useDarkMode) useDarkModeB = TRUE; else useDarkModeB = FALSE;
    HRESULT hr = DwmSetWindowAttribute(
        hwnd,
        20,
        &useDarkModeB,
        sizeof(useDarkModeB)
    );
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(hwnd, SW_SHOW);
}

void Win32_ApplyEmbeddedIcon(void) {
    // Carrega o ícone do EXE (IDI_APP_ICON do .rc)
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), "IDI_ICON");

    // Setar ícone grande e pequeno da janela
    HWND hwnd = GetForegroundWindow();

    SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
}

WinHandle Win32_CreateWindow(int x, int y, int width, int height, WinHandle owner) {
    const char* CLASS_NAME = "PongElementClass";
    static bool classRegistered = false;

    if (!classRegistered) {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = InternalWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClass(&wc);
        classRegistered = true;
    }

    // Criamos inicialmente com WS_EX_LAYERED para poder usar transparência
    HWND hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED, 
        CLASS_NAME, "",
        WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        (HWND)owner, 
        NULL, GetModuleHandle(NULL), NULL
    );
    
    // Configura inicial: Preto transparente (Key = 0,0,0)
    SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, 0); // Estado 0 = Hidden

    return (WinHandle)hwnd;
}

void Win32_SetVisibleMode(WinHandle handle, bool visible) {
    if (!handle) return;
    HWND hwnd = (HWND)handle;

    // Atualiza o estado interno (0 ou 1)
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(visible ? 1 : 0));

    if (visible) {
        // Remove o estilo Layered (janela normal opaca)
        // Isso melhora performance e garante que seja branco puro
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, style & ~WS_EX_LAYERED);
    } else {
        // Adiciona Layered e define ColorKey preto
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
    }

    // Força repintura imediata
    InvalidateRect(hwnd, NULL, TRUE);
}

void Win32_MakeRound(WinHandle handle, int width, int height) {
    if (!handle) return;
    HRGN hRgn = CreateEllipticRgn(0, 0, width, height);
    SetWindowRgn((HWND)handle, hRgn, TRUE);
}

void Win32_SetTopMost(WinHandle handle, bool enable) {
    if (!handle) return;
    HWND order = enable ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos((HWND)handle, order, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Win32_SetWindowPos(WinHandle handle, int x, int y, int width, int height) {
    if (!handle) return;
    SetWindowPos((HWND)handle, NULL, x, y, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
}

void Win32_SetForegroundWindow(WinHandle handle)
{
    // HWND is the native Win32 window handle type.
    SetForegroundWindow((HWND)handle);
}

void Win32_ProcessMessages(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Win32_DestroyWindow(WinHandle handle) {
    if (handle) DestroyWindow((HWND)handle);
}